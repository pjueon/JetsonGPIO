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
#include <iterator>
#include <chrono>
#include <thread>

#include "JetsonGPIO.h"

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

// Numbering Modes
enum class GPIO::NumberingModes{ BOARD, BCM, TEGRA_SOC, CVM, None };

// The use CAN'T use NumberingModes::None
extern const NumberingModes GPIO::BOARD = NumberingModes::BOARD;
extern const NumberingModes GPIO::BCM = NumberingModes::BCM;
extern const NumberingModes GPIO::TEGRA_SOC = NumberingModes::TEGRA_SOC;
extern const NumberingModes GPIO::CVM = NumberingModes::CVM;


//========================================================
// C++ implementation of some python functions

bool startswith(const string& s, const string& prefix) {
	// cerr << "[DEBUG] startswith begin. s: " << s << ", prefix: " << prefix << endl;	

	size_t pre_size = prefix.size();
	if (s.size() < pre_size) return false;
	
	return prefix == s.substr(0, prefix.size());
}

vector<string> split(const string& s, const char d){
    stringstream buffer(s);

    string tmp;
    vector<string> outputVector;

    while (getline(buffer, tmp, d))
        outputVector.push_back(tmp);

    return outputVector;
}

bool os_access(const string& path, int mode){  // os.access
    return access(path.c_str(), mode) == 0;
}

vector<string> os_listdir(const string& path){  // os.listdir
    // cerr << "[DEBUG] os_listdir begin." << endl;

    DIR *dir;
    struct dirent *ent;
    vector<string> outputVector;

    if ((dir = opendir (path.c_str())) != nullptr) {
        while ((ent = readdir (dir)) != nullptr) {
	    // cerr << "[DEBUG] fn: " << ent->d_name << endl;
            outputVector.emplace_back(ent->d_name);
        }
    closedir (dir);
    return outputVector;
    } 
    else {
        throw runtime_error("could not open directory: " + path);
    }
}

bool os_path_exists(const string& path){  // os.path.exists
    return os_access(path, F_OK);
}

string strip(const string& s){
    int start_idx = 0;
    int total = s.size();
    int end_idx = total - 1;
    for (; start_idx < total; start_idx++){
        if(!isspace(s[start_idx]))
            break;
    }
    if(start_idx == total)
        return "";
    for(; end_idx > start_idx; end_idx--){
        if(!isspace(s[end_idx]))
            break;
    }
    return s.substr(start_idx, end_idx - start_idx + 1);
}

//========================================================


// ========================================= Begin of "gpio_pin_data.py" =========================================

/* These vectors contain all the relevant GPIO data for each Jetson 
   The values are use to generate dictionaries that map the corresponding pin
   mode numbers to the Linux GPIO pin number and GPIO chip directory */

struct _GPIO_PIN_DEF{
    const int LinuxPin;           // Linux GPIO pin number
    const string SysfsDir;        // GPIO chip sysfs directory
    const string BoardPin;        // Pin number (BOARD mode)
    const string BCMPin;          // Pin number (BCM mode)
    const string CVMPin;          // Pin name (CVM mode)
    const string TEGRAPin;        // Pin name (TEGRA_SOC mode)
    const string PWMSysfsDir;     // PWM chip sysfs directory
    const int PWMID;              // PWM ID within PWM chip
};  

struct _GPIO_PIN_INFO{
    const int P1_REVISION;
    const string RAM;
    const string REVISION;
    const string TYPE;
    const string MANUFACTURER;
    const string PROCESSOR;
};

