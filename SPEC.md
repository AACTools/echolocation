# Project Specifications

echolocation is a hardware device designed to as assistive technology to help visual impaired users use a standard keyboard. It does this by connecting to a standard keyboard and reading aloud to every keypress. It then connects to a computer as a standard keyboard, and if the user holds down a key it sends that as a keypress to the computer.

This document is written by a human and is the source of truth for AI agents so they know what to build.

## Hardware

The hardware used is [M5Stacks](https://m5stack.com/), this allows us people to create the device without any hardware knowledge.

Specifically we are using the following modules.

- [M5Stack CoreS3 ESP32S3](https://shop.m5stack.com/products/m5stack-cores3-esp32s3-iotdevelopment-kit?srsltid=AfmBOor91B72K9dsifvTqBSTwQAZz7Worq9cJUJ7sw2065vTjUKRdZkM)
- [M5GO Battery Bottom3 (for CoreS3 only)](https://shop.m5stack.com/products/m5go-battery-bottom3-for-cores3-only?srsltid=AfmBOoo9umRlyYjDaAOf5KuTXiz9eLAItGLq1fkmgax-Rf8muDDokVeJ)
- [USB Module with MAX3421E v1.2](https://shop.m5stack.com/products/usb-module-with-max3421e-v1-2?srsltid=AfmBOopdCs196hpYWsUVITGv8l6V6T0btyp5yZUZ5_MbPM_D3WVY4ceW&variant=44512358793473)
- [M5Stack Audio Module (STM32G030)](https://shop.m5stack.com/products/m5stack-audio-module-es8388?srsltid=AfmBOopEP9SpWVf2BRjI0aIFe-F486eIQmofqVBIjPntzBHPF00S2h-M&variant=46249349349633)

## Features

You can connect a keyboard to the device, via bluetooth or usb. As soon as the keyboard is connected to the device it should start reading aloud every key that is pressed. An example of keyboard that needs to work is an Apple Magic Keyboard, but it should work with any keyboard.

You can also connect a computer to the device, via usb or bluetooth. As soon as the computer is connected to the device it should start sending keypresses to the device when the user holds down a key (for the specified duration). It should simulate a normal keyboard keypress.

For each time the user holds the key down it should only send a single keypress to the computer.

The behaviour should be the same for modifier keys.

You should be able to seamlessly switch between different keyboards and computers.

It should work with any keyboard layout.

If the audio is playing and you press a new key it should stop the current audio and start playing the new audio.

A keyboard and a computer can be connected at the same time. Every keypress is spoken; only keys held for the configured duration are sent to the computer.

Settings are saved and persist across reboot.

The settings page should also have a 'debug' button that shows a page with basic debug information that I can use to track down bugs.

### Screen

On the CoreS3 screen it should show the current key that is being pressed. It should also have a battery level indicator. There should also be a settings button that opens a menu with the following configurable options:

- Volume
- Bluetooth
  - Configure Keyboard Connection
  - Configure Computer Connection
- Hold duration
- Factory reset

The screen should also report any errors that occur in a clear and easy to understand way.

### Text to speech

The text to speech should be pre-generated and stored on a microSD. There should be a script to generate all the audio files needed to be loaded onto the microSD card. The script should easily be able to configure the voice of the text.

### Audio

By default the audio should come out of the built in speaker. If you plug in a speaker it should seamlessly switch to the speaker and when you unplug it should switch back

## Coding standards

Code should be kept as simple as possible and variables and functions should have names that very clear explain what they are from.

The code should be easy to alter in the future.

Wherever possible write tests. All tests should pass.

For hardware use C++. For scripts use NodeJS.

Code should have a brief guide on how to execute or load it.

I am a principal software engineer but I am not a hardware engineer, so give variables and functions clear names, that are easy to understand and leave comments.

## Other

It should use the LVGL library for the UI so it feels modern and native.

It should be built with speed as the focus, it should respond to a keypress instantly, any delay is not acceptable. Touches on the screen should also feel snappy and responsive.

If there is any loading on startup make sure to show a loading page.
