#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keyboard.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define ZERO_PIN 5
#define ONE_PIN 4

// I wanted to use interrupts but I couldn't get it working
// I'll research it more later
bool zeroPressed = true;
bool onePressed = true;

unsigned int currentByte = 0;
unsigned int count = 0;

String debugData = "";

/**
 * Welcome to hacky hack ville, population: this code
 * Warning, may(does) contain, bad practices because it's 3AM and I'm tired
 */
void setup() {
  Serial.begin(9600);

  pinMode(ZERO_PIN, INPUT_PULLUP);
  pinMode(ONE_PIN, INPUT_PULLUP);

  Keyboard.begin();


  // Generate display voltage from 3.3 internally - from SSD1306 example
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display failed to load"));
    for (;;); // go into permanenet loop so we don't break stuff 
  }

  drawBits();
}

void loop() {
  delay(10);
  
  if (clicked(&zeroPressed, ZERO_PIN)) {
    addBit(0);
  }
  if (clicked(&onePressed, ONE_PIN)) {
    addBit(1);
  }
}

bool clicked(bool *isPressed, int pin) {
  if (digitalRead(pin) == HIGH && !(*isPressed)) {
    *isPressed = true;
    return true;
  } else if (digitalRead(pin) == LOW) {
    *isPressed = false;
  }

  return false;
}

void addBit(int b) {
  // Shift bits left
  currentByte <<= 1;
  // Modify least significant bit
  currentByte |= b;
  
  count++;

  if (count == 8) {
    Keyboard.write((int)currentByte);
    currentByte = 0;
    count = 0;
  }
  
  drawBits();
}

void drawBits() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,8);
  String output = "-";

  for (int i = count; i > 0; i--) {
    output += ((currentByte >> i-1) & 1) == 0 ? "0" : "1";
  }

  for (int i = count; i <= 7; i++) {
    output += "_";
  }

  output += "-";

  display.println(output);
  display.display();
}
