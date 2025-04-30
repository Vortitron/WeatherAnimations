#include "WeatherAnimationsAnimations.h"

// Include networking libraries
#if defined(ESP8266)
	#include <ESP8266WiFi.h>
	#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
	#include <WiFi.h>
	#include <HTTPClient.h>
#endif

// Default URLs for fetching weather icons
// Using basmilius/weather-icons from GitHub as source
const char* CLEAR_SKY_ANIMATION_URL = "https://raw.githubusercontent.com/basmilius/weather-icons/dev/weather-icon-api/clear-day.json";
const char* CLOUDY_ANIMATION_URL = "https://raw.githubusercontent.com/basmilius/weather-icons/dev/weather-icon-api/cloudy.json";
const char* RAIN_ANIMATION_URL = "https://raw.githubusercontent.com/basmilius/weather-icons/dev/weather-icon-api/rain.json";
const char* SNOW_ANIMATION_URL = "https://raw.githubusercontent.com/basmilius/weather-icons/dev/weather-icon-api/snow.json";
const char* STORM_ANIMATION_URL = "https://raw.githubusercontent.com/basmilius/weather-icons/dev/weather-icon-api/thunderstorms.json";

// Temporary storage for online animation frames
// These will be populated from online resources when needed
uint8_t clearSkyFrame1[1024] = {0};
uint8_t clearSkyFrame2[1024] = {0};
const uint8_t* clearSkyFrames[2] = {clearSkyFrame1, clearSkyFrame2};

uint8_t cloudyFrame1[1024] = {0};
uint8_t cloudyFrame2[1024] = {0};
const uint8_t* cloudySkyFrames[2] = {cloudyFrame1, cloudyFrame2};

uint8_t rainFrame1[1024] = {0};
uint8_t rainFrame2[1024] = {0};
uint8_t rainFrame3[1024] = {0};
const uint8_t* rainFrames[3] = {rainFrame1, rainFrame2, rainFrame3};

uint8_t snowFrame1[1024] = {0};
uint8_t snowFrame2[1024] = {0};
uint8_t snowFrame3[1024] = {0};
const uint8_t* snowFrames[3] = {snowFrame1, snowFrame2, snowFrame3};

uint8_t stormFrame1[1024] = {0};
uint8_t stormFrame2[1024] = {0};
const uint8_t* stormFrames[2] = {stormFrame1, stormFrame2};

// Function to convert hex string to bitmap data
bool hexToBitmap(const String& hexString, uint8_t* bitmap, size_t bitmapSize) {
	if (hexString.length() / 2 != bitmapSize) {
		return false;
	}
	
	for (size_t i = 0; i < bitmapSize; i++) {
		String byteStr = hexString.substring(i * 2, i * 2 + 2);
		bitmap[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
	}
	
	return true;
}

// Function to fetch animation data from a URL
bool fetchAnimationData(const char* url, uint8_t** frames, int frameCount, size_t frameSize) {
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("Cannot fetch animation: WiFi not connected");
		return false;
	}
	
	Serial.print("Fetching animation from URL: ");
	Serial.println(url);
	
	HTTPClient http;
	http.begin(url);
	
	int httpCode = http.GET();
	if (httpCode != 200) {
		Serial.print("HTTP Error: ");
		Serial.println(httpCode);
		http.end();
		return false;
	}
	
	String payload = http.getString();
	http.end();
	
	if (payload.length() < 20) {
		Serial.println("Response too short to be valid");
		return false;
	}
	
	Serial.print("Received data of length: ");
	Serial.println(payload.length());
	
	// Simple JSON parsing to extract frame data
	// In a real implementation, use a proper JSON parser like ArduinoJson
	
	// Example expected format:
	// {"frames": ["HEXDATA1", "HEXDATA2", ...]}
	
	int framesStart = payload.indexOf("\"frames\":[");
	if (framesStart == -1) {
		Serial.println("Could not find frames array in response");
		return false;
	}
	
	framesStart = payload.indexOf("[", framesStart);
	int framesEnd = payload.indexOf("]", framesStart);
	
	if (framesStart == -1 || framesEnd == -1) {
		Serial.println("Invalid frames array format");
		return false;
	}
	
	String framesArray = payload.substring(framesStart + 1, framesEnd);
	
	// Split by commas and extract up to frameCount frames
	int framesParsed = 0;
	int lastPos = 0;
	while (framesParsed < frameCount) {
		int quoteStart = framesArray.indexOf("\"", lastPos);
		if (quoteStart == -1) break;
		
		int quoteEnd = framesArray.indexOf("\"", quoteStart + 1);
		if (quoteEnd == -1) break;
		
		String frameHex = framesArray.substring(quoteStart + 1, quoteEnd);
		
		if (frameHex.length() < frameSize * 2) {
			Serial.print("Frame data too short: ");
			Serial.print(frameHex.length());
			Serial.print(" expected: ");
			Serial.println(frameSize * 2);
			
			// Fill with some recognizable pattern if data is corrupted
			for (size_t i = 0; i < frameSize; i++) {
				// Alternating pattern for visibility
				((uint8_t*)frames[framesParsed])[i] = (i % 2) ? 0xAA : 0x55;
			}
		} else {
			// Convert hex string to binary data
			if (!hexToBitmap(frameHex, (uint8_t*)frames[framesParsed], frameSize)) {
				Serial.println("Failed to convert hex data to bitmap");
				// Don't return false here, try to parse other frames
			}
		}
		
		framesParsed++;
		lastPos = quoteEnd + 1;
	}
	
	Serial.print("Successfully parsed ");
	Serial.print(framesParsed);
	Serial.print(" animation frames out of ");
	Serial.println(frameCount);
	
	return framesParsed > 0;
}

