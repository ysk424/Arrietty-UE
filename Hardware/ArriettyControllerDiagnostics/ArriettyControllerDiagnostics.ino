// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

// Temporary, read-only GPIO diagnostic firmware for the classic ESP32
// Arrietty control panel. This sketch never starts Wi-Fi or Bluetooth and
// never drives a tested GPIO. Every tested pin is configured as INPUT_PULLUP.

namespace {
constexpr uint32_t kBaudRate = 115200;
constexpr uint32_t kStreamIntervalMs = 20;  // 50 Hz, matching production.
constexpr size_t kCommandBufferSize = 96;

// The eight documented switch inputs come first. Four additional safe input
// candidates follow so a connector shifted to a nearby GPIO can be detected.
// UART, flash, boot-strapping, and the four joystick ADC pins are excluded.
constexpr uint8_t kTestPins[] = {
    18, 19, 21, 22, 23, 26, 13, 14,
    16, 17, 27, 33,
};
constexpr size_t kTestPinCount = sizeof(kTestPins) / sizeof(kTestPins[0]);

constexpr uint8_t kJoystick1PhysicalXPin = 35;
constexpr uint8_t kJoystick1PhysicalYPin = 34;
constexpr uint8_t kJoystick2PhysicalXPin = 32;
constexpr uint8_t kJoystick2PhysicalYPin = 25;

char commandBuffer[kCommandBufferSize];
size_t commandLength = 0;
bool streaming = false;
uint32_t sequenceNumber = 0;
uint32_t lastStreamAtMs = 0;

uint16_t ReadLowMask() {
  uint16_t mask = 0;
  for (size_t pinIndex = 0; pinIndex < kTestPinCount; ++pinIndex) {
    if (digitalRead(kTestPins[pinIndex]) == LOW) {
      mask |= static_cast<uint16_t>(1u << pinIndex);
    }
  }
  return mask;
}

void PrintState() {
  Serial.printf(
      "D1,%lu,%u,%d,%d,%d,%d\n",
      static_cast<unsigned long>(sequenceNumber++),
      static_cast<unsigned int>(ReadLowMask()),
      analogRead(kJoystick1PhysicalXPin),
      analogRead(kJoystick1PhysicalYPin),
      analogRead(kJoystick2PhysicalXPin),
      analogRead(kJoystick2PhysicalYPin));
}

void HandleCommand() {
  commandBuffer[commandLength] = '\0';

  if (strcmp(commandBuffer, "PING") == 0) {
    Serial.println("PONG ARRIETTY-DIAGNOSTIC/1");
  } else if (strcmp(commandBuffer, "INFO") == 0) {
    Serial.println(
        "INFO ARRIETTY-DIAGNOSTIC/1 INPUT_PULLUP GPIO "
        "18,19,21,22,23,26,13,14,16,17,27,33");
  } else if (strcmp(commandBuffer, "READ") == 0) {
    PrintState();
  } else if (strcmp(commandBuffer, "STREAM ON") == 0) {
    streaming = true;
    lastStreamAtMs = 0;
    Serial.println("OK STREAM ON 50HZ");
  } else if (strcmp(commandBuffer, "STREAM OFF") == 0) {
    streaming = false;
    Serial.println("OK STREAM OFF");
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

  for (size_t pinIndex = 0; pinIndex < kTestPinCount; ++pinIndex) {
    pinMode(kTestPins[pinIndex], INPUT_PULLUP);
  }

  delay(300);
  Serial.println("READY ARRIETTY-DIAGNOSTIC/1");
  Serial.println("PINS 18,19,21,22,23,26,13,14,16,17,27,33");
}

void loop() {
  const uint32_t nowMs = millis();
  ProcessSerialInput();

  if (streaming && nowMs - lastStreamAtMs >= kStreamIntervalMs) {
    lastStreamAtMs = nowMs;
    PrintState();
  }

  delay(1);
}