const vector<_GPIO_PIN_DEF> JETSON_XAVIER_PIN_DEFS = {
    { 134, "/sys/devices/2200000.gpio", "7", "4", "MCLK05", "SOC_GPIO42", "None", -1 },
    { 140, "/sys/devices/2200000.gpio", "11", "17", "UART1_RTS", "UART1_RTS", "None", -1 },
    { 63, "/sys/devices/2200000.gpio", "12", "18", "I2S2_CLK", "DAP2_SCLK", "None", -1 },
    { 136, "/sys/devices/2200000.gpio", "13", "27", "PWM01", "SOC_GPIO44", "/sys/devices/32f0000.pwm", 0 },
    // Older versions of L4T don"t enable this PWM controller in DT, so this PWM
    // channel may not be available.
    { 105, "/sys/devices/2200000.gpio", "15", "22", "GPIO27", "SOC_GPIO54", "/sys/devices/3280000.pwm", 0 },
    { 8, "/sys/devices/c2f0000.gpio", "16", "23", "GPIO8", "CAN1_STB", "None", -1 },
    { 56, "/sys/devices/2200000.gpio", "18", "24", "GPIO35", "SOC_GPIO12", "/sys/devices/32c0000.pwm", 0 },
    { 205, "/sys/devices/2200000.gpio", "19", "10", "SPI1_MOSI", "SPI1_MOSI", "None", -1 },
    { 204, "/sys/devices/2200000.gpio", "21", "9", "SPI1_MISO", "SPI1_MISO", "None", -1 },
    { 129, "/sys/devices/2200000.gpio", "22", "25", "GPIO17", "SOC_GPIO21", "None", -1 },
    { 203, "/sys/devices/2200000.gpio", "23", "11", "SPI1_CLK", "SPI1_SCK", "None", -1 },
    { 206, "/sys/devices/2200000.gpio", "24", "8", "SPI1_CS0_N", "SPI1_CS0_N", "None", -1},
    { 207, "/sys/devices/2200000.gpio", "26", "7", "SPI1_CS1_N", "SPI1_CS1_N", "None", -1},
    { 3, "/sys/devices/c2f0000.gpio", "29", "5", "CAN0_DIN", "CAN0_DIN", "None", -1 },
    { 2, "/sys/devices/c2f0000.gpio", "31", "6", "CAN0_DOUT", "CAN0_DOUT", "None", -1 },
    { 9, "/sys/devices/c2f0000.gpio", "32", "12", "GPIO9", "CAN1_EN", "None", -1 },
    { 0, "/sys/devices/c2f0000.gpio", "33", "13", "CAN1_DOUT", "CAN1_DOUT", "None", -1 },
    { 66, "/sys/devices/2200000.gpio", "35", "19", "I2S2_FS", "DAP2_FS", "None", -1 },
    // Input-only (due to base board)
    { 141, "/sys/devices/2200000.gpio", "36", "16", "UART1_CTS", "UART1_CTS", "None", -1 },
    { 1, "/sys/devices/c2f0000.gpio", "37", "26", "CAN1_DIN", "CAN1_DIN", "None", -1 },
    { 65, "/sys/devices/2200000.gpio", "38", "20", "I2S2_DIN", "DAP2_DIN", "None", -1 },
    { 64, "/sys/devices/2200000.gpio", "40", "21", "I2S2_DOUT", "DAP2_DOUT", "None", -1 }
};

const vector<string> compats_xavier = {    
    "nvidia,p2972-0000",
    "nvidia,p2972-0006",
    "nvidia,jetson-xavier"
    };


