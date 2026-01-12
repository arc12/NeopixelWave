# NeopixelWave
ESP32 driver for a string of "neopixel" (WS2812B) LEDs using waveform based pattern specifiers.

Uses the zorxx neopixel driver for low level stuff, although actually a forked version which accommodates the fact that some products on the market have
a RGB bit ordering rather than the documented GRB.

Provides a WiFi access point for control via a web server usually on 192.168.4.1.

Hardware used was a 100 LED strip "MyLighting" with remote control (IR) and BT connected phone app "iDeal LED" with the LED string simply unsoldered. A 5V level shifter and 5V supply to the LEDs is required.

Power supply strength is an issue. Powering the ESP32 from a plain PC USB outlet will be OK for low brightness (or few LEDs lit) but the WiFi is found to drop out under higher loads. Used on a powered USB hub designed to support 2A device charging worked fine.

Original hardware was found to deliver data bursts at about 110ms, each with a duration of about 2.85ms, giving an approx frequency (bits) of 840kHz.
