#include <OneWire.h>
#include <Wire.h>
//#include <NewPing.h>

OneWire  ds(8);

#define fadePin 9
//#define fanSpeed 18
#define fanSpeedMonitor 15

/*
#define TRIGGER_PIN 7
#define ECHO_PIN 6
#define MAX_DISTANCE 200
*/

#define speedControl A3

const byte mask = B11111000; // mask bits that are not prescale values

int firstRead = 0;

int maxVal = 0;
unsigned int actualFanSpeed = 0;
float sensorValue = 0.0f;
float percentile = 0.0f;
float temp = 0.0f;

float normalTemp = 0.0f;
char c;

float fanSpeed = 0.0f;
float lastCelsius = 0.0f;

/*
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
*/

void setup(void)
{
  Wire.begin();
  
  Serial.begin(9600);
  //change pwm frequency so it doesn't buzz
  TCCR1B = (TCCR1B & mask) | 5;
  pinMode(fanSpeedMonitor,INPUT_PULLUP);
  //pinMode(fanSpeed,HIGH);
  pinMode(fadePin,OUTPUT);
  
  for(int timeCalib = 0; timeCalib < 4000; timeCalib++)
  {
    sensorValue = analogRead(speedControl);
 
   //Serial.println(sensorPin);
 
   percentile = (sensorValue / maxVal) * 100;
   temp = fmod(percentile,10.0);
   if (temp > 0.5)
   {
       percentile += 0.5;
   }
   if (percentile > 100.3)
   {
       maxVal++;
   }
   else
   {
     Serial.println((int)percentile);
   }
    
  }
  analogWrite(fadePin,255);
  
}

void loop(void)
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;

  actualFanSpeed = pulseIn(fanSpeedMonitor, HIGH);

  /*i2c
  Wire.beginTransmission(0x2F);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(0x2F,14);
  /*while(Wire.available())
  {
    c = Wire.read();
    Serial.print((int)c);
  }*/

//  unsigned int uS = sonar.ping();

/* Example PWM code  
  for(int i = 0; i<360; i++)
  {
    //convert 0-360 angle to radian (needed for sin function)
    float rad = DEG_TO_RAD * i;

    //calculate sin of angle as number between 0 and 255
    int sinOut = constrain((sin(rad) * 128) + 128, 0, 255); 

    if (sinOut < 50)
    {
      sinOut = 50;
    }

    analogWrite(fadePin, sinOut);

    delay(15);
  }
*/  
  
  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
    return;
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  /*instead of delay we just read the fanspeed a lot
  for (int i = 0; i < 750; i++)
  {
      firstRead = pulseIn(fanSpeed,HIGH);
      if (firstRead > 0 && firstRead != 1)
      {
        Serial.println(firstRead);
      }
      if (firstRead == 0)
      {
        Serial.println("No fan!");
      }
  }*/
  
  ds.depower();   // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  unsigned int raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
  }
  celsius = (float)raw / 17.0;
  //fahrenheit = celsius * 1.8 + 32.0;
  Serial.println(celsius);
  //Serial.println(digitalRead(15));


if (celsius <= 100)
{
  normalTemp = ((celsius-19)/(25-19));
  
  /*
  lightLevel = map(lightLevel, 0, 1023, 0, 179); // Scale 1024 values to 180 
pos = constrain(lightLevel, 0, 179); // Scale between 0-179 (180 values)
*/
  
  lastCelsius = celsius;
}
else
{
  Serial.println("Reading temperature broke");
  normalTemp = ((lastCelsius-19)/(25-19));
}
  fanSpeed = normalTemp * 255;
  //Serial.println(normalTemp);

/*
  Serial.print("Ping: ");
  Serial.print(uS / US_ROUNDTRIP_CM); // Convert ping time to distance in cm and print result (0 = outside set distance range)
  Serial.println("cm");


  if ((uS / US_ROUNDTRIP_CM) < 90)
  {*/
    sensorValue = analogRead(speedControl);
    percentile = (sensorValue / maxVal) * 100;

  //if (!(percentile <= 0.0f))
   if (percentile >= 100.0f)
  {
    Serial.println("Auto speed");
    if (normalTemp == 0)
    {
      Serial.print("normal == 0 : ");
      Serial.println(normalTemp);
      Serial.print("Percentile: ");
      Serial.println(percentile);
      fanSpeed = 17;
    }
    else if (normalTemp < 0)
    {
      Serial.print("normal < 0 : ");
      Serial.println(normalTemp);
      Serial.print("Percentile: ");
      Serial.println(percentile);
      fanSpeed = 0;
    }
    else if (normalTemp >= 1)
    {
      Serial.print("normal > 1 : ");
      Serial.println(normalTemp);
      Serial.print("Percentile: ");
      Serial.println(percentile);
      fanSpeed = 255;
    }
    /*
    else
    {
      Serial.print("else: ");
      Serial.println(normalTemp);
      Serial.print("Percentile: ");
      Serial.println(percentile);
    }*/
  }
  //else if (percentile >= 100.0f)
  else if (percentile <= 0.0f)
  {
    //turn off fan due to potentiometer
    Serial.print("Manual TurnOff : ");
    Serial.println(percentile);
    fanSpeed = 0;
  }  
  else
  {
    Serial.println("Manual Speed");
    //we set the fan speed depending on potentiometer
    Serial.print("Percentile: ");
    Serial.println(percentile);
    fanSpeed = (percentile / 100) * 255;
  }
 /* }
  else
  {
    Serial.println("No-one to cool");
    fanSpeed = 0;
  }*/
  
  analogWrite(fadePin,fanSpeed);
  Serial.print("fanSpeed : ");
  Serial.println(fanSpeed);
  Serial.print("actualFanSpeed: ");
  Serial.println(actualFanSpeed);
  
  /*
  if (celsius >= 17)
  {
    analogWrite(fadePin,100);
  }
  else if (celsius >= 21)
  {
      analogWrite(fadePin, 255);
  }
  else
  {
    analogWrite(fadePin,70);
  }*/
  
}
