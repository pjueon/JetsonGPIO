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

#include "JetsonGPIO.h"
#include "private/gpio_pin_data.h"
#include "private/PythonFunctions.h"
#include "private/etc.h"


using namespace GPIO;
using namespace std;
using namespace std::string_literals; // enables s-suffix for std::string literals

// ========================================= Begin of "gpio_pin_data.py" =========================================



// Global variables are wrapped in singleton pattern in order to avoid
// initialization order of global variables in different compilation units problem
class EntirePinData
{
private:
    EntirePinData();

public:
    /* These vectors contain all the relevant GPIO data for each Jetson Platform.
    The values are use to generate dictionaries that map the corresponding pin
    mode numbers to the Linux GPIO pin number and GPIO chip directory */
    const vector<PinDefinition> CLARA_AGX_XAVIER_PIN_DEFS;
    const vector<string> compats_clara_agx_xavier;
    const vector<PinDefinition> JETSON_NX_PIN_DEFS;
    const vector<string> compats_nx;
    const vector<PinDefinition> JETSON_XAVIER_PIN_DEFS;
    const vector<string> compats_xavier;
    const vector<PinDefinition> JETSON_TX2_PIN_DEFS;
    const vector<string> compats_tx2;
    const vector<PinDefinition> JETSON_TX1_PIN_DEFS;
    const vector<string> compats_tx1;
    const vector<PinDefinition> JETSON_NANO_PIN_DEFS;
    const vector<string> compats_nano;

    const map<Model, vector<PinDefinition>> PIN_DEFS_MAP;
    const map<Model, PinInfo> JETSON_INFO_MAP;

    EntirePinData(const EntirePinData &) = delete;
    EntirePinData &operator=(const EntirePinData &) = delete;
    ~EntirePinData() = default;

    static EntirePinData &get_instance()
    {
        static EntirePinData singleton{};
        return singleton;
    }
};

