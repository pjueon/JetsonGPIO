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
#include <tuple>

#include <algorithm>
#include <iterator>

#include "JetsonGPIO.h"
#include "private/gpio_pin_data.h"
#include "private/PythonFunctions.h"
#include "private/PinDefinition.h"
#include "private/ExceptionHandling.h"


using namespace std;
using namespace std::string_literals; // enables s-suffix for std::string literals

// ========================================= Begin of "gpio_pin_data.py" =========================================

namespace GPIO
{
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
        const vector<PinDefinition> JETSON_TX2_NX_PIN_DEFS;
        const vector<string> compats_tx2_nx;    
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
            { "{224: 134, 169: 106}"s, "{169:  PQ.06}"s, "2200000.gpio", "7",  "4",   "MCLK05"s,    "SOC_GPIO42", "None",       -1 },
            { "{224: 140, 169: 112}"s, "{169:  PR.04}"s, "2200000.gpio", "11", "17",  "UART1_RTS",  "UART1_RTS",  "None",       -1 },
            { "{224:  63, 169:  51}"s, "{169:  PH.07}"s, "2200000.gpio", "12", "18",  "I2S2_CLK",   "DAP2_SCLK",  "None",       -1 },
            { "{224: 124, 169:  96}"s, "{169:  PP.04}"s, "2200000.gpio", "13", "27",  "GPIO32",     "SOC_GPIO04", "None",       -1 },
            // Older versions of L4T don"t enable this PWM controller in DT, so this PWM 
            // channel may not be available. 
            { "{224: 105, 169:  84}"s, "{169:  PN.01}"s, "2200000.gpio", "15", "22",  "GPIO27",     "SOC_GPIO54", "3280000.pwm", 0 },
            { "{ 40:   8,  30:   8}"s, "{ 30: PBB.00}"s, "c2f0000.gpio", "16", "23",  "GPIO8",      "CAN1_STB",   "None",       -1 },
            { "{224:  56, 169:  44}"s, "{169:  PH.00}"s, "2200000.gpio", "18", "24",  "GPIO35",     "SOC_GPIO12", "32c0000.pwm", 0 },
            { "{224: 205, 169: 162}"s, "{169:  PZ.05}"s, "2200000.gpio", "19", "10",  "SPI1_MOSI",  "SPI1_MOSI",  "None",       -1 },
            { "{224: 204, 169: 161}"s, "{169:  PZ.04}"s, "2200000.gpio", "21", "9",   "SPI1_MISO",  "SPI1_MISO",  "None",       -1 },
            { "{224: 129, 169: 101}"s, "{169:  PQ.01}"s, "2200000.gpio", "22", "25",  "GPIO17",     "SOC_GPIO21", "None",       -1 },
            { "{224: 203, 169: 160}"s, "{169:  PZ.03}"s, "2200000.gpio", "23", "11",  "SPI1_CLK",   "SPI1_SCK",   "None",       -1 },
            { "{224: 206, 169: 163}"s, "{169:  PZ.06}"s, "2200000.gpio", "24", "8",   "SPI1_CS0_N", "SPI1_CS0_N", "None",       -1 },
            { "{224: 207, 169: 164}"s, "{169:  PZ.07}"s, "2200000.gpio", "26", "7",   "SPI1_CS1_N", "SPI1_CS1_N", "None",       -1 },
            { "{ 40:   3,  30:   3}"s, "{ 30: PAA.03}"s, "c2f0000.gpio", "29", "5",   "CAN0_DIN",   "CAN0_DIN",   "None",       -1 },
            { "{ 40:   2,  30:   2}"s, "{ 30: PAA.02}"s, "c2f0000.gpio", "31", "6",   "CAN0_DOUT",  "CAN0_DOUT",  "None",       -1 },
            { "{ 40:   9,  30:   9}"s, "{ 30: PBB.01}"s, "c2f0000.gpio", "32", "12",  "GPIO9",      "CAN1_EN",    "None",       -1 },
            { "{ 40:   0,  30:   0}"s, "{ 30: PAA.00}"s, "c2f0000.gpio", "33", "13",  "CAN1_DOUT",  "CAN1_DOUT",  "None",       -1 },
            { "{224:  66, 169:  54}"s, "{169:  PI.02}"s, "2200000.gpio", "35", "19", "I2S2_FS",    "DAP2_FS",     "None",       -1 },
            // Input-only (due to base board)
            { "{224: 141, 169: 113}"s, "{169:  PR.05}"s, "2200000.gpio", "36", "16",  "UART1_CTS", "UART1_CTS",   "None",       -1 },
            { "{ 40:   1,  30:   1}"s, "{ 30: PAA.01}"s, "c2f0000.gpio",  "37", "26",  "CAN1_DIN",  "CAN1_DIN",   "None",       -1 },
            { "{224:  65, 169:  53}"s, "{169:  PI.01}"s, "2200000.gpio", "38", "20",  "I2S2_DIN",  "DAP2_DIN",    "None",       -1 },
            { "{224:  64, 169:  52}"s, "{169:  PI.00}"s, "2200000.gpio", "40", "21",  "I2S2_DOUT", "DAP2_DOUT",   "None",       -1 }
        },
        compats_clara_agx_xavier
        {
            "nvidia,e3900-0000+p2888-0004"
        },

        JETSON_NX_PIN_DEFS
        {
            { "{224: 148, 169: 118}"s, "{169:  PS.04}"s, "2200000.gpio", "7", "4",   "GPIO09",    "AUD_MCLK",   "None",       -1 },
            { "{224: 140, 169: 112}"s, "{169:  PR.04}"s, "2200000.gpio", "11", "17", "UART1_RTS", "UART1_RTS",  "None",       -1 },
            { "{224: 157, 169: 127}"s, "{169:  PT.05}"s, "2200000.gpio", "12", "18", "I2S0_SCLK", "DAP5_SCLK",  "None",       -1 },
            { "{224: 192, 169: 149}"s, "{169:  PY.00}"s, "2200000.gpio", "13", "27", "SPI1_SCK",  "SPI3_SCK",   "None",       -1 },
            { "{ 40:  20,  30:  16}"s, "{ 30: PCC.04}"s, "c2f0000.gpio", "15", "22", "GPIO12",    "TOUCH_CLK",  "None",       -1 },
            { "{224: 196, 169: 153}"s, "{169:  PY.04}"s, "2200000.gpio", "16", "23", "SPI1_CS1",  "SPI3_CS1_N", "None",       -1 },
            { "{224: 195, 169: 152}"s, "{169:  PY.03}"s, "2200000.gpio", "18", "24", "SPI1_CS0",  "SPI3_CS0_N", "None",       -1 },
            { "{224: 205, 169: 162}"s, "{169:  PZ.05}"s, "2200000.gpio", "19", "10", "SPI0_MOSI", "SPI1_MOSI",  "None",       -1 },
            { "{224: 204, 169: 161}"s, "{169:  PZ.04}"s, "2200000.gpio", "21", "9",  "SPI0_MISO", "SPI1_MISO",  "None",       -1 },
            { "{224: 193, 169: 150}"s, "{169:  PY.01}"s, "2200000.gpio", "22", "25", "SPI1_MISO", "SPI3_MISO",  "None",       -1 },
            { "{224: 203, 169: 160}"s, "{169:  PZ.03}"s, "2200000.gpio", "23", "11", "SPI0_SCK",  "SPI1_SCK",   "None",       -1 },
            { "{224: 206, 169: 163}"s, "{169:  PZ.06}"s, "2200000.gpio", "24", "8",  "SPI0_CS0",  "SPI1_CS0_N", "None",       -1 },
            { "{224: 207, 169: 164}"s, "{169:  PZ.07}"s, "2200000.gpio", "26", "7",  "SPI0_CS1",  "SPI1_CS1_N", "None",       -1 },
            { "{224: 133, 169: 105}"s, "{169:  PQ.05}"s, "2200000.gpio", "29", "5",  "GPIO01",    "SOC_GPIO41", "None",       -1 },
            { "{224: 134, 169: 106}"s, "{169:  PQ.06}"s, "2200000.gpio", "31", "6",  "GPIO11",    "SOC_GPIO42", "None",       -1 },
            { "{224: 136, 169: 108}"s, "{169:  PR.00}"s, "2200000.gpio", "32", "12", "GPIO07",    "SOC_GPIO44", "32f0000.pwm", 0 },
            { "{224: 105, 169:  84}"s, "{169:  PN.01}"s, "2200000.gpio", "33", "13", "GPIO13",    "SOC_GPIO54", "3280000.pwm", 0 },
            { "{224: 160, 169: 130}"s, "{169:  PU.00}"s, "2200000.gpio", "35", "19", "I2S0_FS",   "DAP5_FS",    "None",       -1 },
            { "{224: 141, 169: 113}"s, "{169:  PR.05}"s, "2200000.gpio", "36", "16", "UART1_CTS", "UART1_CTS",  "None",       -1 },
            { "{224: 194, 169: 151}"s, "{169:  PY.02}"s, "2200000.gpio", "37", "26", "SPI1_MOSI", "SPI3_MOSI",  "None",       -1 },
            { "{224: 159, 169: 129}"s, "{169:  PT.07}"s, "2200000.gpio", "38", "20", "I2S0_DIN",  "DAP5_DIN",   "None",       -1 },
            { "{224: 158, 169: 128}"s, "{169:  PT.06}"s, "2200000.gpio", "40", "21", "I2S0_DOUT", "DAP5_DOUT",  "None",       -1 }
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
            { "{224: 134, 169: 106}"s, "{169:  PQ.06}"s, "2200000.gpio", "7", "4",   "MCLK05",     "SOC_GPIO42", "None",       -1 },
            { "{224: 140, 169: 112}"s, "{169:  PR.04}"s, "2200000.gpio", "11", "17", "UART1_RTS",  "UART1_RTS",  "None",       -1 },
            { "{224:  63, 169:  51}"s, "{169:  PH.07}"s, "2200000.gpio", "12", "18", "I2S2_CLK",   "DAP2_SCLK",  "None",       -1 },
            { "{224: 136, 169: 108}"s, "{169:  PR.00}"s, "2200000.gpio", "13", "27", "PWM01",      "SOC_GPIO44", "32f0000.pwm", 0 },
            // Older versions of L4T don"t enable this PWM controller in DT, so this PWM 
            // channel may not be available. 
            { "{224: 105, 169:  84}"s, "{169:  PN.01}"s, "2200000.gpio", "15", "22", "GPIO27",     "SOC_GPIO54", "3280000.pwm", 0 },
            { "{ 40:   8,  30:   8}"s, "{ 30: PBB.00}"s, "c2f0000.gpio", "16", "23", "GPIO8",      "CAN1_STB",   "None",       -1 },
            { "{224:  56, 169:  44}"s, "{169:  PH.00}"s, "2200000.gpio", "18", "24", "GPIO35",     "SOC_GPIO12", "32c0000.pwm", 0 },
            { "{224: 205, 169: 162}"s, "{169:  PZ.05}"s, "2200000.gpio", "19", "10", "SPI1_MOSI",  "SPI1_MOSI",  "None",       -1 },
            { "{224: 204, 169: 161}"s, "{169:  PZ.04}"s, "2200000.gpio", "21", "9",  "SPI1_MISO",  "SPI1_MISO",  "None",       -1 },
            { "{224: 129, 169: 101}"s, "{169:  PQ.01}"s, "2200000.gpio", "22", "25", "GPIO17",     "SOC_GPIO21", "None",       -1 },
            { "{224: 203, 169: 160}"s, "{169:  PZ.03}"s, "2200000.gpio", "23", "11", "SPI1_CLK",   "SPI1_SCK",   "None",       -1 },
            { "{224: 206, 169: 163}"s, "{169:  PZ.06}"s, "2200000.gpio", "24", "8",  "SPI1_CS0_N", "SPI1_CS0_N", "None",       -1 },
            { "{224: 207, 169: 164}"s, "{169:  PZ.07}"s, "2200000.gpio", "26", "7",  "SPI1_CS1_N", "SPI1_CS1_N", "None",       -1 },
            { "{ 40:   3,  30:   3}"s, "{ 30: PAA.03}"s, "c2f0000.gpio", "29", "5",  "CAN0_DIN",   "CAN0_DIN",   "None",       -1 },
            { "{ 40:   2,  30:   2}"s, "{ 30: PAA.02}"s, "c2f0000.gpio", "31", "6",  "CAN0_DOUT",  "CAN0_DOUT",  "None",       -1 },
            { "{ 40:   9,  30:   9}"s, "{ 30: PBB.01}"s, "c2f0000.gpio", "32", "12", "GPIO9",      "CAN1_EN",    "None",       -1 },
            { "{ 40:   0,  30:   0}"s, "{ 30: PAA.00}"s, "c2f0000.gpio", "33", "13", "CAN1_DOUT",  "CAN1_DOUT",  "None",       -1 },
            { "{224:  66, 169:  54}"s, "{169:  PI.02}"s, "2200000.gpio", "35", "19", "I2S2_FS",    "DAP2_FS",    "None",       -1 },
            // Input-only (due to base board)
            { "{224: 141, 169: 113}"s, "{169:  PR.05}"s, "2200000.gpio", "36", "16", "UART1_CTS",  "UART1_CTS",   "None",      -1 },
            { "{ 40:   1,  30:   1}"s, "{ 30: PAA.01}"s, "c2f0000.gpio", "37", "26", "CAN1_DIN",   "CAN1_DIN",    "None",      -1 },
            { "{224:  65, 169:  53}"s, "{169:  PI.01}"s, "2200000.gpio", "38", "20", "I2S2_DIN",   "DAP2_DIN",    "None",      -1 },
            { "{224:  64, 169:  52}"s, "{169:  PI.00}"s, "2200000.gpio", "40", "21", "I2S2_DOUT",  "DAP2_DOUT",   "None",      -1 }
        },
        compats_xavier
        {
            "nvidia,p2972-0000",
            "nvidia,p2972-0006",
            "nvidia,jetson-xavier",
            "nvidia,galen-industrial",
            "nvidia,jetson-xavier-industrial"
        },

        JETSON_TX2_NX_PIN_DEFS
        {
            { "{192: 76,  140:  66}"s, "{140:  PJ.04}"s, "2200000.gpio", "7",  "4",  "GPIO09",    "AUD_MCLK",  "None",       -1 },
            { "{64:  28,  47:   23}"s, "{47:   PW.04}"s, "c2f0000.gpio", "11", "17", "UART1_RTS", "UART3_RTS", "None",       -1 },
            { "{192: 72,  140:  62}"s, "{140:  PJ.00}"s, "2200000.gpio", "12", "18", "I2S0_SCLK", "DAP1_SCLK", "None",       -1 },
            { "{64:  17,  47:   12}"s, "{47:   PV.01}"s, "c2f0000.gpio", "13", "27", "SPI1_SCK",  "GPIO_SEN1", "None",       -1 },
            { "{192: 18,  140:  16}"s, "{140:  PC.02}"s, "2200000.gpio", "15", "22", "GPIO12",    "DAP2_DOUT", "None",       -1 },
            { "{192: 19,  140:  17}"s, "{140:  PC.03}"s, "2200000.gpio", "16", "23", "SPI1_CS1",  "DAP2_DIN",  "None",       -1 },
            { "{64:  20,  47:   15}"s, "{47:   PV.04}"s, "c2f0000.gpio", "18", "24", "SPI1_CS0",  "GPIO_SEN4", "None",       -1 },
            { "{192: 58,  140:  49}"s, "{140:  PH.02}"s, "2200000.gpio", "19", "10", "SPI0_MOSI", "GPIO_WAN7", "None",       -1 },
            { "{192: 57,  140:  48}"s, "{140:  PH.01}"s, "2200000.gpio", "21", "9",  "SPI0_MISO", "GPIO_WAN6", "None",       -1 },
            { "{64:  18,  47:   13}"s, "{47:   PV.02}"s, "c2f0000.gpio", "22", "25", "SPI1_MISO", "GPIO_SEN2", "None",       -1 },
            { "{192: 56,  140:  47}"s, "{140:  PH.00}"s, "2200000.gpio", "23", "11", "SPI1_CLK",  "GPIO_WAN5", "None",       -1 },
            { "{192: 59,  140:  50}"s, "{140:  PH.03}"s, "2200000.gpio", "24", "8",  "SPI0_CS0",  "GPIO_WAN8", "None",       -1 },
            { "{192: 163, 140: 130}"s, "{140:  PY.03}"s, "2200000.gpio", "26", "7",  "SPI0_CS1",  "GPIO_MDM4", "None",       -1 },
            { "{192: 105, 140:  86}"s, "{140:  PN.01}"s, "2200000.gpio", "29", "5",  "GPIO01",    "GPIO_CAM2", "None",       -1 },
            { "{64:  50,  47:   41}"s, "{47:  PEE.02}"s, "c2f0000.gpio", "31", "6",  "GPIO11",    "TOUCH_CLK", "None",       -1 },
            { "{64:  8,   47:    5}"s, "{47:   PU.00}"s, "c2f0000.gpio", "32", "12", "GPIO07",    "GPIO_DIS0", "3280000.pwm", 0 },
            { "{64:  13,  47:   10}"s, "{47:   PU.05}"s, "c2f0000.gpio", "33", "13", "GPIO13",    "GPIO_DIS5", "32a0000.pwm", 0 },
            { "{192: 75,  140:  65}"s, "{140:  PJ.03}"s, "2200000.gpio", "35", "19", "I2S0_FS",   "DAP1_FS",   "None",       -1 },
            { "{64:  29,  47:   24}"s, "{47:   PW.05}"s, "c2f0000.gpio", "36", "16", "UART1_CTS", "UART3_CTS", "None",       -1 },
            { "{64:  19,  47:   14}"s, "{47:   PV.03}"s, "c2f0000.gpio", "37", "26", "SPI1_MOSI", "GPIO_SEN3", "None",       -1 },
            { "{192: 74,  140:  64}"s, "{140:  PJ.02}"s, "2200000.gpio", "38", "20", "I2S0_DIN",  "DAP1_DIN",  "None",       -1 },
            { "{192: 73,  140:  63}"s, "{140:  PJ.01}"s, "2200000.gpio", "40", "21", "I2S0_DOUT", "DAP1_DOUT", "None",       -1 }       
        },
        compats_tx2_nx
        {
            "nvidia,p3509-0000+p3636-0001"
        },

        JETSON_TX2_PIN_DEFS
        {
            { "{192:  76, 140:  66}"s, "{140:  PJ.04}"s, "2200000.gpio",             "7", "4",   "AUDIO_MCLK",         "AUD_MCLK",     "None", -1 },
            // Output-only (due to base board)                   
            { "{192: 146, 140: 117}"s, "{140:  PT.02}"s, "2200000.gpio",             "11", "17", "UART0_RTS",          "UART1_RTS",    "None", -1 },
            { "{192:  72, 140:  62}"s, "{140:  PJ.00}"s, "2200000.gpio",             "12", "18", "I2S0_CLK",           "DAP1_SCLK",    "None", -1 },
            { "{192:  77, 140:  67}"s, "{140:  PJ.05}"s, "2200000.gpio",             "13", "27", "GPIO20_AUD_INT",     "GPIO_AUD0",    "None", -1 },
            { "                  15"s, "           {}"s, "3160000.i2c/i2c-0/0-0074", "15", "22", "GPIO_EXP_P17",       "GPIO_EXP_P17", "None", -1 },
            // Input-only (due to module):
            { "{ 64:  40,  47:  31}"s, "{ 47: PAA.00}"s, "c2f0000.gpio",             "16", "23", "AO_DMIC_IN_DAT",     "CAN_GPIO0",    "None", -1 },
            { "{192: 161, 140: 128}"s, "{140:  PY.01}"s, "2200000.gpio",             "18", "24", "GPIO16_MDM_WAKE_AP", "GPIO_MDM2",    "None", -1 },
            { "{192: 109, 140:  90}"s, "{140:  PN.05}"s, "2200000.gpio",             "19", "10", "SPI1_MOSI",          "GPIO_CAM6",    "None", -1 },
            { "{192: 108, 140:  89}"s, "{140:  PN.04}"s, "2200000.gpio",             "21", "9",  "SPI1_MISO",          "GPIO_CAM5",    "None", -1 },
            { "                  14"s, "           {}"s, "3160000.i2c/i2c-0/0-0074", "22", "25", "GPIO_EXP_P16",       "GPIO_EXP_P16", "None", -1 },
            { "{192: 107, 140:  88}"s, "{140:  PN.03}"s, "2200000.gpio",             "23", "11", "SPI1_CLK",           "GPIO_CAM4",    "None", -1 },
            { "{192: 110, 140:  91}"s, "{140:  PN.06}"s, "2200000.gpio",             "24", "8",  "SPI1_CS0",           "GPIO_CAM7",    "None", -1 },
            // Board pin 26 is not available on this board   
            { "{192:  78, 140:  68}"s, "{140:  PJ.06}"s, "2200000.gpio",             "29", "5",  "GPIO19_AUD_RST",     "GPIO_AUD1",    "None", -1 },
            { "{ 64:  42,  47:  33}"s, "{ 47: PAA.02}"s, "c2f0000.gpio",             "31", "6",  "GPIO9_MOTION_INT",   "CAN_GPIO2",    "None", -1 },
            // Output-only (due to module):             
            { "{ 64:  41,  47:  32}"s, "{ 47: PAA.01}"s, "c2f0000.gpio",             "32", "12", "AO_DMIC_IN_CLK",     "CAN_GPIO1",    "None", -1 },
            { "{192:  69, 140:  59}"s, "{140:  PI.05}"s, "2200000.gpio",             "33", "13", "GPIO11_AP_WAKE_BT",  "GPIO_PQ5",     "None", -1 },
            { "{192:  75, 140:  65}"s, "{140:  PJ.03}"s, "2200000.gpio",             "35", "19", "I2S0_LRCLK",         "DAP1_FS",      "None", -1 },
            // Input-only (due to base board) IF NVIDIA debug card NOT plugged in   
            // Output-only (due to base board) IF NVIDIA debug card plugged in   
            { "{192: 147, 140: 118}"s, "{140:  PT.03}"s, "2200000.gpio",             "36", "16", "UART0_CTS",          "UART1_CTS",    "None", -1 },
            { "{192:  68, 140:  58}"s, "{140:  PI.04}"s, "2200000.gpio",             "37", "26", "GPIO8_ALS_PROX_INT", "GPIO_PQ4",     "None", -1 },
            { "{192:  74, 140:  64}"s, "{140:  PJ.02}"s, "2200000.gpio",             "38", "20", "I2S0_SDIN",          "DAP1_DIN",     "None", -1 },
            { "{192:  73, 140:  63}"s, "{140:  PJ.01}"s, "2200000.gpio",             "40", "21", "I2S0_SDOUT",         "DAP1_DOUT",    "None", -1 }
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
            { "216"s, "{}"s, "6000d000.gpio",             "7", "4",   "AUDIO_MCLK",         "AUD_MCLK",      "None", -1 },
            // Output-only (due to base board)                            
            { "162"s, "{}"s, "6000d000.gpio",             "11", "17", "UART0_RTS",          "UART1_RTS",     "None", -1 },
            { "11"s,  "{}"s, "6000d000.gpio",             "12", "18", "I2S0_CLK",           "DAP1_SCLK",     "None", -1 },
            { "38"s,  "{}"s, "6000d000.gpio",             "13", "27", "GPIO20_AUD_INT",     "GPIO_PE6",      "None", -1 },
            { "15"s,  "{}"s, "7000c400.i2c/i2c-1/1-0074", "15", "22", "GPIO_EXP_P17",       "GPIO_EXP_P17",  "None", -1 },
            { "37"s,  "{}"s, "6000d000.gpio",             "16", "23", "AO_DMIC_IN_DAT",     "DMIC3_DAT",     "None", -1 },
            { "184"s, "{}"s, "6000d000.gpio",             "18", "24", "GPIO16_MDM_WAKE_AP", "MODEM_WAKE_AP", "None", -1 },
            { "16"s,  "{}"s, "6000d000.gpio",             "19", "10", "SPI1_MOSI",          "SPI1_MOSI",     "None", -1 },
            { "17"s,  "{}"s, "6000d000.gpio",             "21", "9",  "SPI1_MISO",          "SPI1_MISO",     "None", -1 },
            { "14"s,  "{}"s, "7000c400.i2c/i2c-1/1-0074", "22", "25", "GPIO_EXP_P16",       "GPIO_EXP_P16",  "None", -1 },
            { "18"s,  "{}"s, "6000d000.gpio",             "23", "11", "SPI1_CLK",           "SPI1_SCK",      "None", -1 },
            { "19"s,  "{}"s, "6000d000.gpio",             "24", "8",  "SPI1_CS0",           "SPI1_CS0",      "None", -1 },
            { "20"s,  "{}"s, "6000d000.gpio",             "26", "7",  "SPI1_CS1",           "SPI1_CS1",      "None", -1 },
            { "219"s, "{}"s, "6000d000.gpio",             "29", "5",  "GPIO19_AUD_RST",     "GPIO_X1_AUD",   "None", -1 },
            { "186"s, "{}"s, "6000d000.gpio",             "31", "6",  "GPIO9_MOTION_INT",   "MOTION_INT",    "None", -1 },
            { "36"s,  "{}"s, "6000d000.gpio",             "32", "12", "AO_DMIC_IN_CLK",     "DMIC3_CLK",     "None", -1 },
            { "63"s,  "{}"s, "6000d000.gpio",             "33", "13", "GPIO11_AP_WAKE_BT",  "AP_WAKE_NFC",   "None", -1 },
            { "8"s,   "{}"s, "6000d000.gpio",             "35", "19", "I2S0_LRCLK",         "DAP1_FS",       "None", -1 },
            // Input-only (due to base board) IF NVIDIA debug card NOT plugged in
            // Input-only (due to base board) (always reads fixed value) IF NVIDIA debug card plugged in
            { "163"s,  "{}"s, "6000d000.gpio",            "36", "16", "UART0_CTS",          "UART1_CTS",     "None", -1 },
            { "187"s,  "{}"s, "6000d000.gpio",            "37", "26", "GPIO8_ALS_PROX_INT", "ALS_PROX_INT",  "None", -1 },
            { "9"s,    "{}"s, "6000d000.gpio",            "38", "20", "I2S0_SDIN",          "DAP1_DIN",      "None", -1 },
            { "10"s,   "{}"s, "6000d000.gpio",            "40", "21", "I2S0_SDOUT",         "DAP1_DOUT",     "None", -1 }
        },
        compats_tx1
        {
            "nvidia,p2371-2180",
            "nvidia,jetson-cv"
        },

        JETSON_NANO_PIN_DEFS
        {
            { "216"s, "{}"s, "6000d000.gpio", "7", "4",   "GPIO9",     "AUD_MCLK",  "None",        -1 },
            { "50"s,  "{}"s, "6000d000.gpio", "11", "17", "UART1_RTS", "UART2_RTS", "None",        -1 },
            { "79"s,  "{}"s, "6000d000.gpio", "12", "18", "I2S0_SCLK", "DAP4_SCLK", "None",        -1 },
            { "14"s,  "{}"s, "6000d000.gpio", "13", "27", "SPI1_SCK",  "SPI2_SCK",  "None",        -1 },
            { "194"s, "{}"s, "6000d000.gpio", "15", "22", "GPIO12",    "LCD_TE",    "None",        -1 },
            { "232"s, "{}"s, "6000d000.gpio", "16", "23", "SPI1_CS1",  "SPI2_CS1",  "None",        -1 },
            { "15"s,  "{}"s, "6000d000.gpio", "18", "24", "SPI1_CS0",  "SPI2_CS0",  "None",        -1 },
            { "16"s,  "{}"s, "6000d000.gpio", "19", "10", "SPI0_MOSI", "SPI1_MOSI", "None",        -1 },
            { "17"s,  "{}"s, "6000d000.gpio", "21", "9",  "SPI0_MISO", "SPI1_MISO", "None",        -1 },
            { "13"s,  "{}"s, "6000d000.gpio", "22", "25", "SPI1_MISO", "SPI2_MISO", "None",        -1 },
            { "18"s,  "{}"s, "6000d000.gpio", "23", "11", "SPI0_SCK",  "SPI1_SCK",  "None",        -1 },
            { "19"s,  "{}"s, "6000d000.gpio", "24", "8",  "SPI0_CS0",  "SPI1_CS0",  "None",        -1 },
            { "20"s,  "{}"s, "6000d000.gpio", "26", "7",  "SPI0_CS1",  "SPI1_CS1",  "None",        -1 },
            { "149"s, "{}"s, "6000d000.gpio", "29", "5",  "GPIO01",    "CAM_AF_EN", "None",        -1 },
            { "200"s, "{}"s, "6000d000.gpio", "31", "6",  "GPIO11",    "GPIO_PZ0",  "None",        -1 },
            // Older versions of L4T have a DT bug which instantiates a bogus device
            // which prevents this library from using this PWM channel.
            { "168"s, "{}"s, "6000d000.gpio", "32", "12", "GPIO07",    "LCD_BL_PW", "7000a000.pwm", 0 },
            { "38"s,  "{}"s, "6000d000.gpio", "33", "13", "GPIO13",    "GPIO_PE6",  "7000a000.pwm", 2 },
            { "76"s,  "{}"s, "6000d000.gpio", "35", "19", "I2S0_FS",   "DAP4_FS",   "None",        -1 },
            { "51"s,  "{}"s, "6000d000.gpio", "36", "16", "UART1_CTS", "UART2_CTS", "None",        -1 },
            { "12"s,  "{}"s, "6000d000.gpio", "37", "26", "SPI1_MOSI", "SPI2_MOSI", "None",        -1 },
            { "77"s,  "{}"s, "6000d000.gpio", "38", "20", "I2S0_DIN",  "DAP4_DIN",  "None",        -1 },
            { "78"s,  "{}"s, "6000d000.gpio", "40", "21", "I2S0_DOUT", "DAP4_DOUT", "None",        -1 }
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
            { JETSON_TX2_NX, JETSON_TX2_NX_PIN_DEFS },
            { JETSON_TX2, JETSON_TX2_PIN_DEFS },
            { JETSON_TX1, JETSON_TX1_PIN_DEFS },
            { JETSON_NANO, JETSON_NANO_PIN_DEFS }
        },
        JETSON_INFO_MAP
        {
            { CLARA_AGX_XAVIER, {1, "16384M",  "Unknown", "CLARA_AGX_XAVIER", "NVIDIA", "ARM Carmel"} },
            { JETSON_NX, {1, "16384M", "Unknown", "Jetson NX", "NVIDIA", "ARM Carmel"} },
            { JETSON_XAVIER, {1, "16384M", "Unknown", "Jetson Xavier", "NVIDIA", "ARM Carmel"} },
            { JETSON_TX2_NX, {1, "4096M", "Unknown", "Jetson TX2 NX", "NVIDIA", "ARM A57 + Denver"} },
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
            else if (matches(_DATA.compats_tx2_nx))
            {
                model = JETSON_TX2_NX;
                warn_if_not_carrier_board({"3509"s});
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
            map<string, string> gpio_chip_ngpio{};
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

                    string base_fn = gpio_chip_gpio_dir + "/" + fn + "/base";
                    { // scope for f
                        ifstream f(base_fn);
                        stringstream buffer;
                        buffer << f.rdbuf();
                        gpio_chip_base[gpio_chip_name] = stoi(strip(buffer.str()));
                    } // scope ends

                    string ngpio_fn = gpio_chip_gpio_dir + "/" + fn + "/ngpio";
                    { // scope for f
                        ifstream f(ngpio_fn);
                        stringstream buffer;
                        buffer << f.rdbuf();
                        gpio_chip_ngpio[gpio_chip_name] = strip(buffer.str());
                    } // scope ends

                    break;
                }
            }


            auto global_gpio_id_name = [&gpio_chip_base, &gpio_chip_ngpio](DictionaryLike chip_relative_ids, DictionaryLike gpio_names, string gpio_chip_name) -> tuple<int, string>
            {
                if(!is_in(gpio_chip_name, gpio_chip_ngpio))
                    return {-1, "None"};

                auto chip_gpio_ngpio = gpio_chip_ngpio[gpio_chip_name];

                auto chip_relative_id = chip_relative_ids.get(chip_gpio_ngpio);

                auto gpio = gpio_chip_base[gpio_chip_name] + stoi(strip(chip_relative_id));

                auto gpio_name = gpio_names.get(chip_gpio_ngpio);

                if (is_None(gpio_name))
                    gpio_name = format("gpio%i", gpio);

                return { gpio, gpio_name };
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

                auto pwm_chip_pwm_dir = pwm_chip_dir + "/pwm";
                if (!os_path_exists(pwm_chip_pwm_dir))
                    continue;

                for (const auto& fn : os_listdir(pwm_chip_pwm_dir))
                {
                    if (!startswith(fn, "pwmchip"))
                        continue;

                    string pwm_chip_pwm_pwmchipn_dir = pwm_chip_pwm_dir + "/" + fn;
                    pwm_dirs[pwm_chip_name] = pwm_chip_pwm_pwmchipn_dir;
                    break;
                }
            }


            auto model_data = [&global_gpio_id_name, &pwm_dirs, &gpio_chip_dirs]
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

                    auto tmp = global_gpio_id_name(x.LinuxPin, x.ExportedName, x.SysfsDir);
                    auto gpio = get<0>(tmp);
                    auto gpio_name = get<1>(tmp);

                    ret.insert(
                        { 
                            pinName,
                            ChannelInfo
                            { 
                                pinName,
                                gpio_chip_dirs.at(x.SysfsDir),
                                gpio, gpio_name,
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
            throw _error(e, "GPIO::get_data()");
        }
    }
}
