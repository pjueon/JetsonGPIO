/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/


#include <unistd.h>
#include <dirent.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

#include <chrono>
#include <thread>

#include "JetsonGPIO.h"
#include "private/gpio_pin_data.h"
#include "private/PythonFunctions.h"

using namespace GPIO;
using namespace std;

// GPIO directions. UNKNOWN constant is for gpios that are not yet setup
enum class GPIO::Directions{ UNKNOWN, OUT, IN, HARD_PWM };

// The user only can use IN, OUT
extern const Directions GPIO::IN = Directions::IN;
extern const Directions GPIO::OUT = Directions::OUT;

// The use CAN'T use UNKNOW, HARD_PWM
constexpr Directions UNKNOWN = Directions::UNKNOWN;
constexpr Directions HARD_PWM = Directions::HARD_PWM;

// Jetson Models
enum class Model{ JETSON_XAVIER, JETSON_TX2, JETSON_TX1, JETSON_NANO };

extern const Model JETSON_XAVIER = Model::JETSON_XAVIER;
extern const Model JETSON_TX2 = Model::JETSON_TX2;
extern const Model JETSON_TX1 = Model::JETSON_TX1;
extern const Model JETSON_NANO = Model::JETSON_NANO;

// Numbering Modes
enum class GPIO::NumberingModes{ BOARD, BCM, TEGRA_SOC, CVM, None };

// The use CAN'T use NumberingModes::None
extern const NumberingModes GPIO::BOARD = NumberingModes::BOARD;
extern const NumberingModes GPIO::BCM = NumberingModes::BCM;
extern const NumberingModes GPIO::TEGRA_SOC = NumberingModes::TEGRA_SOC;
extern const NumberingModes GPIO::CVM = NumberingModes::CVM;


// ========================================= Begin of "gpio.py" =========================================

constexpr auto _SYSFS_ROOT = "/sys/class/gpio";

bool CheckPermission(){
    string path1 = string(_SYSFS_ROOT) + "/export";
    string path2 = string(_SYSFS_ROOT) + "/unexport";
    if(!os_access(path1, W_OK) || !os_access(path2, W_OK) ){
        cerr << "[ERROR] The current user does not have permissions set to "
                "access the library functionalites. Please configure "
                "permissions or use the root user to run this." << endl;
        throw runtime_error("Permission Denied.");
    }

    return true;
}

const bool CanWrite = CheckPermission();

GPIO_data _gpio_data = get_data();
const Model _model = _gpio_data.model;
const _GPIO_PIN_INFO _JETSON_INFO = _gpio_data.pin_info;
const auto _channel_data_by_mode = _gpio_data.channel_data;

// Originally added function
const string _get_JETSON_INFO(){
    stringstream ss;
    ss << "[JETSON_INFO]\n";
    ss << "P1_REVISION: " << _JETSON_INFO.P1_REVISION << endl;
    ss << "RAM: " << _JETSON_INFO.RAM << endl;
    ss << "REVISION: " << _JETSON_INFO.REVISION << endl;
    ss << "TYPE: " << _JETSON_INFO.TYPE << endl;
    ss << "MANUFACTURER: " << _JETSON_INFO.MANUFACTURER << endl;
    ss << "PROCESSOR: " << _JETSON_INFO.PROCESSOR << endl;
    return ss.str();
}

const char* _get_model(){
    if(_model == Model::JETSON_XAVIER) return "JETSON_XAVIER";
    else if(_model == Model::JETSON_TX1) return "JETSON_TX1";
    else if(_model == Model::JETSON_TX2) return "JETSON_TX2";
    else if(_model == Model::JETSON_NANO) return "JETSON_NANO";
    else{
        throw runtime_error("get_model error");
    };
}

// A map used as lookup tables for pin to linux gpio mapping
map<string, ChannelInfo> _channel_data;

bool _gpio_warnings = true;
NumberingModes _gpio_mode = NumberingModes::None;
map<string, Directions>_channel_configuration;



