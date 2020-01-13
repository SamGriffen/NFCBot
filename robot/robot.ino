/**
 * Code for the robot side of the NFC Robot
 * 
 * Expects a message of the form ([CMD], [NUM]), where [CMD] is one character, and [NUM] is up to 3
 */
#include <Shieldbot.h>
#include <SoftwareSerial.h>

Shieldbot robot = Shieldbot(); // Creates a robot object

// Define the different components of a message
#define M_START '('
#define M_END ')'
#define M_DELIM ','

// Constants for wooden surface
#define CM_PER_SEC 42.0      // Centimeters per second with full motor speed
#define DEG_PER_SEC 700      // Degrees per second with motors at full throttle

// Robot movememnt controls
#define ROTATE_SPEED_PERC 0.25 // Percentage of full speed to rotate at


String commands = "FBLR"; // A string with all the possible commands that the robot can receive, means that any corrupted commands can be picked up

SoftwareSerial radio(6, 7); // Initialise a radio link (RX, TX)

// Struct for message being read
struct message{
  // Members to use when reading data in from the line
  byte curChar; // Stores the character index that is to be written next
  char data[8]; // Stores the unprocessed message - This will be where characters are stored as they are read in

  // Members to be used by the command that will process a command
  byte num;  // Stores the current number
  char cmd;  // Stores the read command

  // Flags
  bool reading; // Stores whether a mesasge is currently being read
  bool unread;  // Stores whether there is a message that has not yet been handled
} msg;

void setup() {
  // Initialise the two serial links that will be used
  Serial.begin(9600);
  radio.begin(9600);

  // Set the initial values of the message
  msg.curChar = 0;
  msg.reading = false;
  msg.unread = false;
  robot.setMaxSpeed(230);//255 is max
}

void loop() {
  //robot.drive(127,-127);
  
  if(radio.available()){
    incomingChar(radio.read()); // Handle the incoming character
    
    if(msg.unread){ // If there is an unread message, pass it through to the robot'
      printCmd();
      runCommand();
      msg.unread = false;
    }
  }
}

/**
 * Runs the current command on the robot
 */
void runCommand(){
  if(msg.cmd == 'F' || msg.cmd == 'B'){
    robot.drive(127 * (msg.cmd == 'F' ? 1 : -1), 127 * (msg.cmd == 'F' ? 1 : -1)); // Set the motors to move in a direction. This will be either forward or backward, determined by the result of msg.cmd == 'F'
    delay((msg.num / (float)CM_PER_SEC)*1000); // Keep the motors running for the desired distance
    robot.stop(); // Stop the robot
  }
  else if(msg.cmd == 'R' || msg.cmd == 'L'){
    robot.drive(ROTATE_SPEED_PERC * 127 * (msg.cmd == 'R' ? 1 : -1), ROTATE_SPEED_PERC * 127 * (msg.cmd == 'R' ? -1 : 1)); // If the command is to turn right, left motor full throttle forward, right full throttle backward. If command is to turn left, do the opposite
    Serial.println((msg.num / (DEG_PER_SEC * ROTATE_SPEED_PERC))*1000);
    delay((msg.num / (DEG_PER_SEC * ROTATE_SPEED_PERC))*1000); // Turn to make the right angle (Requires DEG_PER_SEC to be tuned)
    robot.stop();
  }

}

/**
 * Reads an incoming character
 * 
 * @param char incoming  The incoming character to handle
 */
void incomingChar(char incoming){
  Serial.println(incoming);
  if(incoming == M_START && !msg.reading){ // If this is the start of a message
    msg.reading = true; // Start the struct reading incoming characters
    msg.curChar = 0; // Start at the very beginning of the string
    return;
  }
  else if(!msg.reading){ // If we are not currently reading a message, ignore the character. It is of no interest
    return;
  }
  else if(incoming == M_END){ // If this is the end of the message, set the appropriate flags
    msg.data[msg.curChar] = '\0'; // NULL terminate the command string
    msg.reading = false; // The message has been read
    processCommand();
    return;
  }  

  msg.data[msg.curChar] = incoming; // Save the current character
  msg.curChar++; // Increment the current character
}

/**
 * Processes the current command in msg.data, writes it into the other fields in the struct
 * 
 * Returns true on successful process, false if it has failed
 */
bool processCommand(){
  char *cmd = strtok(msg.data, M_DELIM);  // Get the command from the data
  char *num = cmd+2;                      // Get the number out of the data TODO: Make this not hideous pointer math, but right now, it is late, I want to go to bed, and this does the job

  if(commands.indexOf(*cmd) == -1)return; // If the command is invalid, fail

  msg.cmd = *cmd; // Set the command
  msg.num = atoi(num); // Set the number
  msg.unread = true;
  return true;
}

// Prints the command over the serial interface
void printCmd(){
    Serial.print("COMMAND: ");
    Serial.println(msg.cmd);
    Serial.print("NUMBER: ");
    Serial.println(msg.num);
}
