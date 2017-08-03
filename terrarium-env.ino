#include <OneWire.h>
#include <Wire.h>

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

void setup(void)
{

  Wire.begin();
  
  Serial.begin(57600);
  //change pwm frequency so it doesn't buzz
  TCCR1B = (TCCR1B & mask) | 5;
  pinMode(fanSpeedMonitor,INPUT_PULLUP);
  pinMode(fadePin,OUTPUT);

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
  
  celsius = (float)raw / 16.0;
  Serial.println(celsius);

  speedPotValue = constrain(analogRead(speedControl),0,1023);

  if (speedPotValue >= 1010)
  {
    Serial.println("Auto speed");
    //Compensate for anomalous readings by ignoring anything greater than 100
    if (celsius <= 100)
    {
      //Temperature check and fan speed should actually be dependent on time of day
      //night = 19degC max/min
      //day = 25degC max, 19degC min
      if (celsius >= 18 && celsius <= 24)
      {
        //Map doesn't work, it just sticks at 162
        //fanSpeed = map(celsius,17,25, 70,255);
        Serial.println("celsius >= 18 && celsius < 25");
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
        Serial.println("Reading temperature broke"); //, setting to min temp");
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
  
  analogWrite(fadePin,fanSpeed);
  Serial.print("fanSpeed : ");
  Serial.println(fanSpeed);
  Serial.print("actualFanSpeed: ");
  Serial.println(actualFanSpeed);
  Serial.print("Pot value: ");
  Serial.println(speedPotValue);
  
}