EntirePinData::EntirePinData()
    : 
    CLARA_AGX_XAVIER_PIN_DEFS
    {
        { 134, "2200000.gpio", "7", "4", "MCLK05", "SOC_GPIO42", "None", -1 },
        { 140, "2200000.gpio", "11", "17", "UART1_RTS", "UART1_RTS", "None", -1 },
        { 63, "2200000.gpio", "12", "18", "I2S2_CLK", "DAP2_SCLK", "None", -1 },
        { 124, "2200000.gpio", "13", "27", "GPIO32", "SOC_GPIO04", "None", -1 },
        // Older versions of L4T don"t enable this PWM controller in DT, so this PWM
        // channel may not be available.
        { 105, "2200000.gpio", "15", "22", "GPIO27", "SOC_GPIO54", "3280000.pwm", 0 },
        { 8, "c2f0000.gpio", "16", "23", "GPIO8", "CAN1_STB", "None", -1 },
        { 56, "2200000.gpio", "18", "24", "GPIO35", "SOC_GPIO12", "32c0000.pwm", 0 },
        { 205, "2200000.gpio", "19", "10", "SPI1_MOSI", "SPI1_MOSI", "None", -1 },
        { 204, "2200000.gpio", "21", "9", "SPI1_MISO", "SPI1_MISO", "None", -1 },
        { 129, "2200000.gpio", "22", "25", "GPIO17", "SOC_GPIO21", "None", -1 },
        { 203, "2200000.gpio", "23", "11", "SPI1_CLK", "SPI1_SCK", "None", -1 },
        { 206, "2200000.gpio", "24", "8", "SPI1_CS0_N", "SPI1_CS0_N", "None", -1 },
        { 207, "2200000.gpio", "26", "7", "SPI1_CS1_N", "SPI1_CS1_N", "None", -1 },
        { 3, "c2f0000.gpio", "29", "5", "CAN0_DIN", "CAN0_DIN", "None", -1 },
        { 2, "c2f0000.gpio", "31", "6", "CAN0_DOUT", "CAN0_DOUT", "None", -1 },
        { 9, "c2f0000.gpio", "32", "12", "GPIO9", "CAN1_EN", "None", -1 },
        { 0, "c2f0000.gpio", "33", "13", "CAN1_DOUT", "CAN1_DOUT", "None", -1 },
        { 66, "2200000.gpio", "35", "19", "I2S2_FS", "DAP2_FS", "None", -1 },
        // Input-only (due to base board)
        { 141, "2200000.gpio", "36", "16", "UART1_CTS", "UART1_CTS", "None", -1 },
        { 1, "c2f0000.gpio", "37", "26", "CAN1_DIN", "CAN1_DIN", "None", -1 },
        { 65, "2200000.gpio", "38", "20", "I2S2_DIN", "DAP2_DIN", "None", -1 },
        { 64, "2200000.gpio", "40", "21", "I2S2_DOUT", "DAP2_DOUT", "None", -1 }
    },
    compats_clara_agx_xavier
    {
        "nvidia,e3900-0000+p2888-0004"
    },

    JETSON_NX_PIN_DEFS
    {
        { 148, "2200000.gpio", "7", "4", "GPIO09", "AUD_MCLK", "None", -1 },
        { 140, "2200000.gpio", "11", "17", "UART1_RTS", "UART1_RTS", "None", -1 },
        { 157, "2200000.gpio", "12", "18", "I2S0_SCLK", "DAP5_SCLK", "None", -1 },
        { 192, "2200000.gpio", "13", "27", "SPI1_SCK", "SPI3_SCK", "None", -1 },
        { 20, "c2f0000.gpio", "15", "22", "GPIO12", "TOUCH_CLK", "None", -1 },
        { 196, "2200000.gpio", "16", "23", "SPI1_CS1", "SPI3_CS1_N", "None", -1 },
        { 195, "2200000.gpio", "18", "24", "SPI1_CS0", "SPI3_CS0_N", "None", -1 },
        { 205, "2200000.gpio", "19", "10", "SPI0_MOSI", "SPI1_MOSI", "None", -1 },
        { 204, "2200000.gpio", "21", "9", "SPI0_MISO", "SPI1_MISO", "None", -1 },
        { 193, "2200000.gpio", "22", "25", "SPI1_MISO", "SPI3_MISO", "None", -1 },
        { 203, "2200000.gpio", "23", "11", "SPI0_SCK", "SPI1_SCK", "None", -1 },
        { 206, "2200000.gpio", "24", "8", "SPI0_CS0", "SPI1_CS0_N", "None", -1 },
        { 207, "2200000.gpio", "26", "7", "SPI0_CS1", "SPI1_CS1_N", "None", -1 },
        { 133, "2200000.gpio", "29", "5", "GPIO01", "SOC_GPIO41", "None", -1 },
        { 134, "2200000.gpio", "31", "6", "GPIO11", "SOC_GPIO42", "None", -1 },
        { 136, "2200000.gpio", "32", "12", "GPIO07", "SOC_GPIO44", "32f0000.pwm", 0 },
        { 105, "2200000.gpio", "33", "13", "GPIO13", "SOC_GPIO54", "3280000.pwm", 0 },
        { 160, "2200000.gpio", "35", "19", "I2S0_FS", "DAP5_FS", "None", -1 },
        { 141, "2200000.gpio", "36", "16", "UART1_CTS", "UART1_CTS", "None", -1 },
        { 194, "2200000.gpio", "37", "26", "SPI1_MOSI", "SPI3_MOSI", "None", -1 },
        { 159, "2200000.gpio", "38", "20", "I2S0_DIN", "DAP5_DIN", "None", -1 },
        { 158, "2200000.gpio", "40", "21", "I2S0_DOUT", "DAP5_DOUT", "None", -1 }
    },
    compats_nx
    {
        "nvidia,p3509-0000+p3668-0000",
        "nvidia,p3509-0000+p3668-0001",
        "nvidia,p3449-0000+p3668-0000",
        "nvidia,p3449-0000+p3668-0001"
    },

    JETSON_XAVIER_PIN_DEFS
    {
        { 134, "2200000.gpio", "7", "4", "MCLK05", "SOC_GPIO42", "None", -1 },
        { 140, "2200000.gpio", "11", "17", "UART1_RTS", "UART1_RTS", "None", -1 },
        { 63, "2200000.gpio", "12", "18", "I2S2_CLK", "DAP2_SCLK", "None", -1 },
        { 136, "2200000.gpio", "13", "27", "PWM01", "SOC_GPIO44", "32f0000.pwm", 0 },
        // Older versions of L4T don"t enable this PWM controller in DT, so this PWM
        // channel may not be available.
        { 105, "2200000.gpio", "15", "22", "GPIO27", "SOC_GPIO54", "3280000.pwm", 0 },
        { 8, "c2f0000.gpio", "16", "23", "GPIO8", "CAN1_STB", "None", -1 },
        { 56, "2200000.gpio", "18", "24", "GPIO35", "SOC_GPIO12", "32c0000.pwm", 0 },
        { 205, "2200000.gpio", "19", "10", "SPI1_MOSI", "SPI1_MOSI", "None", -1 },
        { 204, "2200000.gpio", "21", "9", "SPI1_MISO", "SPI1_MISO", "None", -1 },
        { 129, "2200000.gpio", "22", "25", "GPIO17", "SOC_GPIO21", "None", -1 },
        { 203, "2200000.gpio", "23", "11", "SPI1_CLK", "SPI1_SCK", "None", -1 },
        { 206, "2200000.gpio", "24", "8", "SPI1_CS0_N", "SPI1_CS0_N", "None", -1},
        { 207, "2200000.gpio", "26", "7", "SPI1_CS1_N", "SPI1_CS1_N", "None", -1},
        { 3, "c2f0000.gpio", "29", "5", "CAN0_DIN", "CAN0_DIN", "None", -1 },
        { 2, "c2f0000.gpio", "31", "6", "CAN0_DOUT", "CAN0_DOUT", "None", -1 },
        { 9, "c2f0000.gpio", "32", "12", "GPIO9", "CAN1_EN", "None", -1 },
        { 0, "c2f0000.gpio", "33", "13", "CAN1_DOUT", "CAN1_DOUT", "None", -1 },
        { 66, "2200000.gpio", "35", "19", "I2S2_FS", "DAP2_FS", "None", -1 },
        // Input-only (due to base board)
        { 141, "2200000.gpio", "36", "16", "UART1_CTS", "UART1_CTS", "None", -1 },
        { 1, "c2f0000.gpio", "37", "26", "CAN1_DIN", "CAN1_DIN", "None", -1 },
        { 65, "2200000.gpio", "38", "20", "I2S2_DIN", "DAP2_DIN", "None", -1 },
        { 64, "2200000.gpio", "40", "21", "I2S2_DOUT", "DAP2_DOUT", "None", -1 }
    },
    compats_xavier
    {
        "nvidia,p2972-0000",
        "nvidia,p2972-0006",
        "nvidia,jetson-xavier"
    },

    JETSON_TX2_PIN_DEFS
    {
        { 76, "2200000.gpio", "7", "4", "AUDIO_MCLK", "AUD_MCLK", "None", -1 },
        // Output-only (due to base board)
        { 146, "2200000.gpio", "11", "17", "UART0_RTS", "UART1_RTS", "None", -1 },
        { 72, "2200000.gpio", "12", "18", "I2S0_CLK", "DAP1_SCLK", "None", -1 },
        { 77, "2200000.gpio", "13", "27", "GPIO20_AUD_INT", "GPIO_AUD0", "None", -1 },
        { 15, "3160000.i2c/i2c-0/0-0074", "15", "22", "GPIO_EXP_P17", "GPIO_EXP_P17", "None", -1 },
        // Input-only (due to module):
        { 40, "c2f0000.gpio", "16", "23", "AO_DMIC_IN_DAT", "CAN_GPIO0", "None", -1 },
        { 161, "2200000.gpio", "18", "24", "GPIO16_MDM_WAKE_AP", "GPIO_MDM2", "None", -1 },
        { 109, "2200000.gpio", "19", "10", "SPI1_MOSI", "GPIO_CAM6", "None", -1 },
        { 108, "2200000.gpio", "21", "9", "SPI1_MISO", "GPIO_CAM5", "None", -1 },
        { 14, "3160000.i2c/i2c-0/0-0074", "22", "25", "GPIO_EXP_P16", "GPIO_EXP_P16", "None", -1 },
        { 107, "2200000.gpio", "23", "11", "SPI1_CLK", "GPIO_CAM4", "None", -1 },
        { 110, "2200000.gpio", "24", "8", "SPI1_CS0", "GPIO_CAM7", "None", -1 },
        { -1, "None", "26", "7", "SPI1_CS1", "None", "None", -1 },
        { 78, "2200000.gpio", "29", "5", "GPIO19_AUD_RST", "GPIO_AUD1", "None", -1 },
        { 42, "c2f0000.gpio", "31", "6", "GPIO9_MOTION_INT", "CAN_GPIO2", "None", -1 },
        // Output-only (due to module):
        { 41, "c2f0000.gpio", "32", "12", "AO_DMIC_IN_CLK", "CAN_GPIO1", "None", -1 },
        { 69, "2200000.gpio", "33", "13", "GPIO11_AP_WAKE_BT", "GPIO_PQ5", "None", -1 },
        { 75, "2200000.gpio", "35", "19", "I2S0_LRCLK", "DAP1_FS", "None", -1 },
        // Input-only (due to base board) IF NVIDIA debug card NOT plugged in
        // Output-only (due to base board) IF NVIDIA debug card plugged in
        { 147, "2200000.gpio", "36", "16", "UART0_CTS", "UART1_CTS", "None", -1 },
        { 68, "2200000.gpio", "37", "26", "GPIO8_ALS_PROX_INT", "GPIO_PQ4", "None", -1 },
        { 74, "2200000.gpio", "38", "20", "I2S0_SDIN", "DAP1_DIN", "None", -1 },
        { 73, "2200000.gpio", "40", "21", "I2S0_SDOUT", "DAP1_DOUT", "None", -1}
    },
    compats_tx2
    {
        "nvidia,p2771-0000",
        "nvidia,p2771-0888",
        "nvidia,p3489-0000",
        "nvidia,lightning",
        "nvidia,quill",
        "nvidia,storm"
    },

    JETSON_TX1_PIN_DEFS
    {
        { 216, "6000d000.gpio", "7", "4", "AUDIO_MCLK", "AUD_MCLK", "None", -1 },
        // Output-only (due to base board)
        { 162, "6000d000.gpio", "11", "17", "UART0_RTS", "UART1_RTS", "None", -1 },
        { 11, "6000d000.gpio", "12", "18", "I2S0_CLK", "DAP1_SCLK", "None", -1 },
        { 38, "6000d000.gpio", "13", "27", "GPIO20_AUD_INT", "GPIO_PE6", "None", -1 },
        { 15, "7000c400.i2c/i2c-1/1-0074", "15", "22", "GPIO_EXP_P17", "GPIO_EXP_P17", "None", -1 },
        { 37, "6000d000.gpio", "16", "23", "AO_DMIC_IN_DAT", "DMIC3_DAT", "None", -1 },
        { 184, "6000d000.gpio", "18", "24", "GPIO16_MDM_WAKE_AP", "MODEM_WAKE_AP", "None", -1 },
        { 16, "6000d000.gpio", "19", "10", "SPI1_MOSI", "SPI1_MOSI", "None", -1 },
        { 17, "6000d000.gpio", "21", "9", "SPI1_MISO", "SPI1_MISO", "None", -1 },
        { 14, "7000c400.i2c/i2c-1/1-0074", "22", "25", "GPIO_EXP_P16", "GPIO_EXP_P16", "None", -1 },
        { 18, "6000d000.gpio", "23", "11", "SPI1_CLK", "SPI1_SCK", "None", -1 },
        { 19, "6000d000.gpio", "24", "8", "SPI1_CS0", "SPI1_CS0", "None", -1 },
        { 20, "6000d000.gpio", "26", "7", "SPI1_CS1", "SPI1_CS1", "None", -1 },
        { 219, "6000d000.gpio", "29", "5", "GPIO19_AUD_RST", "GPIO_X1_AUD", "None", -1 },
        { 186, "6000d000.gpio", "31", "6", "GPIO9_MOTION_INT", "MOTION_INT", "None", -1 },
        { 36, "6000d000.gpio", "32", "12", "AO_DMIC_IN_CLK", "DMIC3_CLK", "None", -1 },
        { 63, "6000d000.gpio", "33", "13", "GPIO11_AP_WAKE_BT", "AP_WAKE_NFC", "None", -1 },
        { 8, "6000d000.gpio", "35", "19", "I2S0_LRCLK", "DAP1_FS", "None", -1 },
        // Input-only (due to base board) IF NVIDIA debug card NOT plugged in
        // Input-only (due to base board) (always reads fixed value) IF NVIDIA debug card plugged in
        { 163, "6000d000.gpio", "36", "16", "UART0_CTS", "UART1_CTS", "None", -1 },
        { 187, "6000d000.gpio", "37", "26", "GPIO8_ALS_PROX_INT", "ALS_PROX_INT", "None", -1 },
        { 9, "6000d000.gpio", "38", "20", "I2S0_SDIN", "DAP1_DIN", "None", -1 },
        { 10, "6000d000.gpio", "40", "21", "I2S0_SDOUT", "DAP1_DOUT", "None", -1 }
    },
    compats_tx1
    {
        "nvidia,p2371-2180",
        "nvidia,jetson-cv"
    },

    JETSON_NANO_PIN_DEFS
    {
        { 216, "6000d000.gpio", "7", "4", "GPIO9", "AUD_MCLK", "None", -1 },
        { 50, "6000d000.gpio", "11", "17", "UART1_RTS", "UART2_RTS", "None", -1 },
        { 79, "6000d000.gpio", "12", "18", "I2S0_SCLK", "DAP4_SCLK", "None", -1 },
        { 14, "6000d000.gpio", "13", "27", "SPI1_SCK", "SPI2_SCK", "None", -1 },
        { 194, "6000d000.gpio", "15", "22", "GPIO12", "LCD_TE", "None", -1 },
        { 232, "6000d000.gpio", "16", "23", "SPI1_CS1", "SPI2_CS1", "None", -1 },
        { 15, "6000d000.gpio", "18", "24", "SPI1_CS0", "SPI2_CS0", "None", -1 },
        { 16, "6000d000.gpio", "19", "10", "SPI0_MOSI", "SPI1_MOSI", "None", -1 },
        { 17, "6000d000.gpio", "21", "9", "SPI0_MISO", "SPI1_MISO", "None", -1 },
        { 13, "6000d000.gpio", "22", "25", "SPI1_MISO", "SPI2_MISO", "None", -1 },
        { 18, "6000d000.gpio", "23", "11", "SPI0_SCK", "SPI1_SCK", "None", -1 },
        { 19, "6000d000.gpio", "24", "8", "SPI0_CS0", "SPI1_CS0", "None", -1 },
        { 20, "6000d000.gpio", "26", "7", "SPI0_CS1", "SPI1_CS1", "None", -1 },
        { 149, "6000d000.gpio", "29", "5", "GPIO01", "CAM_AF_EN", "None", -1 },
        { 200, "6000d000.gpio", "31", "6", "GPIO11", "GPIO_PZ0", "None", -1 },
        // Older versions of L4T have a DT bug which instantiates a bogus device
        // which prevents this library from using this PWM channel.
        { 168, "6000d000.gpio", "32", "12", "GPIO07", "LCD_BL_PW", "7000a000.pwm", 0 },
        { 38, "6000d000.gpio", "33", "13", "GPIO13", "GPIO_PE6", "7000a000.pwm", 2 },
        { 76, "6000d000.gpio", "35", "19", "I2S0_FS", "DAP4_FS", "None", -1 },
        { 51, "6000d000.gpio", "36", "16", "UART1_CTS", "UART2_CTS", "None", -1 },
        { 12, "6000d000.gpio", "37", "26", "SPI1_MOSI", "SPI2_MOSI", "None", -1 },
        { 77, "6000d000.gpio", "38", "20", "I2S0_DIN", "DAP4_DIN", "None", -1 },
        { 78, "6000d000.gpio", "40", "21", "I2S0_DOUT", "DAP4_DOUT", "None", -1 }
    },
    compats_nano
    {
        "nvidia,p3450-0000",
        "nvidia,p3450-0002",
        "nvidia,jetson-nano"
    },

    PIN_DEFS_MAP
    {
        { CLARA_AGX_XAVIER, CLARA_AGX_XAVIER_PIN_DEFS },
        { JETSON_NX, JETSON_NX_PIN_DEFS },
        { JETSON_XAVIER, JETSON_XAVIER_PIN_DEFS },
        { JETSON_TX2, JETSON_TX2_PIN_DEFS },
        { JETSON_TX1, JETSON_TX1_PIN_DEFS },
        { JETSON_NANO, JETSON_NANO_PIN_DEFS }
    },
    JETSON_INFO_MAP
    {
        { CLARA_AGX_XAVIER, {1, "16384M",  "Unknown", "CLARA_AGX_XAVIER", "NVIDIA", "ARM Carmel"} },
        { JETSON_NX, {1, "16384M", "Unknown", "Jetson NX", "NVIDIA", "ARM Carmel"} },
        { JETSON_XAVIER, {1, "16384M", "Unknown", "Jetson Xavier", "NVIDIA", "ARM Carmel"} },
        { JETSON_TX2, {1, "8192M", "Unknown", "Jetson TX2", "NVIDIA", "ARM A57 + Denver"} },
        { JETSON_TX1, {1, "4096M", "Unknown", "Jetson TX1", "NVIDIA", "ARM A57"} },
        { JETSON_NANO, {1, "4096M", "Unknown", "Jetson nano", "NVIDIA", "ARM A57"} }
    }
{};



