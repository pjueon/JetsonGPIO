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

#include "private/gpio_pin_data.h"
#include "JetsonGPIO.h"
#include "private/PythonFunctions.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <algorithm>
#include <iterator>

using namespace GPIO;
using namespace std;

// ========================================= Begin of "gpio_pin_data.py"
// =========================================

/* These vectors contain all the relevant GPIO data for each Jetson
   The values are use to generate dictionaries that map the corresponding pin
   mode numbers to the Linux GPIO pin number and GPIO chip directory */
bool ids_warned = false;

// Global variables are wrapped in singleton pattern in order to avoid
// initialization order of global variables in different compilation units
// problem
class PIN_DATA {
 private:
  PIN_DATA();

 public:
  const vector<_GPIO_PIN_DEF> JETSON_XAVIER_PIN_DEFS;
  const vector<string> compats_xavier;
  const vector<_GPIO_PIN_DEF> JETSON_TX2_PIN_DEFS;
  const vector<string> compats_tx2;
  const vector<_GPIO_PIN_DEF> JETSON_TX1_PIN_DEFS;
  const vector<string> compats_tx1;
  const vector<_GPIO_PIN_DEF> JETSON_NANO_PIN_DEFS;
  const vector<string> compats_nano;
  const map<Model, vector<_GPIO_PIN_DEF>> PIN_DEFS_MAP;
  const map<Model, _GPIO_PIN_INFO> JETSON_INFO_MAP;

  PIN_DATA(const PIN_DATA&) = delete;
  PIN_DATA& operator=(const PIN_DATA&) = delete;
  ~PIN_DATA() = default;

  static PIN_DATA& get_instance() {
    static PIN_DATA singleton;
    return singleton;
  }
};

