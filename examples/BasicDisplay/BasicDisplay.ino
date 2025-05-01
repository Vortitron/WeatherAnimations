/*
 * Basic SH1106 OLED Display Example for ESP32
 * 
 * This is a minimal example that just displays text on the SH1106 OLED
 * without using the WeatherAnimations library. Use this to verify that
 * your display is working correctly.
 * 
 * Hardware:
 * - ESP32 board
 * - SH1106 OLED display (I2C, address 0x3C)
 * - Push buttons (optional)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Define button pins
const int encoderPUSH = 27; // Button to cycle through screens
const int backButton = 14;  // Back button 
const int leftButton = 12;  // Left button

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// I2C address for the OLED
#define OLED_ADDR 0x3C

// Create display instance
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Button state variables
bool lastEncoderPushState = HIGH;
unsigned long lastEncoderDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Display state
int displayState = 0;
const int NUM_STATES = 5;

// Weather types for display
const char* WEATHER_TYPES[] = {
  "CLEAR",
  "CLOUDY",
  "RAIN",
  "SNOW",
  "STORM"
};

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n===================================");
  Serial.println("Basic SH1106 OLED Display Example");
  Serial.println("===================================");
  
  // Initialize button pins
  pinMode(encoderPUSH, INPUT_PULLUP);
  pinMode(backButton, INPUT_PULLUP);
  pinMode(leftButton, INPUT_PULLUP);
  
  // Initialize display
  Serial.println("Initializing SH1106 OLED display...");
  
  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("SH1106 allocation failed");
    while (1); // Don't proceed if display init failed
  }
  
  Serial.println("OLED initialized successfully");
  
  // Show welcome message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("SH1106 OLED Test");
  display.println("----------------");
  display.println("Press button to");
  display.println("cycle through");
  display.println("weather types");
  display.display();
  
  Serial.println("Setup complete. Press the button to cycle through screens.");
}

void displayWeather(int weatherIndex) {
  display.clearDisplay();
  
  // Display header
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Weather Display");
  display.println("----------------");
  
  // Display weather type
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.println(WEATHER_TYPES[weatherIndex]);
  
  // Draw a simple icon based on weather
  switch (weatherIndex) {
    case 0: // CLEAR - draw a sun
      display.fillCircle(96, 32, 12, SH110X_WHITE);
      break;
    case 1: // CLOUDY - draw a cloud
      display.fillRoundRect(84, 30, 30, 15, 5, SH110X_WHITE);
      display.fillRoundRect(80, 20, 20, 15, 5, SH110X_WHITE);
      break;
    case 2: // RAIN - draw rain drops
      display.fillRoundRect(84, 20, 30, 15, 5, SH110X_WHITE);
      for (int i = 0; i < 4; i++) {
        display.drawLine(86 + i*8, 38, 90 + i*8, 45, SH110X_WHITE);
      }
      break;
    case 3: // SNOW - draw snowflakes
      display.fillRoundRect(84, 20, 30, 15, 5, SH110X_WHITE);
      for (int i = 0; i < 4; i++) {
        display.drawCircle(86 + i*8, 42, 2, SH110X_WHITE);
      }
      break;
    case 4: // STORM - draw lightning
      display.fillRoundRect(84, 20, 30, 15, 5, SH110X_WHITE);
      display.drawLine(100, 38, 94, 48, SH110X_WHITE);
      display.drawLine(94, 48, 100, 48, SH110X_WHITE);
      display.drawLine(100, 48, 94, 58, SH110X_WHITE);
      break;
  }
  
  // Display state number
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("State: ");
  display.print(displayState + 1);
  display.print("/");
  display.print(NUM_STATES);
  
  display.display();
  
  Serial.print("Displaying weather: ");
  Serial.println(WEATHER_TYPES[weatherIndex]);
}

void handleButtons() {
  // Read encoder button state
  bool encoderPushState = digitalRead(encoderPUSH);
  
  // Handle encoder button with debounce
  if (encoderPushState != lastEncoderPushState) {
    lastEncoderDebounceTime = millis();
  }
  
  if ((millis() - lastEncoderDebounceTime) > debounceDelay) {
    // If button is pressed (LOW)
    if (encoderPushState == LOW && lastEncoderPushState == HIGH) {
      Serial.println("Button pressed - changing display state");
      
      // Cycle to next state
      displayState = (displayState + 1) % NUM_STATES;
      
      // Update display
      displayWeather(displayState);
    }
  }
  
  // Update button state
  lastEncoderPushState = encoderPushState;
}

void loop() {
  // Handle button presses
  handleButtons();
  
  // Add a small delay to prevent excessive CPU usage
  delay(10);
} 