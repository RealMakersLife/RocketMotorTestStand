#include <NexButton.h>
#include <NexCheckbox.h>
#include <NexConfig.h>
#include <NexCrop.h>
#include <NexDualStateButton.h>
#include <NexGauge.h>
#include <NexGpio.h>
#include <NexHardware.h>
#include <NexHotspot.h>
#include <NexNumber.h>
#include <NexObject.h>
#include <NexPage.h>
#include <NexPicture.h>
#include <NexProgressBar.h>
#include <NexRadio.h>
#include <NexRtc.h>
#include <NexScrolltext.h>
#include <NexSlider.h>
#include <NexText.h>
#include <NexTimer.h>
#include <NexTouch.h>
#include <NexUpload.h>
#include <NexVariable.h>
#include <NexWaveform.h>
#include <Nextion.h>
#include <doxygen.h>
#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// Create an instance of the HX711 class
HX711 scale;

// Calibration factor (this needs to be adjusted to match your load cell)
float calibration_factor = -7050.0; // You'll need to calibrate this value

// Nextion display
NexText CF_Val = NexText(1, 2, "CF_Val"); // Page 1 (TestPage), Component ID 1, Name "CF_Val"

// Nextion initialization
NexTouch *nex_listen_list[] = {
  &CF_Val,
  NULL
};

int pot_pin = A0;
const int pot_power_pin = 13;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);

  // Initialize serial communication for Nextion display
  Serial2.begin(9600);

  // Initialize the scale
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); // Set the calibration factor
  scale.tare(); // Reset the scale to 0

  // Initialize Nextion display
  nexInit();

  // Initialize potentiometer power pin
  pinMode(pot_power_pin, OUTPUT);
  digitalWrite(pot_power_pin, HIGH); // Provide power to the potentiometer

  // Initialize potentiometer input pin
  pinMode(pot_pin, INPUT);

  Serial.println("Rocket Motor Test Stand Initialized");
  delay(2000); // Wait for the setup to stabilize
}

void loop() {
  // Get the weight from the load cell
  float weight = scale.get_units(10); // Get an average of 10 readings

  // Convert weight to grams
  float weightInGrams = weight * 1000.0;


  int Value1 = map(analogRead(pot_pin), 0, 1024, 0, 255);

  String Tosend = "add ";
  Tosend += "5, ";
  Tosend += "0, ";
  Tosend += Value1;



  // Print the weight to the serial monitor
  Serial.print("Weight: ");
  Serial.print(weightInGrams);
  Serial.println(" g");

  // Send the weight to the Nextion display
  String weightStr = String(Value1, 0); // Convert weight to string with 2 decimal places
  CF_Val.setText(weightStr.c_str());

  delay(500); // Update the weight reading every half second



  Serial2.print(Tosend);
  //Serial2.write(0xff);
  //Serial2.write(0xff);
 // Serial2.write(0xff);
}