void _validate_mode_set(){
    if(_gpio_mode == NumberingModes::None)
        throw runtime_error("Please set pin numbering mode using "
                           "GPIO.setmode(GPIO::BOARD), GPIO.setmode(GPIO::BCM), "
                           "GPIO.setmode(GPIO::TEGRA_SOC) or "
                           "GPIO.setmode(GPIO::CVM)");
}


ChannelInfo _channel_to_info_lookup(const string& channel, bool need_gpio, bool need_pwm){
    if(_channel_data.find(channel) == _channel_data.end())
        throw runtime_error("Channel " + channel + " is invalid");
    ChannelInfo ch_info = _channel_data.at(channel);
    if (need_gpio && ch_info.gpio_chip_dir == "None")
        throw runtime_error("Channel " + channel + " is not a GPIO");
    if (need_pwm && ch_info.pwm_chip_dir == "None")
        throw runtime_error("Channel " + channel + " is not a PWM");
    return ch_info;
}


ChannelInfo _channel_to_info(const string& channel, bool need_gpio = false, bool need_pwm = false){
    _validate_mode_set();
    return _channel_to_info_lookup(channel, need_gpio, need_pwm);
}


vector<ChannelInfo> _channels_to_infos(const vector<string>& channels, bool need_gpio = false, bool need_pwm = false){
    _validate_mode_set();
    vector<ChannelInfo> ch_infos;
    for (const auto& c : channels){
        ch_infos.push_back(_channel_to_info_lookup(c, need_gpio, need_pwm));
    }
    return ch_infos;
}


/* Return the current configuration of a channel as reported by sysfs. 
   Any of IN, OUT, HARD_PWM, or UNKNOWN may be returned. */
Directions _sysfs_channel_configuration(const ChannelInfo& ch_info){
    if (ch_info.pwm_chip_dir != "None") {
        string pwm_dir = ch_info.pwm_chip_dir + "/pwm" + to_string(ch_info.pwm_id);
        if (os_path_exists(pwm_dir))
            return HARD_PWM;
    }

    string gpio_dir = string(_SYSFS_ROOT) + "/gpio" + to_string(ch_info.gpio);
    if (!os_path_exists(gpio_dir))
        return UNKNOWN;  // Originally returns None in NVIDIA's GPIO Python Library

    string gpio_direction;
    { // scope for f
        ifstream f_direction(gpio_dir + "/direction");
        stringstream buffer;
        buffer << f_direction.rdbuf();
        gpio_direction = buffer.str();
        gpio_direction = strip(gpio_direction);
        // lower()
        transform(gpio_direction.begin(), gpio_direction.end(), 
                gpio_direction.begin(), [](unsigned char c){ return tolower(c); });
    } // scope ends

    if (gpio_direction == "in")
        return IN;
    else if (gpio_direction == "out")
        return OUT;
    else
        return UNKNOWN;  // Originally returns None in NVIDIA's GPIO Python Library
}


/* Return the current configuration of a channel as requested by this
   module in this process. Any of IN, OUT, or UNKNOWN may be returned. */
Directions _app_channel_configuration(const ChannelInfo& ch_info){
    if (_channel_configuration.find(ch_info.channel) == _channel_configuration.end())
        return UNKNOWN;   // Originally returns None in NVIDIA's GPIO Python Library
    return _channel_configuration[ch_info.channel];
}


void _export_gpio(const int gpio){
    if(os_path_exists(string(_SYSFS_ROOT) + "/gpio" + to_string(gpio) ))
        return;
    { // scope for f_export
        ofstream f_export(string(_SYSFS_ROOT) + "/export");
        f_export << gpio;
    } // scope ends

    string value_path = string(_SYSFS_ROOT) + "/gpio" + to_string(gpio) + "/value";
    
    int time_count = 0;
    while (!os_access(value_path, R_OK | W_OK)){
        this_thread::sleep_for(chrono::milliseconds(10));
	if(time_count++ > 100) throw runtime_error("Permission denied: path: " + value_path + "\n Please configure permissions or use the root user to run this.");
    }
}


