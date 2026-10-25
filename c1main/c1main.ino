/*
  This program is open sourced under the BSD license.
  This program represents a flight simulator. When the pilot moves the column, read on pin 15 (A1), the arduino writes a PWM signal to the motion system (a servo) on 
*/


#include <Ethernet.h>
#include <EthernetUdp.h>

#include <Servo.h>

bool inDeadzone(int val);
enum State {
  e_initial = 0,
  e_reset,
  e_pilot,
  e_motion,
  e_avionics,
  e_ig
};


String g_stateStrings[] = {"e_initial", "e_reset", "e_pilot", "e_motion", "e_avionics", "e_ig"};

enum State g_currentState = e_initial;

// Servo object  Servo range is 15-180deg, so 97 is halfway
Servo myservo;

String getStateString (const enum State s)
{
  return g_stateStrings[s];
}


void setup() {

  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // pin 2 is L1
  pinMode(2, OUTPUT); 

  // pin 3 is L2
  pinMode(3, OUTPUT);

  // pin 14 is joystick x reading
  pinMode(14, INPUT);

  // pin 15 is joystick y reading
  pinMode(15, INPUT);
  
  // pin 6 is control for servo
  myservo.attach(6);
}

void transitionTo(const enum State newState)
{
  g_currentState = newState;
}

void loop() {
  switch (g_currentState)
  {
    case e_initial:
      transitionTo(e_reset);
      break;

    case e_reset:
      // return servo to 0deg
      myservo.write(20);
      // Turn avionics display off
      digitalWrite(2, LOW);
      // Turn ig off
      digitalWrite(3, LOW);
      transitionTo(e_pilot);
      break;



    case e_pilot:
    {
        // Get joystick input, centering on 0
        int pitch = analogRead(15) - 512;

        // Pilot input
        if (!inDeadzone(pitch))
        {
          delay(5);
          transitionTo(e_motion);
        }
    }
      break;
   
    case e_motion:
      myservo.write(120);
      delay(5);
      transitionTo(e_avionics);
      break;

    case e_avionics:
      digitalWrite(2, HIGH);
      delay(5);
      transitionTo(e_ig);
      break;

    case e_ig:
      digitalWrite(3, HIGH);
      delay(5000);
      transitionTo(e_reset);
      break;
  }
}

bool inDeadzone(int val)
{
  // joy range is 0 to 1023, so deadzone of +-5% gives +-52 from center, which is zero.
  if (val > -52 && val < 52)
    return true;
  else
    return false;
}