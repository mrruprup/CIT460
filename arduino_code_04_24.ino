// ===============================
// Arduino Conveyor Controller with E-stop, start push button, stop pushbutton, and LED indicator lights
// START: Blink 3x then run
// STOP: Blink 3x then stop
// ESTOP: Stop the conveyor immediately and sound alarm for 2 seconds
// ===============================

//define the pin that connects the Arduino to the ESP32 to send a run signal to the ESP32 
#define RUN_SIGNAL 6

// LED output pins
#define RED_LED       2
#define YELLOW_LED    3
#define GREEN_LED     4

// Button inputs
// uses INPUT_PULLUP where pressed = low
#define START_BTN     5
#define STOP_BTN      7
#define ESTOP_BTN     10

// Buzzer inputs
#define BUZZER_PIN    8
#define BUZZER_FREQ   1000 // loudest setting on buzzer (frequency hz)

//IR sensor
#define IR_SENSOR_PIN 11

// --------------------------
// Main Conveyor State Flag
// true = conveyor running
// false = conveyor stopped
// --------------------------
bool conveyorRunning = false;

// --------------------------
// Blink state
// --------------------------

/* 
Variables are defined as found in the Arduino docs. <https://docs.arduino.cc/learn/programming/sketches/>
The docs define the type of variablem the name, and then the value
*/

unsigned long lastBlinkTime = 0;    //last time the LED was toggled
unsigned long blinkInterval = 300;  // time between toggles in ms
int blinkCount = 0;                 //vamount of blinks completed
int blinkTarget = 0;                //vdesired amount of blinks
bool blinking = false;              // is it currently blinking
bool blinkRed = false;
bool blinkGreen = false;
bool buzzerOn = false;

// --------------------------
// Pending Action Flags 
// These flags delay the start and stop until after the blinking finishes
// --------------------------
bool pendingStart = false;
bool pendingStop = false;

// --------------------------
// E-stop state variables
// --------------------------
bool estopActive = false;           //true while e-stop is pressed
unsigned long estopBuzzerStart = 0; //when alarm started
bool estopBuzzerPlaying = false;    //true while alarm is on

bool lastObjectDetected = false;
unsigned int itemCount = 0;

// --------------------------
// Setup: Runs once when it is powered on
// --------------------------
void setup() {
  // serial monitor for debugging
  Serial.begin(115200);
  
  /*
  pinMode can also be found in the Arduino docs. Arduino states that you can pass two parameters to pinMode(), 
  "These parameters are used by the pinMode() function to decide which pin and mode to set." 
  <https://docs.arduino.cc/learn/programming/sketches/>
  */

  // configure the LED pins are outputs
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, INPUT);       // yellow light is currently unused
  pinMode(GREEN_LED, OUTPUT);

  // configure buttons with internal pullups
  pinMode(START_BTN, INPUT_PULLUP);
  pinMode(STOP_BTN, INPUT_PULLUP);
  pinMode(ESTOP_BTN, INPUT_PULLUP);

  // configure buzzer and run signal outputs
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RUN_SIGNAL, OUTPUT);

  //configure IR sensor
  pinMode(IR_SENSOR_PIN, INPUT);

  /*
  digitalWrite can also be found in Arduino docs. Arduino states, 
  "The digitalWrite() functions outputs a value on a pin." 
  <https://docs.arduino.cc/learn/programming/sketches/>
  */

  // make sure the conveyor starts in the off position
  digitalWrite(RUN_SIGNAL, LOW);

  // set LEDs to stopped state
  showStopped(); //function declared below
  Serial.println("System Ready");
}


