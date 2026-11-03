/*
  KV-1 Tank - ESP32 + phone controller + L298N motor driver + three servos

  One motor drives each track. A continuous-rotation servo turns the turret, a
  servo raises and lowers the gun, and a third servo pulls the barrel back when
  the cannon fires, while an LED flashes at the muzzle. Nothing leaves the barrel.

  The ESP32 makes its own Wi-Fi network and serves the controller page. The phone
  needs no app and no internet: join the network and open the address in a browser.

  Arduino IDE setup
    1. Boards Manager: install "esp32" by Espressif
    2. Library Manager: install "WebSockets" by Markus Sattler
    3. Tools > Board > esp32 > "ESP32 Dev Module"

  Connecting the phone
    Wi-Fi network  KV-1-Tank
    Password       the WIFI_PASSWORD below (change it before the first upload)
    Address        http://192.168.4.1

  Controls
    Left stick up/down     forward / reverse
    Middle stick left/right steer
    Right pad left/right   turn the turret
    Right pad up/down      raise / lower the gun
    FOC button             fire the cannon
    Middle button          toggle slow / fast mode (starts in slow)
  The tank and turret stop by themselves if the phone goes quiet, leaves the page
  or disconnects. The gun stays where it is. Only one phone can drive at a time.

  Wiring
    L298N          ESP32
    ENA        ->  25  (remove the ENA jumper)
    IN1        ->  26
    IN2        ->  27
    ENB        ->  18  (remove the ENB jumper)
    IN3        ->  19
    IN4        ->  21
    GND        ->  GND (battery, L298N, 5V module and ESP32 share ground)
    Left track motor  -> L298N OUT1/OUT2
    Right track motor -> L298N OUT3/OUT4

    Part                 ESP32 pin
    Turret servo signal  22
    Gun servo signal     23
    Recoil servo signal  13
    Muzzle LED           4, through a 220 ohm resistor, to GND
    Servo V+ goes to the 5V module output, never to the ESP32.
    Power the ESP32 from USB or the 5V module to VIN, never from the motor battery directly.

  Testing in Wokwi
    Uncomment WOKWI_SIM below. Joysticks, buttons and a switch replace the phone
    (see WokwiSim.h and diagram.json). kv1.test.yaml is a scripted test of the
    same setup. Comment WOKWI_SIM out again before uploading to the real tank.
*/

// #define WOKWI_SIM

#ifdef WOKWI_SIM
#include "WokwiSim.h"
#else
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "PhonePage.h"
#endif

// ---------- Phone link ----------
const char* WIFI_NAME     = "KV-1-Tank";
const char* WIFI_PASSWORD = "schimba-ma";  // at least 8 characters
const unsigned long STOP_AFTER_MS    = 500;   // no command for this long: stop the tank
const unsigned long RELEASE_AFTER_MS = 3000;  // ...and for this long: let another phone drive

// ---------- Pins ----------
const int LEFT_EN    = 25;
const int LEFT_IN1   = 26;
const int LEFT_IN2   = 27;
const int RIGHT_EN   = 18;
const int RIGHT_IN1  = 19;
const int RIGHT_IN2  = 21;
const int TURRET_PIN = 22;
const int GUN_PIN    = 23;
const int RECOIL_PIN = 13;
const int MUZZLE_PIN = 4;

// ---------- Tracks ----------
const bool LEFT_REVERSED  = false;  // flip if that track drives backwards
const bool RIGHT_REVERSED = false;
const int  DEADZONE   = 50;    // stick wobble ignored around center (0-511)
const int  MIN_PWM    = 80;    // tracks need a strong start, raise it if the motors only hum
const int  SLOW_LIMIT = 170;   // top PWM in slow mode (0-255)
const int  FAST_LIMIT = 255;   // top PWM in fast mode
const int  PWM_FREQ   = 5000;  // Hz
const int  PWM_BITS   = 8;     // 0-255 duty

// ---------- Turret (continuous-rotation servo) ----------
const bool TURRET_REVERSED = false;  // flip if the turret turns the wrong way
const int  TURRET_STOP_US  = 1500;   // adjust until the turret stands still
const int  TURRET_RANGE_US = 120;    // top speed each way, keep it slow

// ---------- Gun elevation (normal servo) ----------
const bool GUN_REVERSED  = false;  // flip if the gun moves the wrong way
const int  GUN_HOME_US   = 1500;   // where the gun points at power-on
const int  GUN_LOW_US    = 1400;   // lowest allowed, small first so the linkage can't bind
const int  GUN_HIGH_US   = 1650;   // highest allowed
const float GUN_RATE_US_PER_S = 200;  // speed at full stick

