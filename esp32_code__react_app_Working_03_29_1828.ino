// ===============================
// ESP32-C3 Conveyor Controller
// Works with Arduino using RUN_SIGNAL and MQTT from React app
// ===============================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "esp32_secrets.h"

// <https://docs.hivemq.com/hivemq-cloud/quick-start-guide.html>


WiFiClientSecure espClient;
PubSubClient client(espClient);

// Use RUN_SIGNAL to run Arduino 
#define RUN_SIGNAL D6   // GPIO 6

// Motor Driver: Pin Definitions for TB6612FNG
#define AI1   D2  //Motor A
#define AI2   D3  //Motor A
#define PWMA  D4  //Motor A PWM speed control
#define STBY  D5  //Driver standby (needs to be high to run)
#define PWMB  D8  //Motor B PWM speed control
#define BI2   D9  //Motor B
#define BI1   D10 //Motor B

bool conveyorsRunning = false;     // tracks current state of conveyor
bool desiredConveyorState = false; // final state applied to motors

// Separate flags to track requests from different sources
bool requestedByReact = false;     // state requested by React app
bool requestedByHardware = false;  // state requested by hardware button

int motorSpeed = 200;              // default motor speed (0-255)
const char* speedTopic = "factory/conveyor/speed";


// ====================
// Wi-Fi Connect
// ====================
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ====================
// MQTT Connect
// ====================
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");

    // Generate unique client ID so multiple devices can connect
    String clientId = "ESP32Client-" + String(random(1000, 9999));

    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected!");

      // Subscribe to the command topic to receive React app messages
      if (client.subscribe("factory/conveyor/command")) {
        Serial.println("Subscribed to factory/conveyor/command");
      } else {
        Serial.println("Subscription FAILED");
      }
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(2000); // wait before retrying
    }
  }
}

// ====================
// MQTT Callback
// Handles incoming messages from React app
// ====================
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";

  // Convert payload bytes to string
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Only handle commands from the React app
  if (String(topic) == "factory/conveyor/command") {
    if (message == "START") {
      requestedByReact = true;   // request to start conveyor
      Serial.println("MQTT: START command received");
    }
    else if (message == "STOP") {
      requestedByReact = false;  // request to stop conveyor
      Serial.println("MQTT: STOP command received");
    }
    else if (message.startsWith("SPEED:")) {
      int newSpeed = message.substring(6).toInt();
      motorSpeed = constrain(newSpeed, 0, 255); // keep speed within 0-255
      Serial.print("MQTT: SPEED set to ");
      Serial.println(motorSpeed);

      // apply new speed immediately if conveyors are running
      if (conveyorsRunning) startConveyors();
    }
  }
}

// ====================
// Setup
// ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-C3 Conveyor Controller Started");

  WiFi.mode(WIFI_STA);
  Serial.print("ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());

  connectWiFi();

  // Set up MQTT client
  espClient.setInsecure(); // Use for testing, or add root CA for production
  client.setServer(MQTT_CLUSTER, MQTT_PORT);
  client.setCallback(callback);

  // Configure RUN_SIGNAL as input with pulldown
  pinMode(RUN_SIGNAL, INPUT_PULLDOWN);

  // Configure motor driver pins as outputs
  pinMode(AI1, OUTPUT);
  pinMode(AI2, OUTPUT);
  pinMode(BI1, OUTPUT);
  pinMode(BI2, OUTPUT);
  pinMode(STBY, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);

  // Make sure conveyors start in stopped state
  stopConveyors();

  Serial.println("Setup complete");
}

// ====================
// Main Loop
// ====================
void loop() {
  // Reconnect Wi-Fi if needed
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  // Reconnect MQTT if needed
  if (!client.connected()) connectMQTT();
  
  client.loop(); // handle incoming MQTT messages

  // --- Read hardware button ---
  requestedByHardware = (digitalRead(RUN_SIGNAL) == HIGH);

  // --- Combine requests ---
  // Conveyor will run if either hardware button or React app requests it
  desiredConveyorState = requestedByReact || requestedByHardware;

  // --- Apply state if changed ---
  if (desiredConveyorState != conveyorsRunning) {
    conveyorsRunning = desiredConveyorState;

    if (conveyorsRunning) {
      startConveyors(); // start motors
      Serial.println("Conveyor started");
    } else {
      stopConveyors();  // stop motors
      Serial.println("Conveyor stopped");
    }
  }
}

// ====================
// Start Conveyors
// ====================
void startConveyors() {
  digitalWrite(STBY, HIGH); // enable motor driver
  delay(10);                 // allow time for driver to wake up

  // Motor A forward
  digitalWrite(AI1, HIGH);
  digitalWrite(AI2, LOW);
  analogWrite(PWMA, motorSpeed);

  // Motor B forward
  digitalWrite(BI1, HIGH);
  digitalWrite(BI2, LOW);
  analogWrite(PWMB, motorSpeed);

  Serial.println("Conveyors started - STBY HIGH, Motors set to FORWARD");

  // publish current speed to MQTT
  char speedString[10];
  itoa(motorSpeed, speedString, 10);
  client.publish(speedTopic, speedString);
  Serial.print("Published motor speed: ");
  Serial.println(speedString);
}

// ====================
// Stop Conveyors
// ====================
void stopConveyors() {
  analogWrite(PWMA, 0); // stop Motor A
  analogWrite(PWMB, 0); // stop Motor B

  // clear direction pins
  digitalWrite(AI1, LOW);
  digitalWrite(AI2, LOW);
  digitalWrite(BI1, LOW);
  digitalWrite(BI2, LOW);

  digitalWrite(STBY, LOW); // put driver in standby
  Serial.println("Conveyors stopped");

  client.publish(speedTopic, "0"); // publish stopped speed
  Serial.println("Published motor speed: 0");
}