const vector<_GPIO_PIN_DEF> JETSON_TX2_PIN_DEFS = {
    { 76, "/sys/devices/2200000.gpio", "7", "4", "AUDIO_MCLK", "AUD_MCLK", "None", -1 },
    // Output-only (due to base board)
    { 146, "/sys/devices/2200000.gpio", "11", "17", "UART0_RTS", "UART1_RTS", "None", -1 },
    { 72, "/sys/devices/2200000.gpio", "12", "18", "I2S0_CLK", "DAP1_SCLK", "None", -1 },
    { 77, "/sys/devices/2200000.gpio", "13", "27", "GPIO20_AUD_INT", "GPIO_AUD0", "None", -1 },
    { 15, "/sys/devices/3160000.i2c/i2c-0/0-0074", "15", "22", "GPIO_EXP_P17", "GPIO_EXP_P17", "None", -1 },
    // Input-only (due to module):
    { 40, "/sys/devices/c2f0000.gpio", "16", "23", "AO_DMIC_IN_DAT", "CAN_GPIO0", "None", -1 },
    { 161, "/sys/devices/2200000.gpio", "18", "24", "GPIO16_MDM_WAKE_AP", "GPIO_MDM2", "None", -1 },
    { 109, "/sys/devices/2200000.gpio", "19", "10", "SPI1_MOSI", "GPIO_CAM6", "None", -1 },
    { 108, "/sys/devices/2200000.gpio", "21", "9", "SPI1_MISO", "GPIO_CAM5", "None", -1 },
    { 14, "/sys/devices/3160000.i2c/i2c-0/0-0074", "22", "25", "GPIO_EXP_P16", "GPIO_EXP_P16", "None", -1 },
    { 107, "/sys/devices/2200000.gpio", "23", "11", "SPI1_CLK", "GPIO_CAM4", "None", -1 },
    { 110, "/sys/devices/2200000.gpio", "24", "8", "SPI1_CS0", "GPIO_CAM7", "None", -1 },
    { -1, "None", "26", "7", "SPI1_CS1", "None", "None", -1 },
    { 78, "/sys/devices/2200000.gpio", "29", "5", "GPIO19_AUD_RST", "GPIO_AUD1", "None", -1 },
    { 42, "/sys/devices/c2f0000.gpio", "31", "6", "GPIO9_MOTION_INT", "CAN_GPIO2", "None", -1 },
    // Output-only (due to module):
    { 41, "/sys/devices/c2f0000.gpio", "32", "12", "AO_DMIC_IN_CLK", "CAN_GPIO1", "None", -1 },
    { 69, "/sys/devices/2200000.gpio", "33", "13", "GPIO11_AP_WAKE_BT", "GPIO_PQ5", "None", -1 },
    { 75, "/sys/devices/2200000.gpio", "35", "19", "I2S0_LRCLK", "DAP1_FS", "None", -1 },
    // Input-only (due to base board) IF NVIDIA debug card NOT plugged in
    // Output-only (due to base board) IF NVIDIA debug card plugged in
    { 147, "/sys/devices/2200000.gpio", "36", "16", "UART0_CTS", "UART1_CTS", "None", -1 },
    { 68, "/sys/devices/2200000.gpio", "37", "26", "GPIO8_ALS_PROX_INT", "GPIO_PQ4", "None", -1 },
    { 74, "/sys/devices/2200000.gpio", "38", "20", "I2S0_SDIN", "DAP1_DIN", "None", -1 },
    { 73, "/sys/devices/2200000.gpio", "40", "21", "I2S0_SDOUT", "DAP1_DOUT", "None", -1}
};

const vector<string> compats_tx2 = {    
    "nvidia,p2771-0000",
    "nvidia,p2771-0888",
    "nvidia,p3489-0000",
    "nvidia,lightning",
    "nvidia,quill",
    "nvidia,storm"
    };


const vector<_GPIO_PIN_DEF> JETSON_TX1_PIN_DEFS = {
    { 216, "/sys/devices/6000d000.gpio", "7", "4", "AUDIO_MCLK", "AUD_MCLK", "None", -1 },
    // Output-only (due to base board)
    { 162, "/sys/devices/6000d000.gpio", "11", "17", "UART0_RTS", "UART1_RTS", "None", -1 },
    { 11, "/sys/devices/6000d000.gpio", "12", "18", "I2S0_CLK", "DAP1_SCLK", "None", -1 },
    { 38, "/sys/devices/6000d000.gpio", "13", "27", "GPIO20_AUD_INT", "GPIO_PE6", "None", -1 },
    { 15, "/sys/devices/7000c400.i2c/i2c-1/1-0074", "15", "22", "GPIO_EXP_P17", "GPIO_EXP_P17", "None", -1 },
    { 37, "/sys/devices/6000d000.gpio", "16", "23", "AO_DMIC_IN_DAT", "DMIC3_DAT", "None", -1 },
    { 184, "/sys/devices/6000d000.gpio", "18", "24", "GPIO16_MDM_WAKE_AP", "MODEM_WAKE_AP", "None", -1 },
    { 16, "/sys/devices/6000d000.gpio", "19", "10", "SPI1_MOSI", "SPI1_MOSI", "None", -1 },
    { 17, "/sys/devices/6000d000.gpio", "21", "9", "SPI1_MISO", "SPI1_MISO", "None", -1 },
    { 14, "/sys/devices/7000c400.i2c/i2c-1/1-0074", "22", "25", "GPIO_EXP_P16", "GPIO_EXP_P16", "None", -1 },
    { 18, "/sys/devices/6000d000.gpio", "23", "11", "SPI1_CLK", "SPI1_SCK", "None", -1 },
    { 19, "/sys/devices/6000d000.gpio", "24", "8", "SPI1_CS0", "SPI1_CS0", "None", -1 },
    { 20, "/sys/devices/6000d000.gpio", "26", "7", "SPI1_CS1", "SPI1_CS1", "None", -1 },
    { 219, "/sys/devices/6000d000.gpio", "29", "5", "GPIO19_AUD_RST", "GPIO_X1_AUD", "None", -1 },
    { 186, "/sys/devices/6000d000.gpio", "31", "6", "GPIO9_MOTION_INT", "MOTION_INT", "None", -1 },
    { 36, "/sys/devices/6000d000.gpio", "32", "12", "AO_DMIC_IN_CLK", "DMIC3_CLK", "None", -1 },
    { 63, "/sys/devices/6000d000.gpio", "33", "13", "GPIO11_AP_WAKE_BT", "AP_WAKE_NFC", "None", -1 },
    { 8, "/sys/devices/6000d000.gpio", "35", "19", "I2S0_LRCLK", "DAP1_FS", "None", -1 },
    // Input-only (due to base board) IF NVIDIA debug card NOT plugged in
    // Input-only (due to base board) (always reads fixed value) IF NVIDIA debug card plugged in
    { 163, "/sys/devices/6000d000.gpio", "36", "16", "UART0_CTS", "UART1_CTS", "None", -1 },
    { 187, "/sys/devices/6000d000.gpio", "37", "26", "GPIO8_ALS_PROX_INT", "ALS_PROX_INT", "None", -1 },
    { 9, "/sys/devices/6000d000.gpio", "38", "20", "I2S0_SDIN", "DAP1_DIN", "None", -1 },
    { 10, "/sys/devices/6000d000.gpio", "40", "21", "I2S0_SDOUT", "DAP1_DOUT", "None", -1 }
}; 