void _unexport_gpio(const int gpio){
    if (!os_path_exists(string(_SYSFS_ROOT) + "/gpio" + to_string(gpio)))
        return;

    ofstream f_unexport(string(_SYSFS_ROOT) + "/unexport");
    f_unexport << gpio;
}


void _output_one(const string gpio, const int value){
    ofstream value_file(string(_SYSFS_ROOT) + "/gpio" + gpio + "/value");
    value_file << int(bool(value));
}

void _output_one(const int gpio, const int value){
    _output_one(to_string(gpio), value);
}


void _setup_single_out(const ChannelInfo& ch_info, int initial = -1){
    _export_gpio(ch_info.gpio);

    string gpio_dir_path = string(_SYSFS_ROOT) + "/gpio" + to_string(ch_info.gpio) + "/direction";
    { // scope for direction_file
        ofstream direction_file(gpio_dir_path);
        direction_file << "out";
    } // scope ends

    if(initial != -1)
        _output_one(ch_info.gpio, initial);
    
    _channel_configuration[ch_info.channel] = OUT;
}


void _setup_single_in(const ChannelInfo& ch_info){
    _export_gpio(ch_info.gpio);

    string gpio_dir_path = string(_SYSFS_ROOT) + "/gpio" + to_string(ch_info.gpio) + "/direction";
    { // scope for direction_file
        ofstream direction_file(gpio_dir_path);
        direction_file << "in";
    } // scope ends

    _channel_configuration[ch_info.channel] = IN;
}

string _pwm_path(const ChannelInfo& ch_info){
    return ch_info.pwm_chip_dir + "/pwm" + to_string(ch_info.pwm_id);
}


string _pwm_export_path(const ChannelInfo& ch_info){
    return ch_info.pwm_chip_dir + "/export";
}


string _pwm_unexport_path(const ChannelInfo& ch_info){
    return ch_info.pwm_chip_dir + "/unexport";
}


string _pwm_period_path(const ChannelInfo& ch_info){
    return _pwm_path(ch_info) + "/period";
}


string _pwm_duty_cycle_path(const ChannelInfo& ch_info){
    return _pwm_path(ch_info) + "/duty_cycle";
}


string _pwm_enable_path(const ChannelInfo& ch_info){
    return _pwm_path(ch_info) + "/enable";
}


void _export_pwm(const ChannelInfo& ch_info){
    if(os_path_exists(_pwm_path(ch_info)))
        return;
    
    { // scope for f
        string path = _pwm_export_path(ch_info);
	ofstream f(path);
	//ofstream f(_pwm_export_path(ch_info));
	
	if(!f.is_open()) throw runtime_error("Can't open " + path);

        f << ch_info.pwm_id;
    } // scope ends

    string enable_path = _pwm_enable_path(ch_info);
    
   // cerr << "[DEBUG] _export_pwm, enable_path: " << enable_path << endl;
	
    int time_count = 0;
    while (!os_access(enable_path, R_OK | W_OK)){
        this_thread::sleep_for(chrono::milliseconds(10));
	if(time_count++ > 100) throw runtime_error("Permission denied: path: " + enable_path + "\n Please configure permissions or use the root user to run this.");
    }
}


void _unexport_pwm(const ChannelInfo& ch_info){
    ofstream f(_pwm_unexport_path(ch_info));
    f << ch_info.pwm_id;
}


void _set_pwm_period(const ChannelInfo& ch_info, const int period_ns){
    ofstream f(_pwm_period_path(ch_info));
    f << period_ns;
}


void _set_pwm_duty_cycle(const ChannelInfo& ch_info, const int duty_cycle_ns){
    ofstream f(_pwm_duty_cycle_path(ch_info));
    f << duty_cycle_ns;
}


