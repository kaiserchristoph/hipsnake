#include <FastLED.h>
#include <stdint.h>
#include <vector>
#include <algorithm> // Für std::find
#include "bitmap.h"
#include "Button2.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>



#define LED_PIN     17
#define NUM_LEDS    256
#define BRIGHTNESS  15
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#define LEFT_PIN 18
#define RIGHT_PIN 19
//#define UP_PIN 26
//#define DOWN_PIN 19
#define SELECT_PIN 27

#define NeonGreen CRGB(0,255,0)
#define ElectricViolet CRGB(150,115,255)
#define Natural CRGB(250,245,245)


CRGB leds[NUM_LEDS];

// Params for width and height
const uint8_t kMatrixWidth = 16;
const uint8_t kMatrixHeight = 16;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
const bool    kMatrixVertical = false;



std::vector<std::pair<int, int>> Snake = {{2, 3}, {1, 3}, {0, 3}};
std::pair<int, int> Food;

Button2 button_left;
Button2 button_right;
//Button2 button_up;
//Button2 button_down;
Button2 button_select;

int vx = 1;
int vy = 0;
bool demo = true;
/////////////////////////////////////////////////////////////////

//TODO: snake stops for 1 frame when collecting food

void reset() {
  FastLED.clear();
  FastLED.show();
  Snake = {{2, 3}, {1, 3}, {0, 3}};
  vx = 1;
  vy = 0;
  Food = {8, 3};
}

void tap_left(Button2& btn) {
    Serial.println("tap left");
    if (demo==true)
    {
      demo = false;
      reset();
      return;
    }
    if (vy == 1) {
        vx = -1;
        vy = 0;
    } else if (vy == -1) {
        vx = 1;
        vy = 0;
    } else if (vx == 1) {
        vx = 0;
        vy = 1;
    } else if (vx == -1) {
        vx = 0;
        vy = -1;
    }
}

void tap_right(Button2& btn) {
    Serial.println("tap right");
    if (demo==true)
    {
      demo = false;
      reset();
      return;
    }
    if (vy == 1) {
        vx = 1;
        vy = 0;
    } else if (vy == -1) {
        vx = -1;
        vy = 0;
    } else if (vx == 1) {
        vx = 0;
        vy = -1;
    } else if (vx == -1) {
        vx = 0;
        vy = 1;
    }
}

void tap_up(Button2& btn) {
    Serial.println("tap up");
}

void tap_down(Button2& btn) {
    Serial.println("tap down");
}

void tap_select(Button2& btn) {
    Serial.println("tap select");
}

/////////////////////////////////////////////////////////////////





uint16_t XY( uint8_t x, uint8_t y) {
  uint16_t i;
  uint8_t temp = x;
  x = y;
  y = temp;
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


bool isinSnake(int x, int y) {
  for (std::pair<int, int> segment : Snake) {
    if (segment.first == x && segment.second == y) return true;
  }
  return false;
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
    do {
      int fx = random(0, kMatrixWidth);
      int fy = random(0, kMatrixHeight);
      Food = {fx, fy};
    } while (isinSnake(Food.first, Food.second));
    } else
  Snake.pop_back();
  return true;
}



