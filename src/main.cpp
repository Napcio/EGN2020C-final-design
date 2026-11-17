#include <Arduino.h>
#include <LiquidCrystal.h>
#include <templates.h>

// Global Values
namespace pin
{
    constexpr uint8_t motorLeftForward   = 22;  // IN1
    constexpr uint8_t motorLeftReverse   = 23;  // IN2
    constexpr uint8_t motorRightForward  = 24;  // IN3
    constexpr uint8_t motorRightReverse  = 25;  // IN4
    constexpr uint8_t buttonSel = 9;
    constexpr uint8_t buttonNav = 8;
    constexpr uint8_t lcdRegSel = 12;
    constexpr uint8_t lcdEnable = 11;
    constexpr uint8_t lcdDb4    = 5;
    constexpr uint8_t lcdDb5    = 4;
    constexpr uint8_t lcdDb6    = 3;
    constexpr uint8_t lcdDb7    = 2;
    constexpr uint8_t distTrig = 30;
    constexpr uint8_t distEcho = 31;
    constexpr uint8_t buzzer = 7;
}
LiquidCrystal lcd(pin::lcdRegSel, pin::lcdEnable,
                  pin::lcdDb4, pin::lcdDb5, pin::lcdDb6, pin::lcdDb7);

// Classes
enum class Mode
{
    Idle,
    SetMenu,
    AlarmSet,
    AlarmActive
};

enum class SetMenuOption
{
    Hour,
    Minute,
    Back,
    Set,
    Count // Sentinel value, do not use
};

struct Time
{
    static constexpr uint32_t HOUR = 3600000;
    static constexpr uint16_t MINUTE = 60000; // During addition, multiply by 1UL to avoid overflow
    static constexpr uint16_t SECOND = 1000;

    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;

    Time(unsigned long timeMs)
    {
        hours = (timeMs / HOUR);
        minutes = (timeMs / MINUTE) % 60;
        seconds = (timeMs / SECOND) % 60;
    }

    unsigned long toMs()
    {
        return hours * HOUR + minutes * 1UL * MINUTE + seconds * 1UL * SECOND;
    }
};

// Functions
void handleSetMenu(bool nav, bool sel);
SetMenuOption nextMenuIndex(SetMenuOption current);
bool checkButtonPress(uint8_t pin);
void updateMode(Mode newMode);
void setAlarm(unsigned long duration);
int readUltrasonicDistanceCm(int triggerPin, int echoPin);
void drivingLoop();

// Constants
constexpr uint8_t LCD_WIDTH = 16;
constexpr uint8_t DANGER_DIST = 45; // Distance in cm before the vehicle should avoid an object 
constexpr uint8_t STUCK_DIST = 15; // Distance in cm before the vehicle should back up
constexpr uint16_t REVERSAL_TIME = 3 * Time::SECOND; // Time in seconds to reverse for when the vehicle is stuck

// State
Mode currentMode = Mode::Idle;
SetMenuOption currentSetMenuOption = SetMenuOption::Hour;
bool systemUpdate = true;
unsigned long alarmSetTime = 0;
unsigned long alarmPrevSetDuration = 0;
unsigned long alarmDuration = 0;
uint16_t reversalClock = 0;

void setup() 
{
    Serial.begin(9600);
    
    pinMode(pin::motorLeftForward, OUTPUT);
    pinMode(pin::motorLeftReverse, OUTPUT);
    pinMode(pin::motorRightForward, OUTPUT);
    pinMode(pin::motorRightReverse, OUTPUT);
    pinMode(pin::buttonSel, INPUT);
    pinMode(pin::buttonNav, INPUT);
    pinMode(pin::distTrig, OUTPUT);
    pinMode(pin::distEcho, INPUT);

    // Set motors to max speed (eq to vcc)
    // pinMode(pin::motorLeftEnable, OUTPUT);
    // pinMode(pin::motorRightEnable, OUTPUT);
    // digitalWrite(pin::motorLeftEnable, HIGH);
    // digitalWrite(pin::motorRightEnable, HIGH);

    lcd.begin(16, 2);
}

void loop() 
{
    bool navButtonTrig = checkButtonPress<pin::buttonNav>();
    bool selButtonTrig = checkButtonPress<pin::buttonSel>();

    if (currentMode == Mode::AlarmActive)
        drivingLoop(); // Runs regardless of systemUpdate status

    if ((navButtonTrig || selButtonTrig) ||                             // Update on button press
       ( currentMode == Mode::AlarmSet && millis() % Time::SECOND == 0) // Update every second when alarm set
    )
        systemUpdate = true;

    if (!systemUpdate)
        return;
    systemUpdate = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    
    switch (currentMode)
    {
        case Mode::Idle:
        {   
            // Screen updates
            lcd.print("ALARM IDLE");
            lcd.setCursor(0, 1);
            lcd.print("N:Set  S:UsePrev");

            // Interactions
            if (navButtonTrig)
            {
                currentSetMenuOption = SetMenuOption::Hour;
                updateMode(Mode::SetMenu);
            }
            if (selButtonTrig)
            {
                alarmDuration = alarmPrevSetDuration;
                currentSetMenuOption = SetMenuOption::Set;
                updateMode(Mode::SetMenu);
            }
            break;
        }  
        case Mode::SetMenu:
        {
            handleSetMenu(navButtonTrig, selButtonTrig);
            break;
        }  
        case Mode::AlarmSet:
        {
            const unsigned long alarmEndTime = alarmSetTime + alarmDuration;

            if (millis() > alarmEndTime)
                updateMode(Mode::AlarmActive);
            
            Time timeLeft(alarmEndTime - millis());
            
            // Screen updates
            char buffer[LCD_WIDTH + 1];
            snprintf(buffer, sizeof(buffer), "Time %02dh:%02dm:%02ds", timeLeft.hours, timeLeft.minutes, timeLeft.seconds);
            lcd.print(buffer);
            lcd.setCursor(0, 2);
            lcd.print("N:+5m   S:Cancel");

            // Interactions
            if (navButtonTrig)
            {
                alarmDuration += Time::MINUTE * 1UL * 5;
            } 
            if (selButtonTrig)
            {
                updateMode(Mode::Idle);
            }  
            break;
        }
        case Mode::AlarmActive:
        {
            lcd.print("ALARM EXPIRED");
            lcd.setCursor(0, 2);
            lcd.print("N:+5m  S:Dismiss");
            if (navButtonTrig)
            {
                setAlarm(Time::MINUTE * 1UL * 5);
            }
            if (selButtonTrig)
            {
                updateMode(Mode::Idle);
            }
            break;
        }
    }
}

