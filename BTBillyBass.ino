/*This is my crack at a state-based approach to automating a Big Mouth Billy Bass.
 This code was built on work done by both Donald Bell and github user jswett77. 
 See links below for more information on their previous work.

 In this code you'll find reference to the MX1508 library, which is a simple 
 library I wrote to interface with the extremely cheap 2-channel H-bridges that
 use the MX1508 driver chip. It may also work with other H-bridges that use different
 chips (such as the L298N), so long as you can PWM the inputs.

 This code watches for a voltage increase on input A0, and when sound rises above a
 set threshold it opens the mouth of the fish. When the voltage falls below the threshold,
 the mouth closes.The result is the appearance of the mouth "riding the wave" of audio
 amplitude, and reacting to each voltage spike by opening again. There is also some code
 which adds body movements for a bit more personality while talking.

 Most of this work was based on the code written by jswett77, and can be found here:
 https://github.com/jswett77/big_mouth/blob/master/billy.ino

 Donald Bell wrote the initial code for getting a Billy Bass to react to audio input,
 and his project can be found on Instructables here:
 https://www.instructables.com/id/Animate-a-Billy-Bass-Mouth-With-Any-Audio-Source/

 Author: Jordan Bunker <jordan@hierotechnics.com> 2019
 License: MIT License (https://opensource.org/licenses/MIT)
*/

#include <MX1508.h>

MX1508 bodyMotor(3, 5); // Sets up an MX1508 controlled motor on PWM pins 3 and 5
MX1508 mouthMotor(6, 9); // Sets up an MX1508 controlled motor on PWM pins 6 and 9

int ledPin4 = 2;
int ledPin3 = 11;
int ledPin2 = 4;
int ledPin1 = 12;

int soundPin = A0; // Sound input

int silence = 12; // Threshold for "silence". Anything below this level is ignored.
int bodySpeed = 0; // body motor speed initialized to 0
int soundVolume = 0; // variable to hold the analog audio value
int fishState = 0; // variable to indicate the state Billy is in

bool talking = false; //indicates whether the fish should be talking or not

//these variables are for storing the current time, scheduling times for actions to end, and when the action took place
long currentTime;
long mouthActionTime;
long bodyActionTime;
long lastActionTime;

long nextBlinkigTime;
long blinkingState = 1;

long eyeButtonPin = 7;
long bodyButtonPin = 8;
int eyeButtonState = 0;  
int lastEyeButtonState = 0; 
int bodyButtonState = 0;  
int lastBodyButtonState = 0; 

bool blinkingIsEnabled = true;
bool bodyIsEnabled = true;


void setup() {
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);

  digitalWrite(ledPin1, LOW); 
  digitalWrite(ledPin2, LOW); 
  digitalWrite(ledPin3, LOW); 
  digitalWrite(ledPin4, LOW); 

  pinMode(eyeButtonPin, INPUT);
  pinMode(bodyButtonPin, INPUT);

 
//make sure both motor speeds are set to zero
  bodyMotor.setSpeed(0); 
  mouthMotor.setSpeed(0);

//input mode for sound pin
  pinMode(soundPin, INPUT);

  Serial.begin(9600);
}

void loop() {
  currentTime = millis(); //updates the time each time the loop is run
  updateSoundInput(); //updates the volume level detected
  updateButtons();
  blink();
  SMBillyBass(); //this is the switch/case statement to control the state of the fish
  // dalay(5000);
}

void SMBillyBass() {
  switch (fishState) {
    case 0: //START & WAITING
      // Serial.println("Fish state START & WAITING");
      if (soundVolume > silence) { //if we detect audio input above the threshold
        if (currentTime > mouthActionTime) { //and if we haven't yet scheduled a mouth movement
          talking = true; //  set talking to true and schedule the mouth movement action
          mouthActionTime = currentTime + 100;
          fishState = 1; // jump to a talking state
        }
      } else if (currentTime > mouthActionTime + 100) { //if we're beyond the scheduled talking time, halt the motors
        bodyMotor.halt();
        mouthMotor.halt();
      }
      if (currentTime - lastActionTime > 5000) { //if Billy hasn't done anything in a while, we need to show he's bored
        lastActionTime = currentTime + floor(random(5, 15)) * 1000L; //you can adjust the numbers here to change how often he flaps
        fishState = 2; //jump to a flapping state!
      }
      break;

    case 1: //TALKING
      // Serial.println("Fish state TALKING");
      if (currentTime < mouthActionTime) { //if we have a scheduled mouthActionTime in the future....
        if (talking) { // and if we think we should be talking
          openMouth(); // then open the mouth and articulate the body
          lastActionTime = currentTime;
          articulateBody(true);
        }
      }
      else { // otherwise, close the mouth, don't articulate the body, and set talking to false
        closeMouth();
        articulateBody(false);
        talking = false;
        fishState = 0; //jump back to waiting state
      }
      break;

    case 2: //GOTTA FLAP!
      // Serial.println("Fish state FLAP");
      //Serial.println("I'm bored. Gotta flap.");
      // flap();
      fishState = 0;
      break;
  }
}

