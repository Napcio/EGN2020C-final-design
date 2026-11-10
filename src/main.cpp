#include <Arduino.h>
#include <LiquidCrystal.h>

enum class State
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
    Set
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
constexpr uint16_t HOUR = 3600000;
constexpr uint16_t MINUTE = 60000;

State currentState = State::Idle;
unsigned long alarmSetTime = 0;
unsigned long alarmDuration = 0;
SetMenuIndex currentMenuItem = SetMenuIndex::Hour;

void setup() 
{
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

    switch (currentState)
    {
        case State::Idle:
        {   
            lcd.setCursor(0, 0);
            lcd.print("ALARM IDLE");
            lcd.setCursor(0, 1);
            lcd.print("N:Set  S:UsePrev");
            break;
        }  
        case State::AlarmSet:
        {
            lcd.cursor();
            lcd.blink();

            lcd.setCursor(0,0);
            int hours = (alarmDuration / 3600000) % 24;
            int minutes = (alarmDuration / 60000) % 60;
            char buffer[LCD_WIDTH];
            snprintf(buffer, sizeof(buffer), "%-02dh:%-02dm Back Set", hours, minutes);
            lcd.print(buffer);

            lcd.setCursor(0,1);
            lcd.print("N:Move  S:Action");
        }  
        case State::AlarmActive:
        {
            break;
        }
        case State::AlarmRing:
        {
            break;
        }
    }
}

void setAlarm(unsigned long duration)
{
    alarmSetTime = millis();
    alarmDuration = duration;
    currentState = State::AlarmSet;
}

void cancelAlarm()
{
    alarmSetTime = 0;
    // Alarm duration remains unchanged to reuse on next open
    currentState = State::Idle;
}


void handleSetMenu(bool nav, bool sel)
{
    switch (currentMenuItem)
    {
        case SetMenuIndex::Hour:
        {
            lcd.setCursor(0, 0);
            // Add 5 minutes
            if (sel)
                alarmDuration = (alarmDuration + ) % 60;
            break;
        }
        case SetMenuIndex::Minute:
        {
            lcd.setCursor(4, 0);
            // 
            break;
        }
        case SetMenuIndex::Back:
        {
            lcd.setCursor(8, 0);
            break;
        }
        case SetMenuIndex::Set:
        {
            lcd.setCursor(13, 0);
            break;
        }
    }
}