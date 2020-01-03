# Fireworks Wireless Firing System

I built this device to wirelessly ignite fireworks for New Year's Eve 2020. There are two different modules for this project: a detonator, and a terminal box. In a few words, the way these modules work is that a signal is transmitted from the detonator to the terminal box to turn on one or several of its outputs. Since an electronic fuse is connected to the output, it will ignite the firework that it is linked to when current runs through it. Both of these modules are powered by 12V 3S LiPo batteries.

![Finished Prod](images/IMG_4083.jpeg)

## Detonator

The detonator is the main control that the user operates. It features the following components:
- Arduino Nano
- 20x4 LCD screen
- Navigation Button
- Select Button
- 4 Trigger Buttons
- Safety Switch
- NRF24L01 (to communicate with the terminal box)

The code for the Detonator can be found in this repository at "/CODE/FINAL_DET/FINAL_DET.ino".

In the detonator logic, there are two modes that can be accessed: Fire Mode, and Test Connection.

### Fire Mode
	
This mode automatically runs through the 


## Terminal Box

The terminal box is the "slave" that receives commands and requests from the detonator. It features the following components:
- Arduino Nano
- Armed Switch
- LEDs (to show when the system is armed and if fuses are correctly connected to the ouputs)
- LED Warning Strip
- NRF24L01 (to communicate with the detonator)

The code for the Terminal Box can be found in this repository at "/CODE/FINAL_BOX/FINAL_BOX.ino".


## Circuit Schematic

Please note that the wiring of the fuses at the top of the circuit has been inspired by the following post: https://www.electro-tech-online.com/threads/electronic-firing-system-for-fireworks.21046/post-134180

![Finished Prod](images/Schematic.pdf)