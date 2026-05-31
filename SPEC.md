# Project Specifications

echolocation is a hardware device designed to as assistive technology to help visual impaired users use a standard keyboard. It does this by connecting to a standard keyboard and reading aloud to every keypress. It then connects to a computer as a standard keyboard, and if the user holds down a key it sends that as a keypress to the computer.

This document is written by a human and is the source of truth for AI agents so they know what to build.

## Hardware

The hardware used is [M5Stacks](https://m5stack.com/), this allows us people to create the device without any hardware knowledge.

Specifically we are using the following modules.

* [M5Stack CoreS3 ESP32S3](https://shop.m5stack.com/products/m5stack-cores3-esp32s3-iotdevelopment-kit?srsltid=AfmBOor91B72K9dsifvTqBSTwQAZz7Worq9cJUJ7sw2065vTjUKRdZkM)
* [M5GO Battery Bottom3 (for CoreS3 only)](https://shop.m5stack.com/products/m5go-battery-bottom3-for-cores3-only?srsltid=AfmBOoo9umRlyYjDaAOf5KuTXiz9eLAItGLq1fkmgax-Rf8muDDokVeJ)
* [USB Module with MAX3421E v1.2](https://shop.m5stack.com/products/usb-module-with-max3421e-v1-2?srsltid=AfmBOopdCs196hpYWsUVITGv8l6V6T0btyp5yZUZ5_MbPM_D3WVY4ceW&variant=44512358793473)
* [M5Stack Audio Module (STM32G030)](https://shop.m5stack.com/products/m5stack-audio-module-es8388?srsltid=AfmBOopEP9SpWVf2BRjI0aIFe-F486eIQmofqVBIjPntzBHPF00S2h-M&variant=46249349349633)

## Features

### Text to speech

### Audio

By default the audio should come out of the built in speaker. If you plug in a speaker it should seemlessly switch to the speaker and when you unplug it should switch back

## Coding standards

Code should be kept as simple as possible and variables and functions should have names that very clear explain what they are from.

The code should be easy to alter in the future.

Wherever possible write tests. All tests should pass.

For hardware use C++. For scripts use NodeJS.

Code should have a brief guide on how to execute or load it.