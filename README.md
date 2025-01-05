<p align="center">
  <img src="https://github.com/user-attachments/assets/6d3e3b2f-939f-49c1-91ad-322a55c86a11" alt="Centered Image" width="200"/>
</p>

<h1 align="center">Announcing Hackers Nightlight V2</h1>

<p align="center">Hackers nightlight V2 is inspired by the open source project's idea of creating a covert penetration testing tool to help uncover hardware vulnerabilities, test network security and response readiness. </p>

<p align="center">Hackers nightlight V2 brings new custom hardware packed inside of the same discrete light bulb format, containing a brand new webUI with focus on ease of use, and a suite of new abilities and tools.  </p>

<p align="center">Learn more here: https://hackersnightlight.com </p>


## Hacker-Nightlight

<!-- First image in the `##Hacker-Nightlight` section with added alt-text. -->
[![Hacker Nightlight]][HN1]

The Wi-Fi hacking light bulb. (More models coming soon)


# Context

The Hacker night light project is meant to show the attack possibilities and vulnerabilities with smart light bulbs.

WiFi-connected smart lights are modern lighting solutions that you can control with your phone or voice commands. They work by connecting to your home WiFi network, allowing you to adjust them remotely. You can change the brightness, color, and even set schedules for when they turn on or off. They're easy to install and can be used in various forms like bulbs or light strips. Plus, they help save energy and add convenience to your home by letting you customize your lighting preferences easily.

Smart lights often use simple WiFi-enabled microcontrollers to manage connections and control the LED arrays. These microcontrollers are like tiny computers embedded within the smart light system. They handle tasks such as connecting to your home WiFi network, communicating with your smartphone or other devices, and controlling the LEDs.

<!-- First image in the `#Context` section with added alt-text. -->
[![Govee ‎B60081C3]][C1]
<!-- Caption for the image above. Can be removed due to integrated alt/text that shows on image hover. -->
<p align="center"><code>Image: Govee ‎B60081C3</code></p>

Different models of lights will use different microcontrollers for these operations. Certain microcontrollers possess the capability to perform WiFi network penetration or exploitation.
One model is the ESP32-C3 with capabilities such as:

* PMKID capture
* Deauthentication attacks using various methods
* WPA/WPA2 handshake capture and parsing
* Passive handshake sniffing
* Packet sniffing

