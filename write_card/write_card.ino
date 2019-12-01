/**
 * For writing commands to NFC tags.
 * Takes commands passed by serial console and writes them to the cards
 * NOTE: ENSURE THAT YOUR SERIAL TERMINAL SENDS A \n AFTER A COMMAND IS SENT
 * OTHERWISE THE PROGRAM WILL NOT DETECT THE END OF THE COMMAND
 */
#include <SPI.h>
#include "PN532_SPI.h"
#include "PN532.h"
#include "NfcAdapter.h"

PN532_SPI interface(SPI, 10); //Create a SPI interface for shield at CS10

NfcAdapter nfc = NfcAdapter(interface); //create an NFC adapter object

// Stores a command
String cmd; // Command string

void setup() {
  Serial.begin(115200); //Start reading serial comms
  Serial.println("NDEF Writer");
  nfc.begin();  //begin NFC comms
  Serial.println("Please enter a string to be written to the card: ");
}

void loop() {
  // If the command is empty, we are waiting for a command to come in by serial.
  if(cmd.length() == 0){

    // If serial is available, read the string into the command buffer
    if(Serial.available()){
      cmd = Serial.readStringUntil('\n');
      Serial.print("Received: ");
      Serial.println(cmd);
      Serial.println("Please place an NFC card on the reader.");
    }
  }
  // We are looking for an NFC card to write to
  else if(nfc.tagPresent()){
    Serial.println("Card detected, writing command");
    NdefMessage msg = NdefMessage(); // Create an NdefMessage object for writing to the card
    msg.addUriRecord(cmd); // Add the command to the message

    bool written = nfc.write(msg); // Attempt to write the message to the card
    if(written){ // If the command was successfully written
      Serial.print("Successfully wrote command: '");  
      Serial.print(cmd);
      Serial.println("' to card.");
      Serial.println("Card data: ");
      NfcTag tag = nfc.read(); // Read the now written card
      tag.print(); // Print the tag details
      cmd = ""; // Clear the command string, ready for another one
      Serial.println("\n\nPlease enter the next command to write: ");
    }
    else{ // The write failed
      Serial.println("Write failed. Please try again.");
    }
  }
}
