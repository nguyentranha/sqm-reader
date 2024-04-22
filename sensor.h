/**************************************************************************/
/*
    @author   Ha Tran
    Hanoi Southern Observatory
    Software License Agreement (BSD License)
    All rights reserved.
*/
/**************************************************************************/

#include "SQM_TSL2591.h"
#include <Adafruit_AHTX0.h>

SQM_TSL2591 sqm = SQM_TSL2591(2591);
void readSQM(void);

Adafruit_AHTX0 aht;
int have_aht = 0;
int have_tsl = 0;

void setup_sqm() {
  if (sqm.begin()) {
    Serial.println("Found SQM TSL2591");
    sqm.config.gain = TSL2591_GAIN_HIGH;
    sqm.config.time = TSL2591_INTEGRATIONTIME_600MS;
    sqm.configSensor();
    sqm.showConfig();
    sqm.setCalibrationOffset(0.0);
    have_tsl = 1;
  } else {
    Serial.println("Could not find TSL2591. Check wiring.");
  }
}

void setup_aht() {
  if (!aht.begin()) {
    Serial.println("Could not find AHT. Check wiring.");
  } else {
    have_aht = 1;
  }
}