// Initialize all animations from online resources
bool initializeAnimationsFromOnline() {
	bool success = true;
	
	// Fetch all animation frames
	success &= fetchAnimationData(CLEAR_SKY_ANIMATION_URL, (uint8_t**)clearSkyFrames, 2, 1024);
	success &= fetchAnimationData(CLOUDY_ANIMATION_URL, (uint8_t**)cloudySkyFrames, 2, 1024);
	success &= fetchAnimationData(RAIN_ANIMATION_URL, (uint8_t**)rainFrames, 3, 1024);
	success &= fetchAnimationData(SNOW_ANIMATION_URL, (uint8_t**)snowFrames, 3, 1024);
	success &= fetchAnimationData(STORM_ANIMATION_URL, (uint8_t**)stormFrames, 2, 1024);
	
	return success;
}

// Fallback function to generate simple animations if online fetch fails
void generateFallbackAnimations() {
	// Clear memory first
	for (int i = 0; i < 1024; i++) {
		clearSkyFrame1[i] = 0;
		clearSkyFrame2[i] = 0;
		cloudyFrame1[i] = 0;
		cloudyFrame2[i] = 0;
		rainFrame1[i] = 0;
		rainFrame2[i] = 0;
		rainFrame3[i] = 0;
		snowFrame1[i] = 0;
		snowFrame2[i] = 0;
		snowFrame3[i] = 0;
		stormFrame1[i] = 0;
		stormFrame2[i] = 0;
	}
	
	// ===== CLEAR SKY ANIMATION =====
	// Draw a simple sun in the middle
	// Circle for sun
	for (int y = 20; y < 44; y++) {
		for (int x = 52; x < 76; x++) {
			// Calculate distance from center
			int centerX = 64;
			int centerY = 32;
			int dx = x - centerX;
			int dy = y - centerY;
			float distance = sqrt(dx*dx + dy*dy);
			
			// Draw if within 10 pixels from center
			if (distance <= 10) {
				int bytePos = y * 16 + x / 8;
				int bitPos = 7 - (x % 8);
				clearSkyFrame1[bytePos] |= (1 << bitPos);
				clearSkyFrame2[bytePos] |= (1 << bitPos);
			}
		}
	}
	
	// Add rays to frame 2 (8 rays)
	int rays = 8;
	float angleStep = 2 * PI / rays;
	for (int ray = 0; ray < rays; ray++) {
		float angle = ray * angleStep;
		// Inner point of ray
		int innerX = 64 + cos(angle) * 12;
		int innerY = 32 + sin(angle) * 12;
		// Outer point of ray
		int outerX = 64 + cos(angle) * 18;
		int outerY = 32 + sin(angle) * 18;
		
		// Draw line from inner to outer
		for (float t = 0; t <= 1.0; t += 0.1) {
			int x = innerX + (outerX - innerX) * t;
			int y = innerY + (outerY - innerY) * t;
			
			if (x >= 0 && x < 128 && y >= 0 && y < 64) {
				int bytePos = y * 16 + x / 8;
				int bitPos = 7 - (x % 8);
				clearSkyFrame2[bytePos] |= (1 << bitPos);
			}
		}
	}
	
	// ===== CLOUDY ANIMATION =====
	// Draw cloud shapes
	// First cloud (left side)
	drawCloud(30, 25, 35, 15, cloudyFrame1);
	drawCloud(35, 25, 30, 15, cloudyFrame2);
	
	// Second cloud (right side)
	drawCloud(85, 30, 40, 15, cloudyFrame1);
	drawCloud(80, 30, 40, 15, cloudyFrame2);
	
	// ===== RAIN ANIMATION =====
	// Draw cloud with rain drops
	drawCloud(64, 20, 50, 15, rainFrame1);
	drawCloud(64, 20, 50, 15, rainFrame2);
	drawCloud(64, 20, 50, 15, rainFrame3);
	
	// Add rain drops to different positions in each frame
	// Frame 1 - first set of drops
	for (int i = 0; i < 5; i++) {
		int dropX = 40 + i * 15;
		int dropY = 45;
		drawRainDrop(dropX, dropY, rainFrame1);
	}
	
	// Frame 2 - drops further down
	for (int i = 0; i < 5; i++) {
		int dropX = 40 + i * 15;
		int dropY = 50;
		drawRainDrop(dropX, dropY, rainFrame2);
	}
	
	// Frame 3 - drops at bottom
	for (int i = 0; i < 5; i++) {
		int dropX = 40 + i * 15;
		int dropY = 55;
		drawRainDrop(dropX, dropY, rainFrame3);
	}
	
	// ===== SNOW ANIMATION =====
	// Draw cloud with snowflakes
	drawCloud(64, 20, 50, 15, snowFrame1);
	drawCloud(64, 20, 50, 15, snowFrame2);
	drawCloud(64, 20, 50, 15, snowFrame3);
	
	// Add snowflakes to different positions in each frame
	// Frame 1 - first set of flakes
	for (int i = 0; i < 5; i++) {
		int flakeX = 40 + i * 15;
		int flakeY = 45;
		drawSnowflake(flakeX, flakeY, snowFrame1);
	}
	
	// Frame 2 - flakes in different position
	for (int i = 0; i < 5; i++) {
		int flakeX = 35 + i * 15;
		int flakeY = 50;
		drawSnowflake(flakeX, flakeY, snowFrame2);
	}
	
	// Frame 3 - flakes at bottom
	for (int i = 0; i < 5; i++) {
		int flakeX = 40 + i * 15;
		int flakeY = 55;
		drawSnowflake(flakeX, flakeY, snowFrame3);
	}
	
	// ===== STORM ANIMATION =====
	// Draw dark cloud
	drawCloud(64, 20, 60, 20, stormFrame1);
	drawCloud(64, 20, 60, 20, stormFrame2);
	
	// Add lightning bolt in different positions
	drawLightning(55, 40, stormFrame1);
	drawLightning(75, 38, stormFrame2);
}