// ---------- Cannon (normal servo + LED) ----------
const int RECOIL_HOME_US = 1500;  // barrel forward
const int RECOIL_BACK_US = 1000;  // barrel pulled back, about 45 degrees of servo travel
const unsigned long FLASH_MS       = 60;    // how long the LED flashes
const unsigned long RECOIL_HOLD_MS = 120;   // the servo needs this long to pull back
const unsigned long RETURN_MS      = 600;   // the barrel slides forward slowly
const unsigned long RELOAD_MS      = 1500;  // wait before the next shot

// ---------- Servo signal ----------
const int  SERVO_FREQ = 50;  // Hz, a 20000 us period
const int  SERVO_BITS = 16;
const long SERVO_PERIOD_US = 20000;

const int NO_DRIVER = -1;
int driver = NO_DRIVER;
unsigned long lastCommandMs = 0;
bool fastMode = false;

int   gunCommand = 0;  // -511 (down) .. 511 (up)
float gunUs = GUN_HOME_US;
unsigned long lastGunMs = 0;

enum CannonState { CANNON_READY, CANNON_RECOIL, CANNON_RETURN, CANNON_RELOAD };
CannonState cannonState = CANNON_READY;
unsigned long cannonStateMs = 0;
unsigned long flashStartMs = 0;
bool flashOn = false;

#ifndef WOKWI_SIM
WebServer pageServer(80);
WebSocketsServer phoneSocket(81);
#endif

// Arduino-ESP32 3.x attaches PWM by pin; 2.x needs a channel per pin
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void setupPwm(int pin, int channel, int freq, int bits) {
  ledcAttach(pin, freq, bits);
}
void writePwm(int pin, int channel, int duty) {
  ledcWrite(pin, duty);
}
#else
void setupPwm(int pin, int channel, int freq, int bits) {
  ledcSetup(channel, freq, bits);
  ledcAttachPin(pin, channel);
}
void writePwm(int pin, int channel, int duty) {
  ledcWrite(channel, duty);
}
#endif

// 2.x channels share a timer in pairs (0-1, 2-3, 4-5), so motors and servos stay
// in separate pairs: they need different frequencies
const int LEFT_PWM_CHANNEL   = 0;
const int RIGHT_PWM_CHANNEL  = 1;
const int TURRET_PWM_CHANNEL = 2;
const int GUN_PWM_CHANNEL    = 3;
const int RECOIL_PWM_CHANNEL = 4;

// speed: -255 (full reverse) .. 255 (full forward)
void setMotor(int en, int channel, int in1, int in2, bool reversed, int speed) {
  if (reversed) speed = -speed;
  speed = constrain(speed, -255, 255);

  digitalWrite(in1, speed > 0 ? HIGH : LOW);
  digitalWrite(in2, speed < 0 ? HIGH : LOW);
  writePwm(en, channel, abs(speed));
}

void drive(int left, int right) {
#ifdef WOKWI_SIM
  simPrintMotors(left, right);
#endif
  setMotor(LEFT_EN, LEFT_PWM_CHANNEL, LEFT_IN1, LEFT_IN2, LEFT_REVERSED, left);
  setMotor(RIGHT_EN, RIGHT_PWM_CHANNEL, RIGHT_IN1, RIGHT_IN2, RIGHT_REVERSED, right);
}

void writeServo(int pin, int channel, long pulseUs) {
  writePwm(pin, channel, pulseUs * ((1L << SERVO_BITS) - 1) / SERVO_PERIOD_US);
}

// turn: -511 (full left) .. 511 (full right)
void setTurret(int turn) {
#ifdef WOKWI_SIM
  simPrintTurret(turn);
#endif
  if (TURRET_REVERSED) turn = -turn;
  writeServo(TURRET_PIN, TURRET_PWM_CHANNEL, TURRET_STOP_US + (long)turn * TURRET_RANGE_US / 511);
}

void writeGun() {
  long pulseUs = lround(gunUs);
  if (GUN_REVERSED) pulseUs = 2 * GUN_HOME_US - pulseUs;
  writeServo(GUN_PIN, GUN_PWM_CHANNEL, pulseUs);
}

// Stops everything that moves on its own. The gun holds its angle.
void stopTank() {
  drive(0, 0);
  setTurret(0);
  gunCommand = 0;
}

