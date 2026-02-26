/**
 * OpenX4ArduinoHelper Demo Sketch
 *
 * Tests BatteryMonitor, InputManager, and SDCardManager.
 * Upload to your X4 device and open Serial Monitor at 115200 baud.
 */

#include <OpenX4ArduinoHelper.h>

// Battery monitor on ADC pin 4 with default 2x voltage divider
static constexpr uint8_t BATTERY_ADC_PIN = 4;
BatteryMonitor battery(BATTERY_ADC_PIN);

InputManager input;

// Print battery status every 5 seconds
static constexpr unsigned long BATTERY_INTERVAL_MS = 5000;
unsigned long lastBatteryPrint = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println("=================================");
    Serial.println(" OpenX4ArduinoHelper Demo");
    Serial.println("=================================");
    Serial.println();

    // --- SD Card ---
    Serial.println("--- SD Card Init ---");
    // Uses X4 defaults: CS=21, SCK=8, MISO=7, MOSI=10, 10MHz
    if (SdMan.begin()) {
        Serial.println("SD card ready!");

        // Write a test file
        if (SdMan.writeFile("/hello.txt", "Hello from OpenX4ArduinoHelper!")) {
            Serial.println("Wrote /hello.txt");
        } else {
            Serial.println("Failed to write /hello.txt");
        }

        // Read it back
        String content = SdMan.readFile("/hello.txt");
        Serial.print("Read back: ");
        Serial.println(content);

        // List root directory
        Serial.println("Files on card:");
        auto files = SdMan.listFiles("/", 20);
        for (const auto& f : files) {
            Serial.print("  ");
            Serial.println(f);
        }
    } else {
        Serial.println("SD card init failed! Detailed error:");
        SdMan.printInitError();
    }
    Serial.println();

    // --- Input ---
    input.begin();

    // --- Battery ---
    printBatteryStatus();

    Serial.println("Press any button to test input...");
    Serial.println();
}

void loop() {
    input.update();

    // Report any new button presses
    if (input.wasAnyPressed()) {
        for (uint8_t i = 0; i <= InputManager::BTN_POWER; i++) {
            if (input.wasPressed(i)) {
                Serial.print("[PRESSED]  ");
                Serial.println(InputManager::getButtonName(i));
            }
        }
    }

    // Report any button releases with held duration
    if (input.wasAnyReleased()) {
        for (uint8_t i = 0; i <= InputManager::BTN_POWER; i++) {
            if (input.wasReleased(i)) {
                Serial.print("[RELEASED] ");
                Serial.print(InputManager::getButtonName(i));
                Serial.print("  (held ");
                Serial.print(input.getHeldTime());
                Serial.println(" ms)");
            }
        }
        Serial.println();
    }

    // Periodic battery status
    unsigned long now = millis();
    if (now - lastBatteryPrint >= BATTERY_INTERVAL_MS) {
        lastBatteryPrint = now;
        printBatteryStatus();
    }
}

void printBatteryStatus() {
    Serial.println("--- Battery Status ---");
    Serial.print("  Voltage:    ");
    Serial.print(battery.readVolts(), 2);
    Serial.println(" V");
    Serial.print("  Millivolts: ");
    Serial.print(battery.readMillivolts());
    Serial.println(" mV");
    Serial.print("  Percentage: ");
    Serial.print(battery.readPercentage());
    Serial.println(" %");
    Serial.println("----------------------");
    Serial.println();
}
