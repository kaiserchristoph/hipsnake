#include <FastLED.h>
#include <stdint.h>
#include <vector>
#include <algorithm> // Für std::find
#include "bitmap.h"

#define LED_PIN     2
#define NUM_LEDS    256
#define BRIGHTNESS  255
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// Params for width and height
const uint8_t kMatrixWidth = 16;
const uint8_t kMatrixHeight = 16;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
const bool    kMatrixVertical = false;

std::vector<std::pair<int, int>> Snake = {{2, 3}, {1, 3}, {0, 3}};
std::pair<int, int> Food;

int vx = 1;
int vy = 0;


uint16_t XY( uint8_t x, uint8_t y) {
  uint16_t i;
  
  if( kMatrixSerpentineLayout == false) {
    if (kMatrixVertical == false) {
      i = (y * kMatrixWidth) + x;
    } else {
      i = kMatrixHeight * (kMatrixWidth - (x+1))+y;
    }
  }
 
  if( kMatrixSerpentineLayout == true) {
    if (kMatrixVertical == false) {
      if( y & 0x01) {
        // Odd rows run backwards
        uint8_t reverseX = (kMatrixWidth - 1) - x;
        i = (y * kMatrixWidth) + reverseX;
      } else {
        // Even rows run forwards
        i = (y * kMatrixWidth) + x;
      }
    } else { // vertical positioning
      if ( x & 0x01) {
        i = kMatrixHeight * (kMatrixWidth - (x+1))+y;
      } else {
        i = kMatrixHeight * (kMatrixWidth - x) - (y+1);
      }
    }
  }
  return i;
}


boolean move_snake() {
  int x = Snake[0].first + vx;
  int y = Snake[0].second + vy;
  std::pair<int, int> head = std::make_pair(x, y);

  // Überprüfen, ob der neue Kopf außerhalb des Spielfelds ist
  if (x < 0 || x >= kMatrixWidth || y < 0 || y >= kMatrixHeight) return false;

  // Überprüfen, ob der neue Kopf bereits in der Snake-Liste vorkommt
  if (std::find(Snake.begin(), Snake.end(), head) != Snake.end()) return false;

  // Kopf zur Snake-Liste hinzufügen und das letzte Segment entfernen
  Snake.insert(Snake.begin(), head);

  // Überprüfen, ob der Kopf auf das Food-Feld trifft
  if (head == Food) {
    // Neues Food-Feld generieren
      int fx = random(0, kMatrixWidth);
      int fy = random(0, kMatrixHeight);
      Food = {fx, fy};
      vx = 0;
      vy = 1;
    } else
  Snake.pop_back();
  return true;
}

void draw() {
  FastLED.clear();
  for (std::pair<int, int> segment : Snake) {
    leds[XY(segment.first, segment.second)] = CRGB::Green;
  }
  leds[XY(Food.first, Food.second)] = CRGB::Red;
  FastLED.show();
}

void show_logo(){
  memcpy(&leds[0], &bitmap[0], NUM_LEDS * sizeof(CRGB));
  FastLED.show();
}


void wait_for_serial_connection() {
  uint32_t timeout_end = millis() + 2000;
  Serial.begin(115200);
  while(!Serial && timeout_end > millis()) {}  //wait until the connection to the PC is established
  
}

void setup() {
  Serial.begin(115200);
  wait_for_serial_connection(); // Optional, but seems to help Teensy out a lot.
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  
  Food = {8, 3};
  // Zeichne das Logo
  //show_logo();
  //delay(10000);
  
  // Zeichne die Schlange
  draw();
}

void loop() {
  boolean weiter = move_snake();
  if (!weiter) {
    // Behandle das Ende des Spiels oder die Kollision
    leds[XY(Snake[0].first, Snake[0].second)] = CRGB::Red;
    FastLED.show();
    delay(300);
    show_logo();
    delay(3000);
    // fill_rainbow_circular(leds, NUM_LEDS, 0);
    // FastLED.show();
    // delay(20000);
     return;
  }
  draw();
  delay(200);
}
