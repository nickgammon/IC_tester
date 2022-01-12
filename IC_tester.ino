/*

IC tester

Author: Nick Gammon
Date: 6 Jan 2022
Modified: 12 Jan 2022

Written and tested on Arduino Uno, however should work on other devices with at least
16 spare pins for connecting to the device to be tested.

Concepts and chip test data from: https://www.instructables.com/Arduino-IC-Tester/ by JorBi.

The page the test data was linked to gave permission for its use:

  Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)

This code is released under the same license.

License: https://creativecommons.org/licenses/by-nc-sa/4.0/


DuT = Device under Test

When testing for a LOW output the pin is set to INPUT_PULLUP which will tend to pull that
signal HIGH, unless the device actively drives it low. Unfortunately there is no INPUT_PULLDOWN
on the Uno, so testing for a HIGH signal will not be quite as reliable.

For reliably testing a specific chip you could manually wire pulldown resistors to appropriate
pins.

*/

#define VERSION "1.1"
#define DATE_MODIFIED "12 January 2022"

//-----------------------------
// configuration
//-----------------------------

// Mapping of chip pins (from chip socket) to Arduino pins

// DuT pin:                   1  2                                             3   4
//                           -------------------------------------------------------
const int chipPins4 [4]   = {10, 9,                                           A1, A0};

// DuT pin:                   1  2  3                                      4   5   6
//                           -------------------------------------------------------
const int chipPins6 [6]   = {10, 9, 8,                                    A2, A1, A0};

// DuT pin:                   1  2  3  4                               5   6   7   8
//                           -------------------------------------------------------
const int chipPins8 [8]   = {10, 9, 8, 7,                             A3, A2, A1, A0};

// DuT pin:                   1  2  3  4  5                        6   7   8   9  10
//                           -------------------------------------------------------
const int chipPins10 [10] = {10, 9, 8, 7, 6,                      A4, A3, A2, A1, A0};

// DuT pin:                   1  2  3  4  5  6                 7   8   9  10  11  12
//                           -------------------------------------------------------
const int chipPins12 [12] = {10, 9, 8, 7, 6, 5,               A5, A4, A3, A2, A1, A0};

// DuT pin:                   1  2  3  4  5  6  7          8   9  10  11  12  13  14
//                           -------------------------------------------------------
const int chipPins14 [14] = {10, 9, 8, 7, 6, 5, 4,        12, A5, A4, A3, A2, A1, A0};

// DuT pin:                   1  2  3  4  5  6  7  8   9  10  11  12  13  14  15  16
//                           -------------------------------------------------------
const int chipPins16 [16] = {10, 9, 8, 7, 6, 5, 4, 3, 11, 12, A5, A4, A3, A2, A1, A0};

// for forcing the Gnd pin to be ground, bypassing the current-limiting resistors

const byte DUT_GROUND_A = 7;  // this pin is to be considered Gnd on the DuT (alternative A)
const byte GROUND_PINA = 2;   // which is wired to this pin on the Arduino

const byte DUT_GROUND_B = 8;  // this pin is to be considered Gnd on the DuT (alternative B)
const byte GROUND_PINB = 13;  // which is wired to this pin on the Arduino

// how much serial data from the user we expect before a newline
const unsigned int MAX_INPUT = 16;

// we store each test line in RAM (not PROGMEM) so we need a limit for it
const byte MAX_PINS = 16;  // for storing one test line

const unsigned int CLOCK_PULSE_WIDTH = 10;  // how long to pulse the clock for (ms)
const unsigned int DATA_SETTLE_TIME = 10;   // how long to wait for the outputs to settle (ms)

//-----------------------------
// variables
//-----------------------------

// These two global variables are set up when a requested chip is found
const int * pinsMap = NULL;   // will point to either of the above once chip is located
byte numberOfPins = 0;  // how many pins it has (4, 6, 8, 10, 12, 14 or 16)

// the current chip name
char chipName [MAX_INPUT + 1];

// pointer to current chip info (starts with number of pins)
const char * chipInfo = NULL;

// stringification for Arduino IDE version
#define xstr(s) str(s)
#define str(s) #s

/*

  WARNING:

    Because Gnd and Vcc are supplied through the (recommended) 330 ohm current-limiting resistors
    you may get incorrect results for some chips. In those cases you are advised to manually
    connect Vcc for the DuT to +5V on the Arduino, and Gnd for the DuT to Gnd on the Arduino.
    For many chips Vcc is the highest pin (14 or 16 as the case may be) and Gnd will be pin 7
    (for 14-pin chips) or pin 8 (for 16-pin chips).

  WORK-AROUND:

    The code below attempts to at least connect Gnd directly to pins 7 or 8 (depending on the chip)
    via an additional couple of wires that bypasses the current-limiting resistor, see documentation
    for more details.

    Most (if not all) chips in the test file have ground on pin 7 (for 14-pin chips) and pin 8
    (for 16-pin chips).

 */

