#include <Adafruit_NeoPixel.h>
#include <IRremote.h>
#include <Arduino.h>

const byte IR_RECEIVE_PIN = 10; // пин ИК приемника
#define battery A0              // аналоговый пин индикации батареи
#define BUTTON_PIN 2            // пин кнопки
#define PIXEL_PIN 9             // пин ленты
#define PIXEL_COUNT 38          // количество светодиодов
#define BRIGHTNESS_STEP 12      // шаг регулировки яркости
#define COLOR_WIPE_DELAY 5      //интервал между зажиганием светодиодом при смене цвета
#define MINIMUM_BRIGHTNESS 3    // минимальное значение яркости

bool batteryTestFunction = true; // отображение заряда аккумулятора при включении
int batteryTestTimer = 1500;     // время удержания индикации заряда аккумулятора

int strobeCounter = 50;       // количество вспышек при нажати strobe
int strobeDelay = 25;         // задержка между вспышками при нажати strobe
bool stayAfterStrobe = false; // оставлять включеным последний цвет, если он не ноль

int flashCounter = 5; // количество миганий как полицейская мигалка (flash)

bool securityFunction = false; // код для включения пульта. если false, код не требуется
int pinCode1 = 0;
int pinCode2 = 3;
int pinCode3 = 7;
int pinCode4 = 6;
bool remoteDisableFunction = true; // фукнция отключения пульта по тройному нажатию off

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

bool oldState = HIGH;
uint32_t lastColor = 0;
byte mode = 0;
int brightness = 255;
byte stage = 0;
bool strobe = false;
byte remoteDisableStage = 0;

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void colorWipe(uint32_t color)
{
    for (int i = 0; i < PIXEL_COUNT; i++)
    { // For each pixel in strip...
        strip.setPixelColor(i, color);
        strip.show();            //  Update strip to match
        delay(COLOR_WIPE_DELAY); //  Pause for a moment
        if (color != 0)
        {
            lastColor = color;
        }
    }
}

void batteryTest(int counterBT)
{
    for (int i = 0; i < counterBT; i++)
    {
        strip.setPixelColor(i, 255, 0, 0);
        strip.show();
        delay(15);
    }
    delay(batteryTestTimer);
    colorWipe(strip.Color(0, 0, 0));
}

void setup()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    strip.begin();
    strip.show();
    IrReceiver.begin(IR_RECEIVE_PIN);
    // Serial.begin(115200);

    int raw = analogRead(battery);
    float testvoltage = raw * 5.0 / 1023;
    int val = fmap(testvoltage, 3.6, 4.1, 1, PIXEL_COUNT);
    val = constrain(val, 1, PIXEL_COUNT);

    if (batteryTestFunction == true)
    {
        batteryTest(val);
    }
}

void blinkFor(uint32_t color, byte times)
{
    for (byte t = 0; t < times; t++)
    {
        strip.setPixelColor(0, color);
        strip.show();
        delay(100);
        strip.clear();
        strip.show();
        delay(100);
    }
}

void changeBrightness(int delta)
{
    if (brightness >= 0 && brightness <= 256)
    {
        brightness += delta;
        if (brightness > 255)
        {
            brightness = 255;
        }

        if (brightness < MINIMUM_BRIGHTNESS)
        {
            brightness = MINIMUM_BRIGHTNESS;
        }
    }
    strip.setBrightness(brightness);
    strip.show();
}

void security(int codeValue)
{
    if (codeValue == pinCode1)
    {
        stage = 1;
        delay(250);
    }
    else if (codeValue == pinCode2 && stage == 1)
    {
        stage = 2;
        delay(250);
    }
    else if (codeValue == pinCode3 && stage == 2)
    {
        stage = 3;
        delay(250);
    }
    else if (codeValue == pinCode4 && stage == 3)
    {
        stage = 0;
        securityFunction = false;
        blinkFor(strip.Color(0, 0, 255), 2);
    }
    else
    {
        stage = 0;
    }
}

