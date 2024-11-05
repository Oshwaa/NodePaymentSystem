#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <ESP8266WebServer.h>

const char* ssid = "AAAAAAAAAAAAA!!!!!!!!!!!!!!!";
const char* password = "**************************";
const char* serverUrl = "*****"; //php file location (local or online)

constexpr uint8_t RST_PIN = D3;    
constexpr uint8_t SS_PIN = D4;
constexpr uint8_t SERVO_PIN = D8;   

// Pins for ultrasonic sensor
const int trigPin = D1; // Trig pin of the ultrasonic sensor
const int echoPin = D2; // Echo pin of the ultrasonic sensor
const int maxDistance = 120; // Maximum distance to trigger the lock function (in centimeters)

ESP8266WebServer server(80);

MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo;

String fareData = "";
String driver = "";
int servo_con = 180;
bool unlockTriggered = false;
int up = 0;


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/data", handleRequest);
  server.begin();

  // Initialize SPI communication
  SPI.begin();

  // Initialize the RFID reader
  rfid.PCD_Init();

  // Attach servo to the servo pin
  servo.attach(SERVO_PIN);
  servo.write(180);
  
  delay(1000); // Delay to allow the RFID reader to initialize
}

void loop() {
  server.handleClient();

  // Check if an RFID tag is present
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Read RFID tag data
    String rfidData = readRFID();

    // Make an HTTP request to check the ID on the server
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverUrl);  // Use begin(WiFiClient, url) method

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postData = "rfid=" + rfidData + "&fare=" + fareData + "&driver=" + driver;
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode == HTTP_CODE_OK) {
      String response = http.getString();
      Serial.println("Response from server:");
      Serial.println(response);

      // Process the response here
      if (response == "Unlock" && !unlockTriggered) {
        unlock();
        up = 1;
        unlockTriggered = true;

      }
    } else {
      Serial.println("Error in HTTP request");
    }

    http.end();

    // Halt PICC to stop further communication
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  // Check ultrasonic sensor for obstacles and trigger lock if needed
int distance = getDistance();

  if (servo_con == 0 && distance > maxDistance && up == 0) 
  {
    unlockTriggered = false; // Reset unlock trigger
    delay(1000);
    lock();
    
    
  }
  else if (servo_con == 180 && distance <= maxDistance) {
    up = 0;
    unlock();
    
    unlockTriggered = true;
    
  }
  else if(servo_con == 0 && distance <= maxDistance)
  {
    up = 0;
    unlockTriggered = true;
  }
  
    
    
   
   
}

String readRFID() {
  String rfidData;

  for (byte i = 0; i < rfid.uid.size; i++) {
    rfidData += String(rfid.uid.uidByte[i], HEX);
  }

  Serial.println(rfidData);

  // Return the RFID tag data as a String
  return rfidData;
}

int getDistance() {
  // Send a short ultrasonic pulse to the trigPin
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure the echoPin pulse width in microseconds
  pinMode(echoPin, INPUT);
  long duration = pulseIn(echoPin, HIGH, 30000); 
  
  int distance = duration * 0.034 / 2;
 
  return distance;
}

void unlock() {
  for(int i = 180; i >= 0; i -= 10) {
    servo.write(i);
    servo_con = 0;
    delay(50);
  }
}

void lock() {
  
  for(int i = 0; i <= 180; i += 10) {
    servo.write(i);
    servo_con=180;
    delay(100);
  }
}


void handleRequest() {
  if (server.hasArg("fare")) {
    fareData = server.arg("fare");
    Serial.println("Fare:"+fareData);
    
  }

  if (server.hasArg("driver")) {
    driver = server.arg("driver");
    Serial.println("Driver's RFID:" + driver);
    
  }

  server.send(200, "text/plain", "OK");
}