#include "chipData.h"

void showHelp ()
  {
  Serial.println (F("Enter L to list known chips."));
  Serial.println (F("Enter S4/S6/S8/S10/S12/S14/S16 to scan for 4 to 16-pin chips."));
  Serial.println (F("Otherwise enter a chip code to search for it (eg. '7400')"));
  } // end of showHelp

void setup()
  {

  Serial.begin (115200);
  Serial.println (F("IC tester - written by Nick Gammon."));
  Serial.println (F("Version " VERSION ". Date: " DATE_MODIFIED "."));
  Serial.println (F("Compiled on " __DATE__ " at " __TIME__ " with Arduino IDE " xstr(ARDUINO) "."));
  showHelp ();

  // testLEDs ();  // Uncomment to test your wiring by putting LEDs (via resistors) into the
                   // chip socket pins

  } // end of setup

// For checking your wiring, put LEDs into each pin on the chip socket
// and call this from setup. It should sequence through each pin in pin order.
void testLEDs ()
{

  pinsMap = chipPins16;
  numberOfPins = 16;

  for (int i = 0; i < numberOfPins; i++)
    {
    int pin = pinsMap [i];
    pinMode (pin, OUTPUT);
    digitalWrite (pin, HIGH);
    delay (200);
    digitalWrite (pin, LOW);
    pinMode (pin, INPUT);
    } // end of for each pin

} // end of testLEDs

// List all known chips
void listChips ()
  {
  const char * p = chipData;
  Serial.println (F("Listing of known chips"));
  bool gotChip = false;
  do
    {
    const char c = pgm_read_byte (p++);
    if (c == 0 || c == '&')
      break;
    if (c == '$')
      gotChip = true;
    else if (c == '\n')
      {
      // finish line if we previously had a chip number
      if (gotChip)
        Serial.println ();
      gotChip = false;
      }
    else if (gotChip)
      Serial.print (c);
    } while(true);
  Serial.println ();
  } // end of listChips

// find a chip in the chip table, return true if found, false if not
// Side-effect: chipInfo is set to point to that chip in the chip information array
bool searchForChip (const char * which)
  {
  // point to start of known chips data
  chipInfo = chipData;
  // when we find a $ we note that we have started a new ship
  bool gotChip = false;
  // this is where we are in chipName at present
  char * p;
  byte nameLength;
  do
    {
    const char c = pgm_read_byte (chipInfo++);
    if (c == 0 || c == '&')
      break;
    if (c == '$')
      {
      gotChip = true;
      p = chipName;   // start collecting the name
      nameLength = 0;
      }
    else if (c == '\n')
      {
      // finish name if we previously had a chip number

      if (gotChip)
        {
        *p = 0;  // terminate name string
        if (strcmp (which, chipName) == 0)
          {
          Serial.print (F("Chip "));
          Serial.print (which);
          Serial.println (F(" found! Type T to test it."));
          return true;
          } // if correct chip
        }  // if on chip name line
      gotChip = false;
      }
    else if (gotChip)
      {
      if (nameLength < MAX_INPUT)
        {
        *p++ = c;  // save name of the current chip
        nameLength++;
        }
      }
    } while(true);

  Serial.print (F("Chip "));
  Serial.print (which);
  Serial.println (F(" not found. Type L for a list."));
  chipInfo = NULL;
  chipName [0] = 0;
  return false;
  } // end of searchForChip

// For space-padding a test number (the argument is zero-relative so we add one to it)
void showNumber (const int which)
  {
  if (which < 9)
    Serial.print (F(" "));
  Serial.print (which + 1);
  } // end of showNumber

