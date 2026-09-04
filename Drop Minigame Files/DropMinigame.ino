#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

#define LEFT_BUTTON_PIN 14
#define RIGHT_BUTTON_PIN 15

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void printCentered(String text, int y, int textSize){
  display.setTextSize(textSize);
  display.setCursor((SCREEN_WIDTH - (int)text.length() * 6 * textSize) / 2, y);
  display.print(text);
}

const int PADDLE_WIDTH = 20;
const int PADDLE_HEIGHT = 2;
const int PADDLE_Y = 56;
const int PADDLE_SPEED = 3;
const int PADDLE_START_X = SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2;

int paddleX = PADDLE_START_X;

void drawReadyScreen() {
  display.clearDisplay();
  printCentered("Press any button", 15, 1);
  printCentered("To start", 30, 1);
  display.fillRect(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);
  display.display();
}

void setup() {
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

  Wire.begin();
  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 not found - check wiring/address!");
    while (true) {
      // halt
    }
  }
  Serial.println("Display found!");

  display.setTextColor(SSD1306_WHITE);
  drawReadyScreen();
}

void loop() {
  bool leftPressed = (digitalRead(LEFT_BUTTON_PIN) == LOW);
  bool rightPressed = (digitalRead(RIGHT_BUTTON_PIN) == LOW);

  Serial.print("Left: ");
  Serial.print(leftPressed);
  Serial.print("  Right: ");
  Serial.println(rightPressed);

  if (leftPressed) {
    paddleX -= PADDLE_SPEED;
  }
  if (rightPressed) {
    paddleX += PADDLE_SPEED;
  }

  if (paddleX < 0) {
    paddleX = 0;
  }
  if (paddleX > SCREEN_WIDTH - PADDLE_WIDTH) {
    paddleX = SCREEN_WIDTH - PADDLE_WIDTH;
  }

  display.clearDisplay();
  display.fillRect(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);
  display.display();

  delay(20);
}
