#include <OneWire.h>
#include <Wire.h>
#include <dht.h>

//should really avoid using these for connection reasons
//16,14,15 are SPI
//2,3 are i2c

const int mistViv = 16;
const int dampSensor = A0;
const int fadePin = 9;
const int fanSpeedMonitor = 15;
const int speedControl = A3;
const int tempSensor = 8;
const byte mask = B11111000; // mask bits that are not prescale values

unsigned int actualFanSpeed = 0;

int firstRead = 0;
int sensorValue = 0;
int speedPotValue = 0;
char c;
int fanSpeed = 0;
OneWire ds(tempSensor);
dht DHT;

void setup(void)
{

  Wire.begin();
  
  Serial.begin(115200);
  //change pwm frequency so it doesn't buzz
  TCCR1B = (TCCR1B & mask) | 5;
  pinMode(fanSpeedMonitor,INPUT_PULLUP);
  pinMode(fadePin,OUTPUT);
  pinMode(mistViv,OUTPUT);

  //Activate fans by setting them to max speed just so they turn.
  analogWrite(fadePin,255);
    Serial.println("setup done");
}


void loop(void)
{
  byte i;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;

  //read humidity
  DHT.read11(dampSensor);

  //Fan speed is PWM
  actualFanSpeed = pulseIn(fanSpeedMonitor, HIGH);
  
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
  
  delay(1000); 
  
  ds.depower();   // we might do a ds.depower() here, but the reset will take care of it.
  
  ds.reset();
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

  //calibration correction divider
  celsius = (float)raw / 16.0;


  //mostly ignoring the potentiometer value atm because it's poor quality
  speedPotValue = constrain(analogRead(speedControl),0,1023);

  if (speedPotValue >= 1010)
  {
    //Serial.println("Auto speed");
    //Compensate for anomalous readings by ignoring anything greater than 100
    if (celsius <= 100)
    {
      //Temperature check and fan speed should actually be dependent on time of day and humidity
      //night = 19degC max/min
      //day = 25degC max, 19degC min
      //humidity > 50% and < 80%
      if (celsius >= 18 && celsius <= 24)
      {
        //Map doesn't work, it just sticks at 162
        //fanSpeed = map(celsius,17,25, 70,255);
        //Serial.println("celsius >= 18 && celsius < 25");
        fanSpeed = 0;
      }
      else if (celsius > 24)
      {
        fanSpeed = 255;
      }
      else if (celsius < 18)
      {
        fanSpeed = 0;
      }
      else
      {
        Serial.println("Reading temperature broke"); 
        //fanSpeed = map(lastCelsius,17,25, 70,1023);
      }
    }
  }
  else if (speedPotValue <= 10)
  {
    //turn off fan due to potentiometer
    Serial.print("Manual TurnOff : ");
    Serial.println(speedPotValue);
    fanSpeed = 0;
  }  
  else if (speedPotValue > 10 && speedPotValue < 1010)
  {
    Serial.println("Manual Speed");
    //we set the fan speed depending on potentiometer
	  //map doesn't work, it sticks at 73
	  //fanSpeed = map(speedControl,1,1022, 0,255);
    //fanSpeed = constrain(fanSpeed,70,255);
  }
  if (DHT.humidity < 40)
  {
    digitalWrite(mistViv, HIGH);
    delay(1000); //we don't want to flood the tank, and it takes a while for the sensor to update
    digitalWrite(mistViv, LOW);
    DHT.read11(dampSensor); 
  }
  else if (DHT.humidity > 79)
  {
    //if humidity is high and temperature is low, thermostat should kick in to compensate
    fanSpeed = 255;
    analogWrite(fadePin,fanSpeed);
    while(DHT.humidity > 70)
    {
      //make sure the vivarium is drying out, high temperature might be increasing humidity
      DHT.read11(dampSensor);
    }
    analogWrite(fadePin,0);
  }

  analogWrite(fadePin,fanSpeed);
  DHT.read11(dampSensor);
  //debug output, could do with an LCD screen
  Serial.print("DS18B20 temp: ");
  Serial.println(celsius);
  Serial.print("fanSpeed : ");
  Serial.println(fanSpeed);
  Serial.print("actualFanSpeed: ");
  Serial.println(actualFanSpeed);
  Serial.print("Pot value: ");
  Serial.println(speedPotValue);
  Serial.print("DHT11 Temperature = ");
  Serial.println(DHT.temperature);
  Serial.print("DHT11 humidity = ");
  Serial.print(DHT.humidity);
  Serial.println("%");  
}
