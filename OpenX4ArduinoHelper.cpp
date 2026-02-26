#include "OpenX4ArduinoHelper.h"
#include <esp_idf_version.h>
#if ESP_IDF_VERSION_MAJOR < 5
#include <esp_adc_cal.h>
#endif

// ============================================================================
// BatteryMonitor
// ============================================================================

BatteryMonitor::BatteryMonitor(uint8_t adcPin, float dividerMultiplier)
  : _adcPin(adcPin), _dividerMultiplier(dividerMultiplier) {}

uint16_t BatteryMonitor::readPercentage() const {
    return percentageFromMillivolts(readMillivolts());
}

uint16_t BatteryMonitor::readMillivolts() const {
#if ESP_IDF_VERSION_MAJOR < 5
    const uint16_t raw = analogRead(_adcPin);
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    const uint16_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);
#else
    const uint16_t mv = analogReadMilliVolts(_adcPin);
#endif
    return static_cast<uint16_t>(mv * _dividerMultiplier);
}

double BatteryMonitor::readVolts() const {
    return static_cast<double>(readMillivolts()) / 1000.0;
}

uint16_t BatteryMonitor::percentageFromMillivolts(uint16_t millivolts) {
    double volts = millivolts / 1000.0;
    double y = -144.9390 * volts * volts * volts +
               1655.8629 * volts * volts -
               6158.8520 * volts +
               7501.3202;
    y = std::max(y, 0.0);
    y = std::min(y, 100.0);
    y = round(y);
    return y;
}

// ============================================================================
// InputManager
// ============================================================================

const int InputManager::ADC_RANGES_1[] = {ADC_NO_BUTTON, 3100, 2090, 750, INT32_MIN};
const int InputManager::ADC_RANGES_2[] = {ADC_NO_BUTTON, 1120, INT32_MIN};
const char* InputManager::BUTTON_NAMES[] = {"Back", "Confirm", "Left", "Right", "Up", "Down", "Power"};

InputManager::InputManager()
    : currentState(0),
      lastState(0),
      pressedEvents(0),
      releasedEvents(0),
      lastDebounceTime(0),
      buttonPressStart(0),
      buttonPressFinish(0) {}

void InputManager::begin() {
    pinMode(BUTTON_ADC_PIN_1, INPUT);
    pinMode(BUTTON_ADC_PIN_2, INPUT);
    pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);
    analogSetAttenuation(ADC_11db);
}

int InputManager::getButtonFromADC(const int adcValue, const int ranges[], const int numButtons) {
    for (int i = 0; i < numButtons; i++) {
        if (ranges[i + 1] < adcValue && adcValue <= ranges[i]) {
            return i;
        }
    }
    return -1;
}

uint8_t InputManager::getState() {
    uint8_t state = 0;

    const int adcValue1 = analogRead(BUTTON_ADC_PIN_1);
    const int button1 = getButtonFromADC(adcValue1, ADC_RANGES_1, NUM_BUTTONS_1);
    if (button1 >= 0) {
        state |= (1 << button1);
    }

    const int adcValue2 = analogRead(BUTTON_ADC_PIN_2);
    const int button2 = getButtonFromADC(adcValue2, ADC_RANGES_2, NUM_BUTTONS_2);
    if (button2 >= 0) {
        state |= (1 << (button2 + 4));
    }

    if (digitalRead(POWER_BUTTON_PIN) == LOW) {
        state |= (1 << BTN_POWER);
    }

    return state;
}

void InputManager::update() {
    const unsigned long currentTime = millis();
    const uint8_t state = getState();

    pressedEvents = 0;
    releasedEvents = 0;

    if (state != lastState) {
        lastDebounceTime = currentTime;
        lastState = state;
    }

    if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (state != currentState) {
            pressedEvents = state & ~currentState;
            releasedEvents = currentState & ~state;

            if (pressedEvents > 0 && currentState == 0) {
                buttonPressStart = currentTime;
            }

            if (releasedEvents > 0 && state == 0) {
                buttonPressFinish = currentTime;
            }

            currentState = state;
        }
    }
}

bool InputManager::isPressed(const uint8_t buttonIndex) const {
    return currentState & (1 << buttonIndex);
}

bool InputManager::wasPressed(const uint8_t buttonIndex) const {
    return pressedEvents & (1 << buttonIndex);
}

bool InputManager::wasAnyPressed() const {
    return pressedEvents > 0;
}

bool InputManager::wasReleased(const uint8_t buttonIndex) const {
    return releasedEvents & (1 << buttonIndex);
}

bool InputManager::wasAnyReleased() const {
    return releasedEvents > 0;
}

unsigned long InputManager::getHeldTime() const {
    if (currentState > 0) {
        return millis() - buttonPressStart;
    }
    return buttonPressFinish - buttonPressStart;
}

const char* InputManager::getButtonName(const uint8_t buttonIndex) {
    if (buttonIndex <= BTN_POWER) {
        return BUTTON_NAMES[buttonIndex];
    }
    return "Unknown";
}

bool InputManager::isPowerButtonPressed() const {
    return isPressed(BTN_POWER);
}