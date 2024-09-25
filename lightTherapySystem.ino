/*****************************************************************************************************************
 * Light Therapy System firmware 
 * ****************************************************************************************************************
 * Author:  
 * Name: Dilliri Alibeku
 * email: dillirialibeku@gmail.com  
 * web: dillirialibeku.online
 *****************************************************************************************************************/

/**************************************************************************************************************
 * Includes
 **************************************************************************************************************/
#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>  // Include the ArduinoJson library

#define SECRET_SSID "iPhone"             // WiFi SSID
#define SECRET_PASS "Dilliri05" // WiFi Password
#define modePin 13 // tactile switch for mode control
#define timerPin 12 // tactile switch for time control

/**************************************************************************************************************
 * WiFi Client and Server Params
 **************************************************************************************************************/ 
unsigned long serverPrevMs = 0;  // server previous millis
char ssid[] = SECRET_SSID;       // your network SSID
char pass[] = SECRET_PASS;       // your network password
int status = WL_IDLE_STATUS;     // WiFi status
WiFiClient client;
/**************************************************************************************************************
 * Light Therapy system variables
 **************************************************************************************************************/
const int ledPins[5] = {9, 9, 5, 6, 10};  // LED strip pins
volatile bool sw1 = LOW, sw2 = LOW, swx = LOW; // tactile control switches
unsigned int mode = 0, submode = 0, ts = 1, shift = 0, rgbInd = 0, lum = 255; 
unsigned long ltPrevMs = 1000;
unsigned long waitTime = 20000;  // Initial wait time
bool dataFetched = false;        // Track if data was successfully fetched

/******************************************************************
 * WiFi Initialization
 ******************************************************************/
void initWifi() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass); 
    delay(10000); 
  }
  Serial.println("Connected to WiFi");
}

/******************************************************************
 * Fetch Data from ThingSpeak API
 ******************************************************************/
String sendRequest() {
  String serverMsg = "";
  bool jsonStarted = false;  
  if (client.connect("api.thingspeak.com", 80)) {
    Serial.println("Connected to ThingSpeak");

    client.println("GET /channels/2597694/feeds.json?api_key=42AGWSL2KV96YS24&results=1 HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println();
    
    delay(1000);  
    while (client.available()) {
      char c = client.read();
      if (!jsonStarted) {
        if (c == '{') {
          jsonStarted = true; 
        }
      }
      if (jsonStarted) {
        serverMsg += c;
      }
    }
    if (!client.connected()) {
      client.stop();
    }
    Serial.println("Cleaned JSON Response: ");
  } else {
    Serial.println("Failed to connect to ThingSpeak.");
  }

  return serverMsg;
}

/******************************************************************
 * JSON Parsing and Integer Conversion
 ******************************************************************/
bool parseJson(String jsonString, int &modeValue, int &submodeValue) {
  StaticJsonDocument<2048> doc;  
  if (jsonString.length() == 0) {
    Serial.println("Empty JSON string.");
    return false;
  }
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    return false;
  }

  serializeJsonPretty(doc, Serial);
  JsonArray feeds = doc["feeds"];
  if (feeds.size() > 0) {
    JsonObject feed = feeds[0];  
    String modeStr = feed["field1"].as<String>();
    String submodeStr = feed["field2"].as<String>();
    modeValue = modeStr.toInt();
    submodeValue = submodeStr.toInt();

    Serial.println("Parsed Mode: " + String(modeValue));
    Serial.println("Parsed Submode: " + String(submodeValue));
    return true;
  } else {
    Serial.println("Feeds array is empty or missing.");
  }

  return false;
}

/******************************************************************
 * Light Therapy System Initialization
 ******************************************************************/
