/*Coded by: A.Elizondo 2022*/

#include <Arduino.h>

/*Libreria de contador*/
#include <Ticker.h>

/*Librerias para conexión a internet*/
#include <PubSubClient.h>
#include <ESP8266Wifi.h>
#include <ESP8266WebServer.h>

/*Libreria para sensor de temperatura y humedad*/
#include <dht.h>

/*Librerias para EEPROM, JSON y Hora de internet*/
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <WiFiClient.h>

/*Librerias para pantalla oled*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*Libreria de Servo*/
//#include <Servo.h>

/*Definicion de Servo*/
//#define Servo_PWM 15
//Servo MG995_Servo;


/*Definicion de pantalla*/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*Definición de sensor de temperatura*/
#define dht_apin 0
dht DHT;

/*Definición de ticker*/
Ticker ticker;
byte ledconf = 16;

/*Variables de conexion a internet*/
int contconexion = 0;
char ssid[50];
char pass[50];
const char *ssidconf = "Configuracion Dispositivo";
const char *passconf = "";
//Configuracion IP estatica en access point
IPAddress ip(192, 168, 1, 200);
IPAddress gateway(192, 168, 5, 5);
IPAddress subnet(255, 255, 255, 0);

/*Configuración de internet*/
WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);

char *host = "water-meter.tk";
String strhost = "water-meter.tk";
String strurl = "/registerdevice.php";


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
NTPClient timeClient(ntpUDP, "0.south-america.pool.ntp.org",-18000,6000);

/*Setup de mqtt*/
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_user ="admin";
const char* mqtt_pass = "public";

/*Mensajeria entrante de mqtt*/
long lastMsg = 0;
char msg[25];
char msg2[25];
char msg3[50];

/*Sensor de flujo de agua*/
#define SENSORAGUA 14
#define wifi_btn 13
#define wps_btn 12

/*Variables para el sensor de flujo de agua*/
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1sec =0;
float flowRate;
unsigned long flowMililliLitres;
unsigned long totalMilliLitres;
float flowLitres;
float totalLitres;

/*Variables para tiempos*/
long currentMillis = 0;
long previousMillis = 0;

/*Definición de funciones*/
void setup_wifi();
void callback(char* topic, byte* payload, int length);
void reconnect();
void setuppage();
void saveeeprom();
void save_conf();
void modeconf();
void modewps();
bool startWPSPBC();
void parpadeoled();
String leer(int addr);

/*Funcion para leer pulsos del sensor de flujo de agua*/
void IRAM_ATTR pulseCounter(){
  pulseCount++;
}

