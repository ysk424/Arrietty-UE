// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

// Arrietty wired control panel for a classic ESP32 connected through a
// CH340 USB-to-serial bridge. The painted X/Y labels on each joystick are
// the public coordinate system; electrical module labels are implementation
// details documented in docs/HARDWARE_CONTROLS.md.

namespace {
constexpr uint32_t kBaudRate = 115200;
constexpr uint32_t kStreamIntervalMs = 20;  // 50 Hz
constexpr uint32_t kDebounceMs = 15;
constexpr uint32_t kMinimumReportedPressMs = 100;
constexpr int kAdcMaximum = 4095;
constexpr int kAxisDeadzone = 160;
constexpr size_t kCommandBufferSize = 96;

// Joystick 1 mapping was measured on the assembled unit. Painted X+ is the
// decreasing voltage on GPIO35, and painted Y+ is decreasing on GPIO34.
constexpr uint8_t kJoystick1PhysicalXPin = 35;
constexpr uint8_t kJoystick1PhysicalYPin = 34;
constexpr bool kJoystick1InvertX = true;
constexpr bool kJoystick1InvertY = true;

// Joystick 2 uses the module X/Y wiring. Its complete range was verified;
// polarity can be changed here later without rewiring.
constexpr uint8_t kJoystick2PhysicalXPin = 32;
constexpr uint8_t kJoystick2PhysicalYPin = 25;
constexpr bool kJoystick2InvertX = false;
constexpr bool kJoystick2InvertY = false;

// Bits 0..5 are the six panel buttons; bits 6..7 are joystick push switches.
constexpr uint8_t kDigitalPins[] = {18, 19, 21, 22, 23, 26, 13, 14};
constexpr size_t kDigitalInputCount = sizeof(kDigitalPins) / sizeof(kDigitalPins[0]);

char commandBuffer[kCommandBufferSize];
size_t commandLength = 0;
bool stablePressed[kDigitalInputCount] = {};
bool candidatePressed[kDigitalInputCount] = {};
bool previousRawPressed[kDigitalInputCount] = {};
uint32_t candidateChangedAtMs[kDigitalInputCount] = {};
uint32_t reportPressedUntilMs[kDigitalInputCount] = {};
int filteredAxes[4] = {};
int axisCenters[4] = {};
bool streaming = false;
uint32_t sequenceNumber = 0;
uint32_t lastStreamAtMs = 0;

int ReadAdc(uint8_t pin, bool invert) {
  const int value = analogRead(pin);
  return invert ? kAdcMaximum - value : value;
}

void ReadRawAxes(int (&axes)[4]) {
  axes[0] = ReadAdc(kJoystick1PhysicalXPin, kJoystick1InvertX);
  axes[1] = ReadAdc(kJoystick1PhysicalYPin, kJoystick1InvertY);
  axes[2] = ReadAdc(kJoystick2PhysicalXPin, kJoystick2InvertX);
  axes[3] = ReadAdc(kJoystick2PhysicalYPin, kJoystick2InvertY);
}

void CalibrateAxes() {
  constexpr int kCalibrationSamples = 64;
  int64_t totals[4] = {};

  for (int sampleIndex = 0; sampleIndex < kCalibrationSamples; ++sampleIndex) {
    int axes[4];
    ReadRawAxes(axes);
    for (int axisIndex = 0; axisIndex < 4; ++axisIndex) {
      totals[axisIndex] += axes[axisIndex];
    }
    delay(5);
  }

  for (int axisIndex = 0; axisIndex < 4; ++axisIndex) {
    axisCenters[axisIndex] = static_cast<int>(totals[axisIndex] / kCalibrationSamples);
    filteredAxes[axisIndex] = axisCenters[axisIndex];
  }
}

void UpdateAxes() {
  int rawAxes[4];
  ReadRawAxes(rawAxes);
  for (int axisIndex = 0; axisIndex < 4; ++axisIndex) {
    // A small integer low-pass filter reduces ADC noise without adding
    // noticeable control latency at the 50 Hz report rate.
    filteredAxes[axisIndex] = (filteredAxes[axisIndex] * 3 + rawAxes[axisIndex]) / 4;
  }
}

int NormalizeAxis(int value, int center) {
  int delta = value - center;
  if (abs(delta) <= kAxisDeadzone) {
    return 0;
  }

  if (delta > 0) {
    const int available = max(1, kAdcMaximum - center - kAxisDeadzone);
    return constrain(
        static_cast<int>((static_cast<int64_t>(delta - kAxisDeadzone) * 32767) / available),
        0,
        32767);
  }

  const int available = max(1, center - kAxisDeadzone);
  return constrain(
      static_cast<int>((static_cast<int64_t>(delta + kAxisDeadzone) * 32767) / available),
      -32767,
      0);
}

void UpdateDigitalInputs(uint32_t nowMs) {
  for (size_t inputIndex = 0; inputIndex < kDigitalInputCount; ++inputIndex) {
    const bool pressed = digitalRead(kDigitalPins[inputIndex]) == LOW;
    // Preserve even a short or electrically noisy press for five 50 Hz
    // packets. This prevents a valid edge from disappearing inside the
    // debounce window while the normal stable-state debounce still filters
    // release chatter and sustained input.
    if (pressed && !previousRawPressed[inputIndex]) {
      reportPressedUntilMs[inputIndex] = nowMs + kMinimumReportedPressMs;
    }
    previousRawPressed[inputIndex] = pressed;

    if (pressed != candidatePressed[inputIndex]) {
      candidatePressed[inputIndex] = pressed;
      candidateChangedAtMs[inputIndex] = nowMs;
    } else if (stablePressed[inputIndex] != candidatePressed[inputIndex] &&
               nowMs - candidateChangedAtMs[inputIndex] >= kDebounceMs) {
      stablePressed[inputIndex] = candidatePressed[inputIndex];
    }
  }
}

uint8_t BuildButtonMask() {
  uint8_t mask = 0;
  const uint32_t nowMs = millis();
  for (size_t inputIndex = 0; inputIndex < kDigitalInputCount; ++inputIndex) {
    const bool reportLatchActive =
        static_cast<int32_t>(reportPressedUntilMs[inputIndex] - nowMs) > 0;
    if (stablePressed[inputIndex] || reportLatchActive) {
      mask |= static_cast<uint8_t>(1u << inputIndex);
    }
  }
  return mask;
}

void PrintState() {
  Serial.printf(
      "A1,%lu,%d,%d,%d,%d,%u\n",
      static_cast<unsigned long>(sequenceNumber++),
      NormalizeAxis(filteredAxes[0], axisCenters[0]),
      NormalizeAxis(filteredAxes[1], axisCenters[1]),
      NormalizeAxis(filteredAxes[2], axisCenters[2]),
      NormalizeAxis(filteredAxes[3], axisCenters[3]),
      static_cast<unsigned int>(BuildButtonMask()));
}

void HandleCommand() {
  commandBuffer[commandLength] = '\0';

  if (strcmp(commandBuffer, "PING") == 0) {
    Serial.println("PONG ARRIETTY-CONTROLLER/1");
  } else if (strcmp(commandBuffer, "READ") == 0) {
    PrintState();
  } else if (strcmp(commandBuffer, "STREAM ON") == 0) {
    streaming = true;
    lastStreamAtMs = 0;
    Serial.println("OK STREAM ON 50HZ");
  } else if (strcmp(commandBuffer, "STREAM OFF") == 0) {
    streaming = false;
    Serial.println("OK STREAM OFF");
  } else if (strcmp(commandBuffer, "CAL") == 0) {
    const bool wasStreaming = streaming;
    streaming = false;
    CalibrateAxes();
    streaming = wasStreaming;
    Serial.printf(
        "OK CAL %d,%d,%d,%d\n",
        axisCenters[0], axisCenters[1], axisCenters[2], axisCenters[3]);
  } else if (strcmp(commandBuffer, "INFO") == 0) {
    Serial.println("INFO ARRIETTY-CONTROLLER/1 ESP32-D0WD-V3 CH340 115200 50HZ");
  } else if (commandLength > 0) {
    Serial.println("ERR UNKNOWN_COMMAND");
  }

  commandLength = 0;
}

void ProcessSerialInput() {
  while (Serial.available() > 0) {
    const char input = static_cast<char>(Serial.read());
    if (input == '\n') {
      HandleCommand();
    } else if (input != '\r') {
      if (commandLength < kCommandBufferSize - 1) {
        commandBuffer[commandLength++] = input;
      } else {
        commandLength = 0;
        Serial.println("ERR COMMAND_TOO_LONG");
      }
    }
  }
}
}  // namespace

void setup() {
  Serial.begin(kBaudRate);
  analogReadResolution(12);

  for (size_t inputIndex = 0; inputIndex < kDigitalInputCount; ++inputIndex) {
    pinMode(kDigitalPins[inputIndex], INPUT_PULLUP);
    const bool pressed = digitalRead(kDigitalPins[inputIndex]) == LOW;
    stablePressed[inputIndex] = pressed;
    candidatePressed[inputIndex] = pressed;
    previousRawPressed[inputIndex] = pressed;
    candidateChangedAtMs[inputIndex] = millis();
    reportPressedUntilMs[inputIndex] = pressed
        ? candidateChangedAtMs[inputIndex] + kMinimumReportedPressMs
        : 0;
  }

  delay(300);
  CalibrateAxes();
  Serial.println("READY ARRIETTY-CONTROLLER/1");
}

void loop() {
  const uint32_t nowMs = millis();
  UpdateAxes();
  UpdateDigitalInputs(nowMs);
  ProcessSerialInput();

  if (streaming && nowMs - lastStreamAtMs >= kStreamIntervalMs) {
    lastStreamAtMs = nowMs;
    PrintState();
  }

  delay(1);
}
