/**
 * Code for the NFC base station
 * Reads NFC commands, and sends them to the robot
 * Stores one command, and one number. Deletes both when transmitted to the robot
 * 
 * There are two types of data on the cards, commands and numbers
 * 
 * Commands will tell the robot what to do, when paired with a number
 * 
 * Card data takes the form: [CHARACTER][NUMBER]
 * 
 * If the card is a command, it will simply be a [CHARACTER], if it is a number it will be [TYPE][NUMBER], where TYPE is one of D or A.
 * 
 * Numbers are either "D" type, or "A" type.
 * D - A distance in cm
 * A - An angle in degrees
 * 
 * Commands are structured as follows:
 * F - Forward command, can be paired with a D number
 * B - Backward command, can be paired with a D number
 * L - Turn left command, can be paired with an A number
 * R - Turn right command, can be paired with an A number
 * C - CLEARS THE CURRENT COMMAND/NUMBER SETTINGS
 */
String commands = "FBLRC"; // A string containing all the possible commands, as they are all one letter

#define BUZZER A0 // Define the pin that the buzzer is connected to

#include <SPI.h>
#include "PN532_SPI.h"
#include "PN532.h"
#include "NfcAdapter.h"
#include <SoftwareSerial.h>

PN532_SPI interface(SPI, 10); //Create a SPI interface for shield at CS10

NfcAdapter nfc = NfcAdapter(interface); //create an NFC adapter object

SoftwareSerial radio(2,3); // Create an object for communicating with the robot through - Arguments are (RX, TX)


// Global command data - Stores information about the current stored command
char cmd; // Will be empty if unset
char num_type; // Stores the type of the number in storage, will be empty if number is unset
int num = -1; // Will be -1 if unset

void setup() {
  Serial.begin(115200); //Start reading serial comms
  radio.begin(9600);
  Serial.println("NFC Bot Base Station");
  pinMode(BUZZER, OUTPUT);
  nfc.begin();  //begin NFC comms
}

void loop() {
  // Check if there is an NFC command to be read
  if(nfc.tagPresent()){
    byte error = readCard(); // Read the card. Loads any data that is loaded onto the card into the global variables

    // If the card read failed, give an error and cancel this read
    if(error == 1){ // Read failed for unknown reason
      Serial.println("Card Read Failed");
      readFailedBeep();
      delay(500);
      return;  
    }
    else if(error == 2){ // Card scanned does not pair with the current stored settings
      Serial.println("Invalid Combination of Number and Command");
      invalidCommandBeep();
      delay(500);
      return;
    }
    else if(error == 3){ // Not actually an error, this is a clear memory card, which should wipe the stored settings
      clearMemBeep();
      delay(500);
      return;
    }
    readSuccessBeep();
    
    printCurrent(); // Print the data that currently exists  

    // If there is a valid command in memory, send it to the robot
    if(isCurValid())sendCurrent();
    
    delay(500);
  }
}

/** Reads an NFC Card. Loads the data into the global program variables
 *  returns an error number, as follows
 *  0  - Sucessful read
 *  1 - Read failed 
 *  2 - Card does not combine with current command, card not added
 *  3 - Clear card was scanned
 */
byte readCard(){
  NfcTag tag = nfc.read(); // Read the NFC tag

  // If the card failed to read properly, cancel the rest of the read process and alert the user
  if(!tag.hasNdefMessage())return 1;
  
  NdefMessage message = tag.getNdefMessage(); // Get the NDEF message
  NdefRecord record = message.getRecord(0); // Get the NDEF record
  
  // If there is no record text, or the text is too short, fail
  if(record.getPayloadLength() < 2)return 1;
  
  char data[record.getPayloadLength()]; // Declare an array to store the text from the card
  
  record.getPayload((byte *)data); // Actually get the text
  
  byte error = processCard(data); // Process this data
  
  return error;
}

/**
 * Processes the text from an NFC card. Breaks it down into command component and number component. Note that it all works with data[1], this is because data[0] is always NULL
 * @param *data  A pointer to an array storing the data from the NFC card
 * Returns an error number. As follows:
 * 0  - No error
 * 1 - Illegal command (Command on card is not on list)
 * 2 - Invalid command combination (Move forward 90 degrees makes no sense)
 * 3 - Clear card was scanned
 */
