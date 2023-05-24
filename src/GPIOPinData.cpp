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

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "JetsonGPIO/PublicEnums.h"
#include "private/ExceptionHandling.h"
#include "private/GPIOPinData.h"
#include "private/ModelUtility.h"
#include "private/PinDefinition.h"
#include "private/PythonFunctions.h"

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
        const vector<string> compats_jetson_orins_nano;
        const vector<PinDefinition> JETSON_ORIN_NX_PIN_DEFS;
        const vector<string> compats_jetson_orins_nx;
        const vector<PinDefinition> JETSON_ORIN_PIN_DEFS;
        const vector<string> compats_jetson_orins;
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

        EntirePinData(const EntirePinData&) = delete;
        EntirePinData& operator=(const EntirePinData&) = delete;
        ~EntirePinData() = default;

        static EntirePinData& get_instance()
        {
            static EntirePinData singleton{};
            return singleton;
        }

        // clang-format off
        static vector<PinDefinition> _JETSON_ORIN_NX_PIN_DEFS()
        {
            return {
                { "{164: 144}"s, "{164: PAC.06}"s, "2200000.gpio", "7",  "4",  "GPIO09",     "GP167",            None,          None },
                { "{164: 112}"s, "{164:  PR.04}"s, "2200000.gpio", "11", "17", "UART1_RTS",  "GP72_UART1_RTS_N", None,          None },
                { "{164:  50}"s, "{164:  PH.07}"s, "2200000.gpio", "12", "18", "I2S0_SCLK",  "GP122",            None,          None },
                { "{164: 122}"s, "{164:  PY.00}"s, "2200000.gpio", "13", "27", "SPI1_SCK",   "GP36_SPI3_CLK",    None,          None },
                { "{164:  85}"s, "{164:  PN.01}"s, "2200000.gpio", "15", "22", "GPIO12",     "GP88_PWM1",        "3280000.pwm",    0 },
                { "{164: 126}"s, "{164:  PY.04}"s, "2200000.gpio", "16", "23", "SPI1_CS1",   "GP40_SPI3_CS1_N",  None,          None },
                { "{164: 125}"s, "{164:  PY.03}"s, "2200000.gpio", "18", "24", "SPI1_CS0",   "GP39_SPI3_CS0_N",  None,          None },
                { "{164: 135}"s, "{164:  PZ.05}"s, "2200000.gpio", "19", "10", "SPI0_MOSI",  "GP49_SPI1_MOSI",   None,          None },
                { "{164: 134}"s, "{164:  PZ.04}"s, "2200000.gpio", "21", "9",  "SPI0_MISO",  "GP48_SPI1_MISO",   None,          None },
                { "{164: 123}"s, "{164:  PY.01}"s, "2200000.gpio", "22", "25", "SPI1_MISO",  "GP37_SPI3_MISO",   None,          None },
                { "{164: 133}"s, "{164:  PZ.03}"s, "2200000.gpio", "23", "11", "SPI0_SCK",   "GP47_SPI1_CLK",    None,          None },
                { "{164: 136}"s, "{164:  PZ.06}"s, "2200000.gpio", "24", "8",  "SPI0_CS0",   "GP50_SPI1_CS0_N",  None,          None },
                { "{164: 137}"s, "{164:  PZ.07}"s, "2200000.gpio", "26", "7",  "SPI0_CS1",   "GP51_SPI1_CS1_N",  None,          None },
                { "{164: 105}"s, "{164:  PQ.05}"s, "2200000.gpio", "29", "5",  "GPIO01",     "GP65",             None,          None },
                { "{164: 106}"s, "{164:  PQ.06}"s, "2200000.gpio", "31", "6",  "GPIO11",     "GP66",             None,          None },
                { "{164:  41}"s, "{164:  PG.06}"s, "2200000.gpio", "32", "12", "GPIO07",     "GP113_PWM7",       None,          None },
                { "{164:  43}"s, "{164:  PH.00}"s, "2200000.gpio", "33", "13", "GPIO13",     "GP115",            "32c0000.pwm",    0 },
                { "{164:  53}"s, "{164:  PI.02}"s, "2200000.gpio", "35", "19", "I2S0_FS",    "GP125",            None,          None },
                { "{164: 113}"s, "{164:  PR.05}"s, "2200000.gpio", "36", "16", "UART1_CTS",  "GP73_UART1_CTS_N", None,          None },
                { "{164: 124}"s, "{164:  PY.02}"s, "2200000.gpio", "37", "26", "SPI1_MOSI",  "GP38_SPI3_MOSI",   None,          None },
                { "{164:  52}"s, "{164:  PI.01}"s, "2200000.gpio", "38", "20", "I2S0_SDIN",  "GP124",            None,          None },
                { "{164:  51}"s, "{164:  PI.00}"s, "2200000.gpio", "40", "21", "I2S0_SDOUT", "GP123",            None,          None }     
            };
        }

        static vector<PinDefinition> _JETSON_ORIN_PIN_DEFS()
        {
            return {
                { "{164: 106}"s, "{164:  PQ.06}"s, "2200000.gpio", "7",  "4",  "MCLK05",     "GP66",             None,         None },
                // Output-only (due to base board)       
                { "{164: 112}"s, "{164:  PR.04}"s, "2200000.gpio", "11", "17", "UART1_RTS",  "GP72_UART1_RTS_N", None,         None },
                { "{164:  50}"s, "{164:  PH.07}"s, "2200000.gpio", "12", "18", "I2S2_CLK",   "GP122",            None,         None },
                { "{164: 108}"s, "{164:  PR.00}"s, "2200000.gpio", "13", "27", "PWM01",      "GP68",             None,         None },
                { "{164:  85}"s, "{164:  PN.01}"s, "2200000.gpio", "15", "22", "GPIO27",     "GP88_PWM1",        "3280000.pwm",   0 },
                { "{ 32:   9}"s, "{ 32: PBB.01}"s, "c2f0000.gpio", "16", "23", "GPIO08",     "GP26",             None,         None },
                { "{164:  43}"s, "{164:  PH.00}"s, "2200000.gpio", "18", "24", "GPIO35",     "GP115",            "32c0000.pwm",   0 },
                { "{164: 135}"s, "{164:  PZ.05}"s, "2200000.gpio", "19", "10", "SPI1_MOSI",  "GP49_SPI1_MOSI",   None,         None },
                { "{164: 134}"s, "{164:  PZ.04}"s, "2200000.gpio", "21", "9",  "SPI1_MISO",  "GP48_SPI1_MISO",   None,         None },
                { "{164:  96}"s, "{164:  PP.04}"s, "2200000.gpio", "22", "25", "GPIO17",     "GP56",             None,         None },
                { "{164: 133}"s, "{164:  PZ.03}"s, "2200000.gpio", "23", "11", "SPI1_CLK",   "GP47_SPI1_CLK",    None,         None },
                { "{164: 136}"s, "{164:  PZ.06}"s, "2200000.gpio", "24", "8",  "SPI1_CS0_N", "GP50_SPI1_CS0_N",  None,         None },
                { "{164: 137}"s, "{164:  PZ.07}"s, "2200000.gpio", "26", "7",  "SPI1_CS1_N", "GP51_SPI1_CS1_N",  None,         None },
                { "{ 32:   1}"s, "{ 32: PAA.01}"s, "c2f0000.gpio", "29", "5",  "CAN0_DIN",   "GP18_CAN0_DIN",    None,         None },
                { "{ 32:   0}"s, "{ 32: PAA.00}"s, "c2f0000.gpio", "31", "6",  "CAN0_DOUT",  "GP17_CAN0_DOUT",   None,         None },
                { "{ 32:   8}"s, "{ 32: PBB.00}"s, "c2f0000.gpio", "32", "12", "GPIO09",     "GP25",             None,         None },
                { "{ 32:   2}"s, "{ 32: PAA.02}"s, "c2f0000.gpio", "33", "13", "CAN1_DOUT",  "GP19_CAN1_DOUT",   None,         None },
                { "{164:  53}"s, "{164:  PI.02}"s, "2200000.gpio", "35", "19", "I2S2_FS",    "GP125",            None,         None },
                { "{164: 113}"s, "{164:  PR.05}"s, "2200000.gpio", "36", "16", "UART1_CTS",  "GP73_UART1_CTS_N", None,         None },
                { "{ 32:   3}"s, "{ 32: PAA.03}"s, "c2f0000.gpio", "37", "26", "CAN1_DIN",   "GP20_CAN1_DIN",    None,         None },
                { "{164:  52}"s, "{164:  PI.01}"s, "2200000.gpio", "38", "20", "I2S2_DIN",   "GP124",            None,         None },
                { "{164:  51}"s, "{164:  PI.00}"s, "2200000.gpio", "40", "21", "I2S2_DOUT",  "GP123",            None,         None }    
            };
        }

        static vector<PinDefinition> _CLARA_AGX_XAVIER_PIN_DEFS()
        {
            return {
                { "{224: 134, 169: 106}"s, "{169:  PQ.06}"s, "2200000.gpio", "7",  "4",   "MCLK05",     "SOC_GPIO42", None,       None },
                { "{224: 140, 169: 112}"s, "{169:  PR.04}"s, "2200000.gpio", "11", "17",  "UART1_RTS",  "UART1_RTS",  None,       None },
                { "{224:  63, 169:  51}"s, "{169:  PH.07}"s, "2200000.gpio", "12", "18",  "I2S2_CLK",   "DAP2_SCLK",  None,       None },
                { "{224: 124, 169:  96}"s, "{169:  PP.04}"s, "2200000.gpio", "13", "27",  "GPIO32",     "SOC_GPIO04", None,       None },
                // Older versions of L4T don"t enable this PWM controller in DT, so this PWM 
                // channel may not be available. 
                { "{224: 105, 169:  84}"s, "{169:  PN.01}"s, "2200000.gpio", "15", "22",  "GPIO27",     "SOC_GPIO54", "3280000.pwm", 0 },
                { "{ 40:   8,  30:   8}"s, "{ 30: PBB.00}"s, "c2f0000.gpio", "16", "23",  "GPIO8",      "CAN1_STB",   None,       None },
                { "{224:  56, 169:  44}"s, "{169:  PH.00}"s, "2200000.gpio", "18", "24",  "GPIO35",     "SOC_GPIO12", "32c0000.pwm", 0 },
                { "{224: 205, 169: 162}"s, "{169:  PZ.05}"s, "2200000.gpio", "19", "10",  "SPI1_MOSI",  "SPI1_MOSI",  None,       None },
                { "{224: 204, 169: 161}"s, "{169:  PZ.04}"s, "2200000.gpio", "21", "9",   "SPI1_MISO",  "SPI1_MISO",  None,       None },
                { "{224: 129, 169: 101}"s, "{169:  PQ.01}"s, "2200000.gpio", "22", "25",  "GPIO17",     "SOC_GPIO21", None,       None },
                { "{224: 203, 169: 160}"s, "{169:  PZ.03}"s, "2200000.gpio", "23", "11",  "SPI1_CLK",   "SPI1_SCK",   None,       None },
                { "{224: 206, 169: 163}"s, "{169:  PZ.06}"s, "2200000.gpio", "24", "8",   "SPI1_CS0_N", "SPI1_CS0_N", None,       None },
                { "{224: 207, 169: 164}"s, "{169:  PZ.07}"s, "2200000.gpio", "26", "7",   "SPI1_CS1_N", "SPI1_CS1_N", None,       None },
                { "{ 40:   3,  30:   3}"s, "{ 30: PAA.03}"s, "c2f0000.gpio", "29", "5",   "CAN0_DIN",   "CAN0_DIN",   None,       None },
                { "{ 40:   2,  30:   2}"s, "{ 30: PAA.02}"s, "c2f0000.gpio", "31", "6",   "CAN0_DOUT",  "CAN0_DOUT",  None,       None },
                { "{ 40:   9,  30:   9}"s, "{ 30: PBB.01}"s, "c2f0000.gpio", "32", "12",  "GPIO9",      "CAN1_EN",    None,       None },
                { "{ 40:   0,  30:   0}"s, "{ 30: PAA.00}"s, "c2f0000.gpio", "33", "13",  "CAN1_DOUT",  "CAN1_DOUT",  None,       None },
                { "{224:  66, 169:  54}"s, "{169:  PI.02}"s, "2200000.gpio", "35", "19",  "I2S2_FS",    "DAP2_FS",    None,       None },
                // Input-only (due to base board)
                { "{224: 141, 169: 113}"s, "{169:  PR.05}"s, "2200000.gpio", "36", "16",  "UART1_CTS", "UART1_CTS",   None,       None },
                { "{ 40:   1,  30:   1}"s, "{ 30: PAA.01}"s, "c2f0000.gpio",  "37", "26", "CAN1_DIN",  "CAN1_DIN",    None,       None },
                { "{224:  65, 169:  53}"s, "{169:  PI.01}"s, "2200000.gpio", "38", "20",  "I2S2_DIN",  "DAP2_DIN",    None,       None },
                { "{224:  64, 169:  52}"s, "{169:  PI.00}"s, "2200000.gpio", "40", "21",  "I2S2_DOUT", "DAP2_DOUT",   None,       None }
            };
        }

        static vector<PinDefinition> _JETSON_NX_PIN_DEFS()
        {
            return {
                { "{224: 148, 169: 118}"s, "{169:  PS.04}"s, "2200000.gpio", "7", "4",   "GPIO09",    "AUD_MCLK",   None,       None },
                { "{224: 140, 169: 112}"s, "{169:  PR.04}"s, "2200000.gpio", "11", "17", "UART1_RTS", "UART1_RTS",  None,       None },
                { "{224: 157, 169: 127}"s, "{169:  PT.05}"s, "2200000.gpio", "12", "18", "I2S0_SCLK", "DAP5_SCLK",  None,       None },
                { "{224: 192, 169: 149}"s, "{169:  PY.00}"s, "2200000.gpio", "13", "27", "SPI1_SCK",  "SPI3_SCK",   None,       None },
                { "{ 40:  20,  30:  16}"s, "{ 30: PCC.04}"s, "c2f0000.gpio", "15", "22", "GPIO12",    "TOUCH_CLK",  "c340000.pwm", 0 },
                { "{224: 196, 169: 153}"s, "{169:  PY.04}"s, "2200000.gpio", "16", "23", "SPI1_CS1",  "SPI3_CS1_N", None,       None },
                { "{224: 195, 169: 152}"s, "{169:  PY.03}"s, "2200000.gpio", "18", "24", "SPI1_CS0",  "SPI3_CS0_N", None,       None },
                { "{224: 205, 169: 162}"s, "{169:  PZ.05}"s, "2200000.gpio", "19", "10", "SPI0_MOSI", "SPI1_MOSI",  None,       None },
                { "{224: 204, 169: 161}"s, "{169:  PZ.04}"s, "2200000.gpio", "21", "9",  "SPI0_MISO", "SPI1_MISO",  None,       None },
                { "{224: 193, 169: 150}"s, "{169:  PY.01}"s, "2200000.gpio", "22", "25", "SPI1_MISO", "SPI3_MISO",  None,       None },
                { "{224: 203, 169: 160}"s, "{169:  PZ.03}"s, "2200000.gpio", "23", "11", "SPI0_SCK",  "SPI1_SCK",   None,       None },
                { "{224: 206, 169: 163}"s, "{169:  PZ.06}"s, "2200000.gpio", "24", "8",  "SPI0_CS0",  "SPI1_CS0_N", None,       None },
                { "{224: 207, 169: 164}"s, "{169:  PZ.07}"s, "2200000.gpio", "26", "7",  "SPI0_CS1",  "SPI1_CS1_N", None,       None },
                { "{224: 133, 169: 105}"s, "{169:  PQ.05}"s, "2200000.gpio", "29", "5",  "GPIO01",    "SOC_GPIO41", None,       None },
                { "{224: 134, 169: 106}"s, "{169:  PQ.06}"s, "2200000.gpio", "31", "6",  "GPIO11",    "SOC_GPIO42", None,       None },
                { "{224: 136, 169: 108}"s, "{169:  PR.00}"s, "2200000.gpio", "32", "12", "GPIO07",    "SOC_GPIO44", "32f0000.pwm", 0 },
                { "{224: 105, 169:  84}"s, "{169:  PN.01}"s, "2200000.gpio", "33", "13", "GPIO13",    "SOC_GPIO54", "3280000.pwm", 0 },
                { "{224: 160, 169: 130}"s, "{169:  PU.00}"s, "2200000.gpio", "35", "19", "I2S0_FS",   "DAP5_FS",    None,       None },
                { "{224: 141, 169: 113}"s, "{169:  PR.05}"s, "2200000.gpio", "36", "16", "UART1_CTS", "UART1_CTS",  None,       None },
                { "{224: 194, 169: 151}"s, "{169:  PY.02}"s, "2200000.gpio", "37", "26", "SPI1_MOSI", "SPI3_MOSI",  None,       None },
                { "{224: 159, 169: 129}"s, "{169:  PT.07}"s, "2200000.gpio", "38", "20", "I2S0_DIN",  "DAP5_DIN",   None,       None },
                { "{224: 158, 169: 128}"s, "{169:  PT.06}"s, "2200000.gpio", "40", "21", "I2S0_DOUT", "DAP5_DOUT",  None,       None }
            };
        }

        static vector<PinDefinition> _JETSON_XAVIER_PIN_DEFS()
        {
            return {
                { "{224: 134, 169: 106}"s, "{169:  PQ.06}"s, "2200000.gpio", "7", "4",   "MCLK05",     "SOC_GPIO42", None,       None },
                { "{224: 140, 169: 112}"s, "{169:  PR.04}"s, "2200000.gpio", "11", "17", "UART1_RTS",  "UART1_RTS",  None,       None },
                { "{224:  63, 169:  51}"s, "{169:  PH.07}"s, "2200000.gpio", "12", "18", "I2S2_CLK",   "DAP2_SCLK",  None,       None },
                { "{224: 136, 169: 108}"s, "{169:  PR.00}"s, "2200000.gpio", "13", "27", "PWM01",      "SOC_GPIO44", "32f0000.pwm", 0 },
                // Older versions of L4T don"t enable this PWM controller in DT, so this PWM 
                // channel may not be available. 
                { "{224: 105, 169:  84}"s, "{169:  PN.01}"s, "2200000.gpio", "15", "22", "GPIO27",     "SOC_GPIO54", "3280000.pwm", 0 },
                { "{ 40:   8,  30:   8}"s, "{ 30: PBB.00}"s, "c2f0000.gpio", "16", "23", "GPIO8",      "CAN1_STB",   None,       None },
                { "{224:  56, 169:  44}"s, "{169:  PH.00}"s, "2200000.gpio", "18", "24", "GPIO35",     "SOC_GPIO12", "32c0000.pwm", 0 },
                { "{224: 205, 169: 162}"s, "{169:  PZ.05}"s, "2200000.gpio", "19", "10", "SPI1_MOSI",  "SPI1_MOSI",  None,       None },
                { "{224: 204, 169: 161}"s, "{169:  PZ.04}"s, "2200000.gpio", "21", "9",  "SPI1_MISO",  "SPI1_MISO",  None,       None },
                { "{224: 129, 169: 101}"s, "{169:  PQ.01}"s, "2200000.gpio", "22", "25", "GPIO17",     "SOC_GPIO21", None,       None },
                { "{224: 203, 169: 160}"s, "{169:  PZ.03}"s, "2200000.gpio", "23", "11", "SPI1_CLK",   "SPI1_SCK",   None,       None },
                { "{224: 206, 169: 163}"s, "{169:  PZ.06}"s, "2200000.gpio", "24", "8",  "SPI1_CS0_N", "SPI1_CS0_N", None,       None },
                { "{224: 207, 169: 164}"s, "{169:  PZ.07}"s, "2200000.gpio", "26", "7",  "SPI1_CS1_N", "SPI1_CS1_N", None,       None },
                { "{ 40:   3,  30:   3}"s, "{ 30: PAA.03}"s, "c2f0000.gpio", "29", "5",  "CAN0_DIN",   "CAN0_DIN",   None,       None },
                { "{ 40:   2,  30:   2}"s, "{ 30: PAA.02}"s, "c2f0000.gpio", "31", "6",  "CAN0_DOUT",  "CAN0_DOUT",  None,       None },
                { "{ 40:   9,  30:   9}"s, "{ 30: PBB.01}"s, "c2f0000.gpio", "32", "12", "GPIO9",      "CAN1_EN",    None,       None },
                { "{ 40:   0,  30:   0}"s, "{ 30: PAA.00}"s, "c2f0000.gpio", "33", "13", "CAN1_DOUT",  "CAN1_DOUT",  None,       None },
                { "{224:  66, 169:  54}"s, "{169:  PI.02}"s, "2200000.gpio", "35", "19", "I2S2_FS",    "DAP2_FS",    None,       None },
                // Input-only (due to base board)
                { "{224: 141, 169: 113}"s, "{169:  PR.05}"s, "2200000.gpio", "36", "16", "UART1_CTS",  "UART1_CTS",  None,       None },
                { "{ 40:   1,  30:   1}"s, "{ 30: PAA.01}"s, "c2f0000.gpio", "37", "26", "CAN1_DIN",   "CAN1_DIN",   None,       None },
                { "{224:  65, 169:  53}"s, "{169:  PI.01}"s, "2200000.gpio", "38", "20", "I2S2_DIN",   "DAP2_DIN",   None,       None },
                { "{224:  64, 169:  52}"s, "{169:  PI.00}"s, "2200000.gpio", "40", "21", "I2S2_DOUT",  "DAP2_DOUT",  None,       None }
            };
        }

        static vector<PinDefinition> _JETSON_TX2_NX_PIN_DEFS()
        {
            return {
                { "{192: 76,  140:  66}"s, "{140:  PJ.04}"s, "2200000.gpio", "7",  "4",  "GPIO09",    "AUD_MCLK",  None,       None },
                { "{64:  28,  47:   23}"s, "{47:   PW.04}"s, "c2f0000.gpio", "11", "17", "UART1_RTS", "UART3_RTS", None,       None },
                { "{192: 72,  140:  62}"s, "{140:  PJ.00}"s, "2200000.gpio", "12", "18", "I2S0_SCLK", "DAP1_SCLK", None,       None },
                { "{64:  17,  47:   12}"s, "{47:   PV.01}"s, "c2f0000.gpio", "13", "27", "SPI1_SCK",  "GPIO_SEN1", None,       None },
                { "{192: 18,  140:  16}"s, "{140:  PC.02}"s, "2200000.gpio", "15", "22", "GPIO12",    "DAP2_DOUT", None,       None },
                { "{192: 19,  140:  17}"s, "{140:  PC.03}"s, "2200000.gpio", "16", "23", "SPI1_CS1",  "DAP2_DIN",  None,       None },
                { "{64:  20,  47:   15}"s, "{47:   PV.04}"s, "c2f0000.gpio", "18", "24", "SPI1_CS0",  "GPIO_SEN4", None,       None },
                { "{192: 58,  140:  49}"s, "{140:  PH.02}"s, "2200000.gpio", "19", "10", "SPI0_MOSI", "GPIO_WAN7", None,       None },
                { "{192: 57,  140:  48}"s, "{140:  PH.01}"s, "2200000.gpio", "21", "9",  "SPI0_MISO", "GPIO_WAN6", None,       None },
                { "{64:  18,  47:   13}"s, "{47:   PV.02}"s, "c2f0000.gpio", "22", "25", "SPI1_MISO", "GPIO_SEN2", None,       None },
                { "{192: 56,  140:  47}"s, "{140:  PH.00}"s, "2200000.gpio", "23", "11", "SPI1_CLK",  "GPIO_WAN5", None,       None },
                { "{192: 59,  140:  50}"s, "{140:  PH.03}"s, "2200000.gpio", "24", "8",  "SPI0_CS0",  "GPIO_WAN8", None,       None },
                { "{192: 163, 140: 130}"s, "{140:  PY.03}"s, "2200000.gpio", "26", "7",  "SPI0_CS1",  "GPIO_MDM4", None,       None },
                { "{192: 105, 140:  86}"s, "{140:  PN.01}"s, "2200000.gpio", "29", "5",  "GPIO01",    "GPIO_CAM2", None,       None },
                { "{64:  50,  47:   41}"s, "{47:  PEE.02}"s, "c2f0000.gpio", "31", "6",  "GPIO11",    "TOUCH_CLK", None,       None },
                { "{64:  8,   47:    5}"s, "{47:   PU.00}"s, "c2f0000.gpio", "32", "12", "GPIO07",    "GPIO_DIS0", "3280000.pwm", 0 },
                { "{64:  13,  47:   10}"s, "{47:   PU.05}"s, "c2f0000.gpio", "33", "13", "GPIO13",    "GPIO_DIS5", "32a0000.pwm", 0 },
                { "{192: 75,  140:  65}"s, "{140:  PJ.03}"s, "2200000.gpio", "35", "19", "I2S0_FS",   "DAP1_FS",   None,       None },
                { "{64:  29,  47:   24}"s, "{47:   PW.05}"s, "c2f0000.gpio", "36", "16", "UART1_CTS", "UART3_CTS", None,       None },
                { "{64:  19,  47:   14}"s, "{47:   PV.03}"s, "c2f0000.gpio", "37", "26", "SPI1_MOSI", "GPIO_SEN3", None,       None },
                { "{192: 74,  140:  64}"s, "{140:  PJ.02}"s, "2200000.gpio", "38", "20", "I2S0_DIN",  "DAP1_DIN",  None,       None },
                { "{192: 73,  140:  63}"s, "{140:  PJ.01}"s, "2200000.gpio", "40", "21", "I2S0_DOUT", "DAP1_DOUT", None,       None }    
            };
        }

        static vector<PinDefinition> _JETSON_TX2_PIN_DEFS()
        {
            return {
                { "{192:  76, 140:  66}"s, "{140:  PJ.04}"s, "2200000.gpio",             "7", "4",   "AUDIO_MCLK",         "AUD_MCLK",     None, None },
                // Output-only (due to base board)                   
                { "{192: 146, 140: 117}"s, "{140:  PT.02}"s, "2200000.gpio",             "11", "17", "UART0_RTS",          "UART1_RTS",    None, None },
                { "{192:  72, 140:  62}"s, "{140:  PJ.00}"s, "2200000.gpio",             "12", "18", "I2S0_CLK",           "DAP1_SCLK",    None, None },
                { "{192:  77, 140:  67}"s, "{140:  PJ.05}"s, "2200000.gpio",             "13", "27", "GPIO20_AUD_INT",     "GPIO_AUD0",    None, None },
                { "                  15"s, "           {}"s, "3160000.i2c/i2c-0/0-0074", "15", "22", "GPIO_EXP_P17",       "GPIO_EXP_P17", None, None },
                // Input-only (due to module):
                { "{ 64:  40,  47:  31}"s, "{ 47: PAA.00}"s, "c2f0000.gpio",             "16", "23", "AO_DMIC_IN_DAT",     "CAN_GPIO0",    None, None },
                { "{192: 161, 140: 128}"s, "{140:  PY.01}"s, "2200000.gpio",             "18", "24", "GPIO16_MDM_WAKE_AP", "GPIO_MDM2",    None, None },
                { "{192: 109, 140:  90}"s, "{140:  PN.05}"s, "2200000.gpio",             "19", "10", "SPI1_MOSI",          "GPIO_CAM6",    None, None },
                { "{192: 108, 140:  89}"s, "{140:  PN.04}"s, "2200000.gpio",             "21", "9",  "SPI1_MISO",          "GPIO_CAM5",    None, None },
                { "                  14"s, "           {}"s, "3160000.i2c/i2c-0/0-0074", "22", "25", "GPIO_EXP_P16",       "GPIO_EXP_P16", None, None },
                { "{192: 107, 140:  88}"s, "{140:  PN.03}"s, "2200000.gpio",             "23", "11", "SPI1_CLK",           "GPIO_CAM4",    None, None },
                { "{192: 110, 140:  91}"s, "{140:  PN.06}"s, "2200000.gpio",             "24", "8",  "SPI1_CS0",           "GPIO_CAM7",    None, None },
                // Board pin 26 is not available on this board   
                { "{192:  78, 140:  68}"s, "{140:  PJ.06}"s, "2200000.gpio",             "29", "5",  "GPIO19_AUD_RST",     "GPIO_AUD1",    None, None },
                { "{ 64:  42,  47:  33}"s, "{ 47: PAA.02}"s, "c2f0000.gpio",             "31", "6",  "GPIO9_MOTION_INT",   "CAN_GPIO2",    None, None },
                // Output-only (due to module):             
                { "{ 64:  41,  47:  32}"s, "{ 47: PAA.01}"s, "c2f0000.gpio",             "32", "12", "AO_DMIC_IN_CLK",     "CAN_GPIO1",    None, None },
                { "{192:  69, 140:  59}"s, "{140:  PI.05}"s, "2200000.gpio",             "33", "13", "GPIO11_AP_WAKE_BT",  "GPIO_PQ5",     None, None },
                { "{192:  75, 140:  65}"s, "{140:  PJ.03}"s, "2200000.gpio",             "35", "19", "I2S0_LRCLK",         "DAP1_FS",      None, None },
                // Input-only (due to base board) IF NVIDIA debug card NOT plugged in   
                // Output-only (due to base board) IF NVIDIA debug card plugged in   
                { "{192: 147, 140: 118}"s, "{140:  PT.03}"s, "2200000.gpio",             "36", "16", "UART0_CTS",          "UART1_CTS",    None, None },
                { "{192:  68, 140:  58}"s, "{140:  PI.04}"s, "2200000.gpio",             "37", "26", "GPIO8_ALS_PROX_INT", "GPIO_PQ4",     None, None },
                { "{192:  74, 140:  64}"s, "{140:  PJ.02}"s, "2200000.gpio",             "38", "20", "I2S0_SDIN",          "DAP1_DIN",     None, None },
                { "{192:  73, 140:  63}"s, "{140:  PJ.01}"s, "2200000.gpio",             "40", "21", "I2S0_SDOUT",         "DAP1_DOUT",    None, None }
            };
        }

        static vector<PinDefinition> _JETSON_TX1_PIN_DEFS()
        {
            return {
                { "216"s, "{}"s, "6000d000.gpio",             "7", "4",   "AUDIO_MCLK",         "AUD_MCLK",      None, None },
                // Output-only (due to base board)                            
                { "162"s, "{}"s, "6000d000.gpio",             "11", "17", "UART0_RTS",          "UART1_RTS",     None, None },
                { "11"s,  "{}"s, "6000d000.gpio",             "12", "18", "I2S0_CLK",           "DAP1_SCLK",     None, None },
                { "38"s,  "{}"s, "6000d000.gpio",             "13", "27", "GPIO20_AUD_INT",     "GPIO_PE6",      None, None },
                { "15"s,  "{}"s, "7000c400.i2c/i2c-1/1-0074", "15", "22", "GPIO_EXP_P17",       "GPIO_EXP_P17",  None, None },
                { "37"s,  "{}"s, "6000d000.gpio",             "16", "23", "AO_DMIC_IN_DAT",     "DMIC3_DAT",     None, None },
                { "184"s, "{}"s, "6000d000.gpio",             "18", "24", "GPIO16_MDM_WAKE_AP", "MODEM_WAKE_AP", None, None },
                { "16"s,  "{}"s, "6000d000.gpio",             "19", "10", "SPI1_MOSI",          "SPI1_MOSI",     None, None },
                { "17"s,  "{}"s, "6000d000.gpio",             "21", "9",  "SPI1_MISO",          "SPI1_MISO",     None, None },
                { "14"s,  "{}"s, "7000c400.i2c/i2c-1/1-0074", "22", "25", "GPIO_EXP_P16",       "GPIO_EXP_P16",  None, None },
                { "18"s,  "{}"s, "6000d000.gpio",             "23", "11", "SPI1_CLK",           "SPI1_SCK",      None, None },
                { "19"s,  "{}"s, "6000d000.gpio",             "24", "8",  "SPI1_CS0",           "SPI1_CS0",      None, None },
                { "20"s,  "{}"s, "6000d000.gpio",             "26", "7",  "SPI1_CS1",           "SPI1_CS1",      None, None },
                { "219"s, "{}"s, "6000d000.gpio",             "29", "5",  "GPIO19_AUD_RST",     "GPIO_X1_AUD",   None, None },
                { "186"s, "{}"s, "6000d000.gpio",             "31", "6",  "GPIO9_MOTION_INT",   "MOTION_INT",    None, None },
                { "36"s,  "{}"s, "6000d000.gpio",             "32", "12", "AO_DMIC_IN_CLK",     "DMIC3_CLK",     None, None },
                { "63"s,  "{}"s, "6000d000.gpio",             "33", "13", "GPIO11_AP_WAKE_BT",  "AP_WAKE_NFC",   None, None },
                { "8"s,   "{}"s, "6000d000.gpio",             "35", "19", "I2S0_LRCLK",         "DAP1_FS",       None, None },
                // Input-only (due to base board) IF NVIDIA debug card NOT plugged in
                // Input-only (due to base board) (always reads fixed value) IF NVIDIA debug card plugged in
                { "163"s,  "{}"s, "6000d000.gpio",            "36", "16", "UART0_CTS",          "UART1_CTS",     None, None },
                { "187"s,  "{}"s, "6000d000.gpio",            "37", "26", "GPIO8_ALS_PROX_INT", "ALS_PROX_INT",  None, None },
                { "9"s,    "{}"s, "6000d000.gpio",            "38", "20", "I2S0_SDIN",          "DAP1_DIN",      None, None },
                { "10"s,   "{}"s, "6000d000.gpio",            "40", "21", "I2S0_SDOUT",         "DAP1_DOUT",     None, None }
            };
        }

        static vector<PinDefinition> _JETSON_NANO_PIN_DEFS()
        {
            return {
                { "216"s, "{}"s, "6000d000.gpio", "7", "4",   "GPIO9",     "AUD_MCLK",  None,        None },
                { "50"s,  "{}"s, "6000d000.gpio", "11", "17", "UART1_RTS", "UART2_RTS", None,        None },
                { "79"s,  "{}"s, "6000d000.gpio", "12", "18", "I2S0_SCLK", "DAP4_SCLK", None,        None },
                { "14"s,  "{}"s, "6000d000.gpio", "13", "27", "SPI1_SCK",  "SPI2_SCK",  None,        None },
                { "194"s, "{}"s, "6000d000.gpio", "15", "22", "GPIO12",    "LCD_TE",    None,        None },
                { "232"s, "{}"s, "6000d000.gpio", "16", "23", "SPI1_CS1",  "SPI2_CS1",  None,        None },
                { "15"s,  "{}"s, "6000d000.gpio", "18", "24", "SPI1_CS0",  "SPI2_CS0",  None,        None },
                { "16"s,  "{}"s, "6000d000.gpio", "19", "10", "SPI0_MOSI", "SPI1_MOSI", None,        None },
                { "17"s,  "{}"s, "6000d000.gpio", "21", "9",  "SPI0_MISO", "SPI1_MISO", None,        None },
                { "13"s,  "{}"s, "6000d000.gpio", "22", "25", "SPI1_MISO", "SPI2_MISO", None,        None },
                { "18"s,  "{}"s, "6000d000.gpio", "23", "11", "SPI0_SCK",  "SPI1_SCK",  None,        None },
                { "19"s,  "{}"s, "6000d000.gpio", "24", "8",  "SPI0_CS0",  "SPI1_CS0",  None,        None },
                { "20"s,  "{}"s, "6000d000.gpio", "26", "7",  "SPI0_CS1",  "SPI1_CS1",  None,        None },
                { "149"s, "{}"s, "6000d000.gpio", "29", "5",  "GPIO01",    "CAM_AF_EN", None,        None },
                { "200"s, "{}"s, "6000d000.gpio", "31", "6",  "GPIO11",    "GPIO_PZ0",  None,        None },
                // Older versions of L4T have a DT bug which instantiates a bogus device
                // which prevents this library from using this PWM channel.
                { "168"s, "{}"s, "6000d000.gpio", "32", "12", "GPIO07",    "LCD_BL_PW", "7000a000.pwm", 0 },
                { "38"s,  "{}"s, "6000d000.gpio", "33", "13", "GPIO13",    "GPIO_PE6",  "7000a000.pwm", 2 },
                { "76"s,  "{}"s, "6000d000.gpio", "35", "19", "I2S0_FS",   "DAP4_FS",   None,        None },
                { "51"s,  "{}"s, "6000d000.gpio", "36", "16", "UART1_CTS", "UART2_CTS", None,        None },
                { "12"s,  "{}"s, "6000d000.gpio", "37", "26", "SPI1_MOSI", "SPI2_MOSI", None,        None },
                { "77"s,  "{}"s, "6000d000.gpio", "38", "20", "I2S0_DIN",  "DAP4_DIN",  None,        None },
                { "78"s,  "{}"s, "6000d000.gpio", "40", "21", "I2S0_DOUT", "DAP4_DOUT", None,        None }
            };
        }
    };

    EntirePinData::EntirePinData()
        : 
        compats_jetson_orins_nano
        {
            "nvidia,p3509-0000+p3767-0003",
            "nvidia,p3768-0000+p3767-0003",
            "nvidia,p3509-0000+p3767-0004",
            "nvidia,p3768-0000+p3767-0004",
            "nvidia,p3509-0000+p3767-0005",
            "nvidia,p3768-0000+p3767-0005"
        },
        JETSON_ORIN_NX_PIN_DEFS{ _JETSON_ORIN_NX_PIN_DEFS() },
        compats_jetson_orins_nx
        {
            "nvidia,p3509-0000+p3767-0000",
            "nvidia,p3768-0000+p3767-0000",
            "nvidia,p3509-0000+p3767-0001",
            "nvidia,p3768-0000+p3767-0001"
        },

        JETSON_ORIN_PIN_DEFS{ _JETSON_ORIN_PIN_DEFS() },
        compats_jetson_orins
        {
            "nvidia,p3737-0000+p3701-0000",
            "nvidia,p3737-0000+p3701-0004",
        },

        CLARA_AGX_XAVIER_PIN_DEFS{ _CLARA_AGX_XAVIER_PIN_DEFS() },
        compats_clara_agx_xavier
        {
            "nvidia,e3900-0000+p2888-0004"
        },

        JETSON_NX_PIN_DEFS{ _JETSON_NX_PIN_DEFS() },
        compats_nx
        {
            "nvidia,p3509-0000+p3668-0000",
            "nvidia,p3509-0000+p3668-0001",
            "nvidia,p3449-0000+p3668-0000",
            "nvidia,p3449-0000+p3668-0001",
            "nvidia,p3449-0000+p3668-0003",
        },

        JETSON_XAVIER_PIN_DEFS{ _JETSON_XAVIER_PIN_DEFS() },
        compats_xavier
        {
            "nvidia,p2972-0000",
            "nvidia,p2972-0006",
            "nvidia,jetson-xavier",
            "nvidia,galen-industrial",
            "nvidia,jetson-xavier-industrial"
        },

        JETSON_TX2_NX_PIN_DEFS{ _JETSON_TX2_NX_PIN_DEFS() },
        compats_tx2_nx
        {
            "nvidia,p3509-0000+p3636-0001"
        },

        JETSON_TX2_PIN_DEFS{ _JETSON_TX2_PIN_DEFS() },
        compats_tx2
        {
            "nvidia,p2771-0000",
            "nvidia,p2771-0888",
            "nvidia,p3489-0000",
            "nvidia,lightning",
            "nvidia,quill",
            "nvidia,storm"
        },

        JETSON_TX1_PIN_DEFS{ _JETSON_TX1_PIN_DEFS() },
        compats_tx1
        {
            "nvidia,p2371-2180",
            "nvidia,jetson-cv"
        },

        JETSON_NANO_PIN_DEFS{ _JETSON_NANO_PIN_DEFS() },
        compats_nano
        {
            "nvidia,p3450-0000",
            "nvidia,p3450-0002",
            "nvidia,jetson-nano"
        },

        PIN_DEFS_MAP
        {
            { JETSON_ORIN_NANO, JETSON_ORIN_NX_PIN_DEFS },
            { JETSON_ORIN_NX, JETSON_ORIN_NX_PIN_DEFS },
            { JETSON_ORIN, JETSON_ORIN_PIN_DEFS },
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
            { JETSON_ORIN_NANO, {1, "32768M, 65536M", "Unknown", "JETSON_ORIN_NANO", "NVIDIA", "A78AE"} },
            { JETSON_ORIN_NX, {1, "32768M, 65536M",  "Unknown", "JETSON_ORIN_NX", "NVIDIA", "A78AE"} },
            { JETSON_ORIN, {1, "32768M, 65536M",  "Unknown", "JETSON_ORIN", "NVIDIA", "A78AE"} },
            { CLARA_AGX_XAVIER, {1, "16384M",  "Unknown", "CLARA_AGX_XAVIER", "NVIDIA", "ARM Carmel"} },
            { JETSON_NX, {1, "16384M, 8192M", "Unknown", "Jetson NX", "NVIDIA", "ARM Carmel"} },
            { JETSON_XAVIER, {1, "65536M, 32768M, 16384M, 8192M", "Unknown", "Jetson Xavier", "NVIDIA", "ARM Carmel"} },
            { JETSON_TX2_NX, {1, "4096M", "Unknown", "Jetson TX2 NX", "NVIDIA", "ARM A57 + Denver"} },
            { JETSON_TX2, {1, "8192M, 4096M", "Unknown", "Jetson TX2", "NVIDIA", "ARM A57 + Denver"} },
            { JETSON_TX1, {1, "4096M", "Unknown", "Jetson TX1", "NVIDIA", "ARM A57"} },
            { JETSON_NANO, {1, "4096M, 2048M", "Unknown", "Jetson nano", "NVIDIA", "ARM A57"} }
        }
    {}

    // clang-format on

    std::string PinInfo::JETSON_INFO() const
    {
        stringstream ss{};
        ss << "[JETSON_INFO]\n";
        ss << "P1_REVISION: " << P1_REVISION << endl;
        ss << "RAM: " << RAM << endl;
        ss << "REVISION: " << REVISION << endl;
        ss << "TYPE: " << TYPE << endl;
        ss << "MANUFACTURER: " << MANUFACTURER << endl;
        ss << "PROCESSOR: " << PROCESSOR << endl;
        return ss.str();
    }

    static bool ids_warned = false;

    string find_pmgr_board(const string& prefix)
    {
        constexpr auto ids_path = "/proc/device-tree/chosen/plugin-manager/ids";
        constexpr auto ids_path_k510 = "/proc/device-tree/chosen/ids";

        if (os_path_exists(ids_path))
        {
            for (const auto& file : os_listdir(ids_path))
            {
                if (startswith(file, prefix))
                    return file;
            }
        }
        else if (os_path_exists(ids_path_k510))
        {
            std::ifstream f(ids_path_k510);
            std::string s{};
            while (f >> s)
            {
                if (startswith(s, prefix))
                    return s;
            }
        }
        else if (ids_warned == false)
        {
            ids_warned = true;
            string msg = "WARNING: Plugin manager information missing from device tree.\n"
                         "WARNING: Cannot determine whether the expected Jetson board is present.";
            cerr << msg;
        }

        return None;
    }

    void warn_if_not_carrier_board(const vector<string>& carrier_boards)
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
    }

    set<string> get_compatibles(const string& compatible_path)
    {
        ifstream f(compatible_path);
        vector<string> v(split(read(f), '\x00'));

        // convert to std::set
        set<string> compatibles{v.begin(), v.end()};
        return compatibles;
    }

    Model get_model()
    {
        constexpr auto compatible_path = "/proc/device-tree/compatible";

        // get model info from compatible_path
        if (os_path_exists(compatible_path))
        {
            set<string> compatibles = get_compatibles(compatible_path);

            auto matches = [&compatibles](const vector<string>& vals)
            {
                for (const auto& v : vals)
                {
                    if (is_in(v, compatibles))
                        return true;
                }
                return false;
            };

            auto& _DATA = EntirePinData::get_instance();

            if (matches(_DATA.compats_tx1))
            {
                warn_if_not_carrier_board({"2597"s});
                return JETSON_TX1;
            }
            else if (matches(_DATA.compats_tx2))
            {
                warn_if_not_carrier_board({"2597"s});
                return JETSON_TX2;
            }
            else if (matches(_DATA.compats_clara_agx_xavier))
            {
                warn_if_not_carrier_board({"3900"s});
                return CLARA_AGX_XAVIER;
            }
            else if (matches(_DATA.compats_tx2_nx))
            {
                warn_if_not_carrier_board({"3509"s});
                return JETSON_TX2_NX;
            }
            else if (matches(_DATA.compats_xavier))
            {
                warn_if_not_carrier_board({"2822"s});
                return JETSON_XAVIER;
            }
            else if (matches(_DATA.compats_nano))
            {
                string module_id = find_pmgr_board("3448");

                if (is_None(module_id))
                    throw runtime_error("Could not determine Jetson Nano module revision");
                string revision = split(module_id, '-').back();
                // Revision is an ordered string, not a decimal integer
                if (revision < "200")
                    throw runtime_error("Jetson Nano module revision must be A02 or later");

                warn_if_not_carrier_board({"3449"s, "3542"s});
                return JETSON_NANO;
            }
            else if (matches(_DATA.compats_nx))
            {
                warn_if_not_carrier_board({"3509"s, "3449"s});
                return JETSON_NX;
            }
            else if (matches(_DATA.compats_jetson_orins))
            {
                warn_if_not_carrier_board({"3737"s});
                return JETSON_ORIN;
            }
            else if (matches(_DATA.compats_jetson_orins_nx))
            {
                warn_if_not_carrier_board({"3509"s, "3768"s});
                return JETSON_ORIN_NX;
            }
            else if (matches(_DATA.compats_jetson_orins_nano))
            {
                warn_if_not_carrier_board({"3509"s, "3768"s});
                return JETSON_ORIN_NANO;
            }
        }

        // get model info from the environment variables for docker containers
        const char* _model_name = std::getenv("JETSON_MODEL_NAME");

        if (_model_name != nullptr)
        {
            auto idx = model_name_index(_model_name);

            if (!is_None(idx))
                return index_to_model(idx);

            std::cerr << format("%s is an invalid model name.", _model_name);
        }

        throw runtime_error("Could not determine Jetson model");
    }

    PinData get_data()
    {
        try
        {
            auto& _DATA = EntirePinData::get_instance();
            auto model = get_model();

            vector<PinDefinition> pin_defs = _DATA.PIN_DEFS_MAP.at(model);
            PinInfo jetson_info = _DATA.JETSON_INFO_MAP.at(model);

            map<string, string> gpio_chip_dirs{};
            map<string, int> gpio_chip_base{};
            map<string, string> gpio_chip_ngpio{};
            map<string, string> pwm_dirs{};

            vector<string> sysfs_prefixes = {"/sys/devices/", "/sys/devices/platform/"};

            // Get the gpiochip offsets
            set<string> gpio_chip_names{};
            for (const auto& pin_def : pin_defs)
            {
                if (!is_None(pin_def.SysfsDir))
                    gpio_chip_names.insert(pin_def.SysfsDir);
            }

            for (const auto& gpio_chip_name : gpio_chip_names)
            {
                string gpio_chip_dir = None;
                for (const auto& prefix : sysfs_prefixes)
                {
                    auto d = prefix + gpio_chip_name;
                    if (os_path_isdir(d))
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
                        gpio_chip_base[gpio_chip_name] = stoi(strip(read(f)));
                    } // scope ends

                    string ngpio_fn = gpio_chip_gpio_dir + "/" + fn + "/ngpio";
                    { // scope for f
                        ifstream f(ngpio_fn);
                        gpio_chip_ngpio[gpio_chip_name] = strip(read(f));
                    } // scope ends

                    break;
                }
            }

            auto global_gpio_id_name = [&gpio_chip_base, &gpio_chip_ngpio](DictionaryLike chip_relative_ids,
                                                                           DictionaryLike gpio_names,
                                                                           string gpio_chip_name) -> tuple<int, string>
            {
                if (!is_in(gpio_chip_name, gpio_chip_ngpio))
                    return {None, None};

                auto chip_gpio_ngpio = gpio_chip_ngpio[gpio_chip_name];

                auto chip_relative_id = chip_relative_ids.get(chip_gpio_ngpio);

                auto gpio = gpio_chip_base[gpio_chip_name] + stoi(strip(chip_relative_id));

                auto gpio_name = gpio_names.get(chip_gpio_ngpio);

                if (is_None(gpio_name))
                    gpio_name = format("gpio%i", gpio);

                return {gpio, gpio_name};
            };

            set<string> pwm_chip_names{};
            for (const auto& x : pin_defs)
            {
                if (!is_None(x.PWMSysfsDir))
                    pwm_chip_names.insert(x.PWMSysfsDir);
            }

            for (const auto& pwm_chip_name : pwm_chip_names)
            {
                string pwm_chip_dir = None;
                for (const auto& prefix : sysfs_prefixes)
                {
                    auto d = prefix + pwm_chip_name;
                    if (os_path_isdir(d))
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

            auto model_data =
                [&global_gpio_id_name, &pwm_dirs, &gpio_chip_dirs](NumberingModes key, const auto& pin_defs)
            {
                auto get_or = [](const auto& dictionary, const string& x, const string& defaultValue) -> string
                { return is_in(x, dictionary) ? dictionary.at(x) : defaultValue; };

                map<string, ChannelInfo> ret{};

                for (const auto& x : pin_defs)
                {
                    string pinName = x.PinName(key);

                    if (!is_in(x.SysfsDir, gpio_chip_dirs))
                        throw std::runtime_error("[model_data]"s + x.SysfsDir + " is not in gpio_chip_dirs"s);

                    auto tmp = global_gpio_id_name(x.LinuxPin, x.ExportedName, x.SysfsDir);
                    auto gpio = get<0>(tmp);
                    auto gpio_name = get<1>(tmp);

                    ret.insert({pinName, ChannelInfo{pinName, gpio_chip_dirs.at(x.SysfsDir), gpio, gpio_name,
                                                     get_or(pwm_dirs, x.PWMSysfsDir, None), x.PWMID}});
                }
                return ret;
            };

            map<NumberingModes, map<string, ChannelInfo>> channel_data = {{BOARD, model_data(BOARD, pin_defs)},
                                                                          {BCM, model_data(BCM, pin_defs)},
                                                                          {CVM, model_data(CVM, pin_defs)},
                                                                          {TEGRA_SOC, model_data(TEGRA_SOC, pin_defs)}};

            return {model, jetson_info, channel_data};
        }
        catch (exception& e)
        {
            throw _error(e, "GPIO::get_data()");
        }
    }
} // namespace GPIO
