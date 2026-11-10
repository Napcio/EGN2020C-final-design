#include <Arduino.h>
/**
 * @brief Returns true for 1 clock cycle after a button press.
 * 
 * @tparam PIN Button pin to monitor
 * @return true Button has been triggered.
 * @return false Waiting for next trigger.
 */
template <uint8_t PIN>
bool checkButtonPress()
{
    constexpr uint8_t DEBOUNCE_TIME = 30;

    static bool lastState = false;
    static uint32_t lastStateChange = 0;
    static bool triggerSent = false;

    bool currentButtonState = digitalRead(PIN);
    
    // On state change
    if (currentButtonState != lastState)
    {
        lastStateChange = millis();
        lastState = currentButtonState;
    }

    if (millis() - lastStateChange >= DEBOUNCE_TIME)
    {
        // Bouncing is finished, read values are real
        if (currentButtonState == LOW)
            triggerSent = false;
        else if (!triggerSent)
        {
            triggerSent = true;
            return true;
        }
    }
    return false;
}