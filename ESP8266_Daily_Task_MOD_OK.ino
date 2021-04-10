/*
 * ESP8266 Daily Task
 * Rui Santos 
 * Complete Project Details https://randomnerdtutorials.com
 */
 
#include <ESP8266WiFi.h>
#include <arduino.h>

#define PIN_NUMBER 5 // Pin number for MicroWave Sensor (GPIO5 = D1)
#define AVERAGE 2 // Number of samples for averaging

unsigned int doppler_div = 19;
unsigned int samples[AVERAGE];
unsigned int x;
unsigned int Freq;
static char SpeedTemp[7]; //Temporary Speed holder
float Speed = 0; //Speed value transferrred to Server if OK
float MeasuredSpeed=0;// Measured Speed
float MinSpeed=3;
float MaxSpeed=150;

const long ServerUpdateInterval = 15500; // Sever update interval shall be larger > 15 sec
unsigned long LastMillis = 0;
unsigned long CurrentMillis = 0;

// Replace with your SSID and Password
const char* ssid     = "HomeBox-1560_2.4G";
const char* password = "2d964eacf";

// Replace with your unique Thing Speak WRITE API KEY
const char* apiKey = "6TDSQMJUK9I3OKYS";

const char* resource = "/update?api_key=";

// Thing Speak API server 
const char* server = "api.thingspeak.com";

// Establish a Wi-Fi connection with your router
void initWifi() {
  Serial.print("Connecting to: "); 
  Serial.print(ssid);
  WiFi.begin(ssid, password);  

  int timeout = 10 * 4; // 10 seconds
  while(WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");

  if(WiFi.status() != WL_CONNECTED) {
     Serial.println("Failed to connect, going back to sleep");
  }

  Serial.print("WiFi connected in: "); 
  Serial.print(millis());
  Serial.print(", IP address: "); 
  Serial.println(WiFi.localIP());
}

// Make an HTTP request to Thing Speak
void makeHTTPRequest() {
    dtostrf(Speed, 6, 2, SpeedTemp);
    // You can delete the following Serial.print's, it's just for debugging purposes
    Serial.print("Hastighed: ");
    Serial.print(Speed);
    Serial.print("Connecting to "); 
    Serial.print(server);
  
  WiFiClient client;
  int retries = 10;
  while(!!!client.connect(server, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if(!!!client.connected()) {
     Serial.println("Failed to connect, going back to sleep");
  }
  
  Serial.print("Request resource: "); 
  Serial.println(resource);
  client.print(String("GET ") + resource + apiKey + "&field1=" + SpeedTemp + 
                  " HTTP/1.1\r\n" +
                  "Host: " + server + "\r\n" + 
                  "Connection: close\r\n\r\n");
                  
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
     Serial.println("No response, going back to sleep");
  }
  while(client.available()){
    Serial.write(client.read());
  }
  Speed=0;
  Serial.println("\nclosing connection");
  client.stop();
}


//**********************************************  S E T U P  **************************************//
void setup() {
  // Initializing serial port for debugging purposes
  Serial.begin(115200);
  pinMode(PIN_NUMBER, INPUT); 
  delay(10);
  CurrentMillis=millis();
  LastMillis=millis();
}
//**********************************************  E N D   S E T U P  ******************************//



//**********************************************  L O O P  ****************************************//
void loop() {
  
  noInterrupts();
  pulseIn(PIN_NUMBER, HIGH);
  unsigned int pulse_length = 0;
  for (x = 0; x < AVERAGE; x++){
  pulse_length = pulseIn(PIN_NUMBER, HIGH);
  pulse_length += pulseIn(PIN_NUMBER, LOW);
  samples[x] = pulse_length;
  }
  interrupts();

    // Check for consistency
    bool samples_ok = true;
    unsigned int nbPulsesTime = samples[0];
    for (x = 1; x < AVERAGE; x++){
    nbPulsesTime += samples[x];
    if ((samples[x] > samples[0] * 2) || (samples[x] < samples[0] / 2))
    {
    samples_ok = false;
    }
    }
    if (samples_ok){

    unsigned int Ttime = nbPulsesTime / AVERAGE;

    if (Ttime>0) Freq = 1000000 / Ttime; else Freq = 0;

    MeasuredSpeed=Freq/doppler_div;
    Serial.print("\r\n");
    Serial.print(Freq);
    Serial.print("Hz : ");
    Serial.print(MeasuredSpeed);
    Serial.print("km/h\r\n");
    delay(100);

    if (MeasuredSpeed>MinSpeed && MeasuredSpeed<MaxSpeed){
      if (MeasuredSpeed>Speed){
        Speed=MeasuredSpeed;
      }
    }
    }
    CurrentMillis=millis();
    if (CurrentMillis-LastMillis>=ServerUpdateInterval && Speed>0) { // Only update server every ServerUpdateInterval
      initWifi();
      Serial.print("Now transferring: ");
      Serial.print(Speed);
      makeHTTPRequest();
      LastMillis=CurrentMillis;
    }
}
//**********************************************  E N D   L O O P  ********************************//