byte processCard(char *data){
  // If the data is a number to load
  if(data[1] == 'D' || data[1] == 'A'){
    if(!setNum(atoi(data+2),data[1]))return 2; // Attempt to set the number and number type. Error 2 if fails
  }
  else if(data[1] == 'C'){ // Clear the current settings
    clearMem(); // Clear the memory
    return 3; // return a clear signal
  }
  // The data is a command, rather than a number
  else if(commands.indexOf(data[1]) >= 0){
    if(!setCmd(data[1]))return 2; // Attempt to set the current command, if it fails return a 2 error
   }
  // The command is not in the list of recognised commands
  else{
    Serial.print("Illegal Command: ");
    Serial.print(data[1]);
   return 1;
  }
  return 0; // Success
}

/** Sets the current command
 *  Returns true if the command set successfully, false if the command being set cannot be set with the current number type
 */
bool setCmd(char c){
  if(((c == 'F' || c == 'B') && (num_type == 'A')) || ((c == 'R' || c == 'L') && (num_type == 'D')))return false; // Do not set the command if it does not pair with the current number type
  cmd = c;
  return true;
}

/** Sets the current number
 *  @param val    The number value to be set
 *  @param type   The type of the number to be set ('A' for angle, 'D' for distance)
 *  Returns true if the number set successfully, false if the number being set cannot be set with the current command
 */
bool setNum(int set_num, char set_type){
  if(((cmd == 'F' || cmd == 'B') && (set_type == 'A')) || ((cmd == 'R' || cmd == 'L') && (set_type == 'D')))return false; // Do not set the number if its type does not pair with the command
  num_type = set_type;
  num = set_num;
  return true;
}

/**
 * Sends the current stored command to the robot. Will also clear the current command
 */
void sendCurrent(){
  radio.print("(");
  radio.print(cmd);
  radio.print(",");
  radio.print(num);
  radio.println(")");
  delay(150); // So the beeps aren't weird, yeah I know, using delay() is bad, but here it is fine right?
  sendCmdBeep();
  clearMem();
}

/**
 * Checks if the current command in memory is valid
 * @return True if the command is valid, false if not
 */
bool isCurValid(){
  return  (cmd != NULL && num_type != NULL && num > -1);
}

/**
 * Clears the current command memory
 */
void clearMem(){
  cmd = NULL;
  num = -1; 
  num_type = NULL;  
  Serial.println("Memory Cleared");
}

/** VARIOUS BUZZER FUNCTIONS FOR DIFFERENT STATES **/

// Function to show that a card read has failed. Will be used in future to make a failiure beep
void readFailedBeep(){
  tone(BUZZER, 10000, 100);
  delay(200);
  tone(BUZZER, 5000, 100);
}

// Sounds two high fast beeps when the card is read successfully
void readSuccessBeep(){
  tone(BUZZER, 10000, 50);
  delay(200);
  tone(BUZZER, 10000, 50);
}

// Sounds two long low beeps when the command pair is invalid
void invalidCommandBeep(){
  tone(BUZZER, 2000, 400);
  delay(200);
  tone(BUZZER, 2000, 400);
}

// Sounds three low beeps when the memory is cleared
void clearMemBeep(){
  tone(BUZZER, 2000, 100);
  delay(200);
  tone(BUZZER, 2000, 100);
  delay(200);
  tone(BUZZER, 2000, 100);
}

void sendCmdBeep(){
  tone(BUZZER, 1000, 100);
  delay(150);
  tone(BUZZER, 1500, 100);
  delay(150);
  tone(BUZZER, 2000, 100);
}

// Prints the current data that the program is storing
void printCurrent(){
  Serial.print("Processed.\nNum: ");
  Serial.println(num);
  Serial.print("Type: ");
  Serial.println(num_type);
  Serial.print("Command: ");
  Serial.println(cmd);
  Serial.println("============================\n\n");  
}
