## Hacker-Nightlight

![Capture9](https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/2f7a9811-08fe-47ba-a03e-0092ca4ed871)


# Context

The Hacker night light project is meant to show the attack possibilities and vulnerabilities with smart light bulbs.


WiFi-connected smart lights are modern lighting solutions that you can control with your phone or voice commands. They work by connecting to your home WiFi network, allowing you to adjust them remotely. You can change the brightness, color, and even set schedules for when they turn on or off. They're easy to install and can be used in various forms like bulbs or light strips. Plus, they help save energy and add convenience to your home by letting you customize your lighting preferences easily.


lights often use simple WiFi-enabled microcontrollers to manage connections and control the LED arrays. These microcontrollers are like tiny computers embedded within the smart light system. They handle tasks such as connecting to your home WiFi network, communicating with your smartphone or other devices, and controlling the LEDs

![Untitled-5](https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/72595671-05d8-4ed1-a157-279d740cc9cb) (Govee ‎B60081C3)
                                                          



Different models of lights will use different microcontrollers for these operations. Certain microcontrollers possess the capability to perform WiFi network penetration or exploitation.
One model is the ESP32-C2. Capability of doing such this such as 

PMKID capture
Deauthentication attacks using various methods
WPA/WPA2 handshake capture and parsing
Passive handshake sniffing
Packet sniffing

With many open source firmwares public on github such as https://github.com/risinek/esp32-wifi-penetration-tool

![Capture0](https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/fb5a3753-264c-4b97-b2f8-340ebefa2709) (C3 on light connected to UART)


# Vont Smartlight Pros

Searching Amazon endless for models using ESP32 chips within their hardware I stumbled upon Vonts Smart Light Pro. 

This model of smart light specifically uses a ESP32-C3 with 4 MB of flash. Using these GPIO pins for light control: 


GPIO03    PWM_i 5    Warm White
GPIO04    PWM 4    Cool White
GPIO05    PWM 3    Blue
GPIO06    PWM 1    Red
GPIO07    PWM 2    Green

Tearing into the light bulb reveals UART copper pads allowing someone to easily soder and access the ESP32-C3 on the PCB

![Untitled-1](https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/e68fed69-ae99-4fa5-ab37-bb71a288bb7b)



By exploiting the appropriate firmware, an attacker can readily implant a backdoor using the ESP32-C2 chip, enabling remote connectivity to the PCB for executing attacks or packet sniffing, all while maintaining the appearance and functionality of a typical Smart Light bulb.

(Vont has went out of business and no longer sells smart home application or this model of lightbulb) 

(Old Amazon Link https://www.amazon.com/dp/B09K38ZXYG)

# Soldering Uart connections

Using a $7 USB to TTL on amazon we can connect the TX to RX together on the TTL USB while also grounding IO9 on the light bulb PCB, setting the ESP32-C3 into boot loader mode from there we can flash our new firmware. Once the chip is flashed, unground IO9 and power cycle and the ESP will reboot into the newly flashed firmware

![Capture22](https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/7d2a8d30-3ea9-43a4-a269-bcbafb421ee9)

Once connected we are able to read and write to the flash 

(USB to TTL: https://www.amazon.com/dp/B00LODGRV8)

# Flashing 

Donwload the Flashtool from https://www.espressif.com/en/support/download/other-tools 

when opening the flashtool, you will want to set Chiptype to ESP32-C3 and make sure WorkMode is on Devlope and LoadMode is on UART

Once loaded, set the values as follows 

1. bootloader.bin @ 0x0
2. partitions.bin @ 0x8000
3. firmware.bin @ 0x10000

Ensure your SPI mode is set to QIO and buad rate is at 115200

![Capture66](https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/a9f9c861-25b8-4685-a90f-1d4cd26a7d59)


# [Big thanks to https://github.com/Spooks4576 for assisting in the creation of the firmware]


