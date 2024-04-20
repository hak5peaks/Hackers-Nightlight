## Hacker-Nightlight

# Context

The Hacker night light project is meant to show the attack possibilities and vulnerabilities with smart light bulbs.


WiFi-connected smart lights are modern lighting solutions that you can control with your phone or voice commands. They work by connecting to your home WiFi network, allowing you to adjust them remotely. You can change the brightness, color, and even set schedules for when they turn on or off. They're easy to install and can be used in various forms like bulbs or light strips. Plus, they help save energy and add convenience to your home by letting you customize your lighting preferences easily.


lights often use simple WiFi-enabled microcontrollers to manage connections and control the LED arrays. These microcontrollers are like tiny computers embedded within the smart light system. They handle tasks such as connecting to your home WiFi network, communicating with your smartphone or other devices, and controlling the LEDs

![Untitled-5](https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/72595671-05d8-4ed1-a157-279d740cc9cb)
                (Govee ‎B60081C3)

Different models of lights will use different microcontrollers for these operations. Certain microcontrollers possess the capability to perform WiFi network penetration or exploitation.
One model is the ESP32-C2. Capability of doing such this such as 

PMKID capture
Deauthentication attacks using various methods
WPA/WPA2 handshake capture and parsing
Passive handshake sniffing
Packet sniffing

With many open source firmwares public on github such as https://github.com/risinek/esp32-wifi-penetration-tool

# Vont Smartlight Pros

Searching Amazon endless for models using ESP32 chips within their hardware I stumbled upon Vonts Smart Light Pro. 

This model of smart light specifically uses a ESP32-C2 with 4 MB of flash. Using these GPIO pins for light control: 


GPIO03    PWM_i 5    Warm White
GPIO04    PWM 4    Cool White
GPIO05    PWM 3    Blue
GPIO06    PWM 1    Red
GPIO07    PWM 2    Green

Tearing into the light bulb reveals UART copper pads allowing someone to easily soder and access the ESP32-C3 on the PCB

[picture of UART pads]

By exploiting the appropriate firmware, an attacker can readily implant a backdoor using the ESP32-C2 chip, enabling remote connectivity to the PCB for executing attacks or packet sniffing, all while maintaining the appearance and functionality of a typical Smart Light bulb.

(Vont has went out of business and no longer sells smart home application or this model of lightbulb) 

(Old Amazon Link https://www.amazon.com/dp/B09K38ZXYG)

# [Big thanks to https://github.com/Spooks4576 for assisting in the creation of the firmware]


