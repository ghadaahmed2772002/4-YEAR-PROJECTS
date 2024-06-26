String fan_state = "On";
String window_state = "Open";
String led_state = "On";
//
#include <DHT.h>   // lib for dh11
#include <ESP8266WiFi.h>  
#include <Servo.h>  
#include <PubSubClient.h>  

// Define other variables and functions....


#define DHTPIN D5    
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

int fanPin = D1;     // Pin connected to the fan relay (NodeMCU D1 = Arduino pin D3)
int windowPin = D2;  // Pin connected to the window relay (NodeMCU D3 = Arduino pin D5)
int Speed = 200;    //speed

//photoresistor 
int photoresistor = 0;         
int threshold = 800;          
#define LED_PIN D0

//servo 
Servo windowServo; // Create a servo object
int angle = 90;     // Initial angle for the servo

// Motor Pins    in fan 

#define ENA D1       // Connect ENA to pin D1
#define IN1 D6       // Connect IN1 to pin D6
#define IN2 D7       // Connect IN2 to pin D7

const char* ssid = "DESKTOP-4P5GP29 1448";
const char* password = ")1R6s432";
const char* mqtt_server = "192.168.137.1";  

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {

  if (strcmp(topic, "window_state") == 0) {
    if (strcmp((char*)payload, "Open") == 0) {
      // Code to open the window
      Serial.println("Opening window...");
      windowServo.write(0); // Set servo open
    } else if (strcmp((char*)payload, "Closed") == 0) {
      // Code to close the window
      Serial.println("Closing window...");
      windowServo.write(150); // Set servo to close
    }
  } else if (strcmp(topic, "led_state") == 0) {
    if (strcmp((char*)payload, "On") == 0) {
      // Code to turn on the LED
      Serial.println("Turning LED on...");
      led_on();
    } else if (strcmp((char*)payload, "Off") == 0) {
      // Code to turn off the LED
      Serial.println("Turning LED off...");
      led_off();
    }
  } else if (strcmp(topic, "fan_state") == 0) {
    if (strcmp((char*)payload, "On") == 0) {
      // Code to turn on the fan
      Serial.println("Turning fan on...");
      fan_on();
    } else if (strcmp((char*)payload, "Off") == 0) {
      // Code to turn off the fan
      Serial.println("Turning fan off...");
      fan_stop();
    }
  }
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("NodeMCUClient")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  //motor  in  fan
  pinMode(ENA, OUTPUT);  // Set ENA pin as output
  pinMode(IN1, OUTPUT);  // Set IN1 pin as output
  pinMode(IN2, OUTPUT);  // Set IN2 pin as output

  //photoresistor led out the door 
  pinMode(LED_PIN, OUTPUT); 

  //servo 
  windowServo.attach(windowPin); // Attach the servo to its pin

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);  // arduino subscriber
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(2000);  // Delay between sensor readings

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");
  
  //photsensor 
  photoresistor = analogRead(A0);    
  Serial.println(photoresistor);    
  if (photoresistor < threshold)    
       led_on();    
  else                 
      led_off();     
      delay(100);

  if (temperature < 26) {
    fan_stop();  // Fan is off
    client.publish("fan_state", "Off"); // Publish fan state
  } else {
    fan_on(); // Fan is on
    client.publish("fan_state", "On"); // Publish fan state
  }

  if (humidity >= 40 && humidity <= 60) {
    window_close();  // window closed 
    client.publish("window_state", "Closed"); // Publish window state
  } else {
    window_open();  //window up 
    client.publish("window_state", "Open"); // Publish window state
  }

  windowServo.write(angle); // Move the servo to the specified angle

  char tempString[8];
  dtostrf(temperature, 1, 2, tempString);
  client.publish("temperature", tempString);

  char humString[8];
  dtostrf(humidity, 1, 2, humString);
  client.publish("humidity", humString);

  // Publish LED state
  client.publish("led_state", photoresistor < threshold ? "On" : "Off");

  delay(5000); // Wait 5 seconds before publishing next set of values
}
void window_close() {
    angle = 150; //90
}

void window_open() {
    angle = 0; // Update the global variable
}

void led_on() {
    digitalWrite(LED_PIN, HIGH);
    led_state = "On"; // Update the global variable
    client.publish("led_state", "On"); // Publish LED state
}

void led_off() {
    digitalWrite(LED_PIN, LOW);
    led_state = "Off"; // Update the global variable
    client.publish("led_state", "Off"); // Publish LED state
}

void fan_on() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, Speed);
    fan_state = "On"; // Update the global variable
    client.publish("fan_state", "On"); // Publish LED state
}


void fan_stop() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
    fan_state = "Off"; // Update the global variable
    client.publish("fan_state", "Off"); // Publish LED state
}



// void loop() {
//   if (!client.connected()) {
//     reconnect();
//   }
//   client.loop();

//   delay(2000);  // Delay between sensor readings

//   float humidity = dht.readHumidity();
//   float temperature = dht.readTemperature();

//   if (isnan(humidity) || isnan(temperature)) {
//     Serial.println("Failed to read from DHT sensor!");
//     return;
//   }

//   Serial.print("Humidity: ");
//   Serial.print(humidity);
//   Serial.print(" %\t Temperature: ");
//   Serial.print(temperature);
//   Serial.println(" *C");
  
//   //photsensor 
//   photoresistor = analogRead(A0);    
//   Serial.println(photoresistor);    
//   if (photoresistor < threshold)    
//        digitalWrite(LED_PIN, HIGH);    
//   else                 
//     digitalWrite(LED_PIN, LOW);     
//     delay(100);

//   if (temperature < 26) {
//     fan_stop();  // Fan is off
//   } else {
//     fan_on(); // Fan is on
//   }

//   if (humidity >= 40 && humidity <= 60) {
//     angle = 150;  // window closed 
//   } else {
//     angle = 0;  //window up 
//   }

//   windowServo.write(angle); // Move the servo to the specified angle

//   char tempString[8];
//   dtostrf(temperature, 1, 2, tempString);
//   client.publish("temperature", tempString);

//   char humString[8];
//   dtostrf(humidity, 1, 2, humString);
//   client.publish("humidity", humString);

//   delay(5000); // Wait 5 seconds before publishing next set of values
// }