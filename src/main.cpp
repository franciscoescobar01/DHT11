#import <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <Ethernet.h>
#include <SPI.h>
#include <DHT.h>


//Constantes de coneccion a WiFi
const char* ssid = "CEISUFRO";
const char* password = "DCI.2016";

//Constantes de servidor MQTT
const char* mqtt_server = "ipame.cl";
const char* mqtt_topic = "temperatura/humedad";

//Objeto sensor DHT11
DHT dht(D3, DHT11);


// clientes
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


void setup_wifi() {
  delay(100);
  Serial.print("Conectando a: ");
  Serial.println(ssid);
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("Coneccion WiFi realizada");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Command is : [");
  Serial.print(topic);
  int p =(char)payload[0]-'0';
  int chk = dht.readTemperature(true);
  // if MQTT comes a 0 message, show humidity
  if(p==0)
  {
    Serial.println("to show humidity!]");
    Serial.print(" Humidity is: " );
    Serial.print(dht.readHumidity(), 1);
    Serial.println('%');
  }
  // if MQTT comes a 1 message, show temperature
  if(p==1)
  {
  // digitalWrite(BUILTIN_LED, HIGH);
  Serial.println(" is to show temperature!] ");
  int chk = dht.readTemperature(true);
   Serial.print(" Temp is: " );
   Serial.print(chk, 1);
   Serial.println(" °C");
  }
  Serial.println();
} //end callback

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(6000);
    }
  }
} //end reconnect()

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  int chk = dht.readTemperature(true);
  Serial.print(" Starting Humidity: " );
  Serial.print(dht.readHumidity(), 1);
  Serial.println('%');
  Serial.print(" Starting Temparature ");
  Serial.print(chk, 1);
  Serial.println('C');

  Serial.println("Setup!");
}

void loop() {
  char message[58];
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  // read DHT11 sensor every 6 seconds
  if (now - lastMsg > 6000) {



    lastMsg = now;
    float chk = dht.readTemperature(true);
    chk = (chk-32)/1.80;
    String msg="";
    msg= msg+ chk;
    msg = msg+"," ;
    float hum = dht.readHumidity();
    msg=msg+hum;

    msg.toCharArray(message,58);
    Serial.println(message);
     //publish sensor data to MQTT broker
    client.publish(mqtt_topic, message);
    //webservice datos unidad_medida_1=Temperatura+C&dato_1=90&unidad_medida_2=Humedad++%25&dato_2=55&codigo_sensor=Humedad+y+Temperatura&accion=agregar
    String data = "unidad_medida_1=Temperatura+C&dato_1=";
    data = data + chk;
    data = data + "&unidad_medida_2=Humedad&dato_2=";
    data = data + hum;
    data = data + "&codigo_sensor=Humedad+y+Temperatura&accion=agregar";

    if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status

    HTTPClient http;    //Declare object of class HTTPClient

    http.begin("http://praga.ceisufro.cl/ipame_dev/index.php?eID=obtenerDatos");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpCode = http.POST(data);   //Send the request
    String payload = http.getString();                  //Get the response payload

    Serial.println(httpCode);   //Print HTTP return code
    Serial.println(payload);    //Print request response payload

    http.end();  //Close connection

    }else{

    Serial.println("Error in WiFi connection");
    }

    Serial.println(data);
    Serial.print("Response body from server: ");
    Serial.println(message);
  }


}
