#include <Arduino.h>
#include <LiquidCrystal.h>
#include <templates.h>

// Global Values
namespace pin
{
    constexpr uint8_t motorLeft = 13;
    constexpr uint8_t motorRight = 7;
    constexpr uint8_t lcdEnable = 8;
    constexpr uint8_t lcdRegSel = 6;
    constexpr uint8_t lcdDb4 = 2;
    constexpr uint8_t lcdDb5 = 3;
    constexpr uint8_t lcdDb6 = 4;
    constexpr uint8_t lcdDb7 = 5;
    constexpr uint8_t buttonSel = 12;
    constexpr uint8_t buttonNav = 11;
    constexpr uint8_t distTrig = 10;
    constexpr uint8_t distEcho = 9;
}
LiquidCrystal lcd(pin::lcdRegSel, pin::lcdEnable,
                  pin::lcdDb4, pin::lcdDb5, pin::lcdDb6, pin::lcdDb7);


constexpr uint8_t LCD_WIDTH = 16;
// ms conversions
constexpr uint32_t HOUR = 3600000;
constexpr uint16_t MINUTE = 60000;
constexpr uint16_t SECOND = 1000;

// Classes
enum class Mode
{
    Idle,
    SetMenu,
    AlarmSet,
    AlarmActive
};

enum class SetMenuIndex
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
    static constexpr uint16_t MINUTE = 60000;
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
SetMenuIndex nextMenuIndex(SetMenuIndex current);
bool checkButtonPress(uint8_t pin);
void updateMode(Mode newMode);

// State
Mode currentMode = Mode::Idle;
bool systemUpdate = true;
unsigned long alarmSetTime = 0;
unsigned long alarmDuration = 0;
SetMenuIndex currentMenuItem = SetMenuIndex::Hour;

void setup() 
{
    Serial.begin(9600);
    
    pinMode(pin::motorLeft, OUTPUT);
    pinMode(pin::motorRight, OUTPUT);
    pinMode(pin::buttonSel, INPUT);
    pinMode(pin::buttonNav, INPUT);
    pinMode(pin::distTrig, OUTPUT);
    pinMode(pin::distEcho, INPUT);

    lcd.begin(16, 2);
}

void loop() 
{
    bool navButtonTrig = checkButtonPress<pin::buttonNav>();
    bool selButtonTrig = checkButtonPress<pin::buttonSel>();

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
                updateMode(Mode::SetMenu);
            if (selButtonTrig)
                updateMode(Mode::AlarmSet);
            break;
        }  
        case Mode::SetMenu:
        {
            handleSetMenu(navButtonTrig, selButtonTrig);
            break;
        }  
        case Mode::AlarmSet:
        {
            Time timeLeft(alarmSetTime + alarmDuration - millis());
            
            // Screen updates
            char buffer[LCD_WIDTH + 1];
            snprintf(buffer, sizeof(buffer), "Time %02dh:%02dm:%02ds", timeLeft.hours, timeLeft.minutes, timeLeft.seconds);
            lcd.print(buffer);
            lcd.setCursor(0, 2);
            lcd.print("N:+5m  S:Cancel");

            // Interactions
            if (navButtonTrig)
            {
                alarmDuration += 5 * 1UL * Time::MINUTE;
                systemUpdate = true;
            } 
            if (selButtonTrig)
            {
                updateMode(Mode::Idle);
                systemUpdate = true;
            }  
            break;
        }
        case Mode::AlarmActive:
        {
            break;
        }
    }
}

void setAlarm(unsigned long duration)
{
    alarmSetTime = millis();
    alarmDuration = duration;
    updateMode(Mode::AlarmSet);
}

void cancelAlarm()
{
    alarmSetTime = 0;
    // Alarm duration remains unchanged to reuse on next open
    currentMode = Mode::Idle;
}

SetMenuIndex nextMenuIndex(SetMenuIndex current)
{
    return static_cast<SetMenuIndex>((static_cast<int>(current) + 1) % static_cast<int>(SetMenuIndex::Count));
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
        currentMenuItem = nextMenuIndex(currentMenuItem);
        
    switch (currentMenuItem)
    {
        case SetMenuIndex::Hour:
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
        case SetMenuIndex::Minute:
        {
            lcd.setCursor(4, 0);
            // Add 5 minutes
            if (sel)
            {
                currentDuration.minutes = (currentDuration.minutes + 5) % 60;
                alarmDuration = currentDuration.toMs();
                systemUpdate = true;
            }
            break;
        }
        case SetMenuIndex::Back:
        {
            lcd.setCursor(8, 0);
            if (sel)
                updateMode(Mode::Idle);
            break;
        }
        case SetMenuIndex::Set:
        {
            lcd.setCursor(13, 0);
            if (sel)
                setAlarm(alarmDuration);
            break;
        }
        case SetMenuIndex::Count:
        {
            Serial.write("Control reached invalid switch case.");
            break;
        }
    }
}

void updateMode(Mode newMode)
{
    // Mode-specific system changes
    if (newMode == Mode::SetMenu)
        lcd.cursor();
    if (currentMode == Mode::SetMenu)
        lcd.noCursor();

    systemUpdate = true;
    currentMode = newMode;
}