void _enable_pwm(const ChannelInfo& ch_info){
    ofstream f(_pwm_enable_path(ch_info));
    f << 1;
}


void _disable_pwm(const ChannelInfo& ch_info){
    ofstream f(_pwm_enable_path(ch_info));
    f << 0;
}


void  _cleanup_one(const ChannelInfo& ch_info){
    Directions app_cfg = _channel_configuration[ch_info.channel];
    if (app_cfg == HARD_PWM){
        _disable_pwm(ch_info);
        _unexport_pwm(ch_info);
    }
    else{
        // event.event_cleanup(ch_info.gpio)  // not implemented yet
        _unexport_gpio(ch_info.gpio);
    }
    _channel_configuration.erase(ch_info.channel);
}


void _cleanup_all(){
    for (const auto& _pair : _channel_configuration){
        const auto& channel = _pair.first;
        ChannelInfo ch_info = _channel_to_info(channel);
        _cleanup_one(ch_info);
    }
     _gpio_mode = NumberingModes::None;
}




//==================================================================================
// APIs

extern const string GPIO::model = _get_model();
extern const string GPIO::JETSON_INFO = _get_JETSON_INFO();


/* Function used to enable/disable warnings during setup and cleanup. */
void GPIO::setwarnings(bool state){
    _gpio_warnings = state;
}


// Function used to set the pin mumbering mode. 
// Possible mode values are BOARD, BCM, TEGRA_SOC and CVM
void GPIO::setmode(NumberingModes mode){
    try{
        // check if a different mode has been set
        if(_gpio_mode != NumberingModes::None && mode != _gpio_mode)
            throw runtime_error("A different mode has already been set!");
        
        _channel_data = _channel_data_by_mode.at(mode);
        _gpio_mode = mode;
    }
    catch(exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: setmode())" << endl;
        throw false;
    }
}


// Function used to get the currently set pin numbering mode
NumberingModes GPIO::getmode(){
    return _gpio_mode;
}


/* Function used to setup individual pins or lists/tuples of pins as
   Input or Output. direction must be IN or OUT, initial must be 
   HIGH or LOW and is only valid when direction is OUT  */
void GPIO::setup(const string& channel, Directions direction, int initial){
    try{
        ChannelInfo ch_info = _channel_to_info(channel, true);

        if (_gpio_warnings){
            Directions sysfs_cfg = _sysfs_channel_configuration(ch_info);
            Directions app_cfg = _app_channel_configuration(ch_info);

        if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN){
            cerr << "[WARNING] This channel is already in use, continuing anyway. Use GPIO::setwarnings(false) to disable warnings.\n";
        }
            
        }

        if(direction == OUT){
            _setup_single_out(ch_info, initial);
        }
        else{ // IN
            if(initial != -1) throw runtime_error("initial parameter is not valid for inputs");
            _setup_single_in(ch_info);
        }
    }
    catch(exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: setup())" << endl;
        throw false;
    }
}


void GPIO::setup(int channel, Directions direction, int initial){
    setup(to_string(channel), direction, initial);
}


/* Function used to cleanup channels at the end of the program.
   If no channel is provided, all channels are cleaned */
void GPIO::cleanup(const string& channel){
    try{
        // warn if no channel is setup
        if (_gpio_mode == NumberingModes::None && _gpio_warnings){
            cerr << "[WARNING] No channels have been set up yet - nothing to clean up! "
                    "Try cleaning up at the end of your program instead!";
            return;
        }

        // clean all channels if no channel param provided
        if (channel == "None"){
            _cleanup_all();
            return;
        }

        ChannelInfo ch_info = _channel_to_info(channel);
        if (_channel_configuration.find(ch_info.channel) != _channel_configuration.end()){
            _cleanup_one(ch_info);
        }
    }
    catch (exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: cleanup())" << endl;
        throw false;
    }
}

