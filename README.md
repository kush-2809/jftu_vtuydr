# Smart Tank — Phase 1 starter

This starter verifies the ESP32-S3, ST7789 display, rotary encoder, TTP223 touch input,
backlight PWM and a logic-only pump output.

## Safety for this phase
- Do NOT connect the SSR input to GPIO2 yet.
- Do NOT connect the AC pump or mains wiring.
- GPIO2 is only toggled as a logic-level test output. The sketch initializes it LOW.
- The SSR-40DA and spike suppressor are not used in this phase.

## Hardware assumptions
- ESP32-S3 N16R8
- ST7789 TFT, 240x320, TFT_eSPI
- TFT pins: MOSI=11, SCLK=12, CS=10, DC=9, RST=8, BL=7
- Encoder: A=4, B=5, SW=6
- TTP223: OUT=1
- Buzzer: GPIO21 (not actively driven by this test)
- Simulated pump output: GPIO2

The sketch starts in portrait (`setRotation(0)`) for a 240x320 logical canvas.
Use `setRotation(1)`, `2`, or `3` if your panel orientation differs.

## Build
1. Install Visual Studio Code and PlatformIO.
2. Open this folder as a PlatformIO project.
3. Connect the ESP32-S3 by USB.
4. Select `esp32-s3-main`, then Build and Upload.
5. Open Serial Monitor at 115200 baud.

## TFT_eSPI configuration
The display setup is supplied using PlatformIO build flags in `platformio.ini`.
If the display is blank, verify the panel's actual pinout, reset wiring, backlight circuit,
color order and inversion setting. Do not randomly connect pins or apply 5V to the panel.

## Expected test
- TFT shows SMART TANK / PHASE 1.
- Encoder rotation changes the step counter.
- Touch input changes its status.
- Encoder click toggles GPIO2 HIGH/LOW and reports it in Serial Monitor.
- GPIO2 must remain disconnected from the SSR during this test.

## Important next steps
Before integrating the ultrasonic sensor, confirm its UART TX voltage is safe for ESP32 GPIO26.
Before using LoRa, validate the classic ESP32 strapping-pin behavior for GPIO2 and GPIO5.
The complete system will later use non-blocking tasks/state machines, watchdogs, packet validation,
and safety interlocks. This Phase 1 sketch is only a peripheral verification program.