void initLT() {
  for (int i = 0; i < 5; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  pinMode(modePin, INPUT);
  pinMode(timerPin, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(modePin), changeMode, CHANGE);
  attachInterrupt(digitalPinToInterrupt(timerPin), changeTs, CHANGE);

  Serial.println("Light Therapy System Initialized");
  delay(1000);
  mode = 0;
  ts = 1;
  rgbInd = 0;
  shift = 0;
  ltPrevMs = millis();
  Serial.println("Initial Mode: " + String(mode));
  // Show default light as active (on pin 9)
  showActiveStatus();  // Turn on pin 9 with a default brightness to show activity
}

// Function to display the active state using pin 9 before WiFi and data parsing
void showActiveStatus() {
  pinMode(9, OUTPUT);  // Set pin 9 as OUTPUT
  analogWrite(9, 50); // Set pin 9 (status LED) to full brightness
}

// Function to turn off the active state LED on pin 9
void turnOffActiveStatus() {
  analogWrite(9, 0);  // Turn off the status LED
}
/******************************************************************
 * Switch pin 9 to RGB control after status is done
 ******************************************************************/
void switchPin9ToRGBControl() {
  // Set pin 9 as part of the RGB setup after WiFi and data fetch is done
  pinMode(ledPins[0], OUTPUT);  // Reset pin 9 to RGB use
}

/**************************************************************************************************************
 * Helper Functions for LED Color Control
 **************************************************************************************************************/
void RGB(int code = 0, int brightness = 0) {
  int red = 0, green = 0, blue = 0;

  // Set the base RGB values based on the code (color index)
  switch (code) {
    case 0:  // Red + Blue
      red = 255; 
      blue = 255;
      break;
    case 1:  // Green + Blue
      green = 255; 
      blue = 255;
      break;
    case 2:  // Red + Green
      red = 255; 
      green = 255;
      break;
    case 3:  // Green
      green = 255;
      break;
    case 4:  // Red
      red = 255;
      break;
    case 5:  // Blue
      blue = 255;
      break;
    case 6:  // RGB (White)
      red = 255; 
      green = 255; 
      blue = 255;
      break;
    default:  // Turn off all LEDs
      red = 0; 
      green = 0; 
      blue = 0;
      break;
  }

  // Scale the color intensities by the brightness factor
  red = (red * brightness) / 255;
  green = (green * brightness) / 255;
  blue = (blue * brightness) / 255;

  // Write the scaled brightness to the respective LED pins
  analogWrite(ledPins[2], red);    // Red channel
  analogWrite(ledPins[3], green);  // Green channel
  analogWrite(ledPins[4], blue);   // Blue channel
}

/**************************************************************************************************************
 * Interrupt Handlers
 **************************************************************************************************************/
void changeTs(void) {
  sw2 = HIGH;
  swx = HIGH;
}

void changeMode(void) {
  sw1 = HIGH;
  swx = HIGH;
}
/******************************************************************
 * Updated Light Patterns for All Mode Ranges
 ******************************************************************/
unsigned long effectStartTime = 0;  // Start time for each effect
unsigned int currentEffect = 0;     // Index for the current effect

const unsigned long effectDuration = 180000;  // 3 minutes in milliseconds (3 * 60 * 1000)

// Function to run effects in sequence based on mode range
void runEffectsSequentially(void (*effects[])(), int numEffects) {
  unsigned long currentMillis = millis();

  // Check if it's time to switch to the next effect (after 3 minutes)
  if (currentMillis - effectStartTime >= effectDuration) {
    currentEffect = (currentEffect + 1) % numEffects;  // Cycle through effects
    effectStartTime = currentMillis;  // Reset the start time for the new effect
    Serial.println("Switched to effect: " + String(currentEffect));
  }

  // Run the current effect with the fixed RGB color (rgbInd)
  effects[currentEffect]();
}
// Calming Wave (wave-like LED brightness effect)
void calmingWave() {
    static int brightness = 0;
    static int fadeDirection = 1;
    static unsigned long lastUpdate = 0;
    
    if (millis() - lastUpdate >= 5) {  // Smooth transition every 5ms
        Serial.println("CalmingWave - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Set brightness for calming wave

        brightness += fadeDirection * 5;
        if (brightness >= 255) {
            brightness = 255;  // Cap at max brightness
            fadeDirection = -1;  // Reverse to decrease
        } else if (brightness <= 0) {
            brightness = 0;  // Ensure brightness doesn't go below 0
            fadeDirection = 1;  // Start increasing again
        }
        lastUpdate = millis();
    }
}

// Gradual Illumination on the fixed color
void gradualIllumination() {
    static int brightness = 0;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 10) {  // Smooth transition every 10ms
        Serial.println("GradualIllumination - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        brightness += 5;
        if (brightness >= 255) brightness = 0;  // Reset brightness after full cycle
        lastUpdate = millis();
    }
}

// Rhythmic Stimulation on the fixed color
void rhythmicStimulation() {
    static bool ledState = false;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 25) {  // 25ms on/off flash
        int brightness = ledState ? 255 : 0;
        Serial.println("RhythmicStimulation - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        ledState = !ledState;  // Toggle LED state
        lastUpdate = millis();
    }
}

// Soft Calming Wave (wave-like LED brightness effect on the current RGB color)
void softCalmingWave() {
    static int brightness = 0;
    static int fadeDirection = 1;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 10) {  // Smoother transitions
        Serial.println("SoftCalmingWave - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        brightness += fadeDirection * 5;
        if (brightness >= 255) {
            brightness = 255;
            fadeDirection = -1;
        } else if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;
        }
        lastUpdate = millis();
    }
}

// Relaxing Fade (slow fade in and out on the current RGB color)
void relaxingFade() {
    static int brightness = 0;
    static int fadeDirection = 1;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 10) {  // Slower transition every 10ms
        Serial.println("RelaxingFade - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        brightness += fadeDirection * 5;
        if (brightness >= 255) {
            brightness = 255;
            fadeDirection = -1;
        } else if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;
        }
        lastUpdate = millis();
    }
}