void GPIO::cleanup(int channel){
    string str_channel = to_string(channel);
    cleanup(str_channel);
}

/* Function used to return the current value of the specified channel.
   Function returns either HIGH or LOW */
int GPIO::input(const string& channel){
    try{
        ChannelInfo ch_info = _channel_to_info(channel, true);

        Directions app_cfg = _app_channel_configuration(ch_info);

        if (app_cfg != IN && app_cfg != OUT)
            throw runtime_error("You must setup() the GPIO channel first");

        { // scope for value
            ifstream value(string(_SYSFS_ROOT) + "/gpio" + to_string(ch_info.gpio) + "/value");
            int value_read;
            value >> value_read;
            return value_read;
        } // scope ends
    }
    catch (exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: input())" << endl;
        throw false;
    }
}

int GPIO::input(int channel){
    input(to_string(channel));
}


/* Function used to set a value to a channel.
   Values must be either HIGH or LOW */
void GPIO::output(const string& channel, int value){
    try{
        ChannelInfo ch_info = _channel_to_info(channel, true);
        // check that the channel has been set as output
        if(_app_channel_configuration(ch_info) != OUT)
            throw runtime_error("The GPIO channel has not been set up as an OUTPUT");
        _output_one(ch_info.gpio, value);
    }
    catch (exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: output())" << endl;
        throw false;
    }
}

void GPIO::output(int channel, int value){
    output(to_string(channel), value);
}


/* Function used to check the currently set function of the channel specified. */
Directions GPIO::gpio_function(const string& channel){
    try{
        ChannelInfo ch_info = _channel_to_info(channel);
        return _sysfs_channel_configuration(ch_info);
    }
    catch (exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: gpio_function())" << endl;
        throw false;
    }
}

Directions GPIO::gpio_function(int channel){
    gpio_function(to_string(channel));
}



//PWM class ==========================================================
struct GPIO::PWM::Impl {
    ChannelInfo _ch_info;
    bool _started;
    int _frequency_hz;
    int _period_ns;
    double _duty_cycle_percent;
    int _duty_cycle_ns;
    ~Impl() = default;

    void _reconfigure(int frequency_hz, double duty_cycle_percent, bool start = false);
};

void GPIO::PWM::Impl::_reconfigure(int frequency_hz, double duty_cycle_percent, bool start){
    cerr << "[DEBUG] PWM.pImpl->_reconfigure begin" << endl;

    if (duty_cycle_percent < 0.0 || duty_cycle_percent > 100.0)
            throw runtime_error("invalid duty_cycle_percent");
    bool restart = start || _started;

    if (_started){
        _started = false;
        _disable_pwm(_ch_info);
    }

    _frequency_hz = frequency_hz;
    _period_ns = int(1000000000.0 / frequency_hz);
    _set_pwm_period(_ch_info, _period_ns);

    _duty_cycle_percent = duty_cycle_percent;
    _duty_cycle_ns = int(_period_ns * (duty_cycle_percent / 100.0));
    _set_pwm_duty_cycle(_ch_info, _duty_cycle_ns);

    if (restart){
        _enable_pwm(_ch_info);
        _started = true;
    }

    cerr << "[DEBUG] PWM.pImpl->_reconfigure end" << endl;
}


