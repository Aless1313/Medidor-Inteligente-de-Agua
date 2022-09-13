#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266Wifi.h>
#include <dht.h>
#include <ArduinoJson.h>


#define dht_apin 0
dht DHT;


const char* ssid = "Fisica Pato2";
const char* password = "q1w2e3r4t5";

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_user ="admin";
const char* mqtt_pass = "public";

int led = 16;
WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[25];
char msg2[25];

/*Sensor de flujo de agua*/
#define SENSORAGUA 14
#define wifi_btn 13
#define wps_btn 12


long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1sec =0;
float flowRate;
unsigned long flowMililliLitres;
unsigned long totalMilliLitres;
float flowLitres;
float totalLitres;

void setup_wifi();
void callback(char* topic, byte* payload, int length);
void reconnect();


void IRAM_ATTR pulseCounter(){
  pulseCount++;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) continue;
  pulseCount = 0;
  flowRate = 0;
  flowMililliLitres = 0;
  totalLitres = 0;

  pinMode(wps_btn, INPUT);
  pinMode(wifi_btn, INPUT);

  if(digitalRead(wps_btn) == HIGH){
    Serial.println("wps");
  }

  if(digitalRead(wifi_btn) == HIGH){
    Serial.println("wifi");
  }

  setup_wifi();

  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);



  attachInterrupt(digitalPinToInterrupt(SENSORAGUA), pulseCounter, FALLING);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {

  if(!client.connected()){
    reconnect();
  }
  client.loop();
  // put your main code here, to run repeatedly:
  currentMillis = millis();

  StaticJsonDocument<256> doc;
  doc["Device"]="ESP8266";
  doc["time"]=123123;

   
  if(currentMillis - previousMillis > interval){
    pulse1sec = pulseCount;
    pulseCount = 0;

    flowRate = ((1000.0 /(millis() - previousMillis)) * pulse1sec) / calibrationFactor;
    previousMillis = millis();

     flowLitres = (flowRate/60);
    totalLitres += flowLitres;

    Serial.print("Flujo de agua: ");
    Serial.print(float(flowRate));
    Serial.print("L/Min");
    Serial.print("\t");

    Serial.print("Litros consumidos: ");
    Serial.print(totalLitres);
    Serial.println("L");

    String flujo = String(flowRate, 3); 
    String to_Send = "F:" + flujo + " L:" + totalLitres;
    to_Send.toCharArray(msg, 25);
    client.publish("values", msg);
    Serial.println("Enviado a topico");

   
    
    DHT.read11(dht_apin);
    
    Serial.print("Current humidity = ");
    Serial.print(DHT.humidity);
    Serial.print("%  ");
    Serial.print("temperature = ");
    Serial.print(DHT.temperature); 
    Serial.println("C  ");

    JsonArray data = doc.createNestedArray("data");
    data.add(flowRate);                               //Flujo
    data.add(totalLitres);                            //Litros
    data.add(DHT.temperature);                        //Temperatura
    data.add(DHT.humidity);                           //Humedad
    data.add(0);                                      //Estado de valvula

    char out[128];
    int b = serializeJson(doc, out);
    Serial.print("bytes = ");
    Serial.println(b,DEC);
    
    client.publish("dada", out);

  
   

    
  }
    
  /*
  if(currentMillis - previousMillis > 5000){
    String to_Send = "Test";
    to_Send.toCharArray(msg, 25);

    Serial.println("Enviado a topico");
    client.publish("values", msg);
  }*/

}

void setup_wifi(){
  delay(10);
  Serial.println("");
  Serial.print("Conectando a.. ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(400);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Conectado con IP..");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, int length ){
  String incoming = "";
  Serial.print("Mensaje recibido de ->");
  Serial.print(topic);
  Serial.println("");

  for(int i=0; i<length; i++){
    incoming += (char)payload[i];
  }

  incoming.trim();
  Serial.println("Mensaje -> " + incoming);

  if(incoming == "ON"){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  
  if(incoming == "OFF"){
      digitalWrite(LED_BUILTIN, LOW);
  }
}

void reconnect(){
  while(!client.connected()){
    Serial.print("Conectando a MQTT...");
    String clientId = "ESP8266";
    clientId += String(random(0xffff), HEX);

    if(client.connect(clientId.c_str(), mqtt_user, mqtt_pass)){
      Serial.println("Conectado a mqtt");
      client.subscribe("led");
      Serial.println("Subscrito a topico");
    }else{
      Serial.println("Fallo ->" + client.state());
      delay(2500);
    }
  }
}