// The stick sets how fast the gun moves, so it stays put when the stick springs back
void updateGun() {
  unsigned long now = millis();
  float seconds = (now - lastGunMs) / 1000.0;
  lastGunMs = now;
  if (gunCommand == 0) return;

  float before = gunUs;
  gunUs += gunCommand * GUN_RATE_US_PER_S * seconds / 511;
  gunUs = constrain(gunUs, (float)GUN_LOW_US, (float)GUN_HIGH_US);
  if (gunUs == before) return;

  writeGun();
#ifdef WOKWI_SIM
  simPrintGunLimit(gunUs, GUN_LOW_US, GUN_HIGH_US);
#endif
}

// Input -511..511, output -511..511 with the dead zone removed and rescaled
int applyDeadzone(int v) {
  v = constrain(v, -511, 511);
  if (abs(v) <= DEADZONE) return 0;
  return v > 0 ? map(v, DEADZONE, 511, 0, 511) : map(v, -511, -DEADZONE, -511, 0);
}

// Input -511..511, output signed PWM between MIN_PWM and limit
int toPwm(int v, int limit) {
  if (v == 0) return 0;
  int pwm = map(abs(v), 1, 511, MIN_PWM, limit);
  return v > 0 ? pwm : -pwm;
}

// The page shows the mode on its middle button
void showMode() {
#ifndef WOKWI_SIM
  if (driver != NO_DRIVER) phoneSocket.sendTXT(driver, fastMode ? "fast" : "slow");
#endif
}

// The page greys out its FOC button until the cannon is ready again
void showCannon() {
#ifndef WOKWI_SIM
  if (driver != NO_DRIVER) phoneSocket.sendTXT(driver, cannonState == CANNON_READY ? "ready" : "fire");
#endif
}

void toggleMode() {
  fastMode = !fastMode;
  Serial.println(fastMode ? "Fast mode" : "Slow mode");
  showMode();
}

void setCannonState(CannonState state) {
  cannonState = state;
  cannonStateMs = millis();
}

void fireCannon() {
  if (cannonState != CANNON_READY) return;  // still reloading

  writeServo(RECOIL_PIN, RECOIL_PWM_CHANNEL, RECOIL_BACK_US);
  digitalWrite(MUZZLE_PIN, HIGH);
  flashOn = true;
  flashStartMs = millis();
  setCannonState(CANNON_RECOIL);
  Serial.println("Fire!");
  showCannon();
}

// Never waits, so the tracks keep responding during a shot. A shot always
// finishes, even if the phone disconnects, so the barrel ends up forward.
void updateCannon() {
  unsigned long now = millis();
  unsigned long elapsed = now - cannonStateMs;

  if (flashOn && now - flashStartMs >= FLASH_MS) {
    digitalWrite(MUZZLE_PIN, LOW);
    flashOn = false;
  }

  switch (cannonState) {
    case CANNON_READY:
      break;
    case CANNON_RECOIL:
      if (elapsed >= RECOIL_HOLD_MS) setCannonState(CANNON_RETURN);
      break;
    case CANNON_RETURN:
      if (elapsed >= RETURN_MS) {
        writeServo(RECOIL_PIN, RECOIL_PWM_CHANNEL, RECOIL_HOME_US);
        setCannonState(CANNON_RELOAD);
        Serial.println("Barrel forward");
      } else {
        writeServo(RECOIL_PIN, RECOIL_PWM_CHANNEL,
                   RECOIL_BACK_US + (long)(RECOIL_HOME_US - RECOIL_BACK_US) * (long)elapsed / (long)RETURN_MS);
      }
      break;
    case CANNON_RELOAD:
      if (elapsed >= RELOAD_MS) {
        setCannonState(CANNON_READY);
        Serial.println("Cannon ready");
        showCannon();
      }
      break;
  }
}

void handleCommand(int rawThrottle, int rawSteer, int rawTurret, int rawGun) {
  lastCommandMs = millis();

  int throttle = applyDeadzone(rawThrottle);
  int steer    = applyDeadzone(rawSteer);

  // Arcade mix, then scale down so neither side exceeds full stick
  int left  = throttle + steer;
  int right = throttle - steer;
  int biggest = max(abs(left), abs(right));
  if (biggest > 511) {
    left  = left * 511 / biggest;
    right = right * 511 / biggest;
  }

  int limit = fastMode ? FAST_LIMIT : SLOW_LIMIT;
  drive(toPwm(left, limit), toPwm(right, limit));

  setTurret(applyDeadzone(rawTurret));
  gunCommand = applyDeadzone(rawGun);
}

void onDriverConnected(int id) {
  driver = id;
  lastCommandMs = millis();
  Serial.println("Phone connected");
  showMode();
  showCannon();
}

