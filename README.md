## Flappy Bird *cardboard edition*

[image of final thing]

We turned digital flappy bird, into a real and tangible arcade game with some fun twists. Smash a comically sized button, show off your personal records, and become the ultimate flappy bird champion! 

## Kool Features
- Infinite conveyor belt level system thingiemabob
- Bouncy flappy bird, using a linear pulley and rubber band (thanks lopa)
- A really cool, big bad button to flap the bir
- Super duper nice cardboard, arcady boxy box
- Insane amount of jank by yours truly

## CAD Design

This project is mostly just cardbox boxes, a ton of loose electronics and of course, 2 minimal CAD models for the conveyor and pulley:

<img width="1920" height="960" alt="image" src="https://github.com/user-attachments/assets/c82a9620-8eb3-4ab1-9939-98566827b3a9" />
<img width="1920" height="960" alt="image" src="https://github.com/user-attachments/assets/92406064-d8ba-4d8a-b3ca-95f4745cf2f3" />

## Electronics

This entire project is pretty much built onto of 2 stepper motors, and one DC motor. The stepper motors drive the conveyor belt, and then the DC motor is used to wind the pulley which makes the flappy bird go up and down. There's also an LCD to keep track of the score, and then a cat printer that runs over bluetooth to print out said score on sticker paper you can show off to your friends. All running on the Pi Pico W :D

<img width="1827" height="1788" alt="image" src="https://github.com/user-attachments/assets/ef1b572e-0ebb-452f-86d7-9311e78d3e40" />

## Firmware

The entire firmware is written entirely by @mpkendall and is C++ inside of arduino IDE, on the Pi Pico W. The cat printer runs over bluetooth, and can be used over web UI, nodeJS API or some other methods!

## Bill of Materials (BOM)

- 2x SM-5VDC-DRV stepper motor/driver
- 1x Pi Pico W
- 1x L293D DC motor driver
- 1x N20 DC motor
- 1x 9V battery for DC motor driver
- 1x MXW01 thermal cat-themed printer
- 1x 19x02 LCD with I2C breakout
- Assorted wires/cardboard/breadboards/misc
