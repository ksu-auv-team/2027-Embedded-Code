#pragma once

#include "config.h"
#include "telemetry.h"

#include <stdint.h>

namespace app {

class I2cSlave {
 public:
  explicit I2cSlave(const AppConfig& config);

  void begin();
  void update(TelemetryState& state, bool testMode);

 private:
  // Frame v2: 0=A5, 1=version, 2..7=battery mV (LE), 8=flags
  // (bit 0 killswitch), 9..14=pitch/roll/yaw/surge/sway/heave (signed int8;
  // angles in degrees, acceleration in m/s^2), 15..16=heading degrees (LE),
  // 17..24=thrusters, 25..26=temperature centi-degrees C (signed LE),
  // 27..28=humidity centi-percent (LE), 29=device presence bits,
  // 30=CRC-8/ATM over bytes 0..29. Device bit N maps to config entry N.
  static constexpr uint8_t kPacketLength = 31;

  static I2cSlave* activeInstance_;
  static void receiveCallback(int byteCount);
  void receive(int byteCount);
  bool decodePacket(const uint8_t* packet, TelemetryState& state) const;

  const AppConfig& config_;
  TelemetryState liveState_;
  bool testModeWasEnabled_;
  volatile bool packetReady_;
  volatile uint8_t packetLength_;
  uint8_t receiveBuffer_[kPacketLength];
};

}  // namespace app
