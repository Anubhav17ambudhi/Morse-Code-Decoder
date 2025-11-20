#include <WiFi.h>
#include <WebServer.h>
#include <AESLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <ESPmDNS.h> 

// ---------- WiFi Setup ----------
// !!! CHANGE THESE TO YOUR HOTSPOT/ROUTER DETAILS !!!
const char* AP_SSID = "vks"; 
const char* AP_PASSWORD = "11111113"; 
WebServer server(80);

// ---------- AES Setup (RECEIVER 2's Unique Key) ----------
// This key must match KEY_FOR_R2 in the Sender code exactly
char aesKey[] = "DIFFERENTKEY_R2_"; // **UNIQUE KEY 2** (16 bytes)
AESLib aesLib;
int bits = 128;
byte iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

// ---------- Pins ----------
#define BUZZER_PIN 25

// ---------- OLED Display Setup ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
String lastDisplayed = "Waiting...";

// ---------- AES Decryption Helper (Fixed) ----------
String decryptMessage(String encrypted) {
  char decrypted[128]; 
  
  // Create a mutable (non-const) copy of the encrypted string
  int input_len = encrypted.length();
  char encrypted_mutable[input_len + 1];
  strcpy(encrypted_mutable, encrypted.c_str());

  // Attempt Decryption
  int output_len = aesLib.decrypt64(
    encrypted_mutable,   
    input_len,           
    (byte*)decrypted,   
    (const byte*)aesKey, 
    bits,               
    iv                  
  );
  
  // Check for failure (0 output length)
  // Note: We removed the %16 check to fix the Base64 error
  if (output_len == 0) {
    return "ERROR: Key Mismatch";
  }
  
  decrypted[output_len] = '\0'; // Null-terminate
  return String(decrypted);
}

// ---------- Display Update Function ----------
void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("RECEIVER 2 (KEY 2)");
  display.println("-----------------");
  display.setTextSize(2);
  display.println(lastDisplayed);
  display.display();
}

// ---------- Web Handler for Receiving POST Data ----------
void handleReceive() {
  if (server.method() == HTTP_POST) {
    String encryptedMsg = server.arg(0); // Get the raw POST body
    Serial.print("Received Encrypted: ");
    Serial.println(encryptedMsg);
    
    String decryptedMsg = decryptMessage(encryptedMsg);
    
    if (decryptedMsg.startsWith("ERROR")) {
      lastDisplayed = "Failed: " + decryptedMsg; 
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); // Short buzz
      server.send(200, "text/plain", "Decryption Failed");
    } else {
      lastDisplayed = decryptedMsg; // Success!
      Serial.println("Decrypted: " + lastDisplayed);
      digitalWrite(BUZZER_PIN, HIGH); delay(500); digitalWrite(BUZZER_PIN, LOW); // Long buzz
      server.send(200, "text/plain", "Decryption Successful");
    }
    updateDisplay();
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { Serial.println(F("SSD1306 failed")); for(;;); }
  
  // FIX: Set text color so it is visible!
  display.setTextColor(SSD1306_WHITE);
  updateDisplay();

  // Connect to the central AP
  Serial.print("Connecting to ");
  Serial.println(AP_SSID);
  WiFi.begin(AP_SSID, AP_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  Serial.println("\nWiFi connected.");
  Serial.print("Receiver 2 IP: ");
  Serial.println(WiFi.localIP()); 

  // Initialize mDNS for "receiver2.local"
  if (!MDNS.begin("receiver2")) { 
      Serial.println("Error setting up mDNS!");
  } else {
      Serial.println("mDNS responder started: receiver2.local");
  }

  server.on("/receive", HTTP_POST, handleReceive);
  server.begin();
  updateDisplay();
}

// ---------- Loop ----------
void loop() {
  server.handleClient();
  // No MDNS.update() needed for ESP32
}