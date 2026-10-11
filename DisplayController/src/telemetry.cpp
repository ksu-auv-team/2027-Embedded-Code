#include "telemetry.h"

#include <Arduino.h>

namespace app {

BatteryColor EvaluateBatteryColor(float voltage,
                                 const AppConfig& config) {
  const bool belowLow = voltage < config.batteryVoltageLowThreshold;
  const bool aboveHigh = voltage > config.batteryVoltageHighThreshold;

  if (!belowLow && !aboveHigh) {
    return BatteryColor::kNormal;
  }

  return BatteryColor::kRed;
}

void InitializeTelemetryState(TelemetryState& state,
                             const AppConfig& config) {
  for (uint8_t i = 0; i < kBatteryCount; ++i) {
    state.batteryVoltages[i] = 0.0f;
  }
  state.killswitchOn = false;
  state.pitch = 0;
  state.roll = 0;
  state.yaw = 0;
  state.surge = 0;
  state.sway = 0;
  state.heave = 0;
  state.compassHeading = 0U;
  for (uint8_t i = 0; i < kThrusterCount; ++i) {
    state.thrusterValues[i] = 0U;
  }
  state.temperatureC = 0.0f;
  state.humidityPct = 0.0f;
  state.testModeEnabled = false;
  state.liveDataReceived = false;
  for (uint8_t i = 0; i < kMaxDeviceCount; ++i) {
    state.devicesPresent[i] = (config.devices != nullptr && i < config.knownDeviceCount)
                                 ? config.devices[i].present
                                 : false;
  }
  state.dirty = true;
}

void ApplyTestTelemetry(TelemetryState& state,
                       const AppConfig& config,
                       uint32_t nowMs) {
  (void)config;
  state.batteryVoltages[0] = 14.2f + 3.0f * sinf((nowMs / 200.0f) * 0.1f);
  state.batteryVoltages[1] = 14.2f + 3.0f * cosf((nowMs / 160.0f) * 0.12f);
  state.batteryVoltages[2] = 14.2f + 3.0f * sinf((nowMs / 240.0f) * 0.15f);
  state.killswitchOn = ((nowMs / 750U) % 2U) == 0U;
  state.pitch = static_cast<int8_t>(
      static_cast<int16_t>((nowMs / 80U) % 61U) - 30);
  state.roll = static_cast<int8_t>(
      static_cast<int16_t>((nowMs / 95U) % 61U) - 30);
  state.yaw = static_cast<int8_t>(
      static_cast<int16_t>((nowMs / 110U) % 121U) - 60);
  state.surge = static_cast<int8_t>(
      static_cast<int16_t>((nowMs / 120U) % 21U) - 10);
  state.sway = static_cast<int8_t>(
      static_cast<int16_t>((nowMs / 140U) % 21U) - 10);
  state.heave = static_cast<int8_t>(
      static_cast<int16_t>((nowMs / 160U) % 21U) - 10);
  state.compassHeading = static_cast<uint16_t>((nowMs / 100U) % 360U);

  for (uint8_t i = 0; i < kThrusterCount; ++i) {
    const uint16_t phase = static_cast<uint16_t>(nowMs / 120U + i * 17U);
    state.thrusterValues[i] = static_cast<uint8_t>((phase % 256U));
  }

  state.temperatureC = 24.0f + 0.5f * sinf(nowMs / 1000.0f);
  state.humidityPct = 47.0f + 10.0f * cosf(nowMs / 1300.0f);
  state.testModeEnabled = true;

  for (uint8_t i = 0; i < kMaxDeviceCount; ++i) {
    state.devicesPresent[i] = ((nowMs / 1200U + i) % 5U) != 0U;
  }

  state.dirty = true;
}

}  // namespace app
