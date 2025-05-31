#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Define GPIO pins
#define LED_PIN  2      // Built-in LED
#define BUZZER_PIN  19   // Connect buzzer here

// Replace with your credentials
const char* ssid = "gateway";
const char* password = "SampleQr@99Rs";

// Create AsyncWebServer on port 80
AsyncWebServer server(80);


void logHttp(AsyncWebServerRequest *req, int status, const String &body = "") {
  time_t now = time(nullptr);
  struct tm *t = gmtime(&now);

  char ts[21];  // "YYYY-MM-DDTHH:MM:SSZ"
  strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", t);

  Serial.print(ts);
  Serial.print(" - ");
  Serial.print(req->methodToString());
  Serial.print(" - [");
  Serial.print(status);
  Serial.print("] - [");
  Serial.print(req->client()->remoteIP());
  Serial.print("] - ");
  Serial.print(req->url());
  Serial.print(" - body:");
  Serial.println(body);
}



void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected: " + WiFi.localIP().toString());

  // Define REST API endpoint
  server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request){},
  NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Print raw body
    String rawBody;
    for (size_t i = 0; i < len; i++) {
      rawBody += (char)data[i];
    }

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
      Serial.println("JSON parse error:");
      Serial.println(error.c_str());
      request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }

    // Process input
    if (doc.containsKey("led")) {
      digitalWrite(LED_PIN, doc["led"].as<bool>() ? HIGH : LOW);
    }
    if (doc.containsKey("buzzer")) {
      digitalWrite(BUZZER_PIN, doc["buzzer"].as<bool>() ? HIGH : LOW);
    }
    // log request
    logHttp(request, 200, rawBody);

    // Echo back
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });


  server.begin();
}

void loop() {
  // Nothing needed here due to AsyncWebServer
}

