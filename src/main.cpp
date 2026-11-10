#include <Arduino.h>
#include <LiquidCrystal.h>
#include <templates.h>

enum class Mode
{
    Idle,
    AlarmSet,
    AlarmActive,
    AlarmRing
};

enum class SetMenuIndex
{
    Hour,
    Minute,
    Back,
    Set,
    Count // Sentinel value, do not use
};

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

    if (navButtonTrig || selButtonTrig)
        systemUpdate = true;

    if (!systemUpdate)
        return;
    systemUpdate = false;

    lcd.clear();
    lcd.noCursor();
    
    switch (currentMode)
    {
        case Mode::Idle:
        {   
            lcd.setCursor(0, 0);
            lcd.print("ALARM IDLE");
            lcd.setCursor(0, 1);
            lcd.print("N:Set  S:UsePrev");
            if (navButtonTrig)
                updateMode(Mode::AlarmSet);
            if (selButtonTrig)
                updateMode(Mode::AlarmActive);
            break;
        }  
        case Mode::AlarmSet:
        {
            lcd.cursor();

            handleSetMenu(navButtonTrig, selButtonTrig);
            break;
        }  
        case Mode::AlarmActive:
        {
            lcd.print("ALARM ACTIVE");
            break;
        }
        case Mode::AlarmRing:
        {
            break;
        }
    }
}

void setAlarm(unsigned long duration)
{
    alarmSetTime = millis();
    alarmDuration = duration;
    currentMode = Mode::AlarmSet;
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
    uint8_t hours = (alarmDuration / HOUR);
    uint8_t minutes = (alarmDuration / MINUTE) % 60;

    lcd.setCursor(0,0);
    char buffer[LCD_WIDTH + 1];
    snprintf(buffer, sizeof(buffer), "%02dh:%02dm Back Set", hours, minutes);
    lcd.print(buffer);

    lcd.setCursor(0,1);
    lcd.print("N:Move  S:Action");

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
                hours = (hours + 1) % 24;
                alarmDuration = hours * HOUR + minutes * 1UL * MINUTE;
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
                minutes = (minutes + 5) % 60;
                alarmDuration = hours * HOUR + minutes * 1UL * MINUTE;
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
                updateMode(Mode::Idle);
            break;
        }
    }
}

void updateMode(Mode newMode)
{
    systemUpdate = true;
    currentMode = newMode;
}