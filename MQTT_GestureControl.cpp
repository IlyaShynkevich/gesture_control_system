#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// --- WiFi Settings ---
const char* ssid = "";
const char* password = "";

// --- HiveMQ Cloud Details (INCLUDES SECERTS)
const char* mqtt_server = "YOUR_MQTT_HOST"; 
const int mqtt_port = 8883;
const char* mqtt_username = "YOUR_USERNAME";
const char* mqtt_password = "YOUR_PASSWORD";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// --- Pin Definitions ---
const int LEFT_TRIG_PIN = 8;
const int LEFT_ECHO_PIN = 7;
 
const int RIGHT_TRIG_PIN = 5;
const int RIGHT_ECHO_PIN = 6;
 
// --- Thresholds & Timings ---
const int MAX_DETECT_DIST = 85;            
const int HIGH_THRESH = 15;    //Vertical movement distance required        
const int LOW_THRESH = 5;      //Min Distance from sensor    
 
const unsigned long MAX_SWIPE_TIME = 700; 
const unsigned long MIN_SWIPE_TIME = 300;  
const unsigned long COOLDOWN_TIME = 1000;  
const unsigned long PRINT_INTERVAL = 700;  //And publish
 
// --- State Machine ---
enum State { IDLE, L_TRIGGERED, R_TRIGGERED, COOLDOWN };
State currentState = IDLE;
 
unsigned long triggerTime = 0;
unsigned long cooldownStartTime = 0;
unsigned long lastPrintTime = 0;
 
int startDist = 0;
int count = 1;
 
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  espClient.setInsecure();
 
  pinMode(LEFT_TRIG_PIN, OUTPUT);
  pinMode(LEFT_ECHO_PIN, INPUT);
 
  pinMode(RIGHT_TRIG_PIN, OUTPUT);
  pinMode(RIGHT_ECHO_PIN, INPUT);
 
  Serial.println("ESP32 4-Way Gesture Sensor Initialized!");
  Serial.println("Waiting for Left/Right/Up/Down swipes...");
}
 
int getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  long duration = pulseIn(echoPin, HIGH, 6000);
  if (duration == 0) return 999;
 
  return duration * 0.034 / 2;
}
 
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentTime = millis();

  // Read sensors
  int distL = getDistance(LEFT_TRIG_PIN, LEFT_ECHO_PIN);
  delay(15);
  int distR = getDistance(RIGHT_TRIG_PIN, RIGHT_ECHO_PIN);
 
 
  // Live Distance Feed
  if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
    Serial.print(count);
    Serial.print(": Left: ");
    count++;
    if (distL == 999) {
       Serial.print("---"); 
       client.publish("sensor/left", "---");
    }
    else {
      Serial.print(distL);
      client.publish("sensor/left", String(distL).c_str());
    }
    Serial.print(" cm  |  Right: ");
    if (distR == 999) {
      Serial.print("---");
      client.publish("sensor/right", "---");
    }  
    else {
      Serial.print(distR);
      client.publish("sensor/right", String(distR).c_str());
    }
    Serial.println(" cm");
    lastPrintTime = currentTime;
  }
 
  // Cooldown Handling
  if (currentState == COOLDOWN) {
    if (currentTime - cooldownStartTime > COOLDOWN_TIME && distL > MAX_DETECT_DIST && distR > MAX_DETECT_DIST) {
      currentState = IDLE;
    }
    return;
  }
 
  // Timeout Handling
  if (currentState == L_TRIGGERED || currentState == R_TRIGGERED) {
    if (currentTime - triggerTime > MAX_SWIPE_TIME) {
      currentState = IDLE;
    }
  }
 
  // Core State Machine Logic
  if (currentState == IDLE) {
    if (distL > 0 && distL <= MAX_DETECT_DIST) {
      currentState = L_TRIGGERED;
      triggerTime = currentTime;
      startDist = distL;
    } else if (distR > 0 && distR <= MAX_DETECT_DIST) {
      currentState = R_TRIGGERED;
      triggerTime = currentTime;
      startDist = distR;
    }
  }
 
  else if (currentState == L_TRIGGERED) {
    // Wait to read values from right sensor :P
    if (currentTime - triggerTime < 350) {
        return;
    }
    // Check Horizontal swipe
    if ((distR > 0 && distR <= MAX_DETECT_DIST) && (startDist - 10 <= distR && distR <= startDist + 10)) {
      if (currentTime - triggerTime >= MIN_SWIPE_TIME) {
        Serial.println("\n>>> HORIZONTAL SWIPE: Left to Right >>>\n");
        client.publish("Direction", "Left to Right");
        client.publish("StartSensor", "Left");
        currentState = COOLDOWN;
        cooldownStartTime = currentTime;
      }
    }
    // Check Vertical Swipes
    else {
      if ((startDist >= HIGH_THRESH) && (startDist - distL >= HIGH_THRESH)) {
        Serial.println("\nvvv VERTICAL SWIPE: Left Sensor DOWN vvv\n");
        client.publish("Direction", "Left Sensor DOWN");
        client.publish("StartSensor", "Left");
        currentState = COOLDOWN;
        cooldownStartTime = currentTime;
      }
      else if (startDist >= LOW_THRESH && (distL - startDist >= HIGH_THRESH) && (distL != 999)) {
        if (currentTime - triggerTime >= MIN_SWIPE_TIME) {
            Serial.println("\n^^^ VERTICAL SWIPE: Left Sensor UP ^^^\n");
            client.publish("Direction", "Left Sensor UP");
            client.publish("StartSensor", "Left");
            currentState = COOLDOWN;
            cooldownStartTime = currentTime;
        }
      }
    }
  }
 
  else if (currentState == R_TRIGGERED) {
    // Wait to read from left sensor :P
    if (currentTime - triggerTime < 350) {
        return;
    }
    // Check Horizontal Swipe
    if ((distL > 0 && distL <= MAX_DETECT_DIST) && (startDist - 10 <= distL && distL <= startDist + 10)) {
      if (currentTime - triggerTime >= MIN_SWIPE_TIME) {
        Serial.println("\n<<< HORIZONTAL SWIPE: Right to Left <<<\n");
        client.publish("Direction", "Right to Left");
        client.publish("StartSensor", "Right");
        currentState = COOLDOWN;
        cooldownStartTime = currentTime;
      }
    }
    // Check Vertical Swipe
    else {
      if ((startDist >= HIGH_THRESH) && (startDist - distR >= HIGH_THRESH)) {
        Serial.println("\nvvv VERTICAL SWIPE: Right Sensor DOWN vvv\n");
        client.publish("Direction", "Right Sensor DOWN");
        client.publish("StartSensor", "Right");
        currentState = COOLDOWN;
        cooldownStartTime = currentTime;
      }
      else if (startDist >= LOW_THRESH && (distR - startDist >= HIGH_THRESH) && (distR != 999)) {
        if (currentTime - triggerTime >= MIN_SWIPE_TIME) {
            Serial.println("\n^^^ VERTICAL SWIPE: Right Sensor UP ^^^\n");
            client.publish("Direction", "Right Sensor UP");
            client.publish("StartSensor", "Right");
            currentState = COOLDOWN;
            cooldownStartTime = currentTime;
        }
      }
    }
  }
}