void remoteOFF(int codeValue)
{
    if (codeValue == 0x2 && remoteDisableStage == 0 && remoteDisableFunction == true)
    {
        remoteDisableStage = 1;
    }
    else if (codeValue == 0x2 && remoteDisableStage == 1)
    {
        remoteDisableStage = 2;
    }
    else if (codeValue == 0x2 && remoteDisableStage == 2)
    {
        remoteDisableStage = 0;
        securityFunction = true;
        blinkFor(strip.Color(255, 0, 0), 2);
    }
    else if (codeValue != 0x2)
    {
        remoteDisableStage = 0;
    }
}

void firstHalf(uint32_t color)
{
    int i = 0;
    while (i < PIXEL_COUNT / 2)
    {
        strip.setPixelColor(i, color);
        i++;
    }
    strip.show();
}

void secondHalf(uint32_t color)
{
    int i = PIXEL_COUNT + 1;
    while (i > PIXEL_COUNT / 2 - 1)
    {
        strip.setPixelColor(i, color);
        i--;
    }
    strip.show();
}

void setColor(uint32_t color)
{
    int i = 0;
    while (i < PIXEL_COUNT)
    {
        strip.setPixelColor(i, color);
        i++;
    }
    strip.show();
}

void flash(int times)
{
    for (byte t = 0; t < times; t++)
    {
        if (lastColor == 0)
        {
            setColor(strip.Color(255, 255, 255));
        }
        else
        {
            setColor(lastColor);
        }
        delay(strobeDelay);
        setColor(strip.Color(0, 0, 0));
        delay(strobeDelay);
    }
    if (lastColor != 0 && stayAfterStrobe == true)
    {
        setColor(lastColor);
    }
}

void police(byte times)
{
    for (byte t = 0; t < times; t++)
    {
        strip.clear(); // Set all pixel colors to 'off'
        for (int x = 0; x <= 3; x++)
        {
            firstHalf(strip.Color(0, 0, 255));
            delay(75);
            firstHalf(strip.Color(0, 0, 0));
            delay(75);
        }
        delay(50);

        for (int x = 0; x <= 3; x++)
        {
            secondHalf(strip.Color(255, 0, 0));
            delay(75);
            secondHalf(strip.Color(0, 0, 0));
            delay(75);
        }
        delay(50);
    }
    // alternate police:
    // for (int x = 0; x <= 3; x++)
    // {
    //     setColor(strip.Color(0, 0, 255));
    //     delay(75);
    //     setColor(strip.Color(0, 0, 0));
    //     delay(75);
    // }
    // delay(50);

    // for (int x = 0; x <= 3; x++)
    // {
    //     setColor(strip.Color(255, 0, 0));
    //     delay(75);
    //     setColor(strip.Color(0, 0, 0));
    //     delay(75);
    // }
    // delay(50);
}

