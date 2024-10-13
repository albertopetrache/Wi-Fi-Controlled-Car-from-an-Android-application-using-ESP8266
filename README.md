# Wi-Fi Controlled Car from an Android application using ESP8266

In this project I made a remote controlled car using the ESP8266 module. I used a
chassis with four DC motors, that I controlled using an ATMEL 8‐bit microcontroller
(ATmega1284). The car receives commands through the Android application such as:
forward, brake, backward, horn, left turn signal, right turn signal, hazard lights,
headlights, turn left, turn right and changing the speed by increasing or decreasing the
duty cycle of the PWM signal. The control of the four DC motors is achieved using a
H‐bridge converter (DRV8835). I also designed the printed circuit board (PCB) and
developed the Android application in Android Studio.

The electrical schematic of the remote-controlled car was created using OrCAD Capture CIS 16.6, and later transferred to OrCAD PCB Editor for the design of the printed circuit board (PCB) required for the car's operation.
![image](https://github.com/user-attachments/assets/c6198bc8-a951-4a00-9405-cc71759a3d86)

The printed circuit board (PCB) has the following design details:

PCB dimensions: 90mm x 140mm
Copper foil thickness: 35 µm
Material: FR4, double-layer
Board thickness: 1.6 mm
All components are placed on the top side of the PCB (TOP layer)
The origin is located in the bottom-left corner of the PCB, with all elements having positive coordinates
Components used include both SMD for resistors and capacitors, and THT
A clearance of 50 mil was maintained from the edge of the board
Four mounting holes, each 4 mm in diameter, were designed to secure the PCB to the chassis
GND and VCC interconnection traces are 40 mil wide, while other traces are 20 mil wide.

![image](https://github.com/user-attachments/assets/0edf45a7-08bf-46fc-956f-143aeaba3691)

For the power circuit, I chose to use two voltage regulators: an LM1086IT-3.3 to provide a stable 3.3V supply and an LM7805 to provide a stable 5V supply. The LM1086IT-3.3 regulator is used to power the ESP-01 Wi-Fi module, which operates at 3.3V, while the LM7805 is used to power the other components. Both regulators are powered by six AA batteries, each supplying 1.5V, for a total of 9V. For the four DC motors, a separate power supply consisting of four AA batteries (1.5V each, providing 6V in total) was selected.

![image](https://github.com/user-attachments/assets/47839563-4c5d-4601-96b9-67567f62ec55)


![image](https://github.com/user-attachments/assets/235e1265-170d-46ce-a999-303ad1a066df)
