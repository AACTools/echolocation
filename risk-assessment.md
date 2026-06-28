# ‘echolocation’ \- Risk Assessment

An assistive technology hardware device designed to help visually impaired access a standard keyboard.

## Medical Device

According to the MRHA definition of a medical device, this does not constitute a medical device…..

## Hardware Components

‘echolocation’ relies on off the shelf hardware components. They are purchased from ‘[PiHut](https://thepihut.com/)’ and manufactured by ‘[M5 Stack](https://m5stack.com/)’. 

### M5 Stack Core S3

[Spec Sheet](https://docs.m5stack.com/en/core/CoreS3)

The CoreS3 is the main processor for the device as well as the touch screen display.

It has the following certifications:

* CE Mark  
* SAR Certification

### Base M5GO Bottom3

[Spec Sheet](https://docs.m5stack.com/en/module/M5GO3%20Bottom)

The Base M5GO is the battery expansion for the CoreS3 device. It contains a 500mAh lithium battery.

It has a CE Mark

### USB Module with MAX3421E v1.2

[Spec Sheet](https://docs.m5stack.com/en/module/USB%20v1.2%20Module)

This is the device that adds USB compatibility to the device.

### M5Stack Audio Module (STM32G030)

[Spec Sheet](https://docs.m5stack.com/en/module/Module-Audio)

This is the device that adds an audio output jack to the device.

## Data Privacy

‘echolocation’ collects no data on the user and does not log any data they input.

Although, as ‘echolocation’ is designed to be used with other another device (like an iPad), you will need to consult that device or software to find out how they use keyboard input data

# Risks

Below is a list of risks that are associated with this product. Outlined are the decisions made and design and manufacturing stages taken in order to mitigate them. There are also recommended mitigations that the user should take in order to further mitigate risks.

| Risk | Design Mitigation | Recommended User Mitigation |
| :---- | :---- | :---- |
| Device being destroyed | The device is held together using 4 tightly screwed M3 bolts. The screen is also made of plastic, reducing the risk of sharp glass causing injury | As soon as there is any damage to the device stop using it immediately. |
| Battery |  | When charging, monitor the battery. Don’t overcharge the battery. |
| Charging cables can be a hazard | The device requires a cable, but for use it support bluetooth connections, for input and output reducing the need for cable | Safely store device while charging and use wireless capabilities when you can |

