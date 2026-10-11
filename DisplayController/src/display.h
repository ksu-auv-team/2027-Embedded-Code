#pragma once

#include "config.h"
#include "telemetry.h"

#include <stdint.h>

#include <GxEPD2_4C.h>

namespace app {

using EpaperPanel = GxEPD2_4C<GxEPD2_213c_GDEY0213F51, 16>;

class EpaperDisplay {
 public:
  explicit EpaperDisplay(const AppConfig& config);

  void begin();
  bool render(const TelemetryState& state,
              bool testMode,
              uint32_t nowMs);

 private:
  const AppConfig& config_;
  EpaperPanel panel_;
  TelemetryState displayedState_;
  BatteryColor displayedBatteryColors_[kBatteryCount];
  bool displayedTestMode_;
  bool initialized_;
  bool hasRendered_;
  uint8_t nextRegion_;
  uint32_t lastRefreshMs_;
  uint32_t lastFullRefreshMs_;

  void drawPage(const TelemetryState& state, bool testMode);
};

}  // namespace app