const vector<string> compats_tx1 = {    
    "nvidia,p2371-2180",
    "nvidia,jetson-cv"
    };


const vector<_GPIO_PIN_DEF> JETSON_NANO_PIN_DEFS = {
    { 216, "/sys/devices/6000d000.gpio", "7", "4", "GPIO9", "AUD_MCLK", "None", -1 },
    { 50, "/sys/devices/6000d000.gpio", "11", "17", "UART1_RTS", "UART2_RTS", "None", -1 },
    { 79, "/sys/devices/6000d000.gpio", "12", "18", "I2S0_SCLK", "DAP4_SCLK", "None", -1 },
    { 14, "/sys/devices/6000d000.gpio", "13", "27", "SPI1_SCK", "SPI2_SCK", "None", -1 },
    { 194, "/sys/devices/6000d000.gpio", "15", "22", "GPIO12", "LCD_TE", "None", -1 },
    { 232, "/sys/devices/6000d000.gpio", "16", "23", "SPI1_CS1", "SPI2_CS1", "None", -1 },
    { 15, "/sys/devices/6000d000.gpio", "18", "24", "SPI1_CS0", "SPI2_CS0", "None", -1 },
    { 16, "/sys/devices/6000d000.gpio", "19", "10", "SPI0_MOSI", "SPI1_MOSI", "None", -1 },
    { 17, "/sys/devices/6000d000.gpio", "21", "9", "SPI0_MISO", "SPI1_MISO", "None", -1 },
    { 13, "/sys/devices/6000d000.gpio", "22", "25", "SPI1_MISO", "SPI2_MISO", "None", -1 },
    { 18, "/sys/devices/6000d000.gpio", "23", "11", "SPI0_SCK", "SPI1_SCK", "None", -1 },
    { 19, "/sys/devices/6000d000.gpio", "24", "8", "SPI0_CS0", "SPI1_CS0", "None", -1 },
    { 20, "/sys/devices/6000d000.gpio", "26", "7", "SPI0_CS1", "SPI1_CS1", "None", -1 },
    { 149, "/sys/devices/6000d000.gpio", "29", "5", "GPIO01", "CAM_AF_EN", "None", -1 },
    { 200, "/sys/devices/6000d000.gpio", "31", "6", "GPIO11", "GPIO_PZ0", "None", -1 },
    // Older versions of L4T have a DT bug which instantiates a bogus device
    // which prevents this library from using this PWM channel.
    { 168, "/sys/devices/6000d000.gpio", "32", "12", "GPIO07", "LCD_BL_PW", "/sys/devices/7000a000.pwm", 0 },
    { 38, "/sys/devices/6000d000.gpio", "33", "13", "GPIO13", "GPIO_PE6", "/sys/devices/7000a000.pwm", 2 },
    { 76, "/sys/devices/6000d000.gpio", "35", "19", "I2S0_FS", "DAP4_FS", "None", -1 },
    { 51, "/sys/devices/6000d000.gpio", "36", "16", "UART1_CTS", "UART2_CTS", "None", -1 },
    { 12, "/sys/devices/6000d000.gpio", "37", "26", "SPI1_MOSI", "SPI2_MOSI", "None", -1 },
    { 77, "/sys/devices/6000d000.gpio", "38", "20", "I2S0_DIN", "DAP4_DIN", "None", -1 },
    { 78, "/sys/devices/6000d000.gpio", "40", "21", "I2S0_DOUT", "DAP4_DOUT", "None", -1 }
};

