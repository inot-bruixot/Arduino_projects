#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_FT6206.h>


Adafruit_FT6206 ctp_0=Adafruit_FT6206();
#define tft_cs 15
#define tft_dc 2
#define tft_mosi 23
#define tft_sclk 18

TFT_eSPI tft=TFT_eSPI();



void setup() {

  Serial.begin (115200);
  tft.begin();
  
  if(!ctp_0.begin(40))
  {
    while(1);
  }

  tft.fillScreen(ILI9341_BLACK);

}



void loop() {

  if (!ctp_0.touched())
  {
    delay(10);
    return;  
  }
  
  TS_Point p=ctp_0.getPoint();
  Serial.print(p.x);
  Serial.print(",");
  Serial.print(p.y);
  Serial.println();

  delay(10);
}