PIN_DATA::PIN_DATA()
    : JETSON_XAVIER_PIN_DEFS{{134, "/sys/devices/2200000.gpio", "7", "4",
                              "MCLK05", "SOC_GPIO42", "None", -1},
                             {140, "/sys/devices/2200000.gpio", "11", "17",
                              "UART1_RTS", "UART1_RTS", "None", -1},
                             {63, "/sys/devices/2200000.gpio", "12", "18",
                              "I2S2_CLK", "DAP2_SCLK", "None", -1},
                             {136, "/sys/devices/2200000.gpio", "13", "27",
                              "PWM01", "SOC_GPIO44", "/sys/devices/32f0000.pwm",
                              0},
                             // Older versions of L4T don"t enable this PWM
                             // controller in DT, so this PWM channel may not be
                             // available.
                             {105, "/sys/devices/2200000.gpio", "15", "22",
                              "GPIO27", "SOC_GPIO54",
                              "/sys/devices/3280000.pwm", 0},
                             {8, "/sys/devices/c2f0000.gpio", "16", "23",
                              "GPIO8", "CAN1_STB", "None", -1},
                             {56, "/sys/devices/2200000.gpio", "18", "24",
                              "GPIO35", "SOC_GPIO12",
                              "/sys/devices/32c0000.pwm", 0},
                             {205, "/sys/devices/2200000.gpio", "19", "10",
                              "SPI1_MOSI", "SPI1_MOSI", "None", -1},
                             {204, "/sys/devices/2200000.gpio", "21", "9",
                              "SPI1_MISO", "SPI1_MISO", "None", -1},
                             {129, "/sys/devices/2200000.gpio", "22", "25",
                              "GPIO17", "SOC_GPIO21", "None", -1},
                             {203, "/sys/devices/2200000.gpio", "23", "11",
                              "SPI1_CLK", "SPI1_SCK", "None", -1},
                             {206, "/sys/devices/2200000.gpio", "24", "8",
                              "SPI1_CS0_N", "SPI1_CS0_N", "None", -1},
                             {207, "/sys/devices/2200000.gpio", "26", "7",
                              "SPI1_CS1_N", "SPI1_CS1_N", "None", -1},
                             {3, "/sys/devices/c2f0000.gpio", "29", "5",
                              "CAN0_DIN", "CAN0_DIN", "None", -1},
                             {2, "/sys/devices/c2f0000.gpio", "31", "6",
                              "CAN0_DOUT", "CAN0_DOUT", "None", -1},
                             {9, "/sys/devices/c2f0000.gpio", "32", "12",
                              "GPIO9", "CAN1_EN", "None", -1},
                             {0, "/sys/devices/c2f0000.gpio", "33", "13",
                              "CAN1_DOUT", "CAN1_DOUT", "None", -1},
                             {66, "/sys/devices/2200000.gpio", "35", "19",
                              "I2S2_FS", "DAP2_FS", "None", -1},
                             // Input-only (due to base board)
                             {141, "/sys/devices/2200000.gpio", "36", "16",
                              "UART1_CTS", "UART1_CTS", "None", -1},
                             {1, "/sys/devices/c2f0000.gpio", "37", "26",
                              "CAN1_DIN", "CAN1_DIN", "None", -1},
                             {65, "/sys/devices/2200000.gpio", "38", "20",
                              "I2S2_DIN", "DAP2_DIN", "None", -1},
                             {64, "/sys/devices/2200000.gpio", "40", "21",
                              "I2S2_DOUT", "DAP2_DOUT", "None", -1}},
      compats_xavier{"nvidia,p2972-0000", "nvidia,p2972-0006",
                     "nvidia,jetson-xavier"},
      JETSON_TX2_PIN_DEFS{
          {76, "/sys/devices/2200000.gpio", "7", "4", "AUDIO_MCLK", "AUD_MCLK",
           "None", -1},
          // Output-only (due to base board)
          {146, "/sys/devices/2200000.gpio", "11", "17", "UART0_RTS",
           "UART1_RTS", "None", -1},
          {72, "/sys/devices/2200000.gpio", "12", "18", "I2S0_CLK", "DAP1_SCLK",
           "None", -1},
          {77, "/sys/devices/2200000.gpio", "13", "27", "GPIO20_AUD_INT",
           "GPIO_AUD0", "None", -1},
          {15, "/sys/devices/3160000.i2c/i2c-0/0-0074", "15", "22",
           "GPIO_EXP_P17", "GPIO_EXP_P17", "None", -1},
          // Input-only (due to module):
          {40, "/sys/devices/c2f0000.gpio", "16", "23", "AO_DMIC_IN_DAT",
           "CAN_GPIO0", "None", -1},
          {161, "/sys/devices/2200000.gpio", "18", "24", "GPIO16_MDM_WAKE_AP",
           "GPIO_MDM2", "None", -1},
          {109, "/sys/devices/2200000.gpio", "19", "10", "SPI1_MOSI",
           "GPIO_CAM6", "None", -1},
          {108, "/sys/devices/2200000.gpio", "21", "9", "SPI1_MISO",
           "GPIO_CAM5", "None", -1},
          {14, "/sys/devices/3160000.i2c/i2c-0/0-0074", "22", "25",
           "GPIO_EXP_P16", "GPIO_EXP_P16", "None", -1},
          {107, "/sys/devices/2200000.gpio", "23", "11", "SPI1_CLK",
           "GPIO_CAM4", "None", -1},
          {110, "/sys/devices/2200000.gpio", "24", "8", "SPI1_CS0", "GPIO_CAM7",
           "None", -1},
          {-1, "None", "26", "7", "SPI1_CS1", "None", "None", -1},
          {78, "/sys/devices/2200000.gpio", "29", "5", "GPIO19_AUD_RST",
           "GPIO_AUD1", "None", -1},
          {42, "/sys/devices/c2f0000.gpio", "31", "6", "GPIO9_MOTION_INT",
           "CAN_GPIO2", "None", -1},
          // Output-only (due to module):
          {41, "/sys/devices/c2f0000.gpio", "32", "12", "AO_DMIC_IN_CLK",
           "CAN_GPIO1", "None", -1},
          {69, "/sys/devices/2200000.gpio", "33", "13", "GPIO11_AP_WAKE_BT",
           "GPIO_PQ5", "None", -1},
          {75, "/sys/devices/2200000.gpio", "35", "19", "I2S0_LRCLK", "DAP1_FS",
           "None", -1},
          // Input-only (due to base board) IF NVIDIA debug card NOT plugged in
          // Output-only (due to base board) IF NVIDIA debug card plugged in
          {147, "/sys/devices/2200000.gpio", "36", "16", "UART0_CTS",
           "UART1_CTS", "None", -1},
          {68, "/sys/devices/2200000.gpio", "37", "26", "GPIO8_ALS_PROX_INT",
           "GPIO_PQ4", "None", -1},
          {74, "/sys/devices/2200000.gpio", "38", "20", "I2S0_SDIN", "DAP1_DIN",
           "None", -1},
          {73, "/sys/devices/2200000.gpio", "40", "21", "I2S0_SDOUT",
           "DAP1_DOUT", "None", -1}},
      compats_tx2{"nvidia,p2771-0000", "nvidia,p2771-0888", "nvidia,p3489-0000",
                  "nvidia,lightning",  "nvidia,quill",      "nvidia,storm"},
      JETSON_TX1_PIN_DEFS{
          {216, "/sys/devices/6000d000.gpio", "7", "4", "AUDIO_MCLK",
           "AUD_MCLK", "None", -1},
          // Output-only (due to base board)
          {162, "/sys/devices/6000d000.gpio", "11", "17", "UART0_RTS",
           "UART1_RTS", "None", -1},
          {11, "/sys/devices/6000d000.gpio", "12", "18", "I2S0_CLK",
           "DAP1_SCLK", "None", -1},
          {38, "/sys/devices/6000d000.gpio", "13", "27", "GPIO20_AUD_INT",
           "GPIO_PE6", "None", -1},
          {15, "/sys/devices/7000c400.i2c/i2c-1/1-0074", "15", "22",
           "GPIO_EXP_P17", "GPIO_EXP_P17", "None", -1},
          {37, "/sys/devices/6000d000.gpio", "16", "23", "AO_DMIC_IN_DAT",
           "DMIC3_DAT", "None", -1},
          {184, "/sys/devices/6000d000.gpio", "18", "24", "GPIO16_MDM_WAKE_AP",
           "MODEM_WAKE_AP", "None", -1},
          {16, "/sys/devices/6000d000.gpio", "19", "10", "SPI1_MOSI",
           "SPI1_MOSI", "None", -1},
          {17, "/sys/devices/6000d000.gpio", "21", "9", "SPI1_MISO",
           "SPI1_MISO", "None", -1},
          {14, "/sys/devices/7000c400.i2c/i2c-1/1-0074", "22", "25",
           "GPIO_EXP_P16", "GPIO_EXP_P16", "None", -1},
          {18, "/sys/devices/6000d000.gpio", "23", "11", "SPI1_CLK", "SPI1_SCK",
           "None", -1},
          {19, "/sys/devices/6000d000.gpio", "24", "8", "SPI1_CS0", "SPI1_CS0",
           "None", -1},
          {20, "/sys/devices/6000d000.gpio", "26", "7", "SPI1_CS1", "SPI1_CS1",
           "None", -1},
          {219, "/sys/devices/6000d000.gpio", "29", "5", "GPIO19_AUD_RST",
           "GPIO_X1_AUD", "None", -1},
          {186, "/sys/devices/6000d000.gpio", "31", "6", "GPIO9_MOTION_INT",
           "MOTION_INT", "None", -1},
          {36, "/sys/devices/6000d000.gpio", "32", "12", "AO_DMIC_IN_CLK",
           "DMIC3_CLK", "None", -1},
          {63, "/sys/devices/6000d000.gpio", "33", "13", "GPIO11_AP_WAKE_BT",
           "AP_WAKE_NFC", "None", -1},
          {8, "/sys/devices/6000d000.gpio", "35", "19", "I2S0_LRCLK", "DAP1_FS",
           "None", -1},
          // Input-only (due to base board) IF NVIDIA debug card NOT plugged in
          // Input-only (due to base board) (always reads fixed value) IF NVIDIA
          // debug card plugged in
          {163, "/sys/devices/6000d000.gpio", "36", "16", "UART0_CTS",
           "UART1_CTS", "None", -1},
          {187, "/sys/devices/6000d000.gpio", "37", "26", "GPIO8_ALS_PROX_INT",
           "ALS_PROX_INT", "None", -1},
          {9, "/sys/devices/6000d000.gpio", "38", "20", "I2S0_SDIN", "DAP1_DIN",
           "None", -1},
          {10, "/sys/devices/6000d000.gpio", "40", "21", "I2S0_SDOUT",
           "DAP1_DOUT", "None", -1}},
      compats_tx1{"nvidia,p2371-2180", "nvidia,jetson-cv"},
      JETSON_NANO_PIN_DEFS{
          {216, "/sys/devices/6000d000.gpio", "7", "4", "GPIO9", "AUD_MCLK",
           "None", -1},
          {50, "/sys/devices/6000d000.gpio", "11", "17", "UART1_RTS",
           "UART2_RTS", "None", -1},
          {79, "/sys/devices/6000d000.gpio", "12", "18", "I2S0_SCLK",
           "DAP4_SCLK", "None", -1},
          {14, "/sys/devices/6000d000.gpio", "13", "27", "SPI1_SCK", "SPI2_SCK",
           "None", -1},
          {194, "/sys/devices/6000d000.gpio", "15", "22", "GPIO12", "LCD_TE",
           "None", -1},
          {232, "/sys/devices/6000d000.gpio", "16", "23", "SPI1_CS1",
           "SPI2_CS1", "None", -1},
          {15, "/sys/devices/6000d000.gpio", "18", "24", "SPI1_CS0", "SPI2_CS0",
           "None", -1},
          {16, "/sys/devices/6000d000.gpio", "19", "10", "SPI0_MOSI",
           "SPI1_MOSI", "None", -1},
          {17, "/sys/devices/6000d000.gpio", "21", "9", "SPI0_MISO",
           "SPI1_MISO", "None", -1},
          {13, "/sys/devices/6000d000.gpio", "22", "25", "SPI1_MISO",
           "SPI2_MISO", "None", -1},
          {18, "/sys/devices/6000d000.gpio", "23", "11", "SPI0_SCK", "SPI1_SCK",
           "None", -1},
          {19, "/sys/devices/6000d000.gpio", "24", "8", "SPI0_CS0", "SPI1_CS0",
           "None", -1},
          {20, "/sys/devices/6000d000.gpio", "26", "7", "SPI0_CS1", "SPI1_CS1",
           "None", -1},
          {149, "/sys/devices/6000d000.gpio", "29", "5", "GPIO01", "CAM_AF_EN",
           "None", -1},
          {200, "/sys/devices/6000d000.gpio", "31", "6", "GPIO11", "GPIO_PZ0",
           "None", -1},
          // Older versions of L4T have a DT bug which instantiates a bogus
          // device which prevents this library from using this PWM channel.
          {168, "/sys/devices/6000d000.gpio", "32", "12", "GPIO07", "LCD_BL_PW",
           "/sys/devices/7000a000.pwm", 0},
          {38, "/sys/devices/6000d000.gpio", "33", "13", "GPIO13", "GPIO_PE6",
           "/sys/devices/7000a000.pwm", 2},
          {76, "/sys/devices/6000d000.gpio", "35", "19", "I2S0_FS", "DAP4_FS",
           "None", -1},
          {51, "/sys/devices/6000d000.gpio", "36", "16", "UART1_CTS",
           "UART2_CTS", "None", -1},
          {12, "/sys/devices/6000d000.gpio", "37", "26", "SPI1_MOSI",
           "SPI2_MOSI", "None", -1},
          {77, "/sys/devices/6000d000.gpio", "38", "20", "I2S0_DIN", "DAP4_DIN",
           "None", -1},
          {78, "/sys/devices/6000d000.gpio", "40", "21", "I2S0_DOUT",
           "DAP4_DOUT", "None", -1}},
      compats_nano{"nvidia,p3450-0000", "nvidia,p3450-0002",
                   "nvidia,jetson-nano"},
      PIN_DEFS_MAP{
          {JETSON_XAVIER, JETSON_XAVIER_PIN_DEFS},
          {JETSON_TX2, JETSON_TX2_PIN_DEFS},
          {JETSON_TX1, JETSON_TX1_PIN_DEFS},
          {JETSON_NANO, JETSON_NANO_PIN_DEFS},
      },
      JETSON_INFO_MAP{
          {JETSON_XAVIER,
           {1, "16384M", "Unknown", "Jetson Xavier", "NVIDIA", "ARM Carmel"}},
          {JETSON_TX2,
           {1, "8192M", "Unknown", "Jetson TX2", "NVIDIA", "ARM A57 + Denver"}},
          {JETSON_TX1,
           {1, "4096M", "Unknown", "Jetson TX1", "NVIDIA", "ARM A57"}},
          {JETSON_NANO,
           {1, "4096M", "Unknown", "Jetson nano", "NVIDIA", "ARM A57"}}}

      {};

