# This is work in progress

The cad files will become available, once it is in a workable state.

**TODO**

 - [ ] electronics case
 - [x] end stops for threaded ends (legs)
 - [ ] document the build
 - [ ] document the electronics

# Electronics setup

 These are all off the shelf components, maybe later a PCB will become available to get rid of some wires.

## Components used

  - ESP32-c3 mini
  - rotary encoder with button
  - INA226 current and voltage sensor, the current measure resistor has to be 0.02 Ohm (SMD 2512 3W)
  - IBT-4 motor driver
  - 128x32 OLED display
  - 12V mmotor 36gp-3530, 222 RPM, 8mm shaft
  - for development, a switch to decouple from the main power.

## Wiring

### I2I

  - gpio6 SDA
  - gpio7 SCL

I2C device found at address 0x3C  !
I2C device found at address 0x40  !

### IBT-4

Need two motor pins (pwm/ledc)
  - gpio0 in1
  - gpio1 in2

### rotary encoder

Need 3 switch pins (clk, dt sw)
  - gpio2 clk 
  - gpio3 dt
  - gpio4 sw

# Program

**WONT DO**

There wont be any wifi, in AP mode the esp32-c3 becomes quite hot and it is on the bottom of the nice to have list.

**TODO**

 - [x] IGB4 driver
 - [x] i2c display SSD1306
 - [x] rotary switch
 - [x] make rotary and display work with the motor, needed for testing current and voltage late
 - [x] BUG: pressing the button does not stop the motor
 - [x] voltage/current measurement
 - [x] make it all work together
 - [x] stop when battery voltage is too low
 

