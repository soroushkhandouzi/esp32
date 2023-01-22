/* ====== Libraries which must be installed ====== */

#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <PubSubClient.h>

/* ====== Files that import from the same directory ====== */
#include "mqtt_client_credentials.h"
#include "esp_wpa2.h"
#include "wpa2_enterprise_credentials.h"
#include "ntp_server.h"
#include "time.h"


/* ====== NTP server and PARAMETERS ====== */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntp_ip, 0, 60000);



//---------------------------------------------------------------------------------------
/* ====== Get the current timestamp (unix) ====== */
unsigned long epochTime;
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

//---------------------------------------------------------------------------------------
/* ====== Sensor PARAMETERS ====== */

#define TRIG_PIN 23 // ESP32 pin GIOP23 connected to Ultrasonic Sensor's TRIG pin
#define ECHO_PIN 22 // ESP32 pin GIOP22 connected to Ultrasonic Sensor's ECHO pin

float duration_us, distance_cm,distance_result;

//---------------------------------------------------------------------------------------
/* ====== WIFI PARAMETERS ====== */
  const char* ssid = WIFI_SSID; // your ssid
  #define EAP_IDENTITY "WIFI_Identity" // your user id
  #define EAP_USERNAME "WIFI_Username" 
  #define EAP_PASSWORD "WIFI_Password"

//---------------------------------------------------------------------------------------
/* ====== MQTT PARAMETERS ====== */


#define MQTT_MSG_BUFF_LEN 150 // length of your message
char msg_buff[MQTT_MSG_BUFF_LEN] = {};

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

//---------------------------------------------------------------------------------------
/* ====== MQTT FUNCTIONS ====== */
void sendMqttMessage(String msg)
{
  Serial.print("Publish message: ");
  Serial.println(msg);
  msg.toCharArray(msg_buff, MQTT_MSG_BUFF_LEN);
  mqtt_client.publish(MQTT_PUB_TOPIC, msg_buff);
}

void setupMqttClient()
{
  mqtt_client.setServer(MQTT_BROKER_ADDR, MQTT_BROKER_PORT);
}

bool connectMqttClient()
{
  if (mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS))
  {
    return true;
  }
  else
  {
    return false;
  }
}

float sensordistance(){
// generate 10-microsecond pulse to TRIG pin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // measure duration of pulse from ECHO pin
  duration_us = pulseIn(ECHO_PIN, HIGH);

  // calculate the distance
  distance_cm = 0.017 * duration_us;

  // print the value to Serial Monitor
  Serial.print("distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");
  return distance_cm;

}
void setup()
{
  Serial.begin(9600);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // configure the trigger pin to output mode
  pinMode(TRIG_PIN, OUTPUT);
  // configure the echo pin to input mode
  pinMode(ECHO_PIN, INPUT);
  
  // WPA2 enterprise magic starts here
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA); //init wifi mode

  // Example1 (most common): a cert-file-free eduroam with PEAP (or TTLS)
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
  // WPA2 enterprise magic ends here
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  timeClient.setTimeOffset(3600);

  }

void loop()
  {
  epochTime = getTime();
  timeClient.update();
    
    
  unsigned long unix_time = timeClient.getEpochTime();
  Serial.println(unix_time);
  
  //setup MQTT client
  setupMqttClient();

  do{
    delay(100);
  }while(!connectMqttClient());
  delay(60000);
  if (connectMqttClient())
  {
    distance_result = sensordistance();
    String mac = WiFi.macAddress();
    String conn_msg = "{\"id\":\""+mac+"\",\"timestamp:\":"+unix_time+", \"value:\":"+distance_result+"}";
    sendMqttMessage(conn_msg);
    Serial.println(WiFi.localIP());
    Serial.println("Mandato?");
    
    mqtt_client.disconnect();
  }
  Serial.println("will wait here");
  delay(60000);
  Serial.println("waiting here is finished!!");
  mqtt_client.loop();
  }