const vector<string> compats_nano = {    
    "nvidia,p3450-0000",
    "nvidia,p3450-0002",
    "nvidia,jetson-nano"
    };


const map<Model, vector<_GPIO_PIN_DEF>> PIN_DEFS_MAP = {
    { Model::JETSON_XAVIER, JETSON_XAVIER_PIN_DEFS },
    { Model::JETSON_TX2, JETSON_TX2_PIN_DEFS },
    { Model::JETSON_TX1, JETSON_TX1_PIN_DEFS },
    { Model::JETSON_NANO, JETSON_NANO_PIN_DEFS },
};

const map<Model, _GPIO_PIN_INFO> JETSON_INFO_MAP = {
    { Model::JETSON_XAVIER, {1, "16384M", "Unknown", "Jetson Xavier", "NVIDIA", "ARM Carmel"} },
    { Model::JETSON_TX2, {1, "8192M", "Unknown", "Jetson TX2", "NVIDIA", "ARM A57 + Denver"} },
    { Model::JETSON_TX1, {1, "4096M", "Unknown", "Jetson TX1", "NVIDIA", "ARM A57"} },
    { Model::JETSON_NANO, {1, "4096M", "Unknown", "Jetson nano", "NVIDIA", "ARM A57"} }
};



struct ChannelInfo{
    ChannelInfo(const string& channel, const string& gpio_chip_dir, 
		int chip_gpio, int gpio, const string& pwm_chip_dir, 
		int pwm_id)
	    : channel(channel),
	      gpio_chip_dir(gpio_chip_dir),
	      chip_gpio(chip_gpio), 
	      gpio(gpio), 
	      pwm_chip_dir(pwm_chip_dir),
	      pwm_id(pwm_id) 
	    {}
    ChannelInfo(const ChannelInfo&) = default;

    const string channel;
    const string gpio_chip_dir;
    const int chip_gpio;
    const int gpio;
    const string pwm_chip_dir;
    const int pwm_id;
};


struct GPIO_data{
    Model model;
    _GPIO_PIN_INFO pin_info;
    map<NumberingModes, map<string, ChannelInfo>> channel_data;
};

bool ids_warned = false;


