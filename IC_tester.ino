int chipPins14 [14] = {10, 9, 8, 7, 6, 5, 4,        12, A5, A4, A3, A2, A1, A0};          
int chipPins16 [16] = {10, 9, 8, 7, 6, 5, 4, 3, 11, 12, A5, A4, A3, A2, A1, A0};     

int * pinsMap;  // will point to either of the above
byte numberOfPins;

int INDICATOR_LED = 2;

// how much serial data from the user we expect before a newline
const unsigned int MAX_INPUT = 16;

// we store each test line in RAM (not PROGMEM) so we need a limit for it
const byte MAX_PINS = 16;  // for storing one test line

// the current chip name
char chipName [MAX_INPUT + 1];
// pointer to current chip info (starts with number of pins)
const char * chipInfo = NULL;

// testing data (multi-line raw string)

const char chipData[] PROGMEM = R"(0001
$4013
14
LHC100G001CHLV
HLC001G100CLHV
LHC000G000CHLV
HLC010G010CLHV
$4066
14
0HH000G0HH000V
1HH100G1HH100V
0LL011G0LL011V
1HH111G1HH111V
$4075
14
00000LG0LL000V
00110HG1HH100V
10010HG0HH010V
10110HG1HH110V
11001HG0HH001V
01101HG1HH101V
11011HG0HH011V
11111HG1HH100V
$4081
14
00LH11G11HL00V
10LL10G10LL10V
01LL01G01LL01V
11HL00G00LH11V
$4093
14
00HH00G00HH00V
10HH10G10HH10V
01HH01G01HH01V
11LL11G11LL11V
)";

    
void setup() 
  {

  Serial.begin (115200);
  Serial.println (F("IC tester"));
  Serial.println (F("Enter L to list known chips"));

  } // end of setup

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

// find a chip in the chip table, return a pointer to the start of the chip info
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
      // finish line if we previously had a chip number
      if (gotChip && strcmp (which, chipName) == 0)
        {
        p [0] = 0;  // terminate name string
        Serial.print (F("Chip "));
        Serial.print (which);
        Serial.println (F(" found! Type T to test it."));
        return true;
        }
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
  Serial.println (F(" not found."));
  chipInfo = NULL;
  chipName [0] = 0;
  } // end of searchForChip

void singleTest (const char * testLine)
  {


  } // end of singleTest


void testChip ()
  {
  if (chipInfo == NULL)
    {
    Serial.println (F("No current chip. Enter its code to find it in the table"));
    return;
    }

  Serial.print (F("Testing "));
  Serial.println (chipName);

  const char * p = chipInfo;

  // find number of pins
  char pinsBuf [3];
  pinsBuf [0] = pgm_read_byte (p++);
  pinsBuf [1] = pgm_read_byte (p++);
  pinsBuf [2] = 0;

  numberOfPins = atoi (pinsBuf);

  switch (numberOfPins)
    {
    case 14:
      pinsMap = chipPins14;
      break;
    case 16:
      pinsMap = chipPins16;
      break;
      
    default:
      Serial.print (F("Unsupported number of pins: "));
      Serial.println (pinsBuf);
      return;
    }

  Serial.print (F("Number of pins: "));
  Serial.println (pinsBuf);

  // extract test data line

  bool chipDone = false;
  
  while (true)
    {
    char testLine [MAX_PINS + 1];
    while (isspace (pgm_read_byte (p)))
      p++; // skip newline and any spaces

    for (byte i = 0; i < numberOfPins; i++)
      {
      char c = pgm_read_byte (p++);
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
        case 'C':  // pulse clock
        case 'X':  // ignore this pin
          break;

        default:
          Serial.print (F("Unexpected test data: "));
          Serial.println (c);
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
      Serial.println (F("Got unexpected chip data - test abandoned"));
      return;
      }

    // terminate test buffer (for printing purposes)
    testLine [numberOfPins] = 0;

    Serial.print (F("Testing case: "));
    Serial.println (testLine);

    // now do that test
    singleTest (testLine);
    }   // end of while (each line)
    
  Serial.println (F("Done."));
  } // end of testChip

// here to process incoming serial data after a terminator received
void process_data (const char * data)
  {
  if (strcmp (data, "L") == 0)
    listChips ();
  else if (strcmp (data, "T") == 0)
    testChip ();
  else if (data == "")
    return;
  else
    searchForChip (data);
    
  }  // end of process_data

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


void loop() 
  {

  // if serial data available, process it
  while (Serial.available () > 0)
    processIncomingByte (Serial.read ());

  } // end of loop