const unsigned char bitmapswifino [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xf8, 0x03, 0xfe, 0x00, 0x00, 0x7f, 0xc0, 0x1f, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xe0, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x03, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xfe, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x7f, 0xff, 0xff, 
	0xff, 0xff, 0xe0, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x07, 0xff, 0xff, 
	0xff, 0xff, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0xff, 0xff, 
	0xff, 0xf8, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0x1f, 0xff, 
	0xff, 0xc0, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0x03, 0xff, 
	0xfe, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfe, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x7f, 
	0xf8, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 
	0xe0, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0x00, 0x00, 0x00, 0x07, 
	0x80, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x01, 
	0xc0, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x03, 
	0xf0, 0x00, 0x03, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x0f, 
	0xfc, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x3f, 
	0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 
	0xff, 0xc3, 0xff, 0xff, 0xff, 0xf1, 0xfe, 0x00, 0x00, 0x7f, 0x8f, 0xff, 0xff, 0xff, 0xc3, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xfe, 0x01, 0xff, 0x00, 0x00, 0xff, 0x80, 0x7f, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xe0, 0x01, 0xff, 0x00, 0x00, 0xff, 0x80, 0x07, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0xff, 0x00, 0x00, 0xff, 0x80, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xf8, 0x00, 0x01, 0xff, 0x00, 0x00, 0xff, 0x80, 0x00, 0x1f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xc0, 0x00, 0x01, 0xff, 0x00, 0x00, 0xff, 0x80, 0x00, 0x03, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0xff, 0x80, 0x00, 0x00, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0xff, 0x80, 0x00, 0x00, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xc0, 0x00, 0x01, 0xff, 0x00, 0x00, 0xff, 0x80, 0x00, 0x03, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xf0, 0x00, 0x0f, 0xff, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x0f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfc, 0x00, 0x7f, 0xff, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x3f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
// 'wifisi', 128x64px
const unsigned char bitmapswifisi [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 
	0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 
	0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 
	0xff, 0x80, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x07, 0xff, 
	0xfe, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xff, 
	0xff, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x03, 0xff, 
	0xff, 0xc0, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x0f, 0xff, 
	0xff, 0xe0, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x1f, 0xff, 
	0xff, 0xf8, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x7f, 0xff, 
	0xff, 0xfc, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfc, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x07, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xc7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x0f, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x7f, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0xff, 0xe0, 0x00, 0x00, 0x0f, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x7f, 0x80, 0x00, 0x00, 0x03, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x00, 0xf8, 0x7f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x03, 0xf8, 0x00, 0x00, 0x03, 0xf8, 0x3f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x0f, 0xe0, 0x3f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x3f, 0x80, 0x1f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x07, 0xe0, 0x3e, 0x00, 0xfe, 0x00, 0x1f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x0f, 0xe0, 0x3f, 0x83, 0xf8, 0x00, 0x1f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x0f, 0xf0, 0x0f, 0xef, 0xe0, 0x00, 0x1f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xf0, 0x03, 0xff, 0x80, 0x00, 0x3f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x0f, 0xf8, 0x00, 0xfc, 0x00, 0x00, 0x3f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x1f, 0xfc, 0x00, 0x38, 0x00, 0x00, 0x7f, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x3f, 0xfe, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0x80, 0x00, 0x00, 0x03, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x1f, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
// 'logo', 128x64px
const unsigned char bitmapslogo [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xde, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x7f, 0x80, 0x00, 0x00, 0x00, 0x1f, 0xde, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x1c, 0x1e, 0x3f, 0x1c, 0xf3, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x1f, 0xde, 0x7f, 0xde, 0xfb, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x03, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x1f, 0xde, 0x73, 0xcf, 0xff, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x07, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x1c, 0x1e, 0x73, 0xcf, 0xdf, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x0f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x1c, 0x1e, 0x7f, 0xc7, 0x9f, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x1c, 0x1e, 0x1f, 0x07, 0x9e, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x07, 0xff, 0xff, 0xef, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x07, 0xff, 0xff, 0xf7, 0xf8, 0x00, 0x1c, 0x38, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x0f, 0xff, 0xff, 0xfb, 0xfc, 0x00, 0x1e, 0x7c, 0xf0, 0x00, 0x70, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x1f, 0xff, 0xff, 0xf8, 0xfe, 0x00, 0x0e, 0x7c, 0xe3, 0xf9, 0xfc, 0xfe, 0x3f, 0x80, 
	0x00, 0x00, 0x3f, 0xff, 0xff, 0xfc, 0x7f, 0x00, 0x0f, 0x7f, 0xe3, 0xbd, 0xfd, 0xef, 0x3f, 0x00, 
	0x00, 0x00, 0x7f, 0xff, 0xff, 0xfc, 0x3f, 0x80, 0x07, 0xef, 0xe3, 0xfc, 0xf1, 0xff, 0x38, 0x00, 
	0x00, 0x00, 0xff, 0xff, 0xff, 0xfe, 0x1f, 0x80, 0x07, 0xef, 0xc7, 0xbc, 0xf1, 0xe0, 0x38, 0x00, 
	0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xc0, 0x07, 0xe7, 0xc7, 0xfc, 0xfd, 0xff, 0x38, 0x00, 
	0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x87, 0xe0, 0x03, 0xc7, 0x83, 0xdc, 0x7c, 0x7e, 0x38, 0x00, 
	0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0x83, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xc1, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xe1, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xe0, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x3f, 0x00, 0x3f, 0xe0, 0x00, 0x01, 0xe0, 0x00, 0x00, 
	0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x1f, 0x80, 0x3f, 0xe0, 0x00, 0x01, 0xe0, 0x00, 0x00, 
	0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x0f, 0x80, 0x07, 0x07, 0xf0, 0xfd, 0xff, 0x00, 0x00, 
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x0f, 0xc0, 0x07, 0x0f, 0x79, 0xfd, 0xff, 0x80, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0xc0, 0x07, 0x0f, 0xfb, 0xc1, 0xe7, 0x80, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0xe0, 0x07, 0x0f, 0x03, 0xc1, 0xe7, 0x80, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x87, 0xe0, 0x07, 0x0f, 0xf9, 0xfd, 0xe7, 0x80, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83, 0xe0, 0x07, 0x03, 0xf0, 0xfc, 0xe7, 0x80, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x87, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x87, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x87, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x8f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xef, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x0f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char bitmapsaccespoint [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xff, 
	0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 
	0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x07, 0xff, 0xff, 
	0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x01, 0xff, 0xff, 
	0xff, 0xff, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0xff, 0xff, 
	0xff, 0xfe, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x7f, 0xff, 
	0xff, 0xfc, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x3f, 0xff, 
	0xff, 0xf8, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x1f, 0xff, 
	0xff, 0xf0, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x0f, 0xff, 
	0xff, 0xf0, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x07, 0xff, 
	0xff, 0xe0, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x07, 0xff, 
	0xff, 0xc0, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x03, 0xff, 
	0xff, 0x80, 0x07, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xe0, 0x01, 0xff, 
	0xff, 0x80, 0x07, 0xff, 0xf0, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xe0, 0x01, 0xff, 
	0xff, 0x00, 0x0f, 0xff, 0xe0, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xf0, 0x00, 0xff, 
	0xff, 0x00, 0x1f, 0xff, 0xc0, 0x07, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x03, 0xff, 0xf8, 0x00, 0xff, 
	0xfe, 0x00, 0x3f, 0xff, 0x80, 0x03, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xfc, 0x00, 0x7f, 
	0xfe, 0x00, 0x3f, 0xff, 0x80, 0x07, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xfc, 0x00, 0x7f, 
	0xfe, 0x00, 0x7f, 0xff, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0xff, 0xfe, 0x00, 0x3f, 
	0xfc, 0x00, 0x7f, 0xfe, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x7f, 0xfe, 0x00, 0x3f, 
	0xfc, 0x00, 0x7f, 0xfe, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x7f, 0xfe, 0x00, 0x3f, 
	0xfc, 0x00, 0xff, 0xfc, 0x00, 0x3f, 0xff, 0xe0, 0x07, 0xff, 0xfe, 0x00, 0x3f, 0xff, 0x00, 0x1f, 
	0xf8, 0x00, 0xff, 0xfc, 0x00, 0x7f, 0xff, 0x80, 0x01, 0xff, 0xfe, 0x00, 0x3f, 0xff, 0x00, 0x1f, 
	0xf8, 0x00, 0xff, 0xf8, 0x00, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0x00, 0x1f, 0xff, 0x00, 0x1f, 
	0xf8, 0x00, 0xff, 0xf8, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x1f, 0xff, 0x00, 0x1f, 
	0xf8, 0x01, 0xff, 0xf8, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x1f, 0xff, 0x80, 0x1f, 
	0xf8, 0x01, 0xff, 0xf8, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x1f, 0xff, 0x80, 0x1f, 
	0xf8, 0x01, 0xff, 0xf8, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x1f, 0xff, 0x80, 0x1f, 
	0xf8, 0x01, 0xff, 0xf8, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x1f, 0xff, 0x80, 0x1f, 
	0xf8, 0x01, 0xff, 0xf8, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x1f, 0xff, 0x80, 0x1f, 
	0xf8, 0x01, 0xff, 0xf8, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x1f, 0xff, 0x80, 0x1f, 
	0xf8, 0x00, 0xff, 0xf8, 0x00, 0xff, 0xfe, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x1f, 0xff, 0x00, 0x1f, 
	0xf8, 0x00, 0xff, 0xfc, 0x00, 0x7f, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x3f, 0xff, 0x00, 0x1f, 
	0xf8, 0x00, 0xff, 0xfc, 0x00, 0x7f, 0xff, 0x80, 0x01, 0xff, 0xfe, 0x00, 0x3f, 0xff, 0x00, 0x1f, 
	0xfc, 0x00, 0xff, 0xfc, 0x00, 0x3f, 0xff, 0xf0, 0x0f, 0xff, 0xfc, 0x00, 0x3f, 0xff, 0x00, 0x3f, 
	0xfc, 0x00, 0x7f, 0xfe, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x7f, 0xfe, 0x00, 0x3f, 
	0xfc, 0x00, 0x7f, 0xfe, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x7f, 0xfe, 0x00, 0x3f, 
	0xfe, 0x00, 0x3f, 0xff, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0xff, 0xfc, 0x00, 0x7f, 
	0xfe, 0x00, 0x3f, 0xff, 0x80, 0x03, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xfc, 0x00, 0x7f, 
	0xfe, 0x00, 0x1f, 0xff, 0xc0, 0x03, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xf8, 0x00, 0x7f, 
	0xff, 0x00, 0x1f, 0xff, 0xe0, 0x07, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xf8, 0x00, 0xff, 
	0xff, 0x00, 0x0f, 0xff, 0xf0, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x0f, 0xff, 0xf0, 0x00, 0xff, 
	0xff, 0x80, 0x07, 0xff, 0xf8, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x1f, 0xff, 0xe0, 0x01, 0xff, 
	0xff, 0xc0, 0x07, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xe0, 0x03, 0xff, 
	0xff, 0xc0, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x03, 0xff, 
	0xff, 0xe0, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x07, 0xff, 
	0xff, 0xf0, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xff, 
	0xff, 0xf8, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x1f, 0xff, 
	0xff, 0xfc, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x1f, 0xff, 
	0xff, 0xfc, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x3f, 0xff, 
	0xff, 0xff, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff, 
	0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x01, 0xff, 0xff, 
	0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x03, 0xff, 0xff, 
	0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x07, 0xff, 0xff, 
	0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x8f, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 3120)
const int bitmapsallArray_LEN = 4;
const unsigned char* bitmapsallArray[4] = {
	bitmapslogo,
	bitmapswifino,
	bitmapswifisi,
  bitmapsaccespoint
};


void showBitmaplogo(void) {
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmapslogo, 128, 64, WHITE);
  display.display();
  delay(2000);
}

void showBitmapwifisi(void) {
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmapswifisi, 128, 64, WHITE);
  display.display();
  delay(2000);
}

void showBitmapwifino(void) {
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmapswifino, 128, 64, WHITE);
  display.display();
  delay(2000);
}