// Do one of the test cases and show the results
int singleTest (const char * testLine, const int testNumber, const bool verbose)
  {
  byte i;
  int failed = 0;

  if (verbose)
    {
    Serial.print (F("Testing case "));
    showNumber (testNumber);
    Serial.print (F(": "));
    Serial.print (testLine);
    }

  // set ground to LOW
  for (i = 0; i < numberOfPins; i++)
    if (testLine [i] == 'G')
    {
      digitalWrite (pinsMap [i], LOW);
      pinMode (pinsMap [i], OUTPUT);

      // see if ground override possible
      if ((i + 1) == DUT_GROUND_A)  // i is zero-relative (eg. pin 7 on the DuT)
        {
        digitalWrite (GROUND_PINA, LOW);
        pinMode (GROUND_PINA, OUTPUT);
        }
      else if ((i + 1) == DUT_GROUND_B)  // i is zero-relative (eg. pin 8 on the DuT)
        {
        digitalWrite (GROUND_PINB, LOW);
        pinMode (GROUND_PINB, OUTPUT);
        }
    } // end of for each pin

  // set power to HIGH
  for (i = 0; i < numberOfPins; i++)
    if (testLine [i] == 'V')
    {
      digitalWrite (pinsMap [i], HIGH);
      pinMode (pinsMap [i], OUTPUT);
    }  // end of for each pin

  // set input pins
  for (i = 0; i < numberOfPins; i++)
    {
    // supply a LOW
    if (testLine [i] == '0' || testLine [i] == 'C')
      {
      digitalWrite (pinsMap [i], LOW);
      pinMode (pinsMap [i], OUTPUT);
      }
    // supply a HIGH
    else if (testLine [i] == '1' || testLine [i] == 'c')
      {
      digitalWrite (pinsMap [i], HIGH);
      pinMode (pinsMap [i], OUTPUT);
      }
    } // end of for each pin

  // prepare output pins

  for (i = 0; i < numberOfPins; i++)
    {
    // expect a HIGH
    if (testLine [i] == 'H')
      {
      // briefly write LOW to the pin to try to drain stray
      // capacitance, so a failed output driver will therefore
      // read LOW (still) rather than some random noise value
      //  - we do this because there is not INPUT_PULLDOWN feature
      //  - the current-limiting resistors should protect the Arduino
      //    and the chip from this brief pulse
      digitalWrite (pinsMap [i], LOW);
      pinMode (pinsMap [i], OUTPUT);
      pinMode (pinsMap [i], INPUT);
      }
    // expect a LOW - so set an input pullup
    else if (testLine [i] == 'L')
      {
      pinMode (pinsMap [i], INPUT_PULLUP);
      }
    } // end of for each pin

  // pulse clock (C = LOW->HIGH, c = HIGH->LOW)

  byte clocks = 0;

  for (i = 0; i < numberOfPins; i++)
    {
    if (testLine [i] == 'C')
      {
      clocks++;
      digitalWrite (pinsMap [i], HIGH);
      }
    else if (testLine [i] == 'c')
      {
      clocks++;
      digitalWrite (pinsMap [i], LOW);
      }
    } // end of for each pin

  // save a few ms by not delaying if we had no clock signals
  if (clocks)
    {
    delay (CLOCK_PULSE_WIDTH);

    // put clock back to where it was

    for (i = 0; i < numberOfPins; i++)
      {
      if (testLine [i] == 'C')
        digitalWrite (pinsMap [i], LOW);
      else if (testLine [i] == 'c')
        digitalWrite (pinsMap [i], HIGH);
      } // end of for each pin
    }  // end of if we had any clock signals

  // let the chip settle into its correct outputs
  delay (DATA_SETTLE_TIME);

  // read results

  for (i = 0; i < numberOfPins; i++)
    {
    // expect a HIGH
    if (testLine [i] == 'H')
      {
      if (digitalRead (pinsMap [i]) != HIGH)
        {
        if (verbose)
          {
          if (failed == 0)
            Serial.println ();
          Serial.print (F("  Pin "));
          Serial.print (i + 1);
          Serial.println (F(" should be HIGH but is LOW"));
          } // end of if verbose
        failed++;
        } // end of failed test (should be HIGH but is not)
      }  // end of expecting HIGH
    // expect a LOW
    else if (testLine [i] == 'L')
      {
      if (digitalRead (pinsMap [i]) != LOW)
        {
        if (verbose)
          {
          if (failed == 0)
            Serial.println ();
          Serial.print (F("  Pin "));
          Serial.print (i + 1);
          Serial.println (F(" should be LOW but is HIGH"));
          } // end of if verbose
        failed++;
        } // end of failed test (should be LOW but is not)
      } // end of expecting LOW
    } // end of for each pin

  // Show what the failed test was
  if (verbose)
    {
    if (failed)
      {
      Serial.print (F("  ** Failed test "));
      showNumber (testNumber);
      Serial.print (F(": "));
      Serial.println (testLine);
      }
    else
      Serial.println (F(" ok"));
    } // end of if verbose

  return failed;
  } // end of singleTest

