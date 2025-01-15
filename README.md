# Wi-Fi Controlled Car from an Android application using ESP8266

In this project I made a remote controlled car using the ESP8266 module. I used a
chassis with four DC motors, that I controlled using an ATMEL 8‐bit microcontroller
(ATmega1284). I chose this microcontroller because it has 4 timers (2 timers were used for the motors and an ultrasonic sensor – the car detects when it is driven into an obstacle and brakes using the ultrasonic sensor), while the other 2 timers were used for warnings and signals, as well as for the horn (I used a passive buzzer). The car receives commands through the Android application such as:
forward, brake, backward, horn, left turn signal, right turn signal, hazard lights, headlights, turn left, turn right and changing the speed by increasing or decreasing the duty cycle of the PWM signal. The control of the four DC motors is achieved using a H‐bridge converter (DRV8835). I also designed the printed circuit board (PCB) and developed the Android application in Android Studio.

The electrical schematic of the remote-controlled car was created using OrCAD Capture CIS 16.6, and later transferred to OrCAD PCB Editor for the design of the printed circuit board (PCB) required for the car's operation.

![image](https://github.com/user-attachments/assets/c6198bc8-a951-4a00-9405-cc71759a3d86)
![Picture1](https://github.com/user-attachments/assets/b4aede68-9981-40ed-bf23-e4283e3eec46)


The printed circuit board (PCB) measures 90mm x 140mm, with a copper foil thickness of 35 µm and is made of FR4 material with a double-layer design. The board has a thickness of 1.6 mm, and all components are placed on the top side (TOP layer). The origin is set in the bottom-left corner, ensuring all elements have positive coordinates. Both SMD components, such as resistors and capacitors, and THT components are used. A clearance of 50 mil is maintained from the board's edge. Additionally, four mounting holes with a 4 mm diameter are included to secure the PCB to the chassis. The GND and VCC traces have a width of 40 mil, while other traces are 20 mil wide.

![image](https://github.com/user-attachments/assets/0edf45a7-08bf-46fc-956f-143aeaba3691)

For the power circuit, I chose to use two voltage regulators: an LM1086IT-3.3 to provide a stable 3.3V supply and an LM7805 to provide a stable 5V supply. The LM1086IT-3.3 regulator is used to power the ESP-01 Wi-Fi module, which operates at 3.3V, while the LM7805 is used to power the other components. Both regulators are powered by six AA batteries, each supplying 1.5V, for a total of 9V. For the four DC motors, a separate power supply consisting of four AA batteries (1.5V each, providing 6V in total) was selected.

![image](https://github.com/user-attachments/assets/47839563-4c5d-4601-96b9-67567f62ec55)


![image](https://github.com/user-attachments/assets/235e1265-170d-46ce-a999-303ad1a066df)
