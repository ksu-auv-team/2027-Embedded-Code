#include "config.h"

#include <Arduino.h>

namespace app {
namespace {

const DeviceConfig kDefaultDevices[kMaxDeviceCount] = {
    {"PS", 0x68, false}, // Name, I2C address, initial presence state
    {"IMU", 0x42, false},
    {"ESC", 0x50, false},
    {"DEPTH", 0x51, false},
    {"HYDRO", 0x40, false},
    {"TORP", 0x41, false},
    {"ARM", 0x60, false},
    {"LED", 0x61, false},
};

}  // namespace

AppConfig MakeDefaultConfig() {
  AppConfig config{};
  // E-paper connector: CS, DC, RST, BUSY, CLK, DIN (MOSI).
  config.displayPins = {PA4, PB0, PB1, PB2, PA5, PA7};
  // I2C slave bus: SCL, SDA.
  config.i2cPins = {PB6, PB7};
  config.displayUpdateLedPin = PC6;
  config.displayStartupDelayMs = 3000U; // Milliseconds. Allows USB Serial to init
  config.fullRefreshIntervalMs = 300000U; // Milliseconds. Force a full refresh every 5 minutes
  config.refreshRateMs = 2000U; // Milliseconds. Partial refresh every 2 seconds after one finishes
  config.i2cSlaveAddress = 0x2AU;  
  config.batteryVoltageLowThreshold = 13.0f; // Volts. The battery voltage is considered low when below this value.
  config.batteryVoltageHighThreshold = 17.0f; // Volts. The battery voltage is considered high when above this value.
  config.connectedDeviceColor = DisplayColor::kBlack; // kBlack, kRed, kYellow, kWhite
  config.disconnectedDeviceColor = DisplayColor::kRed; // kBlack, kRed, kYellow, kWhite
  config.knownDeviceCount = kMaxDeviceCount;
  config.devices = kDefaultDevices;
  config.devicePresenceEnabled = true;
  config.testModeOnBoot = true; // Simulate data on boot. Toggles with the PC13 button.
  return config;
}

}  // namespace app