// Helper function to draw a cloud shape
void drawCloud(int centerX, int centerY, int width, int height, uint8_t* buffer) {
	// Draw a rounded rectangle for the cloud
	for (int y = centerY - height/2; y <= centerY + height/2; y++) {
		for (int x = centerX - width/2; x <= centerX + width/2; x++) {
			// Skip corners to make it rounded
			int dx = abs(x - centerX) - width/2;
			int dy = abs(y - centerY) - height/2;
			
			if (dx*dx + dy*dy <= 100 && x >= 0 && x < 128 && y >= 0 && y < 64) {
				int bytePos = y * 16 + x / 8;
				int bitPos = 7 - (x % 8);
				buffer[bytePos] |= (1 << bitPos);
			}
		}
	}
}

// Helper function to draw a rain drop
void drawRainDrop(int x, int y, uint8_t* buffer) {
	// Simple oval shape for a drop
	for (int dy = -2; dy <= 2; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			if (dx*dx*2 + dy*dy <= 5 && x+dx >= 0 && x+dx < 128 && y+dy >= 0 && y+dy < 64) {
				int bytePos = (y+dy) * 16 + (x+dx) / 8;
				int bitPos = 7 - ((x+dx) % 8);
				buffer[bytePos] |= (1 << bitPos);
			}
		}
	}
}

// Helper function to draw a snowflake
void drawSnowflake(int x, int y, uint8_t* buffer) {
	// Simple asterisk shape for a snowflake
	for (int dy = -2; dy <= 2; dy++) {
		for (int dx = -2; dx <= 2; dx++) {
			if ((dx == 0 || dy == 0 || abs(dx) == abs(dy)) && 
				x+dx >= 0 && x+dx < 128 && y+dy >= 0 && y+dy < 64) {
				int bytePos = (y+dy) * 16 + (x+dx) / 8;
				int bitPos = 7 - ((x+dx) % 8);
				buffer[bytePos] |= (1 << bitPos);
			}
		}
	}
}

// Helper function to draw a lightning bolt
void drawLightning(int x, int y, uint8_t* buffer) {
	// Points for a zigzag lightning shape
	int points[7][2] = {
		{x, y},
		{x-3, y+5},
		{x+2, y+10},
		{x-2, y+15},
		{x+3, y+20},
		{x, y+25},
		{x+5, y+15}
	};
	
	// Draw lines between points
	for (int i = 0; i < 6; i++) {
		int x1 = points[i][0];
		int y1 = points[i][1];
		int x2 = points[i+1][0];
		int y2 = points[i+1][1];
		
		// Draw line using Bresenham's algorithm
		int dx = abs(x2 - x1);
		int dy = abs(y2 - y1);
		int sx = (x1 < x2) ? 1 : -1;
		int sy = (y1 < y2) ? 1 : -1;
		int err = dx - dy;
		
		while (true) {
			if (x1 >= 0 && x1 < 128 && y1 >= 0 && y1 < 64) {
				int bytePos = y1 * 16 + x1 / 8;
				int bitPos = 7 - (x1 % 8);
				buffer[bytePos] |= (1 << bitPos);
			}
			
			if (x1 == x2 && y1 == y2) break;
			int e2 = 2 * err;
			if (e2 > -dy) {
				err -= dy;
				x1 += sx;
			}
			if (e2 < dx) {
				err += dx;
				y1 += sy;
			}
		}
	}
} 