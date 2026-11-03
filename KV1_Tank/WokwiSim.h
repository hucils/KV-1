/*
  Wokwi simulator stand-in for the phone.
  Only used when WOKWI_SIM is defined at the top of KV1_Tank.ino.

  diagram.json wires up:
    Left joystick   (up/down)     -> GPIO 34   throttle
    Middle joystick (left/right)  -> GPIO 35   steering
    Right joystick  (left/right)  -> GPIO 36   turret
    Right joystick  (up/down)     -> GPIO 39   gun up / down
    START button                  -> GPIO 32   slow / fast toggle
    FOC button                    -> GPIO 14   fire
    Slide switch                  -> GPIO 33   left (start position) = phone connected, right = disconnected
    4 LEDs show the L298N outputs: green = forward, red = reverse, brightness = speed
    3 servos show the turret (GPIO 22), gun (GPIO 23) and recoil (GPIO 13)
    The yellow LED on GPIO 4 is the muzzle flash

  The switch pulls GPIO 33 to GND for "connected", so a sim build flashed to the
  real tank by mistake never sees a phone and never moves.
*/
#pragma once

const int SIM_LEFT_Y  = 34;
const int SIM_MID_X   = 35;
const int SIM_RIGHT_X = 36;
const int SIM_RIGHT_Y = 39;
const int SIM_START   = 32;
const int SIM_FIRE    = 14;
const int SIM_LINK    = 33;

void simSetup() {
  pinMode(SIM_START, INPUT_PULLUP);
  pinMode(SIM_FIRE, INPUT_PULLUP);
  pinMode(SIM_LINK, INPUT_PULLUP);
}

bool simLinked() { return digitalRead(SIM_LINK) == LOW; }
bool simStartPressed() { return digitalRead(SIM_START) == LOW; }
bool simFirePressed() { return digitalRead(SIM_FIRE) == LOW; }

// Wokwi joysticks read 4095 at top/left; the phone sends +511 for up and right
int simThrottle() { return (analogRead(SIM_LEFT_Y) - 2048) / 4; }
int simSteer() { return (2048 - analogRead(SIM_MID_X)) / 4; }
int simTurret() { return (2048 - analogRead(SIM_RIGHT_X)) / 4; }
int simGun() { return (analogRead(SIM_RIGHT_Y) - 2048) / 4; }

// These print to the serial monitor only when something changes
void simPrintMotors(int left, int right) {
  static int lastLeft = 0;
  static int lastRight = 0;
  if (left == lastLeft && right == lastRight) return;
  lastLeft = left;
  lastRight = right;
  Serial.printf("Motors  L=%d  R=%d\n", left, right);
}

void simPrintTurret(int turn) {
  static int lastTurn = 0;
  if (turn == lastTurn) return;
  lastTurn = turn;
  Serial.printf("Turret=%d\n", turn);
}

void simPrintGunLimit(float gunUs, int lowUs, int highUs) {
  if (gunUs <= lowUs) Serial.println("Gun at lowest");
  if (gunUs >= highUs) Serial.println("Gun at highest");
}