void showBitmapap(void) {
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmapsaccespoint, 128, 64, WHITE);
  display.display();
  delay(2000);
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
  display.setTextSize(1);
  display.setTextColor(WHITE);
  showBitmaplogo();
  display.clearDisplay();
  
  //Definición de botones de configuración
  pinMode(wps_btn, INPUT);
  pinMode(wifi_btn, INPUT);
  pinMode(ledconf, OUTPUT);
  
  //Lectura de estados de botones de configuración
  int estatewps=digitalRead(wps_btn);
  int estateap=digitalRead(wifi_btn);

  /*Modo wps*/
  if(estatewps == HIGH){
    ticker.attach(0.3, parpadeoled);
    display.setCursor(10,0);  //oled display
    showBitmapap();
    display.clearDisplay();
    display.print("Modo WPS, Presione wps en su router por 5 segundos");
    display.display();
    Serial.println("wps");
    delay(3000);
    modewps();
  }

  /*Modo access point*/
  if(estateap == HIGH){
    ticker.attach(0.3, parpadeoled);
    display.setCursor(10,0);  //oled display
    display.print("Modo Access Point");
    display.display();
    Serial.println("wifi");
    modeconf();
  }

  //Leer credenciales wifi guardadas en eeprom
  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(pass, 50);
  setup_wifi(); //Conectarse con credenciales guardadas

  while (WiFi.status() != WL_CONNECTED){
    display.clearDisplay();
    showBitmapwifino();
    setup();  //Forzar al reinicio si no hay internet
  }

  /*Quitar parpadeo de led de configuración*/
  ticker.detach();
  digitalWrite(ledconf, LOW);
  showBitmapwifisi();
  delay(2000);

  //Clientes de mqtt
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);

  //Cliente de reloj de internet
  timeClient.begin(); 

  //Definición de sensor de flujo y led
  attachInterrupt(digitalPinToInterrupt(SENSORAGUA), pulseCounter, FALLING);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //Definicion de Servo
  //MG995_Servo.attach(Servo_PWM);
}