GPIO_data get_data() {
  try {
    PIN_DATA& _DATA = PIN_DATA::get_instance();

    const string compatible_path = "/proc/device-tree/compatible";
    const string ids_path = "/proc/device-tree/chosen/plugin-manager/ids";

    set<string> compatibles;

    {  // scope for f:
      ifstream f(compatible_path);
      stringstream buffer;

      buffer << f.rdbuf();
      string tmp_str = buffer.str();
      vector<string> _vec_compatibles(split(tmp_str, '\x00'));
      // convert to std::set
      copy(_vec_compatibles.begin(), _vec_compatibles.end(),
           inserter(compatibles, compatibles.end()));
    }  // scope ends

    auto matches = [&compatibles](const vector<string>& vals) {
      for (const auto& v : vals) {
        if (compatibles.find(v) != compatibles.end())
          return true;
      }
      return false;
    };

    auto find_pmgr_board = [&](const string& prefix) {
      if (!os_path_exists(ids_path)) {
        if (ids_warned == false) {
          ids_warned = true;
          string msg =
              "WARNING: Plugin manager information missing from device tree.\n"
              "WARNING: Cannot determine whether the expected Jetson board is "
              "present.";
          cerr << msg;
        }
        return "None";
      }

      vector<string> files = os_listdir(ids_path);
      for (const string& file : files) {
        if (startswith(file, prefix))
          return file.c_str();
      }

      return "None";
    };

    auto warn_if_not_carrier_board = [&](string carrier_board) {
      string found = find_pmgr_board(carrier_board + "-");
      if (found == "None") {
        string msg =
            "WARNING: Carrier board is not from a Jetson Developer Kit.\n"
            "WARNNIG: Jetson.GPIO library has not been verified with this "
            "carrier board,\n"
            "WARNING: and in fact is unlikely to work correctly.";
        cerr << msg << endl;
      }
    };

    Model model;

    if (matches(_DATA.compats_tx1)) {
      model = JETSON_TX1;
      warn_if_not_carrier_board("2597");
    } else if (matches(_DATA.compats_tx2)) {
      model = JETSON_TX2;
      warn_if_not_carrier_board("2597");
    } else if (matches(_DATA.compats_xavier)) {
      model = JETSON_XAVIER;
      warn_if_not_carrier_board("2822");
    } else if (matches(_DATA.compats_nano)) {
      model = JETSON_NANO;
      string module_id = find_pmgr_board("3448");

      if (module_id == "None")
        throw runtime_error("Could not determine Jetson Nano module revision");
      string revision = split(module_id, '-').back();
      // Revision is an ordered string, not a decimal integer
      if (revision < "200")
        throw runtime_error("Jetson Nano module revision must be A02 or later");

      warn_if_not_carrier_board("3449");
    } else {
      throw runtime_error("Could not determine Jetson model");
    }

    vector<_GPIO_PIN_DEF> pin_defs = _DATA.PIN_DEFS_MAP.at(model);
    _GPIO_PIN_INFO jetson_info = _DATA.JETSON_INFO_MAP.at(model);

    map<string, int> gpio_chip_base;
    map<string, string> pwm_dirs;

    // Get the gpiochip offsets
    set<string> gpio_chip_dirs;
    for (const auto& pin_def : pin_defs) {
      if (pin_def.SysfsDir != "None")
        gpio_chip_dirs.insert(pin_def.SysfsDir);
    }
    for (const auto& gpio_chip_dir : gpio_chip_dirs) {
      string gpio_chip_gpio_dir = gpio_chip_dir + "/gpio";
      auto files = os_listdir(gpio_chip_gpio_dir);
      for (const auto& fn : files) {
        if (!startswith(fn, "gpiochip"))
          continue;

        string gpiochip_fn = gpio_chip_gpio_dir + "/" + fn + "/base";
        {  // scope for f
          ifstream f(gpiochip_fn);
          stringstream buffer;
          buffer << f.rdbuf();
          gpio_chip_base[gpio_chip_dir] = stoi(strip(buffer.str()));
          break;
        }  // scope ends
      }
    }

    auto global_gpio_id = [&gpio_chip_base](string gpio_chip_dir,
                                            int chip_relative_id) {
      if (gpio_chip_dir == "None" || chip_relative_id == -1)
        return -1;
      return gpio_chip_base[gpio_chip_dir] + chip_relative_id;
    };

    auto pwm_dir = [&pwm_dirs](string chip_dir) {
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
      for (const auto& fn : files) {
        if (!startswith(fn, "pwmchip"))
          continue;

        string chip_pwm_pwmchip_dir = chip_pwm_dir + "/" + fn;
        pwm_dirs[chip_dir] = chip_pwm_pwmchip_dir;
        return chip_pwm_pwmchip_dir.c_str();
        return "None";
      }
      return "None";
    };

    auto model_data = [&global_gpio_id, &pwm_dir](
                          NumberingModes key,
                          const vector<_GPIO_PIN_DEF>& pin_defs) {
      map<string, ChannelInfo> ret;

      for (const auto& x : pin_defs) {
        string pinName;
        if (key == BOARD) {
          pinName = x.BoardPin;
        } else if (key == BCM) {
          pinName = x.BCMPin;
        } else if (key == CVM) {
          pinName = x.CVMPin;
        } else {  // TEGRA_SOC
          pinName = x.TEGRAPin;
        }

        ret.insert({pinName, ChannelInfo{pinName, x.SysfsDir, x.LinuxPin,
                                         global_gpio_id(x.SysfsDir, x.LinuxPin),
                                         pwm_dir(x.PWMSysfsDir), x.PWMID}});
      }
      return ret;
    };

    map<NumberingModes, map<string, ChannelInfo>> channel_data = {
        {BOARD, model_data(BOARD, pin_defs)},
        {BCM, model_data(BCM, pin_defs)},
        {CVM, model_data(CVM, pin_defs)},
        {TEGRA_SOC, model_data(TEGRA_SOC, pin_defs)}};

    return {model, jetson_info, channel_data};
  } catch (exception& e) {
    cerr << "[Exception] " << e.what() << " (catched from: get_data())" << endl;
    throw false;
  }
}