SetMenuOption nextMenuIndex(SetMenuOption current)
{
    return static_cast<SetMenuOption>((static_cast<int>(current) + 1) % static_cast<int>(SetMenuOption::Count));
}

void handleSetMenu(bool nav, bool sel)
{
    Time currentDuration(alarmDuration);

    // Screen updates
    char buffer[LCD_WIDTH + 1];
    snprintf(buffer, sizeof(buffer), "%02dh:%02dm Back Set", currentDuration.hours, currentDuration.minutes);
    lcd.print(buffer);

    lcd.setCursor(0,1);
    lcd.print("N:Move  S:Action");

    // Interactions
    if (nav)
        currentSetMenuOption = nextMenuIndex(currentSetMenuOption);
        
    switch (currentSetMenuOption)
    {
        case SetMenuOption::Hour:
        {
            lcd.setCursor(0, 0);
            // Add 1 hour
            if (sel)
            {
                currentDuration.hours = (currentDuration.hours + 1) % 24;
                alarmDuration = currentDuration.toMs();
                systemUpdate = true;
            }
            break;
        }
        case SetMenuOption::Minute:
        {
            lcd.setCursor(4, 0);
            // Add 1 minute. For normal operation, may be better for this to be set to 5.
            if (sel)
            {
                currentDuration.minutes = (currentDuration.minutes + 1) % 60;
                alarmDuration = currentDuration.toMs();
                systemUpdate = true;
            }
            break;
        }
        case SetMenuOption::Back:
        {
            lcd.setCursor(8, 0);
            if (sel)
            {
                alarmDuration = 0;
                updateMode(Mode::Idle);
            }
            break;
        }
        case SetMenuOption::Set:
        {
            lcd.setCursor(13, 0);
            if (sel)
            {
                alarmPrevSetDuration = alarmDuration;
                setAlarm(alarmDuration);
            }
            break;
        }
        case SetMenuOption::Count:
        {
            Serial.write("Control reached invalid switch case.");
            break;
        }
    }
}

// Adapted from Tinkercad example code
int readUltrasonicDistanceCm(int triggerPin, int echoPin)
{
  pinMode(triggerPin, OUTPUT);  // Clear the trigger
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  // Sets the trigger pin to HIGH state for 10 microseconds
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  pinMode(echoPin, INPUT);
  // Reads the echo pin, and returns the sound wave travel time in microseconds
  return pulseIn(echoPin, HIGH, 30000) * 0.01723;
}

// NOTE: Needs to be reworked if reversing capabilities are added.
void drivingLoop()
{
    int dist = readUltrasonicDistanceCm(pin::distTrig, pin::distEcho);
    if (dist < DANGER_DIST)
    {
        // Turn right
        digitalWrite(pin::motorLeftForward, HIGH);
        digitalWrite(pin::motorLeftReverse, LOW);
        digitalWrite(pin::motorRightForward, LOW);
        digitalWrite(pin::motorRightReverse, HIGH);
    }
    else
    {
        // Drive forward
        digitalWrite(pin::motorLeftForward, HIGH);
        digitalWrite(pin::motorLeftReverse, LOW);
        digitalWrite(pin::motorRightForward, HIGH);
        digitalWrite(pin::motorRightReverse, LOW);
    }
}

void setAlarm(unsigned long duration)
{
    alarmSetTime = millis();
    alarmDuration = duration;
    updateMode(Mode::AlarmSet);
}

void updateMode(Mode newMode)
{
    // Mode-specific system changes
    if (newMode == Mode::SetMenu)
    {
        lcd.cursor();
    }
    if (currentMode == Mode::SetMenu)
        lcd.noCursor();
    if (currentMode == Mode::AlarmSet)
        alarmSetTime = 0;
    if (newMode == Mode::AlarmActive)
    {
        digitalWrite(pin::buzzer, HIGH);
    }
    if (currentMode == Mode::AlarmActive)
    {
        digitalWrite(pin::buzzer, LOW);
        digitalWrite(pin::motorLeftForward, LOW);
        digitalWrite(pin::motorLeftReverse, LOW);
        digitalWrite(pin::motorRightForward, LOW);
        digitalWrite(pin::motorRightReverse, LOW);
    }
    systemUpdate = true;
    currentMode = newMode;
}