#include <Arduino.h>

//Libreria para conexión a internet
#include <PubSubClient.h>
#include <ESP8266Wifi.h>
#include <ESP8266WebServer.h>

//Libreria para sensor de temperatura y humedad
#include <dht.h>

//Librerias para EEPROM, JSON y Hora de internet
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//Librerias para pantalla oled
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*Definicion de pantalla*/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Definición de sensor de temperatura
#define dht_apin 0
dht DHT;

/*Variables de conexion a internet*/
int contconexion = 0;
char ssid[50];
char pass[50];
const char *ssidconf = "Configuracion Dispositivo";
const char *passconf = "";
//Configuracion IP estatica en access point
IPAddress ip(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);


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

/*Setup de reloj*/
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.south-america.pool.ntp.org",-21600,6000);

/*Setup de mqtt*/
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_user ="admin";
const char* mqtt_pass = "public";

int led = 16;

/*Mensajeria entrante de mqtt*/
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

/*Funcion para leer pulsos del sensor de flujo de agua*/
void IRAM_ATTR pulseCounter(){
  pulseCount++;
}

void setup() {
  //Iniciando comunicación serial
  Serial.begin(9600);

  //Iniciando memoria eeprom para guardar credenciales de wifi
  EEPROM.begin(512);

  //Iniciando valores de sensor en 0 para su primer uso
  pulseCount = 0;
  flowRate = 0;
  flowMililliLitres = 0;
  totalLitres = 0;
  
  //Iniciando pantalla oled
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.display();
  delay(500);
  display.clearDisplay();
  
  //Definición de botones de configuración
  pinMode(wps_btn, INPUT);
  pinMode(wifi_btn, INPUT);

  //Lectura de estados de botones de configuración
  int estatewps=digitalRead(wps_btn);
  int estateap=digitalRead(wifi_btn);

  if(estatewps == HIGH){
    display.setCursor(10,0);  //oled display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("Modo WPS, Presione wps en su router por 5segundos");
    display.display();
    Serial.println("wps");
  }

  if(estateap == HIGH){
    display.setCursor(10,0);  //oled display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("Modo Access Point");
    display.display();
    Serial.println("wifi");
    modeconf();
  }

  //Leer credenciales guardadas
  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(pass, 50);
  setup_wifi();

  while (WiFi.status() != WL_CONNECTED){
    display.clearDisplay();
    display.setCursor(10,0);  //oled display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("No se puede iniciar sin internet");
    display.display();
    delay(2000);
    setup();
  }

  //Clientes de mqtt
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);

  //Cliente de reloj de internet
  timeClient.begin(); 

  //Definición de sensor de flujo y led
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
  
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  
  Serial.println("Modo AccessPoint");
  WiFi.softAP(ssidconf, passconf);
  //WiFi.softAPConfig(ip,gateway,subnet);
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);
  Serial.println("Server iniciado");

  display.clearDisplay();
  display.setCursor(10,0);  //oled display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Access Point iniciado en:");
  display.setCursor(10,30);  //oled display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print(myIP);
  display.display();

  server.on("/", setuppage);
  server.on("/guardar_conf", save_conf);
  server.on("/escanear",scan);
  server.begin();

  while(true){
    server.handleClient();
  }
}

/*SETUP WIFI*/
void setup_wifi(){
  WiFi.mode(WIFI_STA); //Iniciando en modo estación
  WiFi.begin(ssid, pass); //Iniciando con credenciales guardadas
  
  display.clearDisplay();
  display.setCursor(10,0);  //oled display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print(ssid);
  display.setCursor(10,50);  //oled display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print(pass);
  display.setCursor(10,80);  //oled display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Intentando conexion");
  display.display();
  

  while(WiFi.status() != WL_CONNECTED and contconexion <50){
    ++contconexion;
    delay(350);
    Serial.print(".");
  }

  if(contconexion<50){
    Serial.print("");
    Serial.println("Conectado");
    Serial.print(WiFi.localIP());
    
    display.clearDisplay();
    display.setCursor(10,0);  //oled display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("Conexion Wifi Establecida");
    display.display();
  }else{
    display.setCursor(10,0);  //oled display
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("Error de conexion");
    display.display();
    Serial.print("Error de conexión");
  }
}