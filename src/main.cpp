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
#define BRIGHTNESS  10
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#define LEFT_PIN 18
#define RIGHT_PIN 19
//#define UP_PIN 26
//#define DOWN_PIN 19
#define SELECT_PIN 27


CRGB leds[NUM_LEDS];
CRGB background[NUM_LEDS];

// Params for width and height
const uint8_t kMatrixWidth = 16;
const uint8_t kMatrixHeight = 16;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
const bool    kMatrixVertical = false;



std::vector<std::pair<int, int>> Snake = {{2, 3}, {1, 3}, {0, 3}};
std::pair<int, int> Food;
CRGB foodColor = CRGB::Red;
CRGB snakeColor = CRGB::Green;
bool rainbow = false;
int foodType = random(0, 10);

Button2 button_left;
Button2 button_right;
//Button2 button_up;
//Button2 button_down;
Button2 button_select;

bool direction_pressed = false;

int vx = 1;
int vy = 0;
int frametime = 200; // Zeit zwischen den Bewegungen in Millisekunden
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
    //Serial.println("tap left");
    if (demo==true)
    {
      demo = false;
      reset();
      return;
    }
    //only change direction if not pressed yet
    if (!direction_pressed) {
      direction_pressed = true;
     
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
}

void tap_right(Button2& btn) {
    //Serial.println("tap right");
    if (demo==true)
    {
      demo = false;
      reset();
      return;
    }
    if (!direction_pressed) {
      direction_pressed = true;
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
          if (foodType < 8) {
        frametime = 200;
        snakeColor = CRGB::Green;
        rainbow = false;
      } else if (foodType == 8) {
        frametime = 150;
        snakeColor = CRGB::White;
        rainbow = false;
      } else if (foodType == 9) {
        frametime = 200;
        snakeColor = CRGB::Black;
        rainbow = true;
      }
    // Neues Food-Feld generieren
    do {
      int fx = random(0, kMatrixWidth);
      int fy = random(0, kMatrixHeight);

      foodType = random(0, 10);
      Food = {fx, fy};
      if (foodType < 8) {
        foodColor = CRGB::Red;
        //snakeColor = CRGB::Green;
      } else if (foodType == 8) {
        foodColor = CRGB::Blue;
        //snakeColor = CRGB::Green;
      }
      else if (foodType == 9) {
        foodColor = CRGB::White;
      }
    } while (isinSnake(Food.first, Food.second));
    } else
  Snake.pop_back();
  return true;
}



void draw() {
  FastLED.clear();
  if (rainbow) {
  memcpy(&leds[0], &background[0], NUM_LEDS * sizeof(CRGB));
  }
  direction_pressed = false;
  leds[XY(Food.first, Food.second)] = foodColor;
  for (std::pair<int, int> segment : Snake) {
    leds[XY(segment.first, segment.second)] = snakeColor;
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


void rainbow_wave(uint8_t thisSpeed, uint8_t deltaHue) {     // The fill_rainbow call doesn't support brightness levels.
 
// uint8_t thisHue = beatsin8(thisSpeed,0,255);                // A simple rainbow wave.
 uint8_t thisHue = beat8(thisSpeed,255);                     // A simple rainbow march.
  
 fill_rainbow(background, NUM_LEDS, thisHue, deltaHue);            // Use FastLED's fill_rainbow routine.
 
} // rainbow_wave()



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
  draw();


}

//load json file

void gameDemo() {
  if (Food.first > Snake[0].first && vx != -1) {
    vy = 0;
    vx = 1;
  } else if (Food.first < Snake[0].first && vx != 1) {
    vy = 0;
    vx = -1;
  } else if (Food.second > Snake[0].second && vy != -1) {
    vy = 1;
    vx = 0;
  } else if (Food.second < Snake[0].second && vy != 1) {
    vy = -1;
    vx = 0;
  }

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
 // pride();
 if (rainbow) {
  rainbow_wave(10, 10);}
  //copy background to leds
  memcpy(&leds[0], &background[0], NUM_LEDS * sizeof(CRGB));
  if (currentTime - lastMoveTime >= frametime) {
    lastMoveTime = currentTime;
    if (demo) gameDemo();
    boolean weiter = move_snake();
    if (!weiter) {
      leds[XY(Snake[0].first, Snake[0].second)] = CRGB::Red;
      FastLED.show();
      delay(1000);
      FastLED.clear();
      drawString("GAME", 0, 9, CRGB::Red);
      drawString("OVER", 0, 2, CRGB::Red);
      FastLED.show();
      delay(4000);
      FastLED.clear();
      drawString(String(Snake.size()-3)+"P", 1, 8, CRGB::Red);
      FastLED.show();
      delay(4000);
      show_logo();
      delay(3000);
      vy = 0;
      vx = 1;
      Snake = {{2, 3}, {1, 3}, {0, 3}};
      demo = true;
      return;
    }
    draw();
  }

  button_left.loop();
  button_right.loop();
  //button_up.loop();
  //button_down.loop();
  button_select.loop();
}
