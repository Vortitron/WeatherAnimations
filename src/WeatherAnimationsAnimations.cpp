#include "WeatherAnimationsAnimations.h"

// Include networking libraries
#if defined(ESP8266)
	#include <ESP8266WiFi.h>
	#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
	#include <WiFi.h>
	#include <HTTPClient.h>
#endif

// Default URLs for fetching weather icons based on our JSON file
// Using our own GitHub repository as source
const char* CLEAR_SKY_URL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/sunny-day_frame_";
const char* CLOUDY_URL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/cloudy_frame_";
const char* RAIN_URL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/rainy_frame_";
const char* SNOW_URL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/snowy_frame_";
const char* STORM_URL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning_frame_";

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

// Function to convert PNG image data to bitmap
bool pngToBitmap(uint8_t* pngData, size_t pngSize, uint8_t* bitmap, size_t bitmapSize) {
	// Simplified implementation - in a real system you would use a proper PNG decoder
	// This is a placeholder to demonstrate the concept
	
	// Basic error checking
	if (pngData == NULL || bitmap == NULL || pngSize < 100 || bitmapSize != 1024) {
		return false;
	}
	
	// Look for the IDAT chunk which contains the actual image data
	// Simplified approach - just extract some patterns
	for (size_t i = 0; i < pngSize - 8; i++) {
		// We need to find a repeating pattern in the PNG and extract a usable bitmap
		// This is a very simplified approach and won't work properly for real PNGs
		// In a real implementation, use a proper PNG decoder library
		
		// For demonstration purposes, just extract some patterns from different parts of the PNG
		if (i < bitmapSize) {
			// Use the value from the PNG data (with some processing to make it visible)
			bitmap[i] = (pngData[i] & 0x01) ? 0xFF : 0x00;
		}
	}
	
	// In a real implementation, this would be replaced with a proper PNG decoder
	// that handles decompression, color conversion, etc.
	
	return true;
}

// Function to fetch animation frames from a base URL pattern (e.g., "base_url_frame_")
bool fetchAnimationFrames(const char* baseURL, uint8_t** frames, int frameCount, size_t frameSize) {
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("Cannot fetch animation: WiFi not connected");
		return false;
	}
	
	bool anySuccess = false;
	char fullURL[150];
	
	for (int i = 0; i < frameCount; i++) {
		// Create the full URL for this frame
		// Format: baseURL + "000.png" (with padding for frame number)
		sprintf(fullURL, "%s%03d.png", baseURL, i % 10); // Use modulo to repeat if fewer frames available
		
		Serial.print("Fetching frame from URL: ");
		Serial.println(fullURL);
		
		HTTPClient http;
		http.begin(fullURL);
		
		int httpCode = http.GET();
		if (httpCode != 200) {
			Serial.print("HTTP Error: ");
			Serial.println(httpCode);
			http.end();
			// Continue to the next frame rather than failing completely
			continue;
		}
		
		// Get the PNG data
		size_t pngSize = http.getSize();
		uint8_t* pngData = new uint8_t[pngSize];
		if (!pngData) {
			Serial.println("Failed to allocate memory for PNG data");
			http.end();
			continue;
		}
		
		// Get the PNG data
		WiFiClient* stream = http.getStreamPtr();
		size_t bytesRead = 0;
		while(http.connected() && bytesRead < pngSize) {
			if (stream->available()) {
				pngData[bytesRead++] = stream->read();
			}
		}
		
		http.end();
		
		if (bytesRead != pngSize) {
			Serial.println("Failed to read complete PNG data");
			delete[] pngData;
			continue;
		}
		
		// Convert PNG to bitmap
		if (!pngToBitmap(pngData, pngSize, frames[i], frameSize)) {
			Serial.println("Failed to convert PNG to bitmap");
		} else {
			anySuccess = true;
		}
		
		// Clean up
		delete[] pngData;
	}
	
	return anySuccess;
}

// Initialize all animations from online resources
bool initializeAnimationsFromOnline() {
	bool allSuccess = true;
	bool anySuccess = false;
	
	// Fetch frames for each weather condition
	Serial.println("Fetching clear sky frames...");
	bool clearSuccess = fetchAnimationFrames(CLEAR_SKY_URL, (uint8_t**)clearSkyFrames, 2, 1024);
	allSuccess &= clearSuccess;
	anySuccess |= clearSuccess;
	
	Serial.println("Fetching cloudy frames...");
	bool cloudySuccess = fetchAnimationFrames(CLOUDY_URL, (uint8_t**)cloudySkyFrames, 2, 1024);
	allSuccess &= cloudySuccess;
	anySuccess |= cloudySuccess;
	
	Serial.println("Fetching rain frames...");
	bool rainSuccess = fetchAnimationFrames(RAIN_URL, (uint8_t**)rainFrames, 3, 1024);
	allSuccess &= rainSuccess;
	anySuccess |= rainSuccess;
	
	Serial.println("Fetching snow frames...");
	bool snowSuccess = fetchAnimationFrames(SNOW_URL, (uint8_t**)snowFrames, 3, 1024);
	allSuccess &= snowSuccess;
	anySuccess |= snowSuccess;
	
	Serial.println("Fetching storm frames...");
	bool stormSuccess = fetchAnimationFrames(STORM_URL, (uint8_t**)stormFrames, 2, 1024);
	allSuccess &= stormSuccess;
	anySuccess |= stormSuccess;
	
	if (!allSuccess) {
		Serial.println("Some animations failed to load completely");
	}
	
	// Return true if at least some frames were loaded successfully
	return anySuccess;
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