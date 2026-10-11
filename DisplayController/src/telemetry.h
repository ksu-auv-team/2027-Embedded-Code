#pragma once

#include "config.h"

#include <stdint.h>

namespace app {

enum class BatteryColor {
  kNormal,
  kRed,
};

struct TelemetryState {
  float batteryVoltages[kBatteryCount];
  bool killswitchOn;
  int8_t pitch;
  int8_t roll;
  int8_t yaw;
  int8_t surge;
  int8_t sway;
  int8_t heave;
  uint16_t compassHeading;
  uint8_t thrusterValues[kThrusterCount];
  float temperatureC;
  float humidityPct;
  bool testModeEnabled;
  bool liveDataReceived;
  bool devicesPresent[kMaxDeviceCount];
  bool dirty;
};

BatteryColor EvaluateBatteryColor(float voltage,
                                 const AppConfig& config);
void InitializeTelemetryState(TelemetryState& state,
                             const AppConfig& config);
void ApplyTestTelemetry(TelemetryState& state,
                       const AppConfig& config,
                       uint32_t nowMs);

}  // namespace app
