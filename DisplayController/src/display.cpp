#include "display.h"

#include <Arduino.h>
#include <SPI.h>
#include <string.h>

namespace app {
namespace {

uint16_t ToPanelColor(DisplayColor color) {
  switch (color) {
    case DisplayColor::kBlack:
      return GxEPD_BLACK;
    case DisplayColor::kRed:
      return GxEPD_RED;
    case DisplayColor::kYellow:
      return GxEPD_YELLOW;
    case DisplayColor::kWhite:
      return GxEPD_WHITE;
  }
  return GxEPD_BLACK;
}

}  // namespace

EpaperDisplay::EpaperDisplay(const AppConfig& config)
    : config_(config),
      panel_(GxEPD2_213c_GDEY0213F51(config.displayPins.cs,
                                    config.displayPins.dc,
                                    config.displayPins.rst,
                                    config.displayPins.busy)),
      displayedState_{},
      displayedBatteryColors_{},
      displayedTestMode_(false),
      initialized_(false),
      hasRendered_(false),
      nextRegion_(0U),
      lastRefreshMs_(0U),
      lastFullRefreshMs_(0U) {}

void EpaperDisplay::begin() {
  pinMode(config_.displayUpdateLedPin, OUTPUT);
  digitalWrite(config_.displayUpdateLedPin, LOW);
  SPI.setMISO(PA6);
  SPI.setMOSI(config_.displayPins.mosi);
  SPI.setSCLK(config_.displayPins.sck);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  SPI.endTransaction();
  Serial.println("EPD init started");
  digitalWrite(config_.displayUpdateLedPin, HIGH);
  panel_.init(0);
  panel_.setRotation(1);
  panel_.setFullWindow();
  initialized_ = true;
  digitalWrite(config_.displayUpdateLedPin, LOW);
  Serial.println("EPD init complete");
}

void EpaperDisplay::drawPage(const TelemetryState& state,
                            bool testMode) {
  panel_.fillScreen(GxEPD_WHITE);
  panel_.setTextWrap(false);
  panel_.setTextSize(1);
  panel_.setTextColor(GxEPD_BLACK);

  panel_.setCursor(4, 1);
  panel_.print("TELEMETRY");
  panel_.setCursor(78, 1);
  panel_.print(testMode ? "TEST" :
               (state.liveDataReceived ? "LIVE" : "WAIT I2C"));
  panel_.drawFastHLine(3, 10, 244, GxEPD_BLACK);
  panel_.drawFastVLine(80, 13, 108, GxEPD_BLACK);
  panel_.drawFastVLine(121, 13, 108, GxEPD_BLACK);
  panel_.drawFastVLine(184, 13, 108, GxEPD_BLACK);
  for (uint8_t i = 0; i < kBatteryCount; ++i) {
    const int16_t y = 14 + static_cast<int16_t>(i) * 13;
    const BatteryColor status =
        EvaluateBatteryColor(state.batteryVoltages[i], config_);
    uint16_t color = GxEPD_BLACK;
    if (status == BatteryColor::kRed) {
      color = GxEPD_RED;
    }
    panel_.setTextColor(GxEPD_BLACK);
    panel_.setCursor(2, y);
    panel_.printf("BAT%u", static_cast<unsigned>(i + 1U));
    panel_.setTextColor(color);
    panel_.setCursor(28, y);
    const uint32_t centivolts = static_cast<uint32_t>(
        state.batteryVoltages[i] * 100.0f + 0.5f);
    panel_.print(centivolts / 100U);
    panel_.print('.');
    if ((centivolts % 100U) < 10U) {
      panel_.print('0');
    }
    panel_.print(centivolts % 100U);
    panel_.print('V');
  }

  constexpr int16_t kKillSwitchX = 4;
  constexpr int16_t kKillSwitchY = 79;
  constexpr int16_t kKillSwitchSize = 40;
  const uint16_t killSwitchFill =
      state.killswitchOn ? GxEPD_RED : GxEPD_WHITE;
  for (int16_t row = 0; row < kKillSwitchSize; ++row) {
    const int16_t inset = row < 10 ? 10 - row
                          : row >= 30 ? row - 29
                                      : 0;
    panel_.drawFastHLine(kKillSwitchX + inset,
                         kKillSwitchY + row,
                         kKillSwitchSize - 2 * inset,
                         killSwitchFill);
  }
  panel_.drawLine(kKillSwitchX + 10, kKillSwitchY,
                  kKillSwitchX + 29, kKillSwitchY, GxEPD_BLACK);
  panel_.drawLine(kKillSwitchX + 29, kKillSwitchY,
                  kKillSwitchX + 39, kKillSwitchY + 10, GxEPD_BLACK);
  panel_.drawLine(kKillSwitchX + 39, kKillSwitchY + 10,
                  kKillSwitchX + 39, kKillSwitchY + 29, GxEPD_BLACK);
  panel_.drawLine(kKillSwitchX + 39, kKillSwitchY + 29,
                  kKillSwitchX + 29, kKillSwitchY + 39, GxEPD_BLACK);
  panel_.drawLine(kKillSwitchX + 29, kKillSwitchY + 39,
                  kKillSwitchX + 10, kKillSwitchY + 39, GxEPD_BLACK);
  panel_.drawLine(kKillSwitchX + 10, kKillSwitchY + 39,
                  kKillSwitchX, kKillSwitchY + 29, GxEPD_BLACK);
  panel_.drawLine(kKillSwitchX, kKillSwitchY + 29,
                  kKillSwitchX, kKillSwitchY + 10, GxEPD_BLACK);
  panel_.drawLine(kKillSwitchX, kKillSwitchY + 10,
                  kKillSwitchX + 10, kKillSwitchY, GxEPD_BLACK);
  panel_.setTextColor(GxEPD_BLACK);
  panel_.setCursor(kKillSwitchX + 8, kKillSwitchY + 16);
  panel_.print("KILL");

  panel_.setTextColor(GxEPD_BLACK);
  panel_.setCursor(2, 65);
  const int32_t temperatureTenths = static_cast<int32_t>(
      state.temperatureC * 10.0f +
      (state.temperatureC >= 0.0f ? 0.5f : -0.5f));
  panel_.print("T");
  panel_.print(temperatureTenths / 10);
  panel_.print('.');
  panel_.print(abs(temperatureTenths % 10));
  panel_.print("C H");
  panel_.print(static_cast<int32_t>(state.humidityPct + 0.5f));
  panel_.print('%');

  for (uint8_t i = 0; i < kThrusterCount; ++i) {
    panel_.setCursor(83, 14 + static_cast<int16_t>(i) * 12);
    panel_.printf("%u:%03u",
                  static_cast<unsigned>(i + 1U),
                  static_cast<unsigned>(state.thrusterValues[i]));
  }

  const int16_t imuY = 13;
  panel_.setTextColor(GxEPD_BLACK);
  const auto drawImuValue = [this](const char* label,
                                   int32_t value,
                                   int16_t y) {
    char valueText[12];
    snprintf(valueText, sizeof(valueText), "%ld", static_cast<long>(value));
    panel_.setCursor(124, y);
    panel_.print(label);
    const int16_t valueX =
        static_cast<int16_t>(180 - strlen(valueText) * 6U);
    panel_.setCursor(valueX, y);
    panel_.print(valueText);
  };
  drawImuValue("PIT ", state.pitch, imuY);
  drawImuValue("ROL ", state.roll, imuY + 12);
  drawImuValue("YAW ", state.yaw, imuY + 24);
  drawImuValue("SUR ", state.surge, imuY + 36);
  drawImuValue("SWY ", state.sway, imuY + 48);
  drawImuValue("HEV ", state.heave, imuY + 60);
  drawImuValue("HDG ", state.compassHeading % 360U, imuY + 72);

  for (uint8_t i = 0; i < config_.knownDeviceCount; ++i) {
    const DeviceConfig& device = config_.devices[i];
    const bool present = state.devicesPresent[i];
    const int16_t y = 14 + static_cast<int16_t>(i) * 12;
    panel_.setTextColor(GxEPD_BLACK);
    const int16_t nameX =
        234 - static_cast<int16_t>(strlen(device.name) * 6U);
    panel_.setCursor(nameX, y);
    panel_.print(device.name);
    const DisplayColor indicatorColor =
        present ? config_.connectedDeviceColor
                : config_.disconnectedDeviceColor;
    panel_.fillCircle(242, y + 3, 3, ToPanelColor(indicatorColor));
  }
}

bool EpaperDisplay::render(const TelemetryState& state,
                           bool testMode,
                           uint32_t nowMs) {
  if (!initialized_) {
    begin();
  }
  if (hasRendered_ && (nowMs - lastRefreshMs_) < config_.refreshRateMs) {
    return false;
  }
  const bool fullRefresh =
      !hasRendered_ ||
      (nowMs - lastFullRefreshMs_) >= config_.fullRefreshIntervalMs;

  constexpr uint8_t kHeaderRegion = 0U;
  constexpr uint8_t kBatteryRegionStart = 1U;
  constexpr uint8_t kKillswitchRegion = kBatteryRegionStart + kBatteryCount;
  constexpr uint8_t kEnvironmentRegion = kKillswitchRegion + 1U;
  constexpr uint8_t kThrusterRegionStart = kEnvironmentRegion + 1U;
  constexpr uint8_t kThrusterRegionCount = kThrusterCount;
  constexpr uint8_t kImuRegionStart =
      kThrusterRegionStart + kThrusterRegionCount;
  constexpr uint8_t kImuRegionCount = 7U;
  constexpr uint8_t kDeviceRegionStart = kImuRegionStart + kImuRegionCount;
  constexpr uint8_t kRegionCount = kDeviceRegionStart + kMaxDeviceCount;

  uint8_t regionToUpdate = kRegionCount;
  uint16_t regionX = 0U;
  uint16_t regionY = 0U;
  uint16_t regionWidth = 0U;
  uint16_t regionHeight = 0U;

  if (hasRendered_ && !fullRefresh) {
    for (uint8_t offset = 0U; offset < kRegionCount; ++offset) {
      const uint8_t region =
          static_cast<uint8_t>((nextRegion_ + offset) % kRegionCount);
      bool changed = false;
      uint16_t x = 0U;
      uint16_t y = 0U;
      uint16_t width = 0U;
      uint16_t height = 0U;

      if (region == kHeaderRegion) {
        changed = testMode != displayedTestMode_ ||
                  state.liveDataReceived != displayedState_.liveDataReceived;
        x = 78U;
        y = 1U;
        width = 48U;
        height = 7U;
      } else if (region < kKillswitchRegion) {
        const uint8_t battery =
            static_cast<uint8_t>(region - kBatteryRegionStart);
        changed =
            state.batteryVoltages[battery] !=
                displayedState_.batteryVoltages[battery] ||
            EvaluateBatteryColor(state.batteryVoltages[battery],
                                 config_) != displayedBatteryColors_[battery];
        x = 28U;
        y = static_cast<uint16_t>(14U + battery * 13U);
        width = 36U;
        height = 7U;
      } else if (region == kKillswitchRegion) {
        changed = state.killswitchOn != displayedState_.killswitchOn;
        x = 4U;
        y = 79U;
        width = 40U;
        height = 40U;
      } else if (region == kEnvironmentRegion) {
        changed = state.temperatureC != displayedState_.temperatureC ||
                  state.humidityPct != displayedState_.humidityPct;
        x = 2U;
        y = 65U;
        width = 78U;
        height = 7U;
      } else if (region < kImuRegionStart) {
        const uint8_t thruster =
            static_cast<uint8_t>(region - kThrusterRegionStart);
        changed = state.thrusterValues[thruster] !=
                  displayedState_.thrusterValues[thruster];
        x = 83U;
        y = static_cast<uint16_t>(14U + thruster * 12U);
        width = 30U;
        height = 7U;
      } else if (region < kDeviceRegionStart) {
        const uint8_t imuField =
            static_cast<uint8_t>(region - kImuRegionStart);
        switch (imuField) {
          case 0U:
            changed = state.pitch != displayedState_.pitch;
            break;
          case 1U:
            changed = state.roll != displayedState_.roll;
            break;
          case 2U:
            changed = state.yaw != displayedState_.yaw;
            break;
          case 3U:
            changed = state.surge != displayedState_.surge;
            break;
          case 4U:
            changed = state.sway != displayedState_.sway;
            break;
          case 5U:
            changed = state.heave != displayedState_.heave;
            break;
          default:
            changed =
                state.compassHeading != displayedState_.compassHeading;
            break;
        }
        x = 124U;
        y = static_cast<uint16_t>(13U + imuField * 12U);
        width = 56U;
        height = 7U;
      } else {
        const uint8_t device =
            static_cast<uint8_t>(region - kDeviceRegionStart);
        changed = device < config_.knownDeviceCount &&
                  state.devicesPresent[device] !=
                      displayedState_.devicesPresent[device];
        x = 239U;
        y = static_cast<uint16_t>(14U + device * 12U);
        width = 7U;
        height = 7U;
      }

      if (changed) {
        regionToUpdate = region;
        regionX = x;
        regionY = y;
        regionWidth = width;
        regionHeight = height;
        break;
      }
    }

    if (regionToUpdate == kRegionCount) {
      return false;
    }
  }

  digitalWrite(config_.displayUpdateLedPin, HIGH);
  Serial.println(fullRefresh ? "EPD full refresh started"
                             : "EPD partial refresh started");
  if (!fullRefresh) {
    panel_.setPartialWindow(regionX, regionY, regionWidth, regionHeight);
  } else {
    panel_.setFullWindow();
  }
  panel_.firstPage();
  bool morePages = false;
  do {
    drawPage(state, testMode);
    morePages = panel_.nextPage();
  } while (morePages);
  lastRefreshMs_ = millis();
  if (fullRefresh) {
    displayedState_ = state;
    displayedTestMode_ = testMode;
    for (uint8_t i = 0; i < kBatteryCount; ++i) {
      displayedBatteryColors_[i] =
          EvaluateBatteryColor(state.batteryVoltages[i], config_);
    }
    hasRendered_ = true;
    lastFullRefreshMs_ = lastRefreshMs_;
    nextRegion_ = 0U;
  } else {
    nextRegion_ = static_cast<uint8_t>((regionToUpdate + 1U) % kRegionCount);
    if (regionToUpdate == kHeaderRegion) {
      displayedTestMode_ = testMode;
      displayedState_.liveDataReceived = state.liveDataReceived;
    } else if (regionToUpdate < kKillswitchRegion) {
      const uint8_t battery =
          static_cast<uint8_t>(regionToUpdate - kBatteryRegionStart);
      displayedState_.batteryVoltages[battery] =
          state.batteryVoltages[battery];
      displayedBatteryColors_[battery] =
          EvaluateBatteryColor(state.batteryVoltages[battery], config_);
    } else if (regionToUpdate == kKillswitchRegion) {
      displayedState_.killswitchOn = state.killswitchOn;
    } else if (regionToUpdate == kEnvironmentRegion) {
      displayedState_.temperatureC = state.temperatureC;
      displayedState_.humidityPct = state.humidityPct;
    } else if (regionToUpdate < kImuRegionStart) {
      const uint8_t thruster =
          static_cast<uint8_t>(regionToUpdate - kThrusterRegionStart);
      displayedState_.thrusterValues[thruster] =
          state.thrusterValues[thruster];
    } else if (regionToUpdate < kDeviceRegionStart) {
      switch (regionToUpdate - kImuRegionStart) {
        case 0U:
          displayedState_.pitch = state.pitch;
          break;
        case 1U:
          displayedState_.roll = state.roll;
          break;
        case 2U:
          displayedState_.yaw = state.yaw;
          break;
        case 3U:
          displayedState_.surge = state.surge;
          break;
        case 4U:
          displayedState_.sway = state.sway;
          break;
        case 5U:
          displayedState_.heave = state.heave;
          break;
        default:
          displayedState_.compassHeading = state.compassHeading;
          break;
      }
    } else {
      const uint8_t device =
          static_cast<uint8_t>(regionToUpdate - kDeviceRegionStart);
      displayedState_.devicesPresent[device] =
          state.devicesPresent[device];
    }
  }
  digitalWrite(config_.displayUpdateLedPin, LOW);
  Serial.println(hasRendered_ && regionToUpdate != kRegionCount
                     ? "EPD partial refresh complete"
                     : "EPD full refresh complete");
  return true;
}

}  // namespace app
