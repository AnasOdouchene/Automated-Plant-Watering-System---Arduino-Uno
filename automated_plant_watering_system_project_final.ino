#include <LiquidCrystal.h>

const int BuzzerPin = 10;              
const int LedRed = 12;                 
const int LedGreen = 11;               
const int SoilMoistureSensor = A0;     
const int WaterPump = 13;               
const int LDRPin = A1;                  

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);   

unsigned long pumpStartTime = 0;       
unsigned long pumpDuration = 0;        
bool pumpActive = false;                
bool wateringOnce = false;              // Flag to track if watering has occurred
bool waterLowAlert = false;             
int currentMessageIndex = 0;            
unsigned long lastUpdate = 0;          

const int TrigPin = 8;                  
const int EchoPin = 9;                  

String messages[3];                    
long waterDistance; // Variable to store the water level distance

void setup() {
  pinMode(WaterPump, OUTPUT);
  pinMode(LedRed, OUTPUT);
  pinMode(LedGreen, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  
  pinMode(TrigPin, OUTPUT);
  pinMode(EchoPin, INPUT);
  
  Serial.begin(9600);
  
  lcd.begin(16, 2);
  lcd.clear();
  
  // Initial message display
  lcd.setCursor(0, 0);
  String welcomeMessage = "Welcome!";

  for (int i = 0; i < welcomeMessage.length(); i++) {
    lcd.print(welcomeMessage.charAt(i));
    delay(100);
  }

  delay(1000); // Pause for 1 second before showing the next messages
  lcd.clear(); // Clear the display

  // Display the main messages
  lcd.setCursor(0, 0);
  String message1 = "Automated Plant";
  String message2 = "Watering System";

  for (int i = 0; i < message1.length(); i++) {
    lcd.print(message1.charAt(i));
    delay(100);
  }

  lcd.setCursor(0, 1);
  for (int i = 0; i < message2.length(); i++) {
    lcd.print(message2.charAt(i));
    delay(100);
  }

  delay(2500);
  lcd.clear();

  // Initialize messages
  messages[0] = "Moisture:      ";
  messages[1] = "Water Pump:          ";
  messages[2] = "Water Level OK  ";
}

void loop() {
  int moistureValue = analogRead(SoilMoistureSensor);         
  int moisturePercentage = map(moistureValue, 0, 876, 0, 99);  
  int ldrValue = analogRead(LDRPin);                           
  int lightLevel = map(ldrValue, 6, 679, 0, 100);              

  // Update the moisture level message
  messages[0] = "Moisture: " + String(moisturePercentage) + "%  ";

  // Measure water level
  long duration;
  digitalWrite(TrigPin, LOW); 
  delayMicroseconds(2);
  digitalWrite(TrigPin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(TrigPin, LOW);
  
  duration = pulseIn(EchoPin, HIGH); 
  waterDistance = (duration * 0.034) / 2; // Update global water distance variable
  
  // Print moisture, light level, and distance to console
  Serial.print("Humidity: ");
  Serial.print(moisturePercentage);
  Serial.print("% | Luminosity: ");
  Serial.print(lightLevel);
  Serial.print("% | Distance: ");
  Serial.print(waterDistance);
  Serial.println(" cm");

  // Check water level
  if (waterDistance < 10) {
    if (!waterLowAlert) {
      lcd.setCursor(0, 1);
      lcd.print("Water low!     ");
      playSound(); 
      waterLowAlert = true; 
    }
    messages[2] = "Water Level: LOW ";
  } else {
    waterLowAlert = false; 
    messages[2] = "Water Level OK   ";
  }

  // Update pump status message
  updatePumpStatusMessage();

  // Display the current message based on time
  if (millis() - lastUpdate >= 2000) { // Change message every 2 seconds
    currentMessageIndex = (currentMessageIndex + 1) % 3; // Cycle through messages
    displayMessage(messages[currentMessageIndex]);
    lastUpdate = millis();
  }

  // Pump control logic
  controlPump(moisturePercentage, lightLevel);
  
  delay(1000);  // Wait for a while
}

void displayMessage(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}

void updatePumpStatusMessage() {
  messages[1] = "Water Pump: " + String(pumpActive ? "ON" : "OFF");
}

void controlPump(int moisturePercentage, int lightLevel) {
  // Check if water level is low
  if (waterDistance < 10) {
    pumpActive = false; // Stop the pump if the water level is low
    digitalWrite(WaterPump, LOW); // Ensure pump is off
    digitalWrite(LedGreen, LOW);
    digitalWrite(LedRed, HIGH); // Indicate low water level with red LED
    Serial.println("Pump stopped due to low water level."); // Debugging line
    return; // Exit the function
  }
  
  // Pump control logic
  if (!wateringOnce && moisturePercentage < 50) { // Check if watering has not occurred
    pumpDuration = (lightLevel > 70) ? 20000 : 10000; // Set duration based on light level
    pumpStartTime = millis();                          
    pumpActive = true;                                
    wateringOnce = true; // Set the watering flag
    digitalWrite(WaterPump, HIGH);                    
    digitalWrite(LedGreen, HIGH);
    digitalWrite(LedRed, LOW);
    playSound();                                      
    Serial.println("Pump activated."); // Debugging line
  }

  if (pumpActive && millis() - pumpStartTime >= pumpDuration) {
    digitalWrite(WaterPump, LOW);     
    pumpActive = false;                
    digitalWrite(LedGreen, LOW);
    digitalWrite(LedRed, HIGH);
    Serial.println("Pump deactivated."); // Debugging line
  }
}

// Function to play sound with the buzzer
void playSound() {
  tone(BuzzerPin, 87, 100);  
  delay(100);                
  noTone(BuzzerPin);        
}
