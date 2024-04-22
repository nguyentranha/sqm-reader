/**************************************************************************/
/*
    @author   Ha Tran
    Hanoi Southern Observatory
    Software License Agreement (BSD License)
    All rights reserved.
*/
/**************************************************************************/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "Wire.h"
#include "display.h"
#include "sensor.h"

#define EEPROM_SIZE 40 // bytes
#define EEPROM_ADDRESS 0 
#define SQM_MEMORY_SIZE 4 // 4 elements in array
#define SMOOTH_LOADING_TIME 20 // ms

// display settings
const int I2C_DISPLAY_ADDRESS = 0x3c;

// I2C pin
const int SDA_PIN = 16;
const int SDC_PIN = 17;

// calibrate AHT20 sensor
const int TEMP_CALIB = 0;
const int HUMID_CALIB = -5;

SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi ui(&display);

// declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label);
void drawMainContent(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state);
void updateData(OLEDDisplay *display);

float sqm_memory[SQM_MEMORY_SIZE] = {};
String temperature = "0.0";
String humidity = "0";

// add frames
FrameCallback frames[SQM_MEMORY_SIZE] = { drawMainContent };
OverlayCallback overlays[] = { drawHeaderOverlay };

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  // reset eeprom for the first time use
  // EEPROM.put(EEPROM_ADDRESS, sqm_memory);

  // initialize display
  display.init();
  display.clear();
  display.display();

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  // initialize User Interface
  ui.setTargetFPS(60);
  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  // get sqm data from eeprom
  EEPROM.get(EEPROM_ADDRESS, sqm_memory);

  // calculate true size of sqm_memory
  int size_of_sqm_memory = 0;
  for (int i = 1; i < SQM_MEMORY_SIZE; i++) {
    // Serial.println(sqm_memory[i]);
    if (sqm_memory[i] > 0) {
      size_of_sqm_memory++;
      frames[size_of_sqm_memory] = drawMainContent;
    }
  }
  size_of_sqm_memory++;  // we always have at least one value in the sqm_memory
  if (size_of_sqm_memory > 1) {
    // ui.setTimePerFrame(8000);
    ui.setTimePerTransition(350);
  } else {
    ui.disableAutoTransition();
  }
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, size_of_sqm_memory);
  ui.setOverlays(overlays, 1);
  ui.init();

  // setup sensor
  setup_aht();
  setup_sqm();

  // reading data from sensor
  updateData(&display);
}

void loop() {
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    // don't do stuff if you are below your time budget.
    OLEDDisplayUiState *state = ui.getUiState();
    int currentFrame = state->currentFrame;
    doc["sqm"] = sqm_memory[currentFrame];
    if (currentFrame == 0) {
      doc["left_overlay"] = humidity + "%";
      doc["right_overlay"] = temperature + "°C";
      ui.setTimePerFrame(8000); // we need to display current data longer
    } else {
      doc["left_overlay"] = "Slot " + String(currentFrame);
      doc["right_overlay"] = "Mem.";
      ui.setTimePerFrame(2000); 
    }
    delay(remainingTimeBudget);
  }
}

void updateData(OLEDDisplay *display) {
  smootherProgress(display, 0, 25, "Reading sensor...");
  readAHT();
  smootherProgress(display, 26, 50, "Screen may flash...");
  readTSL(display);
  smootherProgress(display, 71, 100, "Almost done...");
}

void readAHT() {
  if (have_aht == 1) {
    sensors_event_t humid, temp;
    aht.getEvent(&humid, &temp);
    temperature = String(temp.temperature + TEMP_CALIB, 1);
    humidity = String(humid.relative_humidity + HUMID_CALIB, 0);
  }
}

void readTSL(OLEDDisplay *display) {
  display->clear();
  display->display();
  delay(100);
  float sqm_value = 0.0;
  if (have_tsl == 1) {
    sqm.takeReading();
    sqm_value = sqm.mpsas;
  } else {
    // sqm_value = random(1, 22);  // for test only
  }
  // insert to the first element
  for (int i = SQM_MEMORY_SIZE - 1; i > 0; i--) {
    sqm_memory[i] = sqm_memory[i - 1];
  }
  sqm_memory[0] = sqm_value;
  // save sqm data to eeprom
  EEPROM.put(EEPROM_ADDRESS, sqm_memory);
  EEPROM.commit();
}

void smootherProgress(OLEDDisplay *display, int from, int to, String label) {
  for (int i = from; i <= to; i++) {
    if (i == 100) {
      label = "Done!";
    }
    drawProgress(display, i, label);
    if (i == 100) {
      delay(1000);
    } else {
      delay(SMOOTH_LOADING_TIME);
    }
  }
}