// set all chip pins (plus our two ground pins) to INPUT (high-impedance)
void setAllPinsToInput ()
  {
  // set all pins to input initially
  for (byte i = 0; i < numberOfPins; i++)
    pinMode (pinsMap [i], INPUT);

  pinMode (GROUND_PINA, INPUT);
  pinMode (GROUND_PINB, INPUT);
  } // end of setAllPinsToInput

// do all tests for the requested chip
void testChip (const bool verbose)
  {
  if (chipInfo == NULL)
    {
    Serial.println (F("No current chip. Enter its code to find it in the table"));
    return;
    }

  if (verbose)
    {
    Serial.print (F("Testing "));
    Serial.println (chipName);
    } // end of if verbose

  const char * p = chipInfo;
  byte i;

  // find number of pins - we expect a 2-digit number
  char pinsBuf [3];
  pinsBuf [0] = pgm_read_byte (p++);
  pinsBuf [1] = pgm_read_byte (p++);
  pinsBuf [2] = 0;

  numberOfPins = atoi (pinsBuf);

  // that number should be an even number between 4 and 16
  // if so, point to the correct mapping array of Arduino pins to chip pins

  switch (numberOfPins)
    {
    case  4:
      pinsMap = chipPins4;
      break;
    case  6:
      pinsMap = chipPins6;
      break;
    case  8:
      pinsMap = chipPins8;
      break;
    case 10:
      pinsMap = chipPins10;
      break;
    case 12:
      pinsMap = chipPins12;
      break;
    case 14:
      pinsMap = chipPins14;
      break;
    case 16:
      pinsMap = chipPins16;
      break;

    default:
      Serial.print (F("Unsupported number of pins for "));
      Serial.print (chipName);
      Serial.print (F(": "));
      Serial.println (pinsBuf);
      return;
    }

  if (verbose)
    {
    Serial.print (F("Number of pins: "));
    Serial.println (pinsBuf);
    }

  // extract test data line

  bool chipDone = false;
  int failures = 0;

  // set all pins to input initially
  setAllPinsToInput ();

  int testNumber = 0;

  // for each test case
  while (true)
    {
    char testLine [MAX_PINS + 1];
    while (isspace (pgm_read_byte (p)))
      p++; // skip newline and any spaces

    for (i = 0; i < numberOfPins; i++)
      {
      char c = pgm_read_byte (p++);
      // if we hit a $, ^ or end-of-data we are done
      if ((c == '$' || c == 0 || c == '&') && i == 0)
        {
        chipDone = true;
        break;
        }

      // validate data in test
      switch (c)
        {
        case 'H':  // expect high output
        case 'L':  // expect low output
        case 'V':  // Vcc (supply voltage)
        case 'G':  // Gnd
        case '0':  // set input to 0
        case '1':  // set input to 1
        case 'C':  // pulse clock LOW->HIGH->LOW
        case 'c':  // pulse clock HIGH->LOW->HIGH
        case 'X':  // ignore this pin
          break;

        default:
          setAllPinsToInput ();
          Serial.print (F("Unexpected test data: '"));
          Serial.print (c);
          Serial.print (F("' in test number "));
          showNumber (testNumber);
          Serial.print (F(" for "));
          Serial.println (chipName);
          return;

        } // end of switch

      testLine [i] = c;
      }

   // exit testing loop if we get to the end of this chip
   if (chipDone)
     break;

   // check we reached end
   if (!isspace (pgm_read_byte (p)))
     {
     setAllPinsToInput ();
     Serial.println (F("Expected space/newline, but did not get it in test number "));
     showNumber (testNumber);
     Serial.print (F(" for "));
     Serial.println (chipName);
     return;
     }

    // terminate test buffer (for printing purposes)
    testLine [numberOfPins] = 0;

    // now do that test
    failures += singleTest (testLine, testNumber++, verbose);
    }   // end of while (each line)

  // set all pins to input afterwards
  setAllPinsToInput ();

  if (verbose)
    {
    Serial.print (F("Done - "));
    if (failures)
      {
      Serial.print (F("FAILED - "));
      Serial.print (failures);
      Serial.println (F(" failure(s)"));
      }
    else
      Serial.println (F("passed."));
    } // end of if verbose
  else
    if (failures == 0)
      {
      Serial.print (chipName);
      Serial.println (F(" detected."));
      }

  if (verbose)
    {
    Serial.println ();
    Serial.println (F("Enter T to test another chip of the same type."));
    showHelp ();
    Serial.println ();
    }

  } // end of testChip

