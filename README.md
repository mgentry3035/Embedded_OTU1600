# Embedded_OTU1600
Arduino and Nextion Files for Stepper Motor Controlled oscillating spray tester.
Nextion HMI Interface for Stepper Motor Oscillation Control
==========================================================

Overview
--------
This Nextion HMI project provides the touchscreen interface for controlling the Arduino-based 
oscillation system. It allows the operator to:

- Input test parameters (RPM, Angle, Duration).
- Start oscillation tests.
- Jog the stepper motor forward or reverse.
- Return to Home position.
- Toggle water relay ON/OFF.
- Perform an Emergency STOP.

The HMI communicates with the Arduino Mega 2560 via Serial3 at 9600 baud. All commands are 
terminated with the character 'q'.

HMI Elements
------------
The following elements should be created in the Nextion Editor:

1. **Numeric input fields / sliders:**
   - Angle input
   - Minutes input
   - Seconds input

2. **Buttons:**
   - START (sends: `RPM: <val> ANG: <val> MIN: <val> SEC: <val>q`)
   - JOG FORWARD (`JOGFq`,`JOG0q`)
   - JOG REVERSE (`JOGRq`,`JOG0q`)
   - RETURN HOME (`retq`)
   - WATER ON (`relay1q`)
   - WATER OFF (`relay0q`)
   - STOP (`STOPq`)

Serial Protocol (HMI → Arduino)
-------------------------------
- Start Test: `RPM: <value> ANG: <value> MIN: <value> SEC: <value>q`
- Jog Forward: `JOGFq`
- Jog Reverse: `JOGRq`
- Jog Stop: `JOG0q`
- Return Home: `retq`
- Relay ON: `relay1q`
- Relay OFF: `relay0q`
- Emergency Stop: `STOPq`

Upload Instructions
-------------------

1. In Nextion Editor, compile the project → generates a `.tft` file.
2. Copy the `.tft` file onto a **FAT32-formatted microSD card**.
3. Insert the SD card into the Nextion display while it is powered off.
4. Power ON the display. The upload will begin automatically.
5. When the screen shows "Update Success", power OFF, remove SD card, and reboot.

Testing
-------
1. Power both Arduino Mega and Nextion HMI.
2. After upload, use the HMI interface to send commands.
3. Observe Arduino debug output via Serial Monitor (9600 baud).
4. Verify that motor motion, homing, jogging, and relay toggling all respond as expected.

Notes & Safety
--------------
- Ensure Nextion baud rate matches Arduino (`9600`).
- Keep all command strings terminated with 'q' to be parsed correctly.
- Do not power-cycle Nextion during upload.
- Test serial commands in Arduino IDE Serial Monitor before relying on HMI.