void loop()
{
    boolean newState = digitalRead(BUTTON_PIN);

    // if (strobe == true)
    // {
    //     police();
    // }

    if (IrReceiver.decode())
    {
        unsigned long codeValue = IrReceiver.decodedIRData.command;
        if (securityFunction == true)
        {
            security(codeValue);
        }
        else
        {
            remoteOFF(codeValue);
            switch (codeValue)
            {
            case 0x0:
                changeBrightness(BRIGHTNESS_STEP);
                delay(50);
                break;
            case 0x1:
                changeBrightness(-BRIGHTNESS_STEP);
                delay(50);
                break;
            case 0x2: //OFF
                colorWipe(strip.Color(0, 0, 0));
                break;
            case 0x3: //ON
                if (lastColor == 0)
                {
                    colorWipe(strip.Color(100, 100, 100));
                }
                else
                {
                    colorWipe(lastColor);
                }
                break;
            case 0x4: //R
                colorWipe(strip.Color(255, 0, 0));
                break;
            case 0x5: //G
                colorWipe(strip.Color(0, 255, 0));
                break;
            case 0x6: //B
                colorWipe(strip.Color(0, 0, 255));
                break;
            case 0x7: //W
                colorWipe(strip.Color(255, 255, 255));
                break;
            case 0x8:
                colorWipe(strip.Color(255, 76, 0));
                break;
            case 0x9:
                colorWipe(strip.Color(0, 255, 51));
                break;
            case 0xA:
                colorWipe(strip.Color(0, 153, 255));
                break;
            case 0xB: //FLASH
                police(flashCounter);
                // if (strobe == false)
                // {
                //     strobe = true;
                // }
                // else
                // {
                //     strobe = false;
                // }
                break;
            case 0xC:
                colorWipe(strip.Color(255, 127, 0));
                break;
            case 0xD:
                colorWipe(strip.Color(0, 255, 153));
                break;
            case 0xE:
                colorWipe(strip.Color(255, 0, 204));
                break;
            case 0xF: //STROBE
                flash(strobeCounter);
                //police();
                break;
            case 0x10:
                colorWipe(strip.Color(255, 161, 0));
                break;
            case 0x11:
                colorWipe(strip.Color(0, 255, 204));
                break;
            case 0x12:
                colorWipe(strip.Color(255, 0, 102));
                break;
            case 0x13: //FADE
                strip.setBrightness(3);
                strip.show();
                brightness = 3;
                delay(50);
                break;
            case 0x14:
                colorWipe(strip.Color(255, 204, 0));
                break;
            case 0x15:
                colorWipe(strip.Color(0, 255, 255));
                break;
            case 0x16:
                colorWipe(strip.Color(255, 0, 153));
                break;
            case 0x17: //SMOOTH
                strip.setBrightness(255);
                strip.show();
                brightness = 255;
                delay(50);
                break;
            default: // code not recognized
                break;
            }
        }
        IrReceiver.resume();
    }

    // Check if state changed from high to low (button press).
    if ((newState == LOW) && (oldState == HIGH))
    {
        // Short delay to debounce button.
        delay(50);
        // Check if button is still low after debounce.
        newState = digitalRead(BUTTON_PIN);
        if (newState == LOW)
        { // Yes, still low
            if (++mode > 18)
                mode = 0;
            switch (mode)
            { // Start the new animation...
            case 0:
                colorWipe(strip.Color(0, 0, 0)); // Black/off
                break;
            case 1:
                colorWipe(strip.Color(255, 0, 0)); // Red
                break;
            case 2:
                colorWipe(strip.Color(0, 255, 0)); // Green
                break;
            case 3:
                colorWipe(strip.Color(0, 0, 255)); // Blue
                break;
            case 4:
                colorWipe(strip.Color(255, 255, 255));
                break;
            case 5:
                colorWipe(strip.Color(255, 76, 0));
                break;
            case 6:
                colorWipe(strip.Color(0, 255, 51));
                break;
            case 7:
                colorWipe(strip.Color(0, 153, 255));
                break;
            case 8:
                colorWipe(strip.Color(255, 127, 0));
                break;
            case 9:
                colorWipe(strip.Color(0, 255, 153));
                break;
            case 10:
                colorWipe(strip.Color(255, 0, 204));
                break;
            case 11:
                colorWipe(strip.Color(255, 161, 0));
                break;
            case 12:
                colorWipe(strip.Color(0, 255, 204));
                break;
            case 13:
                colorWipe(strip.Color(255, 0, 102));
                break;
            case 14:
                colorWipe(strip.Color(255, 204, 0));
                break;
            case 15:
                colorWipe(strip.Color(0, 255, 255));
                break;
            case 17:
                colorWipe(strip.Color(255, 0, 153));
                break;
            }
        }
    }

    // Set the last-read button state to the old state.
    oldState = newState;
}