void updateButtons() {
  // read the pushbutton input pin:
  eyeButtonState = digitalRead(eyeButtonPin);
  bodyButtonState = digitalRead(bodyButtonPin);

  if (eyeButtonState != lastEyeButtonState && eyeButtonState) {
    blinkingIsEnabled = !blinkingIsEnabled;
    Serial.println("blinkingIsEnabled: ");
    Serial.println(blinkingIsEnabled);
  }
  lastEyeButtonState = eyeButtonState;


  if (bodyButtonState != lastBodyButtonState && bodyButtonState) {
    bodyIsEnabled = !bodyIsEnabled;
    Serial.println("bodyIsEnabled: ");
    Serial.println(bodyIsEnabled);
  }
  lastBodyButtonState = bodyButtonState;

}

int updateSoundInput() {
  soundVolume = analogRead(soundPin);
  Serial.print(soundVolume);
  Serial.println("");
}

void openMouth() {
  // Serial.println("Open mouth");
  mouthMotor.halt(); //stop the mouth motor
  mouthMotor.setSpeed(220); //set the mouth motor speed
  mouthMotor.forward(); //open the mouth
}

void closeMouth() {
  // Serial.println("Close mouth");
  mouthMotor.halt(); //stop the mouth motor
  mouthMotor.setSpeed(0); //set the mouth motor speed
  mouthMotor.backward(); // close the mouth
}

void articulateBody(bool talking) { //function for articulating the body
  if (talking && bodyIsEnabled) { //if Billy is talking
    if (currentTime > bodyActionTime) { // and if we don't have a scheduled body movement
      int r = floor(random(0, 8)); // create a random number between 0 and 7)
      if (r < 1) {
        bodySpeed = 0; // don't move the body
        bodyActionTime = currentTime + floor(random(5000, 10000)); //schedule body action for .5 to 1 seconds from current time
        bodyMotor.forward(); //move the body motor to raise the head

      } else if (r < 3) {
        bodySpeed = 150; //move the body slowly
        bodyActionTime = currentTime + floor(random(500, 1000)); //schedule body action for .5 to 1 seconds from current time
        bodyMotor.forward(); //move the body motor to raise the head

      } else if (r == 4) {
        bodySpeed = 200;  // move the body medium speed
        bodyActionTime = currentTime + floor(random(500, 1000)); //schedule body action for .5 to 1 seconds from current time
        bodyMotor.forward(); //move the body motor to raise the head

      } else if ( r == 5 ) {
        bodySpeed = 0; //set body motor speed to 0
        bodyMotor.halt(); //stop the body motor (to keep from violent sudden direction changes)
        bodyMotor.setSpeed(255); //set the body motor to full speed
        bodyMotor.backward(); //move the body motor to raise the tail
        bodyActionTime = currentTime + floor(random(900, 1200)); //schedule body action for .9 to 1.2 seconds from current time
      }
      else {
        bodySpeed = 255; // move the body full speed
        bodyMotor.forward(); //move the body motor to raise the head
        bodyActionTime = currentTime + floor(random(1500, 3000)); //schedule action time for 1.5 to 3.0 seconds from current time
      }
    }

    bodyMotor.setSpeed(bodySpeed); //set the body motor speed
  } else {
    if (currentTime > bodyActionTime) { //if we're beyond the scheduled body action time
      bodyMotor.halt(); //stop the body motor
      bodyActionTime = currentTime + floor(random(20, 50)); //set the next scheduled body action to current time plus .02 to .05 seconds
    }
  }
}

void blink() {
  if (blinkingState == 0 || blinkingIsEnabled == false) {
      digitalWrite(ledPin1, LOW); 
      digitalWrite(ledPin2, LOW); 
      digitalWrite(ledPin3, LOW); 
      digitalWrite(ledPin4, LOW); 
    return;
  }
  
  if (currentTime > nextBlinkigTime) {
    if (blinkingState == 1) {
      digitalWrite(ledPin1, HIGH);
      nextBlinkigTime = currentTime + 100;
      blinkingState = 2;
    }
    else if (blinkingState == 2) {
      digitalWrite(ledPin2, HIGH);
      nextBlinkigTime = currentTime + 100;
      blinkingState = 3;
    }
    else if (blinkingState == 3) {
      digitalWrite(ledPin3, HIGH);
      nextBlinkigTime = currentTime + 200;
      blinkingState = 4;
    }
    else if (blinkingState == 4) {
      digitalWrite(ledPin4, HIGH);
      nextBlinkigTime = currentTime + 200;
      blinkingState = 5;
    }
    else if (blinkingState == 5) {
      digitalWrite(ledPin4, LOW);
      nextBlinkigTime = currentTime + 200;
      blinkingState = 6;
    }
    else if (blinkingState == 6) {
      digitalWrite(ledPin3, LOW);
      nextBlinkigTime = currentTime + 200;
      blinkingState = 7;
    }
    else if (blinkingState == 7) {
      digitalWrite(ledPin3, LOW);
      nextBlinkigTime = currentTime + 200;
      blinkingState = 8;
    }
    else if (blinkingState == 8) {
      digitalWrite(ledPin2, LOW);
      nextBlinkigTime = currentTime + 200;
      blinkingState = 9;
    }
    else if (blinkingState == 9) {
      digitalWrite(ledPin1, LOW);
      nextBlinkigTime = currentTime + 100;
      blinkingState = 10;
    }
    else if (blinkingState == 10) {
      nextBlinkigTime = currentTime + 5000;
      blinkingState = 1;
    }
  }
}

void flap() {
  bodyMotor.setSpeed(180); //set the body motor to full speed
  bodyMotor.backward(); //move the body motor to raise the tail
  delay(500); //wait a bit, for dramatic effect
  bodyMotor.halt(); //halt the motor
}