void draw() {
  FastLED.clear();
  leds[XY(Food.first, Food.second)] = CRGB::Red;
  for (std::pair<int, int> segment : Snake) {
    leds[XY(segment.first, segment.second)] = CRGB::Green;
  }
  
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

// Function to draw a character from the JSON file onto the LED matrix
void drawChar(char c, int xStart, int yStart, CRGB color) {
  // Load the JSON file
  File file = SPIFFS.open("/slumbers.json", "r");
  if (!file) {
    Serial.println("Failed to open file");
    return;
  }

  // Allocate a temporary buffer to store the JSON data
  size_t size = file.size();
  std::unique_ptr<char[]> buf(new char[size]);

  // Read the file into the buffer
  file.readBytes(buf.get(), size);

  // Parse the JSON data
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  // Get the character's pixel data
  JsonObject glyph = doc["glyphs"][String(c)];
  if (glyph.isNull()) {
    Serial.println("Character not found in JSON");
    return;
  }

  JsonArray pixels = glyph["pixels"];
  int offsetX = glyph["offset"];

  // Clear the LED matrix
  //FastLED.clear();

  // Draw the character's pixels onto the LED matrix
  for (int y = 0; y < pixels.size(); y++) {
    JsonArray row = pixels[y];
    for (int x = 0; x < row.size(); x++) {
      if (row[x] == 1) {
        leds[XY(x+xStart + offsetX, pixels.size()-y+yStart-1)] = color;
      }
    }
  }

  // Show the updated LED matrix
  //FastLED.show();
}

void drawString(String s, int xStart, int yStart, CRGB color) {
  for (int i = 0; i < s.length(); i++) {
    drawChar(s[i], xStart + i * 4, yStart, color);
  }
}




CRGB* myImage = nullptr;
int imgW, imgH;
/**
 * Lädt ein 24-bit BMP direkt in ein FastLED CRGB Array.
 * @return Pointer auf CRGB Array (muss mit free() gelöscht werden)
 */
CRGB* loadBMPtoCRGB(const char* path, int* w, int* h) {
    File file = SPIFFS.open(path, "r");
    if (!file) return nullptr;

    uint8_t header[54];
    file.read(header, 54);

    if (header[0] != 'B' || header[1] != 'M') {
        file.close();
        return nullptr;
    }

    *w = *(int32_t*)&header[18];
    *h = *(int32_t*)&header[22];
    uint32_t dataOffset = *(uint32_t*)&header[10];

    // BMP Reihen-Größe inkl. Padding (4-Byte Alignment)
    int rowSize = (*w * 3 + 3) & ~3; 
    
    // Speicher für das FastLED Array reservieren
    CRGB* ledArray = (CRGB*) malloc((*w) * (*h) * sizeof(CRGB));
    if (!ledArray) {
        file.close();
        return nullptr;
    }

    file.seek(dataOffset);
    uint8_t* lineBuffer = (uint8_t*) malloc(rowSize);

    for (int y = 0; y < *h; y++) {
        file.read(lineBuffer, rowSize);
        
        // BMP ist Bottom-Up. Wir kehren es für die Matrix-Logik um.
        int targetY = (*h - 1 - y);
        
        for (int x = 0; x < *w; x++) {
            int srcIdx = x * 3;
            int destIdx = targetY * (*w) + x;

            // BMP (BGR) -> FastLED (RGB)
            // CRGB erlaubt direkten Zugriff auf .r, .g, .b
            ledArray[destIdx].r = lineBuffer[srcIdx + 2];
            ledArray[destIdx].g = lineBuffer[srcIdx + 1];
            ledArray[destIdx].b = lineBuffer[srcIdx];
        }
    }

    free(lineBuffer);
    file.close();
    return ledArray;
}


// Einfacher Zugriff auf x/y Koordinaten
void setPixel(CRGB* arr, int x, int y, int width, CRGB color) {
    arr[y * width + x] = color;
}

CRGB getPixel(CRGB* arr, int x, int y, int width) {
    return arr[y * width + x];
}



void drawCRGBArray(CRGB* arr, int width, int height) {
  FastLED.clear();

  
  for (int offset = 0; offset < width - kMatrixWidth; offset++) {
    for (int y = 0; y < kMatrixHeight; y++) {
      for (int x = 0; x < kMatrixWidth; x++) {
        leds[XY(x, y)] = getPixel(arr, x + offset, y, width);
      }
    }
    FastLED.show();
    delay(150);
  }
}



void drawDualColorArray(CRGB* arr, int width, int height, CRGB whiteColor, CRGB blackColor) {
  FastLED.clear();

  for (int offset = 0; offset < width - kMatrixWidth; offset++) {
    for (int y = 0; y < kMatrixHeight; y++) {
      for (int x = 0; x < kMatrixWidth; x++) {
        if (getPixel(arr, x + offset, y, width) == CRGB::White) {
            leds[XY(x, y)] = whiteColor;
        } else {
            leds[XY(x, y)] = blackColor;
        }
      }
    }
    FastLED.show();
    delay(150);
  }
}



void setup() {
  Serial.begin(115200);
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  button_left.begin(LEFT_PIN);
  button_right.begin(RIGHT_PIN);
  //button_up.begin(UP_PIN);
  //button_down.begin(DOWN_PIN);
  button_select.begin(SELECT_PIN);

  // setTapHandler() is called by any type of click, longpress or shortpress
  button_left.setTapHandler(tap_left);
  button_right.setTapHandler(tap_right);
  //button_up.setTapHandler(tap_up);
  //button_down.setTapHandler(tap_down);
  button_select.setTapHandler(tap_select);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  
  Food = {8, 3};

  Serial.println("setup done");

  // Zeichne das Logo
  //show_logo();
  //delay(10000);
  
  if (SPIFFS.begin()) {
        myImage = loadBMPtoCRGB("/39c3pixel.bmp", &imgW, &imgH);
        
    }



  // Zeichne die Schlange
  //draw();
  


}


void loop() {
  static unsigned long lastMoveTime = 0;
  unsigned long currentTime = millis();

    // if (myImage) {
    //     Serial.println("Bild erfolgreich geladen!");
    // }
    // else {
    //     Serial.println("Fehler beim Laden des Bildes!");
    // }

  if (currentTime - lastMoveTime >= 200) {
    lastMoveTime = currentTime;
      //drawCRGBArray(myImage, imgW, imgH);
      drawDualColorArray(myImage, imgW, imgH, CRGB::Black , CRGB::White);
      drawDualColorArray(myImage, imgW, imgH, CRGB::White, CRGB::Black);
      drawDualColorArray(myImage, imgW, imgH, CRGB::Black , NeonGreen);
      drawDualColorArray(myImage, imgW, imgH, NeonGreen, CRGB::Black);
      drawDualColorArray(myImage, imgW, imgH, CRGB::Black , ElectricViolet);
      drawDualColorArray(myImage, imgW, imgH, ElectricViolet , CRGB::Black);
 
      return;
    }
}
