/*
  This file is licensed under the BSD license.
  This program controls an arduino that represents a Transport Delay Measurement System (TDMS). It is connected to two pho*/


#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>


Adafruit_MPU6050 mpu; // Create an instance of the mpu6050

const int photocurrentThreshold = 900; // at this point, the LEDs are definately on

enum State {
  e_initial = 0,
  e_reset,
  e_pilot,
  e_events,
  e_report,

};

enum State g_currentState = e_initial;

String g_stateStrings[] = {"e_initial", "e_reset", "e_pilot", "e_events", "e_report"};

String getStateString (const enum State s)
{
  return g_stateStrings[s];
}

unsigned long g_stopwatch = millis();
const unsigned int g_maxUInt = 65535;
unsigned int g_motionTime = g_maxUInt;
unsigned int g_avionicsTime = g_maxUInt;
unsigned int g_igTime = g_maxUInt;

bool g_motionDetected = false;
bool g_avionicsDetected = false;
bool g_igDetected = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  // Try to initialize!
  if (!mpu.begin()) 
  {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Pin modes for joy, avionics, and ig inputs
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

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
    {
      Serial.println ("************************* NEW TEST *************************");
      g_motionTime = g_maxUInt;
      g_avionicsTime = g_maxUInt;
      g_igTime = g_maxUInt;

      g_motionDetected = false;
      g_avionicsDetected = false;
      g_igDetected = false;

      transitionTo(e_pilot);
      Serial.println("Waiting for pilot input");
      break;
    }
    case e_pilot:
    {
      
      int pitch = analogRead(A2) - 512;
      if (!inDeadzone(pitch))
      {
        g_stopwatch = millis();
        transitionTo(e_events);
      }
      break;
    }
    case e_events:
    {
      // poll for rotation threshold
      sensors_event_t _, g, __;
      mpu.getEvent(&_, &g, &__);  

      if (!g_motionDetected && fabsf(g.gyro.z) > 1)
      {
        g_motionTime = millis() - g_stopwatch; // it is ok to store it in an int because the wait will time out after 5 seconds so there will be no opportunity for it to reach 64000 milliseconds.
        g_motionDetected = true;
      }
      
      // poll for avionics threshold
      if (!g_avionicsDetected && analogRead(A1) > photocurrentThreshold)
      {
        g_avionicsTime = millis() - g_stopwatch;
        g_avionicsDetected = true;
      }
      
      // poll for ig threshold
      if (!g_igDetected && analogRead(A0) > photocurrentThreshold)
      {
        g_igTime = millis() - g_stopwatch;
        g_igDetected = true;
      }

      // check if all events recorded. Since timeout is 5 seconds, the reset time values will never be attained by a true event time.
      if (g_motionTime != g_maxUInt && g_avionicsTime != g_maxUInt && g_igTime != g_maxUInt)
      {
        transitionTo(e_report);
        break;
      }

      // check for timeout
      if (millis() - g_stopwatch > 5000)
      {
        transitionTo(e_report);
      }

      break;
    }

    case e_report:
    {

      // report servo time
      if (g_motionTime == g_maxUInt)
      {
        Serial.println("Motion did not fire");
        
      }
      else
      {
        Serial.print("Motion delay (ms): ");
        Serial.println(g_motionTime);
      }

      // report avionics time
      if (g_avionicsTime == g_maxUInt)
      {
        Serial.println("display did not turn on");
      }
      else
      {
        Serial.print("Avionics delay (ms): ");
        Serial.println(g_avionicsTime);
      }

      // report ig time
      if (g_igTime == g_maxUInt)
      {
        Serial.println("Image generator did not turn on");
      }
      else
      {
        Serial.print("Image generator delay (ms): ");
        Serial.println(g_igTime);
      }

      delay(1000);
      transitionTo(e_reset);
      break;
    }
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