GPIO::PWM::PWM(int channel, int frequency_hz)
    : pImpl(make_unique<Impl>(Impl{_channel_to_info(to_string(channel), false, true), 
            false, 0, 0, 0.0, 0}))  //temporary values
{
    try{
	cerr << "[DEBUG] PWM cunstroctor begin" << endl;

        Directions app_cfg = _app_channel_configuration(pImpl->_ch_info);
        if(app_cfg == HARD_PWM)
            throw runtime_error("Can't create duplicate PWM objects");
        /*
        Apps typically set up channels as GPIO before making them be PWM,
        because RPi.GPIO does soft-PWM. We must undo the GPIO export to
        allow HW PWM to run on the pin.
        */
        if(app_cfg == IN || app_cfg == OUT) cleanup(channel);

        if (_gpio_warnings){
            auto sysfs_cfg = _sysfs_channel_configuration(pImpl->_ch_info);
            app_cfg = _app_channel_configuration(pImpl->_ch_info);

            // warn if channel has been setup external to current program
            if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN){
                cerr <<  "[WARNING] This channel is already in use, continuing anyway. "
                            "Use GPIO::setwarnings(false) to disable warnings" << endl;
            }
        }
        
        _export_pwm(pImpl->_ch_info);
        pImpl->_reconfigure(frequency_hz, 50.0);
        _channel_configuration[to_string(channel)] = HARD_PWM;

    }
    catch (exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: PWM::PWM())" << endl;
        throw false;
    }

    cerr << "[DEBUG] PWM constructor end!" << endl;
}

GPIO::PWM::~PWM(){
    auto itr = _channel_configuration.find(pImpl->_ch_info.channel);
    if (itr == _channel_configuration.end() || itr->second != HARD_PWM){
        /* The user probably ran cleanup() on the channel already, so avoid
           attempts to repeat the cleanup operations. */
       return;
    }
    try{
        stop();
        _unexport_pwm(pImpl->_ch_info);
        _channel_configuration.erase(pImpl->_ch_info.channel);
        }
    catch(bool b){
        cerr << "[Exception] ~PWM Exception! shut down the program." << endl;
        _cleanup_all();
        terminate();
    }
}

void GPIO::PWM::start(double  duty_cycle_percent){
    cerr << "[DEBUG] PWM::start begin!" << endl;
    try{
        pImpl->_reconfigure(pImpl->_frequency_hz, duty_cycle_percent, true);
    }
    catch(exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: PWM::start())" << endl;
        throw false;
    }
    cerr << "[DEBUG] PWM::start end!" << endl;
}

void GPIO::PWM::ChangeFrequency(int frequency_hz){
    try{
        pImpl->_reconfigure(frequency_hz, pImpl->_duty_cycle_percent);
    }
    catch(exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: PWM::ChangeFrequency())" << endl;
        throw false;
    }
}

void GPIO::PWM::ChangeDutyCycle(double duty_cycle_percent){
    try{
        pImpl->_reconfigure(pImpl->_frequency_hz, duty_cycle_percent);
    }
    catch(exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: PWM::ChangeDutyCycle())" << endl;
        throw false;
    }
}

void GPIO::PWM::stop(){
    try{
        if (!pImpl->_started)
            return;
        
        _disable_pwm(pImpl->_ch_info);
    }
    catch(exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: PWM::stop())" << endl;
        throw false;
    }
}

//====================================================================


//=========================== Originally added ===========================
struct _cleaner{
private:
    _cleaner() = default;

public:
    _cleaner(const _cleaner&) = delete;
    _cleaner& operator=(const _cleaner&) = delete;

    static _cleaner& get_instance(){
        static _cleaner singleton;
        return singleton;
    }

    ~_cleaner(){
        try{  
            // When the user forgot to call cleanup() at the end of the program,
            // _cleaner object will call it.
            _cleanup_all(); 
           // cout << "[DEBUG] ~_cleaner() worked" << endl;
        }
        catch(exception& e){
            cerr << "Exception: " << e.what() << endl;
            cerr << "Exception from destructor of _cleaner class." << endl;
        }
    }
};

// AutoCleaner will be destructed at the end of the program, and call _cleanup_all().
// It COULD cause a problem because at the end of the program,
// _channel_configuration and _gpio_mode MUST NOT be destructed before AutoCleaner.
// But the static objects are destructed in the reverse order of construction,
// and objects defined in the same compilation unit will be constructed in the order of definition.
// So it's supposed to work properly. 
_cleaner& AutoCleaner = _cleaner::get_instance(); 

//=========================================================================
