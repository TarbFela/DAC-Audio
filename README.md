# Sine Wave on PCM1502 DAC
Adapted (_read: copied_) from https://github.com/malacalypse/rp2040_i2s_example
## Several Functionalities, Several Commits
While it would be more prudent to separate this project out into component functionalities,
it is currently being done via commits. Here is a list, from most to least recent:
1. **Analog audio input pitch detection and matching**

This uses code from my Pitch Detection project. The files are `adc_stream` and `pitch_analysis`.
An audio amplifier circuit (to line level <1.5Vp) is fed into a biasing/filtering circuit (resistor ladder and a DC decoupler capacitor).
The audio stream is analyzed for pitch and volume.

TL;DR: Sound in, if it's loud, sound out will match the pitch.
2. **Potentiometer-controlled oscillator**

Turn the dial (set up a pot on ADC 27, I think) and get a pitch. Another
version of this uses the pot for volume control.
3. **Simple oscillator (LUT)**
4. **Simple oscillator (float trig)**
## Device: PCM1502
Can be found on Amazon:
https://www.amazon.com/dp/B08SLPHY2Z?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_1
## Pinout
![](IMG_8276.JPG)
_This is how I did the pinout. The pins on the breakout board are in order of the rainbow on the jumper cables, so pin VIN is brown, GND is red, etc etc_

| PCM1502 | PICO      | (GPIO) |
|---------|-----------|--------|
| VIN     | VBUS (5V) | --     |
| GND     | GND       | --     |
| LCK     | P12       | GP9    |
| DIN     | P9        | GP6    |
| BCK     | P11       | GP8    |
| SCK     | GND       | --     |