void scanChips (const byte pins)
{
  // point to start of known chips
  chipInfo = chipData;
  Serial.println (F("Scanning for known chips ..."));
  bool gotChip = false;
  // this is where we are in chipName at present
  char * p;
  byte nameLength;
  int count = 0;

  do
    {
    const char c = pgm_read_byte (chipInfo++);
    if (c == 0 || c == '&')
      break;
    if (c == '$')
      {
      gotChip = true;
      p = chipName;   // start collecting the name
      nameLength = 0;
      }
    else if (c == '\n')
      {
      // finish name if we previously had a chip number

      if (gotChip)
        {
        *p = 0;  // terminate name string

        // find number of pins - we expect a 2-digit number
        char pinsBuf [3];
        pinsBuf [0] = pgm_read_byte (chipInfo);
        pinsBuf [1] = pgm_read_byte (chipInfo + 1);
        pinsBuf [2] = 0;

        numberOfPins = atoi (pinsBuf);

        // that number should be an even number between 4 and 16
        // if so, point to the correct mapping array of Arduino pins to chip pins

        switch (numberOfPins)
          {
          case  4:
            pinsMap = chipPins4;
            break;
          case  6:
            pinsMap = chipPins6;
            break;
          case  8:
            pinsMap = chipPins8;
            break;
          case 10:
            pinsMap = chipPins10;
            break;
          case 12:
            pinsMap = chipPins12;
            break;
          case 14:
            pinsMap = chipPins14;
            break;
          case 16:
            pinsMap = chipPins16;
            break;

          default:
            Serial.print (F("Unsupported number of pins for "));
            Serial.print (chipName);
            Serial.print (F(": "));
            Serial.println (pinsBuf);
            return;

          } // end of switch

        // if our chip matches the number of pins for this test, then see if it tests OK

        if (pins == numberOfPins)
          {
          testChip (false);  // not verbose
          count++;
          }

        }  // if on chip name line
      gotChip = false;
      }
    else if (gotChip)
      {
      if (nameLength < MAX_INPUT)
        {
        *p++ = c;  // save name of the current chip
        nameLength++;
        }
      }
    } while(true);

   chipInfo = NULL;  // we haven't found a specific chip yet
   Serial.println (F("Chip scan completed."));
   Serial.print (F("Checked against data for "));
   Serial.print (count);
   Serial.print (F(" types of "));
   Serial.print (pins);
   Serial.println (F("-pin chips."));

}  // end of scanChips

// here to process incoming serial data after a newline received
void process_data (const char * data)
  {
  if (strcmp (data, "L") == 0)
    listChips ();
  else if (strcmp (data, "T") == 0)
    testChip (true);  // verbose output
  else if (strcmp (data, "S4") == 0)
    scanChips (4);  // scan for 4-pin chips
  else if (strcmp (data, "S6") == 0)
    scanChips (6);  // scan for 6-pin chips
  else if (strcmp (data, "S8") == 0)
    scanChips (8);  // scan for 8-pin chips
  else if (strcmp (data, "S10") == 0)
    scanChips (10);  // scan for 10-pin chips
  else if (strcmp (data, "S12") == 0)
    scanChips (12);  // scan for 12-pin chips
  else if (strcmp (data, "S14") == 0)
    scanChips (14);  // scan for 14-pin chips
  else if (strcmp (data, "S16") == 0)
    scanChips (16);  // scan for 16-pin chips
  else if (strcmp (data, "") == 0)
    return;  // ignore an empty line
  else
    searchForChip (data);

  }  // end of process_data

// Handle incoming serial data (ie. input from the user)
// This modified version converts the string to upper case and ignores spaces
void processIncomingByte (const byte inByte)
  {
  static char input_line [MAX_INPUT];
  static unsigned int input_pos = 0;

  switch (inByte)
    {

    case '\n':   // end of text
      input_line [input_pos] = 0;  // terminating null byte

      // terminator reached! process input_line here ...
      process_data (input_line);

      // reset buffer for next time
      input_pos = 0;
      break;

    case '\r':   // discard carriage return
    case ' ':    // discard space
      break;

    default:
      // keep adding if not full ... allow for terminating null byte
      if (input_pos < (MAX_INPUT - 1))
        input_line [input_pos++] = toupper (inByte);
      break;

    }  // end of switch

  } // end of processIncomingByte

// main loop - just collect serial data and process it
void loop()
  {

  // if serial data available, process it
  while (Serial.available () > 0)
    processIncomingByte (Serial.read ());

  } // end of loop