void loop() {

  /*Actualización de reloj de internet*/
  timeClient.update();
  
  /*Conectando a cliente mqtt*/
  if(!client.connected()){
    reconnect();
  }
  client.loop();

  //Variable de contadora de tiempo
  currentMillis = millis();

  /*Documento json para enviar información por mqtt*/
  StaticJsonDocument<256> doc;
  doc["Device"]=WiFi.macAddress();
  doc["time"]=timeClient.getFormattedTime();
  doc["date"]=timeClient.getDay();
 
  /*Definir tiempo de intervalo entre actualizaciones mqtt*/
  if(currentMillis - previousMillis > interval){
    pulse1sec = pulseCount;
    pulseCount = 0;

    /*Conteo de pulsos en un segundo en el sensor de flujo*/
    flowRate = ((1000.0 /(millis() - previousMillis)) * pulse1sec) / calibrationFactor;
    previousMillis = millis();

    /*Conversión de flujo a litros*/
    flowLitres = (flowRate/60);
    totalLitres += flowLitres;

    /*Lectura sensor de temperatura*/
    DHT.read11(dht_apin);

    Serial.print("Flujo de agua: ");
    Serial.print(float(flowRate));
    Serial.print("L/Min");
    Serial.print("\t");

    Serial.print("Litros consumidos: ");
    Serial.print(totalLitres);
    Serial.println("L");

    display.clearDisplay();
    display.setCursor(0,0);  //oled display
    display.print("Flujo de agua:");
    display.setCursor(95,0);
    display.print(flowRate);
    display.setCursor(0,20);
    display.print("Litros consumidos: ");
    display.setCursor(0,30);
    display.print(totalLitres);
    display.setCursor(0,50);
    display.print(timeClient.getFormattedTime());
    display.setCursor(80,50);
    display.print(DHT.temperature);
    display.display();

    String flujo = String(flowRate, 3); 
    String to_Send = flujo;
    to_Send.toCharArray(msg, 25);
    client.publish("values", msg);
    Serial.println("Enviado a topico");   
    
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

    char out2[256];
    String tosendd = String(flujo) + "," + String(totalLitres) + "," + String(totalLitres/1000) + "," + String(WiFi.macAddress()) + "," + String(DHT.temperature) + "," + String(DHT.humidity) + "," + "$85"; 
    tosendd.toCharArray(msg3, 100);
    client.publish("commands", msg3);
    
  }
/*
  MG995_Servo.write(0);
  delay(3000);
  MG995_Servo.detach();
  delay(2000);
  MG995_Servo.attach(Servo_PWM);
  MG995_Servo.write(180);
  delay(3000);
  MG995_Servo.detach();
  delay(2000);
  MG995_Servo.attach(Servo_PWM);
*/
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

  if(incoming == "on"){
    digitalWrite(ledconf, HIGH);
  }
  
  if(incoming == "off"){
      digitalWrite(ledconf, LOW);
  }
}

