#include <Wire.h>
#include <string.h>
#include "rgb_lcd.h"

const int Monitoring = 1;
const int WaterOn = 2;
const int WaterOff = 3;
const int Soaking = 4;
const int Evaluating = 5;
const int PauseWatering = 6;

int currentState;
const int StartWateringLevel = 400;
const int DoneWateringLevel = 380;
const int relayPin = 2;
const int buttonPin = 3;
const int moistureSensorPin = 0;
const int delayInSeconds = 4;
bool display = false;
int waterCount = 0;

rgb_lcd lcd;

class StateData {
  private:
    int waterCountForCurrentCycle;
    int numCyclesInPause;
    int numWaterPerCycle = 10;
    int maxCyclesInPause = 900;

  public:
    StateData() {
      waterCountForCurrentCycle = 0;
      numCyclesInPause = 0;
    }

    void startNewCycle() {
      waterCountForCurrentCycle = 0;
      numCyclesInPause = 0;
    }

    void water() {
      waterCountForCurrentCycle++;
    }

    void addPauseCycle() {
      numCyclesInPause ++;
    }

    bool isPauseWatering() {
      return numWaterPerCycle < waterCountForCurrentCycle;
    }

    bool isDonePausing() {
      return numCyclesInPause >= maxCyclesInPause;
    }
};

StateData* stateData;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  currentState = Monitoring;
  Serial.print(Monitoring);
  stateData = new StateData();
}

void loop() {
  int moistureSensorValue = analogRead(moistureSensorPin);
  currentState = GetNextState(moistureSensorValue);
  display = true; //digitalRead(buttonPin);
  Serial.print("Moisture Sensor Reading: ");
  Serial.println(moistureSensorValue);
  Serial.print("State is ");
  Serial.println(currentState);
  delay(delayInSeconds * 1000);
}

int GetNextState(int moistureSensorReading) {
  int nextState = currentState;
  switch (currentState) {
    case Monitoring: nextState = ProcessMonitoringState(moistureSensorReading);
      break;

    case WaterOn: nextState = ProcessWaterOn(moistureSensorReading);
      break;

    case WaterOff: nextState = ProcessWaterOff(moistureSensorReading);
      break;

    case Soaking: nextState = ProcessSoaking(moistureSensorReading);
      break;

    case Evaluating: nextState = ProcessEvaluating(moistureSensorReading);
      break;

    case PauseWatering: nextState = ProcessPausing();
  }
  return nextState;
}

int ProcessMonitoringState(int moistureSensorReading) {
  WriteToDisplay("Monitoring ", moistureSensorReading);
  if (moistureSensorReading > StartWateringLevel) {
    return WaterOn;
  }
  return Monitoring;
}

int ProcessWaterOn(int moistureSensorReading) {
  WriteToDisplay("Watering ", moistureSensorReading);
  digitalWrite(relayPin, HIGH);
  ++waterCount;
  stateData->water();
  return WaterOff;
}

int ProcessWaterOff(int moistureSensorReading) {
  WriteToDisplay("Water Off ", moistureSensorReading);
  digitalWrite(relayPin, LOW);
  return Soaking;
}

int ProcessSoaking(int moistureSensorReading) {
  WriteToDisplay("Soaking ", moistureSensorReading);
  return Evaluating;
}

int ProcessEvaluating(int moistureSensorReading) {
  WriteToDisplay("Evaluating ", moistureSensorReading);
  if (moistureSensorReading < DoneWateringLevel) {
    stateData->startNewCycle();
    return Monitoring;
  } if (stateData->isPauseWatering()) {
    return PauseWatering;
  }
  else {
    return WaterOn;
  }
}

int ProcessPausing() {
  if (stateData->isDonePausing()) {
    stateData->startNewCycle();
    return Monitoring;
  } else {
    stateData->addPauseCycle();
    return PauseWatering;
  }
}

int WriteToDisplay(String msg, int moisturSensorReading) {
  if (display) {
    lcd.display();
    lcd.setRGB(255, 255, 255);
  } else {
    lcd.noDisplay();
    lcd.setRGB(0, 0, 0);
  }
  lcd.clear();
  lcd.print(msg + " " + String(moisturSensorReading));
  lcd.setCursor(0, 1);
  lcd.print("Water Count " + String(waterCount));
  //lcd.println("MS Reading: " + String(moisturSensorReading));
  Serial.println(msg);
  return 0;
}

