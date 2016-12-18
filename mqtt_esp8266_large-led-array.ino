/*

  TCL IOT script
  Tim Waizenegger
  ESP8266 arduino, pusub mqtt client
  created:        12/2016
  Last change:
*/
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
//#include <ArduinoOTA.h>

const char* ssid = "hive_iot";
const char* password = "xxx";
const char* mq_id = "large-led-array";
const char* mq_user = "large-led-array";
const char* mq_authtoken = "xxx";
const char* mq_clientId = mq_id;
const char* mq_serverUrl = "timsrv.de";

const int pin_r = 13;
const int pin_g = 14;
const int pin_b = 12;
const int pin_mqttled = 16;

int current_r = 0;
int current_g = 0;
int current_b = 0;
boolean is_on = false;

WiFiClient espClient;
PubSubClient client(mq_serverUrl, 1883, espClient);




/*
 * *************************************************************************************
   MQTT handler
 * *************************************************************************************
*/
void publish_states() {
  char values[30];
  int l = sprintf(values, "%d,%d,%d", current_r, current_g, current_b);
  values[l] = 0;
  Serial.println(values);
  client.publish("large_led_array/rgb/get", values);
  if (is_on) {
    client.publish("large_led_array/state/get", "ON");
  }
  else {
    client.publish("large_led_array/state/get", "OFF");
  }

}


void receive_message(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char msg[length + 2];
  for (int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  msg[length] = ','; // add a , at the end so that we can use strchr for all three values
  msg[length + 1] = 0; // null-terminate the string
  Serial.println();


  /*
    Here we need to split the command string into its fields:
    input looks like this 13,36,234 we modified it to: 13,36,234,
    and we need to get three ints

  */
  if (strcmp(topic, "large_led_array/rgb/set") == 0) {
    Serial.println("RGB SET");
    char* r_raw = strtok(msg, ",");
    char* pos1 = strchr(msg, ',');
    char* g_raw = strtok(pos1, ",");
    char* pos2 = strchr(pos1, ',');
    char* b_raw = strtok(pos1, ",");

    int r = atoi(r_raw);
    int g = atoi(g_raw);
    int b = atoi(b_raw);

    set_rgb(r, g, b);

  }
  if (strcmp(topic, "large_led_array/state/set") == 0) {

    Serial.println(msg);
    if (strcmp(msg, "OFF,") == 0) {
      Serial.println("state set off");
      is_on = false;
      // quit the PWM and turn all LEDs off
      analogWrite(pin_r, 1023);
      analogWrite(pin_g, 1023);
      analogWrite(pin_b, 1023);

      // we still set/publish a PWM state for consistency
      current_r = 0;
      current_g = 0;
      current_b = 0;
    }
    else if (strcmp(msg, "ON,") == 0)  {
      Serial.println("state set on");
      is_on = true;
      /*
        the ON command is sent with each RGB change so here we only want to
        handle the situation where ON was sent when the state was OFF before
        otherwise, ON doesn't need to be handled.
      */
      if (current_r + current_g + current_b == 0) {
        set_rgb(150, 40, 40);
      }

    }
  }


  // publish the changed states
  publish_states();

}


/*
 * *************************************************************************************
   LEDs / PWM handling
 * *************************************************************************************
*/

void set_rgb(int r, int g, int b) {
  /*
    input is in the range 0 .. 255 and 255 is the brightest
    we need to adjust the PWM driver in the range 0 .. 1023 and 0 is the brightest
    but the actual range is 0 .. 100 because 100 is already dark/off
  */
  current_r = r;
  current_g = g;
  current_b = b;

  int pwm_r = 255 - current_r;
  int pwm_g = 255 - current_g;
  int pwm_b = 255 - current_b;

  analogWrite(pin_r, pwm_r);
  analogWrite(pin_g, pwm_g);
  analogWrite(pin_b, pwm_b);

}




/*
 * *************************************************************************************
   ARDUINO API
 * *************************************************************************************
*/



void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  delay(100);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}




void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mq_clientId, mq_user, mq_authtoken)) {
      Serial.println("connected");
      digitalWrite(pin_mqttled, 1);
      // ... and resubscribe
      client.subscribe("large_led_array/state/set");
      client.subscribe("large_led_array/rgb/set");
    } else {
      Serial.print("failed");
      Serial.println(" try again in 1 second");
      // Wait 1 second before retrying
      delay(1000);
    }
  }
}




//////////////////////////////////////////////////////////////////////////////////

void setup() {
  pinMode(pin_mqttled, OUTPUT);
  pinMode(pin_r, OUTPUT);
  pinMode(pin_g, OUTPUT);
  pinMode(pin_b, OUTPUT);
  digitalWrite(pin_mqttled, 0);
  // set the LEDs dark initially
  digitalWrite(pin_r, 1);
  digitalWrite(pin_g, 1);
  digitalWrite(pin_b, 1);


  Serial.begin(57600);
  Serial.println("hi");
  Serial.println(ESP.getFreeSketchSpace());


  client.setCallback(receive_message);
  setup_wifi();


  reconnect();

  //ArduinoOTA.setPort(8266);
  //ArduinoOTA.setHostname(mq_id);
  //ArduinoOTA.setPassword(mq_authtoken);
  //ArduinoOTA.begin();
}



void loop() {

  if (!client.connected()) {
    digitalWrite(pin_mqttled, 0);
    reconnect();
  }


  client.loop();
  //ArduinoOTA.handle();



}