void reconnect(){
  while(!client.connected()){
    Serial.print("Conectando a MQTT...");
    String clientId = "ESP8266";
    clientId += String(random(0xffff), HEX);

    if(client.connect(clientId.c_str(), mqtt_user, mqtt_pass)){
      Serial.println("Conectado a mqtt");
      client.subscribe("servo");
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
  display.print("Access Point iniciado en:");
  display.setCursor(10,30);  //oled display
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
  display.print("Red almacenada:");
  display.setCursor(10,20);
  display.print(ssid);
  display.setCursor(10,30);  //oled display
  display.print(pass);
  display.setCursor(10,50);  //oled display
  display.print("Intentando conexion...");
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
    display.print("Conexion Wifi Establecida");
    display.display();
  }else{
    display.setCursor(10,0);  //oled display
    display.print("Error de conexion");
    display.display();
    Serial.print("Error de conexión");
  }
}

/*Setup wps*/
bool startWPSPBC(){
  WiFi.mode(WIFI_STA);
  WiFi.begin("foobar", "");
  while(WiFi.status() == WL_CONNECTED){
    display.setCursor(10,0);  //oled display
    display.print("Conectando...");
    display.display();
  }
  bool wpsSuccess = WiFi.beginWPSConfig();
  if(wpsSuccess){
    String newSSID = WiFi.SSID();
    if(newSSID.length() > 0 ){
      display.clearDisplay();
      display.setCursor(10,0);  //oled display
      display.print("WPS finished. Connected successfull to SSID");
      display.setCursor(30,20);
      display.print(newSSID.c_str());
      display.display();

      saveeeprom(0, newSSID);
      saveeeprom(50, WiFi.psk());
    }else{
      wpsSuccess = false;
    }
  }
  return wpsSuccess;
}

void modewps(){
  delay(2000);
  startWPSPBC();
}

void parpadeoled(){
  byte stateled = digitalRead(ledconf);
  digitalWrite(ledconf, !stateled);
}

