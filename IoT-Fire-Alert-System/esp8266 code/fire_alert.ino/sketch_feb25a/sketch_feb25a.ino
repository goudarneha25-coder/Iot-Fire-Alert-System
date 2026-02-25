#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// -------- WIFI --------
#define WIFI_SSID "RAAVANA"
#define WIFI_PASSWORD "JAMESBOND"

// -------- FIREBASE --------
#define API_KEY "AIzaSyC1agUl8MNoEZ-6AtlcLI62Jai4_BlEOcU"
#define DATABASE_URL "https://fire-alert-d5570-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// -------- PINS --------
#define MQ2 A0
#define FLAME D2
#define BUZZER D5
#define GREEN_LED D6
#define RED_LED D7

int baseline = 0;
int threshold = 0;

unsigned long previousMillis = 0;
const long interval = 2000;

void calibrateSensor() {
  Serial.println("Calibrating...");
  long sum = 0;

  for (int i = 0; i < 30; i++) {
    sum += analogRead(MQ2);
    delay(200);
  }

  baseline = sum / 30;
  threshold = baseline + 100;

  Serial.print("Baseline: ");
  Serial.println(baseline);
  Serial.print("Threshold: ");
  Serial.println(threshold);
}

void setup() {

  Serial.begin(115200);

  pinMode(FLAME, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(GREEN_LED, HIGH);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  // Firebase Config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Anonymous SignUp
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase SignUp OK");
  } else {
    Serial.printf("SignUp error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  calibrateSensor();
}

void loop() {

  if (millis() - previousMillis > interval) {
    previousMillis = millis();

    int gasValue = analogRead(MQ2);
    bool flameDetected = digitalRead(FLAME) == LOW;
    bool fireStatus = (gasValue > threshold) || flameDetected;

    Serial.print("Gas: ");
    Serial.print(gasValue);
    Serial.print(" | Fire: ");
    Serial.println(fireStatus);

    if (fireStatus) {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BUZZER, HIGH);
    } else {
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(BUZZER, LOW);
    }

    // ---- FIREBASE UPDATE ----
    if (Firebase.ready()) {

      Firebase.RTDB.setInt(&fbdo, "/gasLevel", gasValue);
      Firebase.RTDB.setBool(&fbdo, "/fireStatus", fireStatus);
      Firebase.RTDB.setInt(&fbdo, "/threshold", threshold);

      Serial.println("Firebase Updated");
    }
  }
}