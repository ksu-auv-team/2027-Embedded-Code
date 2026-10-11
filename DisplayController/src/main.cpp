#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "i2c_slave.h"
#include "telemetry.h"

namespace {

const app::AppConfig kConfig = app::MakeDefaultConfig();
app::TelemetryState telemetry{};
app::I2cSlave i2cSlave(kConfig);
app::EpaperDisplay display(kConfig);

bool testModeEnabled = kConfig.testModeOnBoot;
bool lastButtonState = false;
bool displayStarted = false;
uint32_t lastToggleMs = 0U;
uint32_t bootMs = 0U;
uint32_t lastRenderMs = 0U;

}  // namespace

void setup() {
  Serial.begin(115200);
  bootMs = millis();
  // The WeAct board's PC13 key is pulled down and reads HIGH while pressed.
  pinMode(PC13, INPUT_PULLDOWN);

  app::InitializeTelemetryState(telemetry, kConfig);
  telemetry.testModeEnabled = testModeEnabled;
  i2cSlave.begin();
}

void loop() {
  const uint32_t nowMs = millis();

  if (!displayStarted &&
      (nowMs - bootMs) >= kConfig.displayStartupDelayMs) {
    Serial.println("Starting e-paper after USB enumeration delay");
    display.begin();
    displayStarted = true;
    Serial.println("E-paper startup complete");
  }

  const bool buttonPressed = (digitalRead(PC13) == HIGH);
  if (buttonPressed != lastButtonState) {
    if ((nowMs - lastToggleMs) > 50U) {
      lastToggleMs = nowMs;
      lastButtonState = buttonPressed;
      if (buttonPressed) {
        testModeEnabled = !testModeEnabled;
        telemetry.testModeEnabled = testModeEnabled;
        Serial.printf("Test mode %s\n", testModeEnabled ? "enabled" : "disabled");
      }
    }
  }

  if (testModeEnabled) {
    i2cSlave.update(telemetry, true);
    app::ApplyTestTelemetry(telemetry, kConfig, nowMs);
  } else {
    i2cSlave.update(telemetry, false);
    if ((nowMs - lastRenderMs) > 200U) {
      telemetry.dirty = true;
    }
  }

  if (displayStarted && telemetry.dirty) {
    if (display.render(telemetry, testModeEnabled, nowMs)) {
      telemetry.dirty = false;
      lastRenderMs = millis();
    }
  }
}
