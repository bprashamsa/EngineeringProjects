#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

//Pin assignments for different pieces
// The OLED uses a software/bit-banged I2C bus on these two pins
// (Wire.begin(SDA, SCL) is called with these below).
#define SCREEN_DATA_PIN    16   // I2C SDA
#define SCREEN_CLOCK_PIN   4    // I2C SCL
#define LEFT_BUTTON_PIN    14   // Moves paddle left when pressed
#define RIGHT_BUTTON_PIN   27   // Moves paddle right when pressed

//Display geometry
#define SCREEN_WIDTH       128
#define SCREEN_HEIGHT      64
#define SCREEN_ADDRESS     0x3C   // Typical address for SH1106 OLEDs

// Display driver object
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Simple state machine for the app's 3 screens
enum Screen { READY, GAME, SCORE };
Screen currentScreen = READY;

// Draws a line of text horizontally centered at a given y coordinate.
// Adafruit_GFX's default font is 6px wide per character (before scaling
// by textSize), which is why "6 * textSize" is used to estimate width.
void printCentered(String text, int y, int textSize){
  display.setTextSize(textSize);
  display.setCursor((SCREEN_WIDTH - (int)text.length() * 6 * textSize) / 2, y);
  display.print(text);
}

//Button debouncing state
const unsigned long DEBOUNCE_TIME = 50;   
bool buttonWasDown = false;               
unsigned long lastButtonChangeTime = 0;   

// Returns true exactly once, at the moment either button transitions
// from "up" to "down" (after the debounce window has passed).
// Buttons are wired with INPUT_PULLUP, so LOW means "pressed.""
bool wasButtonJustPressed(){
  bool isDownNow = digitalRead(LEFT_BUTTON_PIN) == LOW || digitalRead(RIGHT_BUTTON_PIN) == LOW;
  if (isDownNow == buttonWasDown) return false;                      // no change in state
  if (millis() - lastButtonChangeTime < DEBOUNCE_TIME) return false; // too soon, ignore 
  buttonWasDown = isDownNow;
  lastButtonChangeTime = millis();
  return isDownNow;   // true only on the up->down transition
}

//Game constants
const int PADDLE_WIDTH = 28;
const int PADDLE_HEIGHT = 3;
const int PADDLE_Y = 58;                 // fixed vertical position of the paddle
const int PADDLE_SPEED = 4;              // pixels moved per game tick while a button is held
const int PADDLE_START_X = SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2;  // centered start position
const int BALL_SIZE = 3;                 // ball radius
const float BALL_SPEED = 3.5;            // pixels the ball falls per game tick
const int BALL_START_Y = 14;             // y coordinate where a new ball appears
const unsigned long GAME_LENGTH = 30000; // total round length in ms (30 seconds)

//Game state
int paddleX;
int ballX;
int ballY;
int score;
unsigned long gameStartTime;

// Draws the idle "title screen" with instructions and a static paddle.
void drawReadyScreen(){
  display.clearDisplay();
  printCentered("Press any button", 20, 1);
  printCentered("to start", 32, 1);
  display.fillRect(PADDLE_START_X, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, SH110X_WHITE);
  display.display();
}

// Spawns a new ball at a random x position at the top drop point.
void dropNewBall(){
  ballX = random(BALL_SIZE, SCREEN_WIDTH - BALL_SIZE);
  ballY = BALL_START_Y;
}

// Resets score/paddle/timer, spawns the first ball, and switches to GAME state.
void startGame(){
  randomSeed(millis());
  score = 0;
  paddleX = PADDLE_START_X;
  gameStartTime = millis();
  dropNewBall();
  currentScreen = GAME;
}

// Renders one frame of active gameplay: score, remaining time, ball, paddle.
void drawGame(){
  int secondsLeft = (GAME_LENGTH - (millis() - gameStartTime)) / 1000;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Score: " + String(score));
  display.setCursor(73,0);
  display.print("Time: "+ String(secondsLeft));
  display.fillCircle(ballX, ballY, BALL_SIZE, SH110X_WHITE);
  display.fillRect(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, SH110X_WHITE);
  display.display();
}

// Runs one pixel of game logic:
// - moves the paddle based on button input (clamped to screen bounds)
// - moves the ball down
// - when the ball reaches the paddle's row, checks if it landed within
//   the paddle's width; if so, increments the score
// - spawns a new ball either way, then redraws and briefly pauses
void updateGame(){
  if(digitalRead(LEFT_BUTTON_PIN) == LOW) paddleX -= PADDLE_SPEED;
  if(digitalRead(RIGHT_BUTTON_PIN) == LOW) paddleX += PADDLE_SPEED;
  if(paddleX < 0) paddleX = 0;
  if(paddleX > SCREEN_WIDTH - PADDLE_WIDTH) paddleX = SCREEN_WIDTH - PADDLE_WIDTH;

  ballY += BALL_SPEED;
  if(ballY >= PADDLE_Y){
    if(ballX > paddleX && ballX < paddleX + PADDLE_WIDTH){
      score = score +1;   
    }
    dropNewBall();        
  }

  drawGame();
  delay(30);   // simple frame pacing ( there is a 33 FPS cap)
}

//Score screen state
const unsigned long SCORE_SCREEN_WAIT = 2000;  // minimum time before a button press is accepted
unsigned long scoreScreenStartTime;

// Draws the final score and a prompt to press a button to return to the ready screen.
void drawScoreScreen(){
  display.clearDisplay();
  printCentered("Score: " + String(score), 18, 2);
  printCentered("Press any button",52,1);
  display.display();
}

void setup() {
  // Buttons pulled high internally; wiring pulls them LOW when pressed.
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

  
  Wire.begin(SCREEN_DATA_PIN, SCREEN_CLOCK_PIN);
  display.begin(SCREEN_ADDRESS, true);
  display.setTextColor(SH110X_WHITE);

  drawReadyScreen();
}

// Main loop implements the 3-state state machine: READY -> GAME -> SCORE -> READY ...
void loop() {
  bool buttonPressed = wasButtonJustPressed();
  switch (currentScreen) {
    case READY:
      // Any button press starts a new round.
      if (buttonPressed) {
        startGame();
      }
      break;

    case GAME:
      // Once time runs out, show the score screen; otherwise keep updating gameplay.
      if (millis() - gameStartTime > GAME_LENGTH){
        drawScoreScreen();
        scoreScreenStartTime = millis();
        currentScreen = SCORE;
      } else {
        updateGame();
      }
      break;

    case SCORE:
      // After a short mandatory wait (so the same press that ended the game
      // doesn't quickly dismiss the score screen), a button press returns
      // to the ready screen.
      if (buttonPressed && millis() - scoreScreenStartTime > SCORE_SCREEN_WAIT){
        drawReadyScreen();
        currentScreen = READY;
      }
      break;
  }
}