// Soothing Pulse (single LED pulse on the current RGB color)

void soothingPulse() {
    static int brightness = 0;
    static int fadeDirection = 1;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 5) {  // Smooth pulse every 5ms
        Serial.println("SoothingPulse - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        brightness += fadeDirection * 5;
        if (brightness >= 255) {
            brightness = 255;
            fadeDirection = -1;
        } else if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;
        }
        lastUpdate = millis();
    }
}

// Brightening Pattern on the current RGB color
void brighteningPattern() {
  static int brightness = 0;  // Start with zero brightness for a full cycle
  static unsigned long lastUpdate = 0;

  // Increase brightness smoothly every 10 milliseconds
  if (millis() - lastUpdate >= 10) {  
      Serial.println("BrighteningPattern - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
      RGB(rgbInd, brightness);  // Apply brightness to the current RGB color (rgbInd)

      brightness += 1;  // Gradual increase in brightness for smoother transitions
      if (brightness > 255) brightness = 0;  // Reset after reaching full brightness
      lastUpdate = millis();
  }
}

// Stimulating Pulse on the current RGB color
void stimulatingPulse() {
    static int brightness = 0;
    static int fadeDirection = 1;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 5) {  // 5ms delay for faster pulse
        Serial.println("StimulatingPulse - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        brightness += fadeDirection * 5;
        if (brightness >= 255) {
            brightness = 255;
            fadeDirection = -1;
        } else if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;
        }
        lastUpdate = millis();
    }
}

// Gradual Exposure on the current RGB color
void gradualExposure() {
    static int brightness = 0;
    static int fadeDirection = 1;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 10) {  // 10ms delay for smooth exposure
        Serial.println("GradualExposure - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        brightness += fadeDirection * 5;
        if (brightness >= 255) {
            brightness = 255;
            fadeDirection = -1;
        } else if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;
        }
        lastUpdate = millis();
    }
}

// Rhythmic Stimulation (long) on the current RGB color
void rhythmicStimulationLong() {
    static bool ledState = false;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 50) {  // 50ms on/off for slower rhythmic flash
        int brightness = ledState ? 255 : 0;
        Serial.println("RhythmicStimulationLong - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        ledState = !ledState;  // Toggle LED state
        lastUpdate = millis();
    }
}

// Alternating Pattern on the current RGB color
void alternatingPattern() {
    static bool ledState = false;
    static unsigned long lastUpdate = 0;
    static int ledIndex = 0;

    if (millis() - lastUpdate >= 25) {  // 25ms on/off alternating
        int brightness = ledState ? 255 : 0;
        Serial.println("AlternatingPattern - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(ledIndex, brightness);  // Apply effect on the current LED, based on ledIndex

        ledState = !ledState;  // Toggle LED state
        ledIndex = (ledIndex + 1) % 6;  // Move to the next LED
        lastUpdate = millis();
    }
}

// Soft Pulse on the current RGB color
void softPulse() {
    static int brightness = 0;
    static int fadeDirection = 1;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 5) {  // 5ms delay for smooth pulse
        Serial.println("SoftPulse - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        brightness += fadeDirection * 5;
        if (brightness >= 255) {
            brightness = 255;
            fadeDirection = -1;
        } else if (brightness <= 0) {
            brightness = 0;
            fadeDirection = 1;
        }
        lastUpdate = millis();
    }
}

// Dimming Pattern on the current RGB color
void dimmingPattern() {
    static int brightness = 255;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 5) {  // 5ms delay for dimming
        Serial.println("DimmingPattern - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        brightness -= 5;
        if (brightness <= 0) brightness = 255;  // Reset brightness after dimming
        lastUpdate = millis();
    }
}

// Melatonin Stimulation on the current RGB color
void melatoninStimulation() {
    static bool ledState = false;
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate >= 25) {  // 25ms on/off rhythmic stimulation
        int brightness = ledState ? 255 : 0;
        Serial.println("MelatoninStimulation - rgbInd: " + String(rgbInd) + " Brightness: " + String(brightness));
        RGB(rgbInd, brightness);  // Apply effect on the current RGB color (rgbInd)

        ledState = !ledState;  // Toggle LED state
        lastUpdate = millis();
    }
}

// Satisfaction Pattern (solid cold white + faint red)
void satisfactionPattern() {
    RGB(6, 255);  // Solid cold white + faint red
    delay(100);   // Hold pattern for 100ms
}

// Happiness Pattern (solid RGB + cold white + faint red)
void happinessPattern() {
    RGB(6, 255);  // Solid RGB (white + cold white + faint red)
    delay(100);   // Hold pattern for happiness
}


/******************************************************************
 * Fetch data and control timing
 ******************************************************************/