void onDriverLost() {
  driver = NO_DRIVER;
  stopTank();
  Serial.println("Phone disconnected, stopping.");
}

#ifdef WOKWI_SIM
void startPhoneLink() {
  simSetup();
  Serial.println("Simulator: no phone needed. Switch left = connected, right = phone lost.");
}

// The slide switch plays the part of the phone connecting and disconnecting
void updatePhoneLink() {
  static bool startWasPressed = false;
  static bool fireWasPressed = false;

  if (!simLinked()) {
    if (driver != NO_DRIVER) onDriverLost();
    return;
  }
  if (driver == NO_DRIVER) {
    onDriverConnected(0);
    startWasPressed = false;
    fireWasPressed = false;
  }

  bool startPressed = simStartPressed();
  if (startPressed && !startWasPressed) toggleMode();
  startWasPressed = startPressed;

  bool firePressed = simFirePressed();
  if (firePressed && !fireWasPressed) fireCannon();
  fireWasPressed = firePressed;

  handleCommand(simThrottle(), simSteer(), simTurret(), simGun());
}
#else
// Messages from the page: "throttle,steer,turret,gun" about 20 times a second,
// "mode" or "fire"
void onDriverMessage(const char* text) {
  if (strcmp(text, "mode") == 0) {
    toggleMode();
    return;
  }
  if (strcmp(text, "fire") == 0) {
    fireCannon();
    return;
  }
  int throttle, steer, turret, gun;
  if (sscanf(text, "%d,%d,%d,%d", &throttle, &steer, &turret, &gun) == 4) {
    handleCommand(throttle, steer, turret, gun);
  }
}

void onSocketEvent(uint8_t id, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    if (driver != NO_DRIVER) {
      phoneSocket.sendTXT(id, "busy");  // someone is already driving
      phoneSocket.disconnect(id);
      return;
    }
    onDriverConnected(id);
  } else if (type == WStype_DISCONNECTED) {
    if (id == driver) onDriverLost();
  } else if (type == WStype_TEXT) {
    if (id == driver) onDriverMessage((const char*)payload);
  }
}

void startPhoneLink() {
  if (!WiFi.softAP(WIFI_NAME, WIFI_PASSWORD)) {
    Serial.println("Could not start Wi-Fi. WIFI_PASSWORD needs at least 8 characters.");
    return;
  }
  WiFi.setSleep(false);  // keeps the steering from lagging

  pageServer.on("/", []() {
    pageServer.send_P(200, "text/html; charset=utf-8", PHONE_PAGE);
  });
  pageServer.onNotFound([]() {
    pageServer.sendHeader("Location", "/");
    pageServer.send(302);
  });
  pageServer.begin();

  phoneSocket.begin();
  phoneSocket.onEvent(onSocketEvent);

  Serial.printf("Wi-Fi: %s, then open http://%s\n", WIFI_NAME, WiFi.softAPIP().toString().c_str());
}

void updatePhoneLink() {
  pageServer.handleClient();
  phoneSocket.loop();

  if (driver != NO_DRIVER && millis() - lastCommandMs > RELEASE_AFTER_MS) {
    phoneSocket.disconnect(driver);
    if (driver != NO_DRIVER) onDriverLost();
  }
}
#endif

void setup() {
  Serial.begin(115200);

  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);
  pinMode(MUZZLE_PIN, OUTPUT);
  digitalWrite(MUZZLE_PIN, LOW);

  setupPwm(LEFT_EN, LEFT_PWM_CHANNEL, PWM_FREQ, PWM_BITS);
  setupPwm(RIGHT_EN, RIGHT_PWM_CHANNEL, PWM_FREQ, PWM_BITS);
  setupPwm(TURRET_PIN, TURRET_PWM_CHANNEL, SERVO_FREQ, SERVO_BITS);
  setupPwm(GUN_PIN, GUN_PWM_CHANNEL, SERVO_FREQ, SERVO_BITS);
  setupPwm(RECOIL_PIN, RECOIL_PWM_CHANNEL, SERVO_FREQ, SERVO_BITS);

  stopTank();
  writeGun();
  writeServo(RECOIL_PIN, RECOIL_PWM_CHANNEL, RECOIL_HOME_US);
  lastGunMs = millis();

  startPhoneLink();
  Serial.println("Waiting for a phone...");
}

void loop() {
  updatePhoneLink();

  // The phone repeats its command all the time, so silence means it is gone
  if (driver == NO_DRIVER || millis() - lastCommandMs > STOP_AFTER_MS) {
    stopTank();
  }

  updateGun();
  updateCannon();

  delay(10);
}