GPIO_data get_data(){
//    cerr << "[DEBUG] get_data begin" << endl;
    try{
        const string compatible_path = "/proc/device-tree/compatible";
        const string ids_path = "/proc/device-tree/chosen/plugin-manager/ids";

        set<string> compatibles;
        
        { // scope for f:
            ifstream f(compatible_path);
            stringstream buffer;
            
            buffer << f.rdbuf();
            string tmp_str = buffer.str();
            vector<string> _vec_compatibles(split(tmp_str, '\x00'));
            // convert to std::set
            copy(_vec_compatibles.begin(), _vec_compatibles.end(), inserter(compatibles, compatibles.end()));
        } // scope ends

        auto matches = [&compatibles](const vector<string>& vals) {
            // cerr << "[DEBUG] matches begin" << endl;
        for(const auto& v : vals)
                if(compatibles.find(v) != compatibles.end()) return true;     

            return false;
        };

        auto find_pmgr_board = [&](const string& prefix){
    //        cerr << "[DEBUG] find_pmgr_board begin" << endl;

        if (!os_path_exists(ids_path)){ 
                if (ids_warned == false){
                    ids_warned = true;
                    string msg = "WARNING: Plugin manager information missing from device tree.\n"
                                "WARNING: Cannot determine whether the expected Jetson board is present." ;
                    cerr << msg;
                }
                return "None";
            }

            vector<string> files = os_listdir(ids_path);
            for (const string& file : files){
            // cerr << "[DEBUG] check, file: " << file << ", prefix: " << prefix <<endl; 
                if (startswith(file, prefix))
                    return file.c_str();
            }

        return "None";
        };
        
        auto warn_if_not_carrier_board = [&](string carrier_board){
            // cerr << "[DEBUG] warn_if_not_carrier_board begin" << endl;

        string found = find_pmgr_board(carrier_board + "-");
            if (found == "None"){
                string msg = "WARNING: Carrier board is not from a Jetson Developer Kit.\n"
                            "WARNNIG: Jetson.GPIO library has not been verified with this carrier board,\n"
                            "WARNING: and in fact is unlikely to work correctly.";
                cerr << msg << endl;
            }
        };

        Model model;

        if (matches(compats_tx1)){
            model = Model::JETSON_TX1;
            warn_if_not_carrier_board("2597");
        }
        else if (matches(compats_tx2)){
            model = Model::JETSON_TX2;
            warn_if_not_carrier_board("2597");
        }
        else if (matches(compats_xavier)){
            model = Model::JETSON_XAVIER;
            warn_if_not_carrier_board("2822");
        }
        else if (matches(compats_nano)){
        // cerr << "[DEBUG] model = Model::JETSON_NANO" << endl;
            model = Model::JETSON_NANO;
            string module_id = find_pmgr_board("3448");

            if (module_id == "None")
                throw runtime_error("Could not determine Jetson Nano module revision");
            string revision = split(module_id, '-').back();
            // Revision is an ordered string, not a decimal integer
            if (revision < "200")
                throw runtime_error("Jetson Nano module revision must be A02 or later");
            
            warn_if_not_carrier_board("3449");
        }
        else{
            throw runtime_error("Could not determine Jetson model");
        }

        vector<_GPIO_PIN_DEF> pin_defs = PIN_DEFS_MAP.at(model);
        _GPIO_PIN_INFO jetson_info = JETSON_INFO_MAP.at(model);

        map<string, int> gpio_chip_base;
        map<string, string> pwm_dirs;

        // Get the gpiochip offsets
        set<string> gpio_chip_dirs;
        for (const auto& pin_def : pin_defs){
            if(pin_def.SysfsDir != "None")
                gpio_chip_dirs.insert(pin_def.SysfsDir);
        }
        for (const auto& gpio_chip_dir : gpio_chip_dirs){
            string gpio_chip_gpio_dir = gpio_chip_dir + "/gpio";
            auto files = os_listdir(gpio_chip_gpio_dir);
            for (const auto& fn : files){
                if (!startswith(fn, "gpiochip"))
                    continue;
                
                string gpiochip_fn = gpio_chip_gpio_dir + "/" + fn + "/base";
                { // scope for f
                    ifstream f(gpiochip_fn);
                    stringstream buffer;
                    buffer << f.rdbuf();
                    gpio_chip_base[gpio_chip_dir] = stoi(strip(buffer.str()));
                    break;
                } // scope ends 
            }
        }


        auto global_gpio_id = [&gpio_chip_base](string gpio_chip_dir, int chip_relative_id){
            if (gpio_chip_dir == "None" || chip_relative_id == -1)
                return -1;
            return gpio_chip_base[gpio_chip_dir] + chip_relative_id;
        };


        auto pwm_dir = [&pwm_dirs](string chip_dir){
            if (chip_dir == "None")
                return "None";
            if (pwm_dirs.find(chip_dir) != pwm_dirs.end())
                return pwm_dirs[chip_dir].c_str();

            string chip_pwm_dir = chip_dir + "/pwm";
            /* Some PWM controllers aren't enabled in all versions of the DT. In
            this case, just hide the PWM function on this pin, but let all other
            aspects of the library continue to work. */
            if (!os_path_exists(chip_pwm_dir))
                return "None";
            auto files = os_listdir(chip_pwm_dir);
            for (const auto& fn : files){
                if (!startswith(fn, "pwmchip"))
                    continue;
                
                string chip_pwm_pwmchip_dir = chip_pwm_dir + "/" + fn;
                pwm_dirs[chip_dir] = chip_pwm_pwmchip_dir;
                return chip_pwm_pwmchip_dir.c_str();
            return "None";
            }
        };


        auto model_data = [&global_gpio_id, &pwm_dir](NumberingModes key, const vector<_GPIO_PIN_DEF>& pin_defs){
            map<string, ChannelInfo> ret;
            
            for (const auto& x : pin_defs){
                string pinName;
                if(key == BOARD){
                    pinName = x.BoardPin;
                }
                else if(key == BCM){
                    pinName = x.BCMPin;
                }
                else if(key == CVM){
                    pinName = x.CVMPin;
                }
                else{ // TEGRA_SOC
                    pinName = x.TEGRAPin;
                }
                
                ret.insert({ pinName, 
                            ChannelInfo{ pinName,
                                        x.SysfsDir,
                                        x.LinuxPin,
                                        global_gpio_id(x.SysfsDir, x.LinuxPin),
                                        pwm_dir(x.PWMSysfsDir),
                                        x.PWMID }                                        
                            }
                        );
            }
            return ret;
        };

        map<NumberingModes, map<string, ChannelInfo>> channel_data = 
        {
            { BOARD, model_data(BOARD, pin_defs) }, 
            { BCM, model_data(BCM, pin_defs) }, 
            { CVM, model_data(CVM, pin_defs) }, 
            { TEGRA_SOC, model_data(TEGRA_SOC, pin_defs) } 
        };

	return {model, jetson_info, channel_data};
    }
    catch(exception& e){
        cerr << "[Exception] " << e.what() << " (catched from: get_data())" << endl;
      	throw false;	
    }
}

