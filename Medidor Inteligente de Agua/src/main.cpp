#include <Arduino.h>

/*Sensor de flujo de agua*/
#define SENSORAGUA 14

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

  attachInterrupt(digitalPinToInterrupt(SENSORAGUA), pulseCounter, FALLING);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  currentMillis = millis();

  if(currentMillis - previousMillis > interval){
    pulse1sec = pulseCount;
    pulseCount = 0;

    flowRate = ((1000.0 /(millis() - previousMillis)) * pulse1sec) / calibrationFactor;
    previousMillis = millis();

     flowLitres = (flowRate/60);
    totalLitres += flowLitres;

    Serial.print("Flow Rate: ");
    Serial.print(float(flowRate));
    Serial.print("L/Min");
    Serial.print("\t");

    Serial.print("Output Liquid Quantify: ");
    Serial.print(totalLitres);
    Serial.println("L");

    digitalWrite(LED_BUILTIN, HIGH);
  }

  digitalWrite(LED_BUILTIN, LOW);
}