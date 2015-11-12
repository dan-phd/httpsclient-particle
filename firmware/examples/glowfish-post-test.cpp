// Dependencies: httpsclient-particle, Sparkjson
// Overview: Collect system information from the photon, post it to Glowfi.sh
//           Specifically, data posted to signal_extract endpoint of glowfish
//           glowfi.sh then posts it to librato for data visualization
//           You can have a look at one of the photons posting here:
//           https://metrics.librato.com/s/public/cumm5prtk?duration=86400
//           signal_extract is a simple glowfi.sh endpoint which despikes
#include "httpsclient-particle/httpsclient-particle.h"

static int anomalyLed = D7;
static int heartbeatLed = D7;

const bool g_https_trace = true;  // This controls debug info print to Serial
const char host [] = "api.glowfi.sh";
const char ad_endpoint [] = "/v1/anomaly_detect/";
const char se_endpoint [] = "/v1/signal_extract/";
const int g_port = 443; // Don't change this, unless you know what you are doing
static unsigned int freemem;
bool g_https_complete;
uint32 g_bytes_received;
unsigned char *received_msg;

TCPClient client;

#define GF_JSON_SIZE 300

// Replace XXXX...XXX with base64 encoding if your gf username:password
// If you don't know how to generate the base64 encoding go here:
//    http://www.tuxgraphics.org/toolbox/base64-javascript.html
// CAUTION: Do NOT remove/replace the word Basic from the string above,
//          it's part of http standard.
unsigned char httpRequestContent[] = "POST %s HTTP/1.0\r\n"
  "User-Agent: MatrixSSL/" MATRIXSSL_VERSION "\r\n"
  "Authorization: Basic XXXX...XXX\r\n"
  "Host: api.glowfi.sh\r\n"
  "Accept: */*\r\n"
  "Content-Type: applcation/json\r\n"
  "Content-Length: %d\r\n\r\n%s";

void setup() {
  if (g_https_trace) {
    Serial.begin(9600);
  }
  pinMode(anomalyLed, OUTPUT);
  httpsclientSetup(host, se_endpoint);
}

unsigned int nextTime = 0;    // Next time to contact the server
int g_connected;
void loop() {
  unsigned int t = millis();
  if (nextTime > t) return;
  StaticJsonBuffer<GF_JSON_SIZE> glowfishJson;
  JsonObject& top = glowfishJson.createObject();
  top["device_id"] = (const char *) System.deviceID();
  JsonObject& data_set = top.createNestedObject("data_set");
  // TODO: I don't quite understand how much memory needs to get allocated
  //JsonObject& time = top.createNestedArray("time");
  JsonArray& freememJ = data_set.createNestedArray("freemem");
  freememJ.add(System.freeMemory());
  JsonArray& timeup = data_set.createNestedArray("timeup");
  timeup.add(t/1000);
  char jsonBuf[GF_JSON_SIZE];
  size_t bufsize = top.printTo(jsonBuf, sizeof(jsonBuf));
  g_connected = client.connect(host, g_port);
  if (!g_connected) {
    client.stop();
    // If TCP Client can't connect to host, exit here.
    return;
  }
  g_https_complete = false;
  g_bytes_received = 0;
  if (g_https_trace) {
    Serial.print("free memory: ");
    Serial.println(freemem);
  }
  int rc;
  httpsclientSetPath(se_endpoint);
  if ((rc = httpsClientConnection(httpRequestContent, bufsize, jsonBuf, received_msg)) < 0) {
    // TODO: When massive FAIL
    if (g_https_trace) {
      Serial.print("httpsClientConnection Returned ");
      Serial.println(rc);
    }
    httpsclientCleanUp();
    // Blink an LED twice to indicate trouble
    digitalWrite(anomalyLed, HIGH);
    delay(500);
    digitalWrite(anomalyLed, LOW);
    delay(500);
    digitalWrite(anomalyLed, HIGH);
    delay(500);
    digitalWrite(anomalyLed, LOW);
    return;
  } else {
    // Blink an LED once to indicate success
	Serial.println(received_msg);
    digitalWrite(heartbeatLed, HIGH);
    delay(250);
    digitalWrite(heartbeatLed, LOW);
  }
  client.stop();
  nextTime = millis() + 5000;
}
