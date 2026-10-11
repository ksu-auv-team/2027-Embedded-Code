#include "i2c_slave.h"

#include <Arduino.h>
#include <Wire.h>

namespace app {

namespace {

constexpr uint8_t kFrameMagic = 0xA5U;
constexpr uint8_t kProtocolVersion = 2U;
constexpr uint8_t kCrcOffset = 30U;

uint16_t ReadUint16(const uint8_t* bytes) {
  return static_cast<uint16_t>(bytes[0]) |
         (static_cast<uint16_t>(bytes[1]) << 8U);
}

uint8_t CalculateCrc8(const uint8_t* bytes, uint8_t length) {
  uint8_t crc = 0U;
  for (uint8_t i = 0; i < length; ++i) {
    crc ^= bytes[i];
    for (uint8_t bit = 0; bit < 8U; ++bit) {
      crc = (crc & 0x80U) != 0U
                ? static_cast<uint8_t>((crc << 1U) ^ 0x07U)
                : static_cast<uint8_t>(crc << 1U);
    }
  }
  return crc;
}

}  // namespace

I2cSlave* I2cSlave::activeInstance_ = nullptr;

I2cSlave::I2cSlave(const AppConfig& config)
    : config_(config),
      liveState_{},
      testModeWasEnabled_(false),
      packetReady_(false),
      packetLength_(0U),
      receiveBuffer_{} {
  InitializeTelemetryState(liveState_, config_);
}

void I2cSlave::begin() {
  activeInstance_ = this;
  Wire.setSCL(config_.i2cPins.scl);
  Wire.setSDA(config_.i2cPins.sda);
  Wire.begin(static_cast<int>(config_.i2cSlaveAddress));
  Wire.onReceive(receiveCallback);
}

void I2cSlave::receiveCallback(int byteCount) {
  if (activeInstance_ != nullptr) {
    activeInstance_->receive(byteCount);
  }
}

void I2cSlave::receive(int byteCount) {
  uint8_t length = 0U;
  bool overflow = byteCount < 0 ||
                  byteCount > static_cast<int>(kPacketLength);

  while (Wire.available() > 0) {
    const int value = Wire.read();
    if (length < kPacketLength) {
      receiveBuffer_[length] = static_cast<uint8_t>(value);
    } else {
      overflow = true;
    }
    if (length < UINT8_MAX) {
      ++length;
    }
  }

  packetLength_ = overflow ? 0U : length;
  packetReady_ = true;
}

bool I2cSlave::decodePacket(const uint8_t* packet,
                            TelemetryState& state) const {
  if (packet[0] != kFrameMagic || packet[1] != kProtocolVersion ||
      CalculateCrc8(packet, kCrcOffset) != packet[kCrcOffset]) {
    return false;
  }

  for (uint8_t i = 0; i < kBatteryCount; ++i) {
    const uint16_t millivolts = ReadUint16(&packet[2U + i * 2U]);
    state.batteryVoltages[i] = millivolts / 1000.0f;
  }

  state.killswitchOn = (packet[8] & 0x01U) != 0U;
  state.pitch = static_cast<int8_t>(packet[9]);
  state.roll = static_cast<int8_t>(packet[10]);
  state.yaw = static_cast<int8_t>(packet[11]);
  state.surge = static_cast<int8_t>(packet[12]);
  state.sway = static_cast<int8_t>(packet[13]);
  state.heave = static_cast<int8_t>(packet[14]);
  state.compassHeading = ReadUint16(&packet[15]) % 360U;
  for (uint8_t i = 0; i < kThrusterCount; ++i) {
    state.thrusterValues[i] = packet[17U + i];
  }

  const int16_t temperatureCentiC =
      static_cast<int16_t>(ReadUint16(&packet[25]));
  const uint16_t humidityCentiPct = ReadUint16(&packet[27]);
  state.temperatureC = temperatureCentiC / 100.0f;
  state.humidityPct = humidityCentiPct / 100.0f;

  const uint8_t presentMask = packet[29];
  for (uint8_t i = 0; i < kMaxDeviceCount; ++i) {
    state.devicesPresent[i] =
        config_.devicePresenceEnabled && ((presentMask & (1U << i)) != 0U);
  }
  return true;
}

void I2cSlave::update(TelemetryState& state, bool testMode) {
  uint8_t localPacket[kPacketLength];
  uint8_t localLength = 0U;

  noInterrupts();
  if (packetReady_) {
    localLength = packetLength_;
    if (localLength == kPacketLength) {
      for (uint8_t i = 0; i < kPacketLength; ++i) {
        localPacket[i] = receiveBuffer_[i];
      }
    }
    packetReady_ = false;
  }
  interrupts();

  bool newLivePacket = false;
  if (localLength == kPacketLength &&
      decodePacket(localPacket, liveState_)) {
    newLivePacket = true;
    state.liveDataReceived = true;
  }

  if (!testMode && (newLivePacket || testModeWasEnabled_)) {
    state.batteryVoltages[0] = liveState_.batteryVoltages[0];
    state.batteryVoltages[1] = liveState_.batteryVoltages[1];
    state.batteryVoltages[2] = liveState_.batteryVoltages[2];
    state.killswitchOn = liveState_.killswitchOn;
    state.pitch = liveState_.pitch;
    state.roll = liveState_.roll;
    state.yaw = liveState_.yaw;
    state.surge = liveState_.surge;
    state.sway = liveState_.sway;
    state.heave = liveState_.heave;
    state.compassHeading = liveState_.compassHeading;
    for (uint8_t i = 0; i < kThrusterCount; ++i) {
      state.thrusterValues[i] = liveState_.thrusterValues[i];
    }
    state.temperatureC = liveState_.temperatureC;
    state.humidityPct = liveState_.humidityPct;
    for (uint8_t i = 0; i < kMaxDeviceCount; ++i) {
      state.devicesPresent[i] = liveState_.devicesPresent[i];
    }
    state.dirty = true;
  }

  testModeWasEnabled_ = testMode;
}

}  // namespace app
