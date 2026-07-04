# Required Project: smartpumps_example_freertos

## Context
i need a template project for a custom Hardware to use it in futures projects. This template must be based in Arduino Framework and FreeRTOS using the ESP32 microcontroller. Next i describe the hardware and the template project requeriments.
## Custom Hardware Setup
### Buzzer
The buzzer is activated by a PWM output at 2.7KHz 50% duty cicle.
### Keyboard (KB)
The keyboard (KB) is a 4x4 matrix button, where a positive signal is injected (out) en each row  one at time sequencially, the colums are read at every row activation to detect if a key is pressed
### LCD
The LCD is a 168x64 monochrome display controlled via LCD. The u8g2 arduino library can control this.
### Digital Input and Output
The digital input and output are provided by two TC9555 IC's named TC9555_IN and TC9555_OUT respectively. These chips are controlled by the same I2C, where TC9555_IN I2C direction is 0x20 and TLC9555_OUT is 0x21. 
#### Pinout TC9555_OUT
- P16 is output 2
- P15 is output 3
- P14 is output 4 
- P00 is output 7
- P01 is output 8
- P02 is output 9
- P03 is output 10
- P04 is output 11
- P05 is output 12
- P06 is output 13
- P07 is output 14
- P10 is output 15
- P11 is output 16 
#### Pinout TC9555_IN
- P13 is input 1
- P14 is input 2
- P15 is input 3
- P12 is input 5
- P11 is input 6
- P10 is input 7
- P00 is input 8
- P01 is input 9
- P03 is input 11
- P04 is input 12
- P05 is input 13
- P06 is input 14
- P07 is input 15

## Microcontroller
The microcontroller used is the ESP32-WROOM-32U
### PINOUT ESP32
 - IO4 interrupt from tc9555 input ports
 - IO17 output to control LCD baklight,active high
 - IO5 CS for LCD SPI
 - IO18 CLK for LCD SPI
 - IO19 for LCD MISO
 - IO21 for I2C SDA
 - IO22 for I2C SCL
 - IO23 for LCD MOSI 
 - IO13 for Buzzer
 - IO14 for KB out4
 - IO27 for KB out3
 - IO26 for KB out2
 - IO25 for KB out1
 - IO35 for KB in4
 - IO34 for KB in3
 - SENSOR_VN for KB in2
 - SENSOR_VP for KB in1

## Software Requeriments
The template project must have four tasks: LCDTask, InputTask, KeyboardTask and UserTask. Each task is described below.
### InputTask
- this task must be executed every 10ms
- their priority is the highest
- read all inputs in each cycle
- if in 4 read cycles the input remain the same at least three cycles, the input is considered stable and actulize the a read to a only read buffer shared with the others task with the input values.
- do not use interrupts
## Keyboard task
- execute this task every 20ms
- the priority is high
- read the key pressed in the keyboard in each cycle
- if for 5 cycles the read is the same, actualize the read keys to a only read buffer shared with the others tasks
- each time that a key is pressed, activate the buzzer for 200ms without hangout the task
### LCDTask
- this task must be executed every 200ms
- their priority is the lowest priority
-  divide the display in four lines
- in the first line print the numbers of the active inputs separated by comas
- in the four line print the numbers of active output separated by comas 
- in the second line print the line of the active keys separated by comas
- in the third line print a message given by UserTask by means a shared buffer
## UserTask
- execute every 100ms
- get all shared buffers
- write in the lcd buffer "Hello: " and the hex values read for input buffer and keyboard buffer
- if a keyboard key is pressed turn on all outputs, if another key is pressed torn off all the outputs. the initial state of outputs is off.