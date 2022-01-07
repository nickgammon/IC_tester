/*

IC tester

Author: Nick Gammon
Date: 6 Jan 2022

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

// Mapping of chip pins (from chip socket) to Arduino pins

// DuT pin:             1  2  3  4  5  6  7          8   9  10  11  12  13  14
//                     -------------------------------------------------------
int chipPins14 [14] = {10, 9, 8, 7, 6, 5, 4,        12, A5, A4, A3, A2, A1, A0};

// DuT pin:             1  2  3  4  5  6  7  8   9  10  11  12  13  14  15  16
//                     -------------------------------------------------------
int chipPins16 [16] = {10, 9, 8, 7, 6, 5, 4, 3, 11, 12, A5, A4, A3, A2, A1, A0};

// These two global variables are set up when a requested chip is found
int * pinsMap = NULL;   // will point to either of the above once chip is located
byte numberOfPins = 0;  // how many pins it has (14 or 16)

// for forcing the Gnd pin to be ground, bypassing the current-limiting resistors
byte PIN7_GROUND = 2;
byte PIN8_GROUND = 13;

// how much serial data from the user we expect before a newline
const unsigned int MAX_INPUT = 16;

// we store each test line in RAM (not PROGMEM) so we need a limit for it
const byte MAX_PINS = 16;  // for storing one test line

// the current chip name
char chipName [MAX_INPUT + 1];

// pointer to current chip info (starts with number of pins)
const char * chipInfo = NULL;


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
  Serial.println (F("Enter S14 to scan for 14-pin chips, or S16 to scan for 16-pin chips."));
  Serial.println (F("Otherwise enter a chip code to search for it (eg. '7400')"));
  } // end of showHelp
  
void setup()
  {

  Serial.begin (115200);
  Serial.println (F("IC tester - written by Nick Gammon."));
  Serial.println (F("Version 1.0. Date: 6 January 2022."));
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
      if (i == 6)  // that is pin 7 since this is zero-relative
        {
        digitalWrite (PIN7_GROUND, LOW);
        pinMode (PIN7_GROUND, OUTPUT);
        }
      else if (i == 7)  // that is pin 8 since this is zero-relative
        {
        digitalWrite (PIN8_GROUND, LOW);
        pinMode (PIN8_GROUND, OUTPUT);
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
    delay (10);  // 10 ms

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
  delay (10);  // 10 ms

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

  pinMode (PIN7_GROUND, INPUT);
  pinMode (PIN8_GROUND, INPUT);
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

  // that number should be 14 or 16
  // if so, point to the correct mapping array of Arduino pins to chip pins

  switch (numberOfPins)
    {
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
      
        // that number should be 14 or 16
        // if so, point to the correct mapping array of Arduino pins to chip pins
      
        switch (numberOfPins)
          {
          case 14:
            pinsMap = chipPins14;
            if (pins == 14)
              testChip (false);   // not verbose
            break;
          case 16:
            pinsMap = chipPins16;
            if (pins == 16)
              testChip (false);   // not verbose
            break;
      
          default:
            Serial.print (F("Unsupported number of pins for "));
            Serial.print (chipName);
            Serial.print (F(": "));
            Serial.println (pinsBuf);
            break;
            
          } // end of switch
        
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
   
}  // end of scanChips

// here to process incoming serial data after a newline received
void process_data (const char * data)
  {
  if (strcmp (data, "L") == 0)
    listChips ();
  else if (strcmp (data, "T") == 0)
    testChip (true);  // verbose output
  else if (strcmp (data, "S14") == 0)
    scanChips (14);  // scan for 14-pin chips
  else if (strcmp (data, "S16") == 0)
    scanChips (16);  // scan for 16-pin chips
  else if (strcmp (data, "") == 0)
    return;
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