// ========================================= End of "gpio_pin_data.py" =========================================

// ========================================= Begin of "gpio.py" =========================================

const string _SYSFS_ROOT = "/sys/class/gpio";

bool CheckPermission(){
    string path1 = _SYSFS_ROOT + "/export";
    string path2 = _SYSFS_ROOT + "/unexport";
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
const char* _get_JETSON_INFO(){
    stringstream ss;
    ss << "[JETSON_INFO]\n";
    ss << "P1_REVISION: " << _JETSON_INFO.P1_REVISION << endl;
    ss << "RAM: " << _JETSON_INFO.RAM << endl;
    ss << "REVISION: " << _JETSON_INFO.REVISION << endl;
    ss << "TYPE: " << _JETSON_INFO.TYPE << endl;
    ss << "MANUFACTURER: " << _JETSON_INFO.MANUFACTURER << endl;
    ss << "PROCESSOR: " << _JETSON_INFO.PROCESSOR << endl;
    return ss.str().c_str();
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

    string gpio_dir = _SYSFS_ROOT + "/gpio" + to_string(ch_info.gpio);
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
    if(os_path_exists(_SYSFS_ROOT + "/gpio" + to_string(gpio) ))
        return;
    { // scope for f_export
        ofstream f_export(_SYSFS_ROOT + "/export");
        f_export << gpio;
    } // scope ends

    string value_path = _SYSFS_ROOT + "/gpio" + to_string(gpio) + "/value";
    
    int time_count = 0;
    while (!os_access(value_path, R_OK | W_OK)){
        this_thread::sleep_for(chrono::milliseconds(10));
	if(time_count++ > 100) throw runtime_error("Permission denied: path: " + value_path + "\n Please configure permissions or use the root user to run this.");
    }
}


void _unexport_gpio(const int gpio){
    if (!os_path_exists(_SYSFS_ROOT + "/gpio" + to_string(gpio)))
        return;

    ofstream f_unexport(_SYSFS_ROOT + "/unexport");
    f_unexport << gpio;
}


void _output_one(const string gpio, const int value){
    ofstream value_file(_SYSFS_ROOT + "/gpio" + gpio + "/value");
    value_file << int(bool(value));
}

void _output_one(const int gpio, const int value){
    _output_one(to_string(gpio), value);
}


void _setup_single_out(const ChannelInfo& ch_info, int initial = -1){
    _export_gpio(ch_info.gpio);

    string gpio_dir_path = _SYSFS_ROOT + "/gpio" + to_string(ch_info.gpio) + "/direction";
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

    string gpio_dir_path = _SYSFS_ROOT + "/gpio" + to_string(ch_info.gpio) + "/direction";
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
            ifstream value(_SYSFS_ROOT + "/gpio" + to_string(ch_info.gpio) + "/value");
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

