#include <Arduino.h>

#include <PubSubClient.h>
#include <ESP8266Wifi.h>
#include <ESP8266WebServer.h>
#include <dht.h>

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define dht_apin 0
dht DHT;

int contconexion = 0;
char ssid[50];
char pass[50];
const char *ssidconf = "Configuracion Dispositivo";
const char *passconf = "";
//Configuracion IP estatica en access point
IPAddress ip(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

//-----------CODIGO HTML PAGINA DE CONFIGURACION---------------
String pagina = "<!DOCTYPE html>"
"<html>"
"<head>"
"<title>Tutorial Eeprom</title>"
"<meta charset='UTF-8'>"
"</head>"
"<body>"
"</form>"
"<form action='guardar_conf' method='get'>"
"SSID:<br><br>"
"<input class='input1' name='ssid' type='text'><br>"
"PASSWORD:<br><br>"
"<input class='input1' name='pass' type='password'><br><br>"
"<input class='boton' type='submit' value='GUARDAR'/><br><br>"
"</form>"
"<a href='escanear'><button class='boton'>ESCANEAR</button></a><br><br>";
String paginafin = "</body>"
"</html>";
String mensaje = "";

/*SETUP WIFI*/
void setup_wifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidconf,passconf);
  while(WiFi.status() != WL_CONNECTED and contconexion <50){
    ++contconexion;
    delay(350);
    Serial.print(".");
  }
  if(contconexion<50){
    Serial.print("");
    Serial.println("Conectado");
    Serial.print(WiFi.localIP());
  }else{
    Serial.print("Error de conexión");
  }
}

/*Setup de reloj*/
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.south-america.pool.ntp.org",-21600,6000);

/*Setup de mqtt*/
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_user ="admin";
const char* mqtt_pass = "public";

int led = 16;
WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);

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
void setuppage();
void saveeeprom();
void save_conf();
void modeconf();
String leer(int addr);


void IRAM_ATTR pulseCounter(){
  pulseCount++;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
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
    modeconf();
  }

  modeconf();

  delay(3000);
  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(pass, 50);

  while (WiFi.status() != WL_CONNECTED){
  setup_wifi();
  }

  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);

  timeClient.begin(); 

 

  attachInterrupt(digitalPinToInterrupt(SENSORAGUA), pulseCounter, FALLING);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  timeClient.update();
  if(!client.connected()){
    reconnect();
  }
  client.loop();
  // put your main code here, to run repeatedly:
  currentMillis = millis();

  StaticJsonDocument<256> doc;
  doc["Device"]=WiFi.macAddress();
  doc["time"]=timeClient.getFormattedTime();
  doc["date"]=timeClient.getDay();
 
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
    String to_Send = flujo;
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

    char out[256];
    int b = serializeJson(doc, out);
    Serial.print("bytes = ");
    Serial.println(out);
    Serial.print(b);
    
    client.publish("dada", out);
    
  }
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

String leer(int addr){
  byte lectura;
  String strlectura;
  for(int i = addr; i < addr+50; i++){
    lectura = EEPROM.read(i);
    if(lectura != 255){
      strlectura += (char)lectura;
    }
  }
  return strlectura;
}

void setuppage(){
    server.send(200, "text/html", pagina + mensaje + paginafin); 
}

void scan(){
  int n = WiFi.scanNetworks();
  Serial.println("Escaneo terminado");
  if(n == 0){
    Serial.println("Sin redes disponibles");
    mensaje = "Sin redes disponibles";
  }else{
    Serial.print(n);
    Serial.println("Redes disponibles");
    mensaje="";
    for(int i = 0; i<n; ++i){
      mensaje = (mensaje) + "<p>" + String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") Ch: " + WiFi.channel(i) + " Enc: " + WiFi.encryptionType(i) + " </p>\r\n";
      delay(10);
    }
    Serial.println(mensaje);
    setuppage();
  }
}

/*Funcion para guardar red configurada en la eeprom*/
void saveeeprom(int addr, String a){
  int tamano = a.length();
  char inchar[50];
  a.toCharArray(inchar, tamano+1);
  for(int i = 0; i < tamano; i++){
    EEPROM.write(addr+i, inchar[i]);
  }
  for(int i = tamano; i<50; i++){
    EEPROM.write(addr+i, 255);
  }
  EEPROM.commit();
}

void save_conf(){
  Serial.println(server.arg("ssid"));
  saveeeprom(0, server.arg("ssid"));
  Serial.println(server.arg("pass"));
  saveeeprom(50,server.arg("pass"));
  mensaje="Red configurada en el dispositivo";
  setuppage();
}

void modeconf(){
  Serial.println("Modo AccessPoint");
  WiFi.softAP(ssidconf, passconf);
  //WiFi.softAPConfig(ip,gateway,subnet);
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);
  Serial.println("Server iniciado");

  server.on("/", setuppage);
  server.on("/guardar_conf", save_conf);
  server.on("/escanear",scan);
  server.begin();

  while(true){
    server.handleClient();
  }
}
