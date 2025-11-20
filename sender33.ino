#include <WiFi.h>
#include <HTTPClient.h>
#include <AESLib.h>
#include <string.h>

// --- CONSTANTS ---
// These keys MUST match the keys set in the Receiver code files.
const char KEY_FOR_R1[] = "MYSECRETKEY_R1_"; // Key used by Receiver 1
const char KEY_FOR_R2[] = "DIFFERENTKEY_R2_"; // Key used by Receiver 2
const char EXIT_CODE = 'X'; // Morse code for 'X' (-..-) to send and exit selection

// --- STATE MACHINE ---
enum SenderState {
  SELECTING_RECEIVER,
  ENTERING_MESSAGE
};

SenderState currentState = SELECTING_RECEIVER;
String selectedTarget = ""; // Stores the mDNS hostname (e.g., "receiver1.local")
char currentEncryptionKey[17]; // Buffer for the 16-byte key + null terminator

// ---------- WiFi Connection (Connect to a Central Router/AP) ----------
const char* AP_SSID = "vks"; // CHANGE THIS
const char* AP_PASSWORD = "11111113"; // CHANGE THIS

// ---------- Receiver Hostnames (mDNS) ----------
// These names replace the need for static IP addresses.
const char* RECEIVER_1_HOSTNAME = "receiver1.local";
const char* RECEIVER_2_HOSTNAME = "receiver2.local";
const int RECEIVER_PORT = 80;

// ---------- AES Setup ----------
AESLib aesLib;
int bits = 128;
byte iv[] = { 
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F 
};

// ---------- Pins, Morse Decoding ----------
#define BUTTON_PIN 18
#define BUZZER_PIN 25
String morseSequence = "";
String lastMessage = "";
unsigned long lastPressTime = 0;
unsigned long lastReleaseTime = 0;
bool buttonPressed = false;

