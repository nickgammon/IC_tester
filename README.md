# IC Tester

A simple chip tester for retro 4, 6, 8, 10, 12, 14 or 16-pin chips, designed around an Arduino Uno, some resistors, a ZIF socket, and a couple of breadboards.

Full description at <http://www.gammon.com.au/forum/?id=14898>

To use the code unmodified, wire up as follows:

 ZIF pin |Arduino pin | Notes
--------:|-----------:|---------------------
1        | 10         | Via 330 ohm resistor
2        |  9         | Via 330 ohm resistor
3        |  8         | Via 330 ohm resistor
4        |  7         | Via 330 ohm resistor
5        |  6         | Via 330 ohm resistor
6        |  5         | Via 330 ohm resistor
7        |  4         | Via 330 ohm resistor
8        |  3         | Via 330 ohm resistor
9        |  11        | Via 330 ohm resistor
10       |  12        | Via 330 ohm resistor
11       |  A5        | Via 330 ohm resistor
12       |  A4        | Via 330 ohm resistor
13       |  A3        | Via 330 ohm resistor
14       |  A2        | Via 330 ohm resistor
15       |  A1        | Via 330 ohm resistor
16       |  A0        | Via 330 ohm resistor
7        |  2         | Direct to ZIF pin
8        |  13        | Direct to ZIF pin
16       |  +5V       | (Optional - see text of article)

Compile and upload sketch (tested on an Arduino Uno). Open the Serial Monitor, set baud rate to 115200, line ending: newline.

Follow prompts in the serial monitor. Specifically:

* L : lists all known chips
* S4, S6, S8 ... S16 : scan for chips with that many pins
* \<chip number\> : search for that chip
* T : test a chip previously found by searching

Not all tests have been verified by the author. Many are not exhaustive, but should be adequate to identify a chip, or to see
if its basic logic functions and output drivers are working.

Only connect pin 16 on the ZIF chip to +5V if your target chip expects power (Vcc) on that pin.

Chips are inserted at the **top** of the ZIF holder. That is, pin 1 is always in the same place.