// --------------------------
// Main Loop: runs repeatedly
// --------------------------
void loop() {
  //grab current time for non-blocking timing
  unsigned long currentMillis = millis();

  int sensorState = digitalRead(IR_SENSOR_PIN);
  bool objectDetected = (sensorState == LOW);

  // Only trigger when object FIRST appears
  if (objectDetected && !lastObjectDetected) {
    itemCount++;

    Serial.print("Item count: ");
    Serial.println(itemCount);

    // Send pulse to ESP32
    digitalWrite(RUN_SIGNAL, HIGH);
    delay(20);  // short pulse
    digitalWrite(RUN_SIGNAL, LOW);
  }

// Save state for next loop
lastObjectDetected = objectDetected;

// --------------------------
// Emergency Stop
// Immediate stop and two second alarm
// --------------------------

/*
digitalRead reads the value from the specified digital pin. 
Obtained from Arduino docs <https://docs.arduino.cc/language-reference/en/functions/digital-io/digitalread/>
*/
  if (digitalRead(ESTOP_BTN) == LOW) {
    if (!estopActive) {  // Only trigger once per press
      estopActive = true;
      
      // Cancel any pending actions
      pendingStart = false;
      pendingStop = false;
      
      // IMMEDIATELY stop conveyor
      conveyorRunning = false;
      blinking = false;  // Stop any blinking sequence
      
      // Force run signal low (stops conveyor immediately)
      digitalWrite(RUN_SIGNAL, LOW);
      
      // Set LED to solid red immediately
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      
      /*
      tone is also found in the Arduino docs. Arduino states, "Generates a square wave of the 
      specified frequency (and 50% duty cycle) on a pin. A duration can be specified, 
      otherwise the wave continues until a call to noTone()."
      <https://docs.arduino.cc/language-reference/en/functions/advanced-io/tone/>
      */

      // Start 2-second buzzer alarm
      tone(BUZZER_PIN, BUZZER_FREQ);

      /*
      millis is found in Arduino docs. Current millis comes from defined variable above. 
      Declaring the variable ensures that it is called once and not repetively in the loops. 
      Arduino states, "This function returns the number of milliseconds passed since the 
      program started. Data type: unsigned long." This value is being stored and can be reused, such
      as here. 
      <https://docs.arduino.cc/language-reference/en/functions/time/millis/>
      */

      estopBuzzerStart = currentMillis;
      estopBuzzerPlaying = true;
      
      Serial.println("Emergency Stop Pressed");
    }
  } else {
    // When E-stop button is released
    if (estopActive) {
      estopActive = false;
    }
  }

  // Handle 2-second buzzer timing for E-stop
  if (estopBuzzerPlaying) {
    if (currentMillis - estopBuzzerStart >= 2000) {
      noTone(BUZZER_PIN);
      estopBuzzerPlaying = false;
      Serial.println("Alarm finished");
    }
  }

  // --------------------------
  // Start Button 
  // --------------------------

  // If the estop is not active, the buzzer is not on, and the light is not blinking
  // then the start button will work
  if (!estopActive && !estopBuzzerPlaying && !blinking) {
    if (digitalRead(START_BTN) == LOW && !conveyorRunning) {
      // Start blinking green 3 times, but don't start conveyor yet
      startBlink(true, 3); // green blink 3 times
      pendingStart = true;  // start after blinking
      pendingStop = false;  // Cancel any pending stop
      Serial.println("Start requested");
    }
    // --------------------------
    // Stop Button
    // --------------------------

    // if the stop button is not active and the conveyor is running, begin the stopping process
    if (digitalRead(STOP_BTN) == LOW && conveyorRunning) {
      // Start blinking red 3 times, but don't stop conveyor yet
      startBlink(false, 3); // red blink 3 times
      pendingStop = true;   // Mark that we want to stop after blinking
      pendingStart = false; // Cancel any pending start
      Serial.println("Stop requested");
    }
  }
  // --------------------------
  // Handle Blinking
  // --------------------------
  if (!estopActive && !estopBuzzerPlaying && blinking && currentMillis - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = currentMillis;

    if (buzzerOn) {
      // Turn off LED and buzzer: end of one blink
      if (blinkGreen) digitalWrite(GREEN_LED, LOW);
      if (blinkRed)   digitalWrite(RED_LED, LOW);
      //buzzer is turned off if it is still on
      noTone(BUZZER_PIN);
      buzzerOn = false;

      blinkCount++;  // count completed blink
    } else {
      // Turn on LED and buzzer → start of blink
      if (blinkGreen) digitalWrite(GREEN_LED, HIGH);
      if (blinkRed)   digitalWrite(RED_LED, HIGH);
      tone(BUZZER_PIN, BUZZER_FREQ);
      buzzerOn = true;
    }

    // Check if blinking is done
    if (blinkCount >= blinkTarget) {
      blinking = false;
      buzzerOn = false;
      noTone(BUZZER_PIN);

      // Execute pending action AFTER blinking is complete
      if (pendingStart) {
        // Start the conveyor after blinking
        conveyorRunning = true;
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED, LOW);
        digitalWrite(RUN_SIGNAL, HIGH);
        pendingStart = false;
        
        Serial.println("Conveyor STARTED");
      }
      else if (pendingStop) {
        // Stop the conveyor
        conveyorRunning = false;
        digitalWrite(RED_LED, HIGH);
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RUN_SIGNAL, LOW);
        pendingStop = false;
        
        Serial.println("Conveyor STOPPED");
      }
      else {
        // Just blinking without action (catch all)
        if (blinkGreen) {
          digitalWrite(GREEN_LED, LOW);
          digitalWrite(RED_LED, HIGH);
        } else if (blinkRed) {
          digitalWrite(RED_LED, HIGH);
          digitalWrite(GREEN_LED, LOW);
        }
      }
    }
  }
}

// =========================
// Start a blink sequence
// green=true green light on, red=false red light on
// =========================
void startBlink(bool green, int times) {
  blinking = true;
  blinkGreen = green;
  blinkRed = !green;
  blinkCount = 0;
  blinkTarget = times; // 3 complete blinks
  lastBlinkTime = millis();
  buzzerOn = false;

  // Make sure the other LED is off during blinking
  if (green) {
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW); // Start off
  } else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);   // Start off
  }
  
  // RUN_SIGNAL does NOT change during blinking
  // It will change AFTER blinking when pending action executes
}

// =========================
// Show stopped state
// =========================
// used above for e-stop, ensures run signal is turned off so conveyor stops
void showStopped() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  digitalWrite(RUN_SIGNAL, LOW);
}