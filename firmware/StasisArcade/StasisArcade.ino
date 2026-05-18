#include <AccelStepper.h>
#include <pico/time.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BIG_ASS_BUTTON 0
#define LOSE_DETECTION_PIN 15

#define DC_IN1 10
#define DC_IN2 11
#define DC_IN3 12
#define DC_IN4 13
#define DC_EN 1

#define NEUTRAL_CONSTANT 100
#define POINT_TIME 3015

#define DEBOUNCE_DELAY_MS 50

#define JUMP_TIME 100
#define MAX_QUEUED_JUMPS 20

AccelStepper stepper1(AccelStepper::HALF4WIRE, 2, 4, 3, 5);
AccelStepper stepper2(AccelStepper::HALF4WIRE, 6, 8, 7, 9);

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool buttonState = HIGH;
bool lastButtonReading = HIGH;

bool inGame = false;

unsigned long lastDebounceTime = 0;

int buttonPresses = 0;

unsigned long startTime = 0;
unsigned long lastPointTime = 0;

int score = 0;

// queued jump system
int queuedJumps = 0;
bool jumpActive = false;
unsigned long jumpStartTime = 0;

enum direction {
  forward = 0,
  backward = 1,
  stop = 2,
};

static struct repeating_timer timer;

void setup() {
  Serial.begin(115200);

  pinMode(BIG_ASS_BUTTON, INPUT_PULLUP);

  pinMode(LOSE_DETECTION_PIN, INPUT);

  pinMode(DC_IN1, OUTPUT);
  pinMode(DC_IN2, OUTPUT);
  pinMode(DC_IN3, OUTPUT);
  pinMode(DC_IN4, OUTPUT);
  pinMode(DC_EN, OUTPUT);

  Wire.setSDA(16);
  Wire.setSCL(17);
  Wire.begin();

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("score: ");

  stepper1.setMaxSpeed(1200);
  stepper1.setSpeed(1200);

  stepper2.setMaxSpeed(1200);
  stepper2.setSpeed(1200);
}

void loop() {
  bool reading = digitalRead(BIG_ASS_BUTTON);

  // debounce
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        Serial.println("big ass button PRESSED");

        if (!buttonPresses) {
          add_repeating_timer_us(-1000, timerCallback, NULL, &timer);

          inGame = true;

          startTime = millis();
          lastPointTime = millis();
        }

        // stack jumps
        if (queuedJumps < MAX_QUEUED_JUMPS) {
          queuedJumps++;
        }

        buttonPresses++;
      }
    }
  }

  lastButtonReading = reading;

  // handle queued jumps
  handleJumpQueue();

  if (inGame) {
    // active-high lose detection
    if (digitalRead(LOSE_DETECTION_PIN) == HIGH) {
      lose();
      return;
    }

    if (millis() - lastPointTime >= POINT_TIME) {
      lastPointTime += POINT_TIME;

      Serial.println("scored another point");

      score++;

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("score: ");
      lcd.print(score);

      lcd.setCursor(0, 1);
      lcd.print("queued: ");
      lcd.print(queuedJumps);
    }
  }
}

bool timerCallback(struct repeating_timer *t) {
  stepper1.runSpeed();
  stepper2.runSpeed();

  return true;
}

void driveMotors(direction motorDirection, int speed = 255) {
  analogWrite(DC_EN, speed);

  switch (motorDirection) {
    case forward:
      digitalWrite(DC_IN1, HIGH);
      digitalWrite(DC_IN2, LOW);
      break;

    case backward:
      digitalWrite(DC_IN1, LOW);
      digitalWrite(DC_IN2, HIGH);
      break;

    case stop:
      digitalWrite(DC_IN1, LOW);
      digitalWrite(DC_IN2, LOW);
      break;
  }
}

void handleJumpQueue() {
  // start jump
  if (!jumpActive && queuedJumps > 0) {
    queuedJumps--;

    jumpActive = true;
    jumpStartTime = millis();

    driveMotors(backward, 175);
  }

  // end jump
  if (jumpActive &&
      millis() - jumpStartTime >= JUMP_TIME) {

    driveMotors(stop);

    jumpActive = false;
  }
}

void lose() {
  Serial.println("YOU SUCK");

  cancel_repeating_timer(&timer);

  inGame = false;

  driveMotors(stop);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("you lost dumbass");

  lcd.setCursor(0, 1);
  lcd.print("score: ");
  lcd.print(score);

  delay(5000);

  buttonPresses = 0;
  score = 0;

  queuedJumps = 0;
  jumpActive = false;
}