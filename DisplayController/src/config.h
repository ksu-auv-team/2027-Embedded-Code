#pragma once

#include <stdint.h>

namespace app {

constexpr uint8_t kBatteryCount = 3;
constexpr uint8_t kThrusterCount = 8;
constexpr uint8_t kMaxDeviceCount = 8;

struct DisplayPins {
  uint32_t cs;
  uint32_t dc;
  uint32_t rst;
  uint32_t busy;
  uint32_t sck;
  uint32_t mosi;
};

struct I2cPins {
  uint32_t scl;
  uint32_t sda;
};

struct DeviceConfig {
  const char* name;
  uint8_t address;
  bool present;
};

enum class DisplayColor : uint8_t {
  kBlack,
  kRed,
  kYellow,
  kWhite,
};

struct AppConfig {
  DisplayPins displayPins;
  I2cPins i2cPins;
  uint32_t displayUpdateLedPin;
  // Wait this long after boot before initializing the display, allowing USB to enumerate.
  uint32_t displayStartupDelayMs;
  // Force a full display refresh after this many milliseconds.
  uint32_t fullRefreshIntervalMs;
  uint32_t refreshRateMs;
  uint8_t i2cSlaveAddress;
  float batteryVoltageLowThreshold;
  float batteryVoltageHighThreshold;
  DisplayColor connectedDeviceColor;
  DisplayColor disconnectedDeviceColor;
  // Number of valid entries in devices; must not exceed kMaxDeviceCount.
  uint8_t knownDeviceCount;
  // Device names, I2C addresses, and initial presence states used by the display.
  const DeviceConfig* devices;
  // Enable device-presence updates from the I2C telemetry frame.
  bool devicePresenceEnabled;
  bool testModeOnBoot;
};

AppConfig MakeDefaultConfig();

}  // namespace app