There are many open source firmwares public on github such as [esp32-wifi-penetration-tool](https://github.com/risinek/esp32-wifi-penetration-tool).


# Vont Smartlight Pros

Searching Amazon endlessly for models using the __ESP32__ chips within their hardware I stumbled upon Vonts Smart Light Pro. 

The 3 models vulnerable to this are _Vont Color Pro SLB04_, _Vont Color SLB02_, and _Vont Color SLB01_. (More to come)

These smart light models specifically uses a __ESP32-C3__ with 4 MB of flash. 

<!-- First image in the `#Vont Smartlight Pros` section with added alt-text. -->
[![C3-UART]][VSP1]
<!-- Caption for the image above. Can be removed due to integrated alt/text that shows on image hover. -->
<p align="center"><code>Image: C3 on Light Connected to UART</code></p>

These GPIO pins are used for light control: 

* GPIO03    PWM_i 5    Warm White
* GPIO04    PWM 4    Cool White
* GPIO05    PWM 3    Blue
* GPIO06    PWM 1    Red
* GPIO07    PWM 2    Green

Tearing into the light bulb reveals UART copper pads -- allowing someone to easily soder and access the ESP32-C3 via UART on the PCB.

<!-- Second image in the `#Vont Smartlight Pros` section with added alt-text. -->
[![UART-Pads]][VSP2]
<!-- Caption for the image above. Can be removed due to integrated alt/text that shows on image hover. -->
<p align="center"><code>Image: Lightbulb Internals and UART Copper Pads</code></p>

By leveraging the appropriate firmware, an attacker can readily implant a backdoor using the ESP32-C3 chip. This provides remote connectivity to the PCB for executing attacks or packet sniffing, all while maintaining the appearance and functionality of a typical Smart Light bulb.

(Vont has went out of business and no longer sells thier smart home application or this model of the lightbulb.)

(Old Amazon Link https://www.amazon.com/dp/B09K38ZXYG)


# Soldering Uart connections

Using a $7 [USB to TTL on amazon](https://www.amazon.com/dp/B00LODGRV8), we can connect the TX to RX together on the TTL USB while also grounding IO9 on the light bulb PCB. This sets the ESP32-C3 into boot loader mode allowing us to flash our new firmware. Once the chip is flashed, unground IO9, power cycle, and the ESP will reboot into the newly flashed firmware.

<!-- First image in the `#Soldering Uart connections` section with added alt-text. -->
[![Soldered-Sample]][SUC1]
<!-- Caption for the image above. Can be removed due to integrated alt/text that shows on image hover. -->
<p align="center"><code>Image: Soldered Example</code></p>

Once connected we are able to read and write to the flash.

<!-- Second image in the `#Soldering Uart connections` section with added alt-text. -->
[![Mode-Schematic]][SUC2]
<!-- Caption for the image above. Can be removed due to integrated alt/text that shows on image hover. -->
<p align="center"><code>Image: Modes Soldering Schematic</code></p>


# Flashing 

Download the Flashtool from [https://www.espressif.com](https://www.espressif.com/en/support/download/other-tools). 

__DO NOT FLASH OR PLUG IN USB ADAPTER WHILE LIGHTBULB IS ON 120V or 240V POWER SOURCE!!!__

When opening the flashtool, you will want to set Chiptype to _ESP32-C3_, make sure WorkMode is on _Develop_, and LoadMode is on _UART_.

Once loaded, set the values as follows:

1. `bootloader.bin` @ `0x0`
2. `partitions.bin` @ `0x8000`
3. `firmware.bin` @ `0x10000`

Ensure your SPI mode is set to `QIO` and BAUD rate is at `115200`.

<!-- First image in the `#Flashing` section with added alt-text. -->
[![Flashing]][F1]
<!-- Caption for the image above. Can be removed due to integrated alt/text that shows on image hover. -->
<p align="center"><code>Image: Flashtool Settings</code></p>


# After flashing

Once you flash the ESP32-C3 you will want to unpower the device and remove it from bootloader. Once you have done that you can either: 

1. Plug in the USB to UART. (The LED array will not be on)
2. Remove UART cables from USB and plug light into E26 socket. (Lamp or light socket)

**DEFAULT SSID:** `Nightlight`

**DEFAULT Password:** `Nightlight12345`

From there you will want to connect to the Nightlight AP using the Password "_Nightlight12345_" then navigate to the web page hosted on `192.168.4.1`.

<!-- First image in the `#After Flashing` section with added alt-text. -->
[![Nightlight-Dashboard]][AF1]
<!-- Caption for the image above. Can be removed due to integrated alt/text that shows on image hover. -->
<p align="center"><code>Image: Nightlight AP Web Dashboard</code></p>

To change light colors scroll down to the light control page

<!-- Second image in the `#After Flashing` section with added alt-text. -->
[![Light-Controller]][AF2]
<!-- Caption for the image above. Can be removed due to integrated alt/text that shows on image hover. -->
<p align="center"><code>Image: Nightlight AP Light Control Page</code></p>

Clicking the box will give you a color picker to select a RGB code for the light.

__YOU CANNOT HAVE RGB AND WARM/COLD LIGHTS ON AT THE SAME TIME!__ If you move the warm/cold slider it will switch modes to the warm/cold lights. To turn RGB back on -- slide the warm/cold slider back down, and the light will switch back to the RGB mode.

**Tiktok Demo:** https://www.tiktok.com/@o.mg_peaks/video/7360587336507280683


# Wyze Bulb Color (WLPA19CV2) 

The Wyze bulb colors use the exact same MCU as the Vont Color Lights (ESP32-C3), however they have a diffrent flashing process.

<!-- First image in the `#Wyze Bulb Color (WLPA19CV2)` section with added alt-text. -->
[![Wyze-Internals]][WBC1]
<!-- Caption for the image above. Can be removed due to integrated alt/text that shows on image hover. -->
<p align="center"><code>Image: Wyze Bulb Internals and GPIO Pins</code></p>

Rather then grounding a single GPIO pin to set the chip into boot loader, these bulbs require that GPIO8 needs to be set to high (jump to 3.3v) and GPIO9 needs to be grounded

```
GPIO8 -> HIGH
GPIO9 -> LOW
```

The rest of the flashing process remains the same.

> **THESE LIGHTS USE BP5758 LED DRIVER, FULL LED CONTROL FROM THE WEB SERVER IS STILL WIP.**


# Contributors

Big thanks to [Spooks4576](https://github.com/Spooks4576) for assisting in the creation of the firmware!


<!--
The first list is of the image link and the caption/alt-text in the following format.
[variable-name]: image-source-link (caption/alt-text)
-->
[Hacker Nightlight]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/678e1534-e29e-462e-a4bd-22b98f3bd397 (Hacker Nightlight)
[Govee ‎B60081C3]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/72595671-05d8-4ed1-a157-279d740cc9cb (Govee ‎B60081C3)
[C3-UART]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/fb5a3753-264c-4b97-b2f8-340ebefa2709 (C3 on Light Connected to UART)
[UART-Pads]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/e68fed69-ae99-4fa5-ab37-bb71a288bb7b (Lightbulb Internals and UART Copper Pads)
[Soldered-Sample]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/7d2a8d30-3ea9-43a4-a269-bcbafb421ee9 (Soldered Example)
[Mode-Schematic]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/3faa23ab-5814-478f-b56d-15a533a8d59e (Modes Soldering Schematic)
[Flashing]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/a9f9c861-25b8-4685-a90f-1d4cd26a7d59 (Flashtool Settings)
[Nightlight-Dashboard]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/28d1392a-6f49-4e97-bf9e-9aef69b9064e (Nightlight AP Web Dashboard)
[Light-Controller]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/4542043f-0afd-4369-8c15-6d5f36757b61 (Nightlight AP Light Control Page)
[Wyze-Internals]: https://github.com/hak5peaks/Hackers-Nightlight/assets/115900893/f475e295-e994-411f-8fcc-7a32f0029c96 (Wyze Bulb Internals and GPIO Pins)

<!--
The second list is of the link to the image source in the following format.
[variable-name]: image-source-link
-->
[HN1]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/678e1534-e29e-462e-a4bd-22b98f3bd397
[C1]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/72595671-05d8-4ed1-a157-279d740cc9cb
[VSP1]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/fb5a3753-264c-4b97-b2f8-340ebefa2709
[VSP2]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/e68fed69-ae99-4fa5-ab37-bb71a288bb7b
[SUC1]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/7d2a8d30-3ea9-43a4-a269-bcbafb421ee9
[SUC2]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/3faa23ab-5814-478f-b56d-15a533a8d59e
[F1]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/a9f9c861-25b8-4685-a90f-1d4cd26a7d59
[AF1]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/28d1392a-6f49-4e97-bf9e-9aef69b9064e
[AF2]: https://github.com/Peaakss/Hacker-Nightlight/assets/115900893/4542043f-0afd-4369-8c15-6d5f36757b61
[WBC1]: https://github.com/hak5peaks/Hackers-Nightlight/assets/115900893/f475e295-e994-411f-8fcc-7a32f0029c96