static bool ids_warned = false;

PinData get_data()
{
    try
    {
        EntirePinData& _DATA = EntirePinData::get_instance();

        const string compatible_path = "/proc/device-tree/compatible";
        const string ids_path = "/proc/device-tree/chosen/plugin-manager/ids";

        set<string> compatibles{};

        { // scope for f:
            ifstream f(compatible_path);
            stringstream buffer{};

            buffer << f.rdbuf();
            string tmp_str = buffer.str();
            vector<string> _vec_compatibles(split(tmp_str, '\x00'));
            // convert to std::set
            copy(_vec_compatibles.begin(), _vec_compatibles.end(), inserter(compatibles, compatibles.end()));
        } // scope ends

        auto matches = [&compatibles](const vector<string>& vals) 
        {
            for(const auto& v : vals)
            {
                if(is_in(v, compatibles)) 
                    return true;
            }
            return false;
        };

        auto find_pmgr_board = [&](const string& prefix) -> string
        {
            if (!os_path_exists(ids_path))
            {
                    if (ids_warned == false)
                    {
                        ids_warned = true;
                        string msg = "WARNING: Plugin manager information missing from device tree.\n"
                                     "WARNING: Cannot determine whether the expected Jetson board is present.";
                        cerr << msg;
                    }

                    return "None";
            }

            for (const auto& file : os_listdir(ids_path))
            {
                if (startswith(file, prefix))
                    return file;
            }

            return "None";
        };

        auto warn_if_not_carrier_board = [&find_pmgr_board](const vector<string>& carrier_boards)
        {
            auto found = false;

            for (auto&& b : carrier_boards)
            {
                found = !is_None(find_pmgr_board(b + "-"s));
                if (found)
                    break;
            }

            if (found == false)
            {
                string msg = "WARNING: Carrier board is not from a Jetson Developer Kit.\n"
                             "WARNNIG: Jetson.GPIO library has not been verified with this carrier board,\n"
                             "WARNING: and in fact is unlikely to work correctly.";
                cerr << msg << endl;
            }
        };

        Model model{};

        if (matches(_DATA.compats_tx1))
        {
            model = JETSON_TX1;
            warn_if_not_carrier_board({"2597"s});
        }
        else if (matches(_DATA.compats_tx2))
        {
            model = JETSON_TX2;
            warn_if_not_carrier_board({"2597"s});
        }
        else if (matches(_DATA.compats_clara_agx_xavier))
        {
            model = CLARA_AGX_XAVIER;
            warn_if_not_carrier_board({"3900"s});
        }
        else if (matches(_DATA.compats_xavier))
        {
            model = JETSON_XAVIER;
            warn_if_not_carrier_board({"2822"s});
        }
        else if (matches(_DATA.compats_nano))
        {
            model = JETSON_NANO;
            string module_id = find_pmgr_board("3448");

            if (is_None(module_id))
                throw runtime_error("Could not determine Jetson Nano module revision");
            string revision = split(module_id, '-').back();
            // Revision is an ordered string, not a decimal integer
            if (revision < "200")
                throw runtime_error("Jetson Nano module revision must be A02 or later");

            warn_if_not_carrier_board({"3449"s, "3542"s});
        }
        else if (matches(_DATA.compats_nx))
        {
            model = JETSON_NX;
            warn_if_not_carrier_board({"3509"s, "3449"s});
        }
        else
        {
            throw runtime_error("Could not determine Jetson model");
        }

        vector<PinDefinition> pin_defs = _DATA.PIN_DEFS_MAP.at(model);
        PinInfo jetson_info = _DATA.JETSON_INFO_MAP.at(model);

        map<string, string> gpio_chip_dirs{};
        map<string, int> gpio_chip_base{};
        map<string, string> pwm_dirs{};

        vector<string> sysfs_prefixes = { "/sys/devices/", "/sys/devices/platform/" };

        // Get the gpiochip offsets
        set<string> gpio_chip_names{};
        for (const auto& pin_def : pin_defs)
        {
            if(!is_None(pin_def.SysfsDir))
                gpio_chip_names.insert(pin_def.SysfsDir);
        }

        for (const auto& gpio_chip_name : gpio_chip_names)
        {
            string gpio_chip_dir = "None";
            for (const auto& prefix : sysfs_prefixes)
            {
                auto d = prefix + gpio_chip_name;
                if(os_path_isdir(d))
                {
                    gpio_chip_dir = d;
                    break;
                }
            }

            if (is_None(gpio_chip_dir))
                throw runtime_error("Cannot find GPIO chip " + gpio_chip_name);

            gpio_chip_dirs[gpio_chip_name] = gpio_chip_dir;
            string gpio_chip_gpio_dir = gpio_chip_dir + "/gpio";
            auto files = os_listdir(gpio_chip_gpio_dir);
            for (const auto& fn : files)
            {
                if (!startswith(fn, "gpiochip"))
                    continue;

                string gpiochip_fn = gpio_chip_gpio_dir + "/" + fn + "/base";
                { // scope for f
                    ifstream f(gpiochip_fn);
                    stringstream buffer;
                    buffer << f.rdbuf();
                    gpio_chip_base[gpio_chip_name] = stoi(strip(buffer.str()));
                    break;
                } // scope ends
            }
        }


        auto global_gpio_id = [&gpio_chip_base](string gpio_chip_name, int chip_relative_id) -> int
        {
            if (is_None(gpio_chip_name) ||
                !is_in(gpio_chip_name, gpio_chip_base) || 
                chip_relative_id == -1)
                return -1;
            return gpio_chip_base[gpio_chip_name] + chip_relative_id;
        };


        set<string> pwm_chip_names{};
        for(const auto& x : pin_defs)
        {
            if(!is_None(x.PWMSysfsDir))
                pwm_chip_names.insert(x.PWMSysfsDir);
        }

        for(const auto& pwm_chip_name : pwm_chip_names)
        {
            string pwm_chip_dir = "None";
            for (const auto& prefix : sysfs_prefixes)
            {
                auto d = prefix + pwm_chip_name;
                if(os_path_isdir(d))
                {
                    pwm_chip_dir = d;
                    break;
                }

            }
            /* Some PWM controllers aren't enabled in all versions of the DT. In
            this case, just hide the PWM function on this pin, but let all other
            aspects of the library continue to work. */
            if (is_None(pwm_chip_dir))
                continue;

            auto chip_pwm_dir = pwm_chip_dir + "/pwm";
            if (!os_path_exists(chip_pwm_dir))
                continue;

            for (const auto& fn : os_listdir(chip_pwm_dir))
            {
                if (!startswith(fn, "pwmchip"))
                    continue;

                string chip_pwm_pwmchip_dir = chip_pwm_dir + "/" + fn;
                pwm_dirs[pwm_chip_name] = chip_pwm_pwmchip_dir;
                break;
            }
        }


        auto model_data = [&global_gpio_id, &pwm_dirs, &gpio_chip_dirs]
                          (NumberingModes key, const auto& pin_defs)
        {
            auto get_or = [](const auto& dictionary, const string& x, const string& defaultValue) -> string
            {
                return is_in(x, dictionary) ? dictionary.at(x) : defaultValue;
            };


            map<string, ChannelInfo> ret{};

            for (const auto& x : pin_defs)
            {
                string pinName = x.PinName(key);
                
                if(!is_in(x.SysfsDir, gpio_chip_dirs))
                    throw std::runtime_error("[model_data]"s + x.SysfsDir + " is not in gpio_chip_dirs"s);

                ret.insert(
                    { 
                        pinName,
                        ChannelInfo
                        { 
                            pinName,
                            gpio_chip_dirs.at(x.SysfsDir),
                            x.LinuxPin,
                            global_gpio_id(x.SysfsDir, x.LinuxPin),
                            get_or(pwm_dirs, x.PWMSysfsDir, "None"),
                            x.PWMID 
                        }
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
    catch(exception& e)
    {
        _rethrow_exception(e, "GPIO::get_data()");
    }
}
