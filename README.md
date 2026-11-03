# The KV-1 Tank

## Warning

The project is in Romanian and isn't completely translated. You have been warned.

## What you are building

A 3D-printed KV-1 tank, at 1:10 scale, that you drive from your phone. It runs on tracks, the turret rotates, and the gun elevates and depresses. When you press **FIRE**, the barrel kicks back and an LED lights at the muzzle. **Nothing comes out of the barrel, and nothing should be put in it.**

The model is by **Salin10**. You can find it on [Printables](https://www.printables.com/model/219806-tank-kv-1-rc-model-110), [Thingiverse](https://www.thingiverse.com/thing:5069466), and [Cults3D](https://cults3d.com/en/3d-model/game/heavy-tank-kv-1-rc-model-1-10).

## Read this first

This README was written **without access to the model page or its assembly guide**: the sites refused automated requests. No build videos for this model were found either, so none were watched.

| Certain, from the model description | Check yourself in the guide |
|---|---|
| 1:10 scale, the June 1941 version | every assembly step |
| motor drive, rotating turret, gun that elevates and depresses | which motors and which gearbox fit |
| 8× M4x50 screws and 2× M4x85 screws (the M4x85 screws pass through part 35) | the rest of the screws and bearings |
| includes a kit for the KV-1E and a KV-2 turret | how the barrel is mounted, and whether it can slide |

**The guide in the model archive has the last word.** When it says something different from this README about printed parts, screws, or assembly order, follow the guide. The electronics and the program here are separate from the model. We chose them; Salin10 did not.

## What you need

### The model

1. Make an account on Printables and download every file for the model, including the assembly guide. Among the files you should find `MODEL.rar` (the model, with the changes from 13 November 2021) and `UP01.rar` (the modified parts).
2. Read the whole guide **before** you print anything. Make your own list of parts, screws, and motors.
3. Also read the author's notes about the changes. There are variants for parts 19+20, 20+21, 23, and 24, the gun shield (T47) was redesigned, and the pulleys had their outside diameter changed. Print the latest versions.

### The electronics

- an **ESP32** board (ESP32 Dev Module) and a USB data cable
- two geared motors, one for each track. The yellow TT motors from toy cars are not suitable: the tank comes out about 67 cm long and 33 cm wide (calculated from the real tank's dimensions). Use what the guide recommends
- an **L298N** module. Remove the jumpers from ENA and ENB. It carries at most 2A per channel and drops about 2V from the battery voltage. Measure your motors' current with the track stalled for a moment. If it goes well past 2A, get a stronger driver with the same three inputs (PWM, IN1, IN2) that accepts 3.3V signals
- a **continuous-rotation** servo (360°, FS90R type) for the turret
- two ordinary servos, MG90S type, with metal gears: one to elevate the gun, one for the recoil
- a yellow or white LED and a 220Ω resistor
- a battery matched to the motor voltage. For 6V motors, a 2S 7.4V LiPo works
- **a 5V module (step-down / buck) of at least 3A**, for the board and the three servos. Adjust it and measure with a multimeter that it outputs 5V **before** you connect anything to it
- a capacitor of about 1000µF between 5V and GND, near the servos. Mind the polarity: the stripe on it goes to GND
- a switch and a fuse of about 5A on the battery positive

**If the guide uses an ordinary motor for the turret, not a servo:** the current program does not know how to drive it. Either find a place for the 360° servo, or the program has to be changed.

Charge a LiPo battery only with a LiPo charger, with someone present. If it has swollen or been damaged, do not use it.

## How to print

The settings here are a general starting point, not the author's. If the guide gives different ones, use those.

- 0.4 mm nozzle, 0.2 mm layer, 3 or 4 walls, 20–30% infill
- PLA is fine for the hull. Parts that take load, such as the gears and the motor mounts, are stronger in PETG
- check that the largest parts fit on your printer bed
- print a single track and one motor mount, then fit them. Better to learn now that the tolerance needs adjusting than after you have printed the whole tank

## How to wire it

The grounds of the battery, the L298N module, the 5V module, the board, and the servos must be **tied together**.

| L298N | ESP32 |
|---|---|
| ENA | 25 |
| IN1 | 26 |
| IN2 | 27 |
| ENB | 18 |
| IN3 | 19 |
| IN4 | 21 |
| GND | GND |

- left motor: OUT1 and OUT2
- right motor: OUT3 and OUT4

| Part | Signal on the ESP32 |
|---|---|
| turret servo | 22 |
| gun servo | 23 |
| recoil servo | 13 |
| muzzle LED | 4, through the 220Ω resistor. The other leg to GND |

All three servos take their positive (V+) from **the 5V module output, not from the board**. The board takes 5V from the same module, on the VIN pin. When the board is on USB, disconnect the wire from VIN.

The turret has no slip rings, so the wires inside it twist. **Do not turn it more than one revolution in the same direction.** The program cannot stop you. Leave the wires long enough and run them through the center of the turret.

## How to put the program on the board

Do this first on the bench, with the board connected only by USB. The motors can wait.

1. Install the Arduino IDE from [arduino.cc/en/software](https://www.arduino.cc/en/software).
2. **Tools > Board > Boards Manager**. Search for `esp32`, the one from Espressif, and press Install. The first time takes a while.
3. **Tools > Manage Libraries**. Search for `WebSockets`, the one by Markus Sattler, and press Install.
4. **File > Open** and open `KV1_Tank/KV1_Tank.ino`.
5. Near the top of the file is the line `// #define WOKWI_SIM`. Leave it like that, with `//` in front. That line is only for the simulator.
6. In the same file you will find `WIFI_PASSWORD`. **Put your own password**, at least 8 characters. Anyone who has the password can drive the tank.
7. Connect the board to the computer with a USB data cable. A cable that only charges a phone will not upload the program.
8. **Tools > Board > esp32 > ESP32 Dev Module**.
9. **Tools > Port**. Choose the port that appeared when you plugged in the cable.
10. Press the Upload arrow. If it stalls on "Connecting", hold BOOT, press EN or RST once, release BOOT, and upload again.
11. Open the Serial Monitor, the magnifying glass at the top right, at **115200** baud. You should see:

    `Waiting for a phone...`

## How to connect the phone

1. Turn the tank on.
2. On the phone, in Wi-Fi, choose the network **KV-1-Tank** and enter your password.
3. If the phone says the network has no internet, choose to stay connected.
4. Open the browser and type exactly **`http://192.168.4.1`**
5. Turn the phone sideways. When the screen says **Connected**, you can drive.

You do not install any app, and you do not need internet. It works on both iPhone and Android. Only one phone drives at a time.

## The controls

| On screen | What it does |
|---|---|
| left stick, up and down | forward and reverse |
| second stick, left and right | steering. The tank can spin in place |
| middle button | switches between slow and fast. It starts on slow |
| right dial, left and right | rotates the turret |
| right dial, up and down | raises and lowers the gun |
| **FOC** | fires. The button stays grey for about two seconds, until the gun "reloads" |

When you lift your finger, the stick returns to the center. The gun stays where you left it. If the phone goes out of range, locks, gets a call, or you leave the page, the tracks and the turret stop on their own within half a second.

## How to make the recoil

We have not confirmed that the model has a barrel that slides, so you may have to build the recoil mechanism yourself.

1. Look in the guide at how the barrel is mounted. It needs to be able to slide about 1 cm forward and back, freely and without side play.
2. If it is fixed, make a printed bushing for it to slide through.
3. Put the recoil servo in the turret and link its arm to the barrel with a stiff wire.
4. Run two thin wires through the barrel to the LED, with enough slack that they do not pull tight when the gun elevates.

The model is licensed **CC BY-NC-ND**. You may print and modify it for yourself, but you may not sell it and **you may not publish** modified parts. That is why the model files are not in this repo.

## First start-up

Do the steps in order. A servo wired the wrong way can break printed parts.

1. **Servos without arms.** Start the program and press FIRE (FOC), move the dial. Check that they all move and that the board does not restart.
2. **Mount the servos with the program running**, so they are already at center. Only then attach the arms and the linkages.
3. **The turret.** If it creeps on its own with your finger lifted, change `TURRET_STOP_US` a little at a time until it holds still. If it runs the wrong way, set `TURRET_REVERSED` to `true`.
4. **The gun.** The limits in the program start small: `GUN_LOW_US = 1400` and `GUN_HIGH_US = 1650`. Increase them by 25 at a time and stop before the gun hits anything. If it runs the wrong way, set `GUN_REVERSED` to `true`.
5. **The recoil.** On each FIRE, the barrel kicks back quickly and returns slowly. How far it travels is set by `RECOIL_BACK_US`. A value of 1000 means about 45° at the servo, which is close to 1 cm with a 12 mm arm. **Before you attach the wire, set it to 1300**, then increase it gradually toward 1000, so you do not break something if the barrel jams. If it pulls the wrong way, use 1700 and work up toward 2000. Stop when the barrel reaches the end of its travel without the servo buzzing.
6. **The tracks, with the tank on a stand**, so they do not touch the table. The left stick pushed up should turn both of them forward. If one runs backward, set `LEFT_REVERSED` or `RIGHT_REVERSED` to `true`.
7. **On the floor, at low speed.** The values in the program are only a starting point. You adjust them after you see how your tank behaves.

## If something does not work

- **The page does not open.** Turn off mobile data on the phone and try again. Check that `http://` is in front and that the phone is on KV-1-Tank.
- **The KV-1-Tank network does not appear.** Look in the Serial Monitor. If it says `Could not start Wi-Fi`, the password is under 8 characters.
- **The board restarts when you press FIRE (FOC) or when the motors start.** The battery voltage is sagging too far, or the 5V module is too weak. Charge the battery, check the thick wires and the capacitor.
- **The tank drives forward but will not spin in place.** The tracks need a lot of force to skid. Raise `MIN_PWM` and `SLOW_LIMIT`, or try a smooth floor. If it still will not, the motors are too weak.
- **The motors only buzz at low speed.** Raise `MIN_PWM`.
- **A track comes off the wheels.** Check its tension and the sprockets, following the guide.
- **The L298N gets very hot.** If you cannot keep a finger on it, the motors are drawing too much. Do not leave them stalled, let it cool, and consider a stronger driver.
- **A servo buzzes and heats up while sitting still.** It is pushing against something. Reduce its limit in the program.
- **The barrel does not return.** It is rubbing in the bushing. Sand the part and check that the wire is not bending.

## The simulator

You can try the program with no parts at all, in Wokwi. Three joysticks, two buttons, and a switch stand in for the phone. The LEDs show the motors and the firing, and the three servos show the turret, the gun, and the recoil.

1. In VS Code, install the **Wokwi** extension.
2. Remove the `//` from in front of the line `#define WOKWI_SIM`.
3. In the Arduino IDE: **Sketch > Export Compiled Binary**.
4. Open `KV1_Tank/diagram.json` in VS Code and start the simulation. The message `Waiting for a phone...` is normal. The switch stands in for the phone: to the left, as it starts, the phone is connected and you should see `Phone connected`. If you move it to the right, it is as if the phone lost the connection. Only then do the joysticks and buttons respond.
5. **Put the `//` back before you upload to the tank.** With the line active, the program does not start Wi-Fi.

The file `kv1.test.yaml` checks the tracks, the turret, the gun limits, and firing on its own. It is run with the [Wokwi CLI](https://docs.wokwi.com/wokwi-ci/getting-started), which needs a token from Wokwi:

`wokwi-cli . --scenario kv1.test.yaml --timeout 30000`