struct MorseMap { const char* code; char letter; };
MorseMap morseDict[] = {
  {".-", 'A'},{"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},
  {".", 'E'}, {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'},
  {"..", 'I'},  {".---", 'J'}, {"-.-", 'K'}, {".-..", 'L'},
  {"--", 'M'},  {"-.", 'N'},  {"---", 'O'}, {".--.", 'P'},
  {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
  {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'},
  {"-.--", 'Y'}, {"--..", 'Z'},
  {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
  {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'},
  {"---..", '8'}, {"----.", '9'}
};
const int morseDictSize = sizeof(morseDict) / sizeof(morseDict[0]);

// ---------- AES Encryption Helper (Uses dynamically set key) ----------
String encryptMessage(String plainText) {
  char encrypted[128];
  int input_len = plainText.length() + 1;

  aesLib.encrypt64(
    (const byte*)plainText.c_str(),
    input_len,
    encrypted,
    (const byte*)currentEncryptionKey, 
    bits,
    iv
  );
  return String(encrypted);
}

// ---------- Morse Decoder ----------
char decodeMorse(String code) {
  for (int i = 0; i < morseDictSize; i++) {
    if (code.equals(morseDict[i].code)) {
      return morseDict[i].letter;
    }
  }
  return '?';
}

// ---------- Send Message (BROADCASTS to both receivers using HOSTNAMES) ----------
void transmitMessage(String intendedTargetHostname, String encryptedMsg) {
  Serial.println("\n--- 📡 BROADCASTING MESSAGE ---");
  
  // Send to Receiver 1
  String url1 = "http://" + String(RECEIVER_1_HOSTNAME) + ":" + String(RECEIVER_PORT) + "/receive";
  Serial.print("Sending to 1 ("); Serial.print(url1); Serial.println(")...");
  
  HTTPClient http1;
  http1.begin(url1);
  http1.addHeader("Content-Type", "text/plain");
  http1.POST(encryptedMsg);
  http1.end();
  
  // Send to Receiver 2
  String url2 = "http://" + String(RECEIVER_2_HOSTNAME) + ":" + String(RECEIVER_PORT) + "/receive";
  Serial.print("Sending to 2 ("); Serial.print(url2); Serial.println(")...");

  HTTPClient http2;
  http2.begin(url2);
  http2.addHeader("Content-Type", "text/plain");
  http2.POST(encryptedMsg);
  http2.end();
  
  Serial.print("Broadcast Complete. Intended Target was: ");
  Serial.println(intendedTargetHostname);
}

// ---------- State Machine Logic ----------
void processDecodedCharacter(char decoded) {
  if (currentState == SELECTING_RECEIVER) {
    // --- STATE: SELECTING_RECEIVER ---
    if (decoded == '1') {
      selectedTarget = RECEIVER_1_HOSTNAME; // Assign Hostname
      memcpy(currentEncryptionKey, KEY_FOR_R1, 17); // Set key to R1's key
      currentState = ENTERING_MESSAGE;
      Serial.println("🎯 Receiver 1 Selected (Key R1). Enter message. (Use 'X' to Send/Exit)");
    } else if (decoded == '2') {
      selectedTarget = RECEIVER_2_HOSTNAME; // Assign Hostname
      memcpy(currentEncryptionKey, KEY_FOR_R2, 17); // Set key to R2's key
      currentState = ENTERING_MESSAGE;
      Serial.println("🎯 Receiver 2 Selected (Key R2). Enter message. (Use 'X' to Send/Exit)");
    } else {
      Serial.print("Invalid Selection ('"); Serial.print(decoded); Serial.println("'). Use '1' or '2'.");
    }
  } 
  
  else if (currentState == ENTERING_MESSAGE) {
    // --- STATE: ENTERING_MESSAGE ---
    
    if (decoded == EXIT_CODE) {
      // 1. Check if there's a message to send
      if (lastMessage.length() > 0 && selectedTarget != "") {
        // 2. Encrypt the message using the currently set key
        String encrypted = encryptMessage(lastMessage);
        
        // 3. Broadcast the encrypted message
        transmitMessage(selectedTarget, encrypted);
      } else {
        Serial.println("❌ No message entered or no receiver selected. Aborting send.");
      }
      
      // 4. Reset state and message buffer
      lastMessage = "";
      selectedTarget = "";
      currentState = SELECTING_RECEIVER;
      Serial.println("\n--- MODE RESET --- Waiting for new receiver selection ('1' or '2').");
    } 
    
    else {
      // 5. Build the message
      lastMessage += decoded;
      Serial.print("Message Building: ");
      Serial.println(lastMessage);
    }
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.print("Connecting to ");
  Serial.println(AP_SSID);
  WiFi.begin(AP_SSID, AP_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  Serial.print("Sender IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("\n--- Initializing ---");
  Serial.println("Mode: SELECTING_RECEIVER. Morse code '1' or '2' to begin.");
}

// ---------- Loop ----------
// ---------- Improved Loop with Debouncing & Debugging ----------
void loop() {
  int buttonState = digitalRead(BUTTON_PIN);
  unsigned long currentMillis = millis();

  // 1. Detect Button Press (with Debounce)
  if (buttonState == LOW && !buttonPressed) {
    delay(50); // 🛑 DEBOUNCE: Wait 50ms to ignore noise
    if (digitalRead(BUTTON_PIN) == LOW) { // Check if still pressed
        buttonPressed = true;
        lastPressTime = currentMillis;
        digitalWrite(BUZZER_PIN, HIGH);
        Serial.print("vvv "); // Visual feedback for press
    }
  }

  // 2. Detect Button Release (with Debounce)
  if (buttonState == HIGH && buttonPressed) {
    delay(50); // 🛑 DEBOUNCE
    if (digitalRead(BUTTON_PIN) == HIGH) { // Check if still released
        buttonPressed = false;
        lastReleaseTime = currentMillis;
        digitalWrite(BUZZER_PIN, LOW);

        unsigned long pressDuration = lastReleaseTime - lastPressTime;
        
        // Determine Dot or Dash
        if (pressDuration < 250) { // Reduced threshold slightly for snappier response
            morseSequence += ".";
            Serial.print(". ");
        } else {
            morseSequence += "-";
            Serial.print("- ");
        }
    }
  }

  // 3. Decode Character Gap (End of Character)
  if (!buttonPressed && morseSequence.length() > 0) {
    unsigned long gap = currentMillis - lastReleaseTime;

    // Wait 1.2 seconds before deciding the character is finished
    if (gap > 1200) { 
      Serial.print("\nSequence detected: ");
      Serial.println(morseSequence); // 👀 SEE WHAT WAS ACTUALLY TYPE

      char decoded = decodeMorse(morseSequence);
      Serial.print("Decoded into: ");
      Serial.println(decoded);

      morseSequence = ""; // Reset for next character
      processDecodedCharacter(decoded);
    }
  }
  
  // 4. Word Gap (Space) logic
  if (!buttonPressed && currentState == ENTERING_MESSAGE && morseSequence.length() == 0) {
    unsigned long gap = currentMillis - lastReleaseTime;
    // Only add space if we haven't already (check if last char wasn't space)
    if (gap > 2500 && lastMessage.length() > 0 && lastMessage.charAt(lastMessage.length()-1) != ' ') {
      lastMessage += " ";
      Serial.println(" [Space added]");
    }
  }
}