void fetchDataAndControlTiming() {
  unsigned long currentMillis = millis();
  if (currentMillis - serverPrevMs >= waitTime) {
    String jsonString = sendRequest();
    if (jsonString.length() > 0) {
      int fetchedMode = 0, fetchedSubmode = 0;
      if (parseJson(jsonString, fetchedMode, fetchedSubmode)) {
        Serial.println("Mode: " + String(fetchedMode) + " Submode: " + String(fetchedSubmode));
        lightTherapyMain(fetchedMode, fetchedSubmode); // Pass both mode and submode
      }
      waitTime = 1020000;
    } else {
      waitTime = 20000;
    }
    serverPrevMs = currentMillis;
  }
}

/******************************************************************
 * Main Light Therapy Logic
 ******************************************************************/
void lightTherapyMain(int modeValue, int submodeValue) {
  // Turn off the status LED once the data is fetched and parsed
  turnOffActiveStatus();
  
 switchPin9ToRGBControl();  // Switch pin 9 back to RGB control after status


  mode = modeValue;
  submode = submodeValue;

  if ((millis() - ltPrevMs) >= 60000) {
    ltPrevMs = millis();
    shift++;
    onTick();
  }

  if (swx) {
    onTick();
  }

  emotionDamper();
}

/******************************************************************
 * Interrupt Handlers
 ******************************************************************/
void onTick() {
  delay(500);
  
  if (sw1) {
    mode++;
    submode++;
  }
  if (sw2) {
    ts++;
  }

  if (mode > 40) {
    mode = 0;
  }
  if (ts < 1 || ts > 10) {
    ts = 1;
  }
  if (shift > 4) {
    shift = 0;
  }

  Serial.println("Mode: " + String(mode) + " Submode: " + String(submode));
  sw1 = LOW; 
  sw2 = LOW;
  swx = LOW;
  emotionDamper();
}
/**************************************************************************************************************
 * Set Intensity
 **************************************************************************************************************/
void setIntensity(int maxMode = 10) {
  if (submode > maxMode || submode < 1) submode = 1;
  lum = 256 - submode * 255 / maxMode;
  if (lum > 255) lum = 255;
  if (lum < 20) lum = 20;
}

/******************************************************************
 * Updated Light Patterns for Mode Ranges with 3-Minute Timing
 ******************************************************************/


void emotionDamper() {
  // Mode ≥ 0 and Mode < 2: Basic color patterns (Red + Blue)
  if (mode >= 0 && mode < 2) {
    rgbInd = 0;  // Color for mode 0-1
    Serial.println("rgbInd: " + String(rgbInd));

    setIntensity(2);
    void (*effects[5])() = {calmingWave, soothingPulse, relaxingFade, gradualIllumination, rhythmicStimulation};
    runEffectsSequentially(effects, 5);  // Pass effects array and number of effects
  } 
  // Mode ≥ 2 and Mode < 4: Sequential execution of light effects
  else if (mode >= 2 && mode < 4) {
    rgbInd = 1;  // Color for mode 2-3 (e.g., Green)
    Serial.println("rgbInd: " + String(rgbInd));

    setIntensity(2);  // Set intensity for Green
    void (*effects[5])() = {calmingWave, soothingPulse, relaxingFade, gradualIllumination, rhythmicStimulation};
    runEffectsSequentially(effects, 5);  // Run effects on Green
  }
  // Mode ≥ 4 and Mode < 12: Brightening, stimulating pulse, etc.
  else if (mode >= 4 && mode < 12) {
    rgbInd = 2;  // Color for mode 4-11
    Serial.println("rgbInd: " + String(rgbInd));

    setIntensity(8);
    void (*effects[5])() = {brighteningPattern, stimulatingPulse, calmingWave, gradualIllumination, rhythmicStimulation};
    runEffectsSequentially(effects, 5);  // Run all 5 effects on the fixed color
  }
  // Mode ≥ 12 and Mode < 16: Gradual exposure, rhythmic stimulation (long), etc.
  else if (mode >= 12 && mode < 16) {
    rgbInd = 3;  // Color for mode 12-15
    Serial.println("rgbInd: " + String(rgbInd));

    setIntensity(4);
    void (*effects[5])() = {gradualExposure, stimulatingPulse, softPulse, gradualIllumination, rhythmicStimulationLong};
    runEffectsSequentially(effects, 5);  // Run all 5 effects in sequence
  }
  // Continue similar logic for other mode ranges...
}
/******************************************************************
 * System Setup
 ******************************************************************/
void setup() {
  Serial.begin(9600);
  showActiveStatus();  // Show that the system is active before connecting to WiFi
  initLT();
  initWifi();
}

void loop() {
  fetchDataAndControlTiming();
  emotionDamper();  // Ensure that light effects are continuously running
}

