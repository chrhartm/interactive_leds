# Interactive Leds

Remote-control LED strips
- From a button box (Arduino)
- Based on live music (Raspberry Pi)

[Sample images](https://photos.app.goo.gl/HkHUnnAh49a3waEM7)

## Components

- Emitter (Arduino): Arduino Nano V3.0 + separate wireless transceiver module 2.4G 1100m NRF24L01+PA+LNA
- Emitter (Raspberry Pi): Raspberry Pi 3b + USB sound card + USB gamepad + NRF24L01
- Receiver: Keywish RF-Nano for Arduino Nano V3.0, Micro USB Nano Board ATmega328P QFN32 5V 16M CH340, Integrate NRF24l01+2.4G wireless
- LED strips: WS2812B Smart pixel led strip 2m, 60LED/m, IP67

## Libraries used

- [FastLED](https://www.arduino.cc/reference/en/libraries/fastled/)
- [RF24](https://www.arduino.cc/reference/en/libraries/rf24/)

## Circuit diagrams

### Emitter

![Emitter circuit diagram](Docs/emitter.png?raw=true "Emitter")

### Receiver

![Receiver circuit diagram](Docs/receiver.png?raw=true "Receiver")

## Configs

- Emitter
  - Board: Arduino Nano
  - Processor: ATmega328P (Old Bootloader)
  - Programmer: AVRISP mkll
- Receiver
  - Board: Arduino Nano
  - Processor: ATmega328P
  - Programmer: AVRISP mkll
  
