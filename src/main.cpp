#include <LiquidCrystal.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

void printAddress(DeviceAddress deviceAddress);
void printTemperature(float t);
void readEnc();
void printRot(int v);

const int rs = 3, en = 4, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int CLK = 11;  // Pin 11 to clk on encoder
int DT = 10;  // Pin 10 to DT on encoder
int encoderPosCount = 0;
int clkLast;
int aVal;
boolean bCW;
float tempC = -99.9;

void setup() {
  // initializes the LCD with the size in chars (16x2)
  lcd.begin(16, 2);
  lcd.print("Loading...");
  //lcd.setCursor(4, 1);
  //lcd.print("LCD 1604");
  // start serial port
  delay(1000);
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");


  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
  else {
    // show the addresses we found on the bus
    Serial.print("Device 0 Address: ");
    printAddress(insideThermometer);
    Serial.println();

    // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
    sensors.setResolution(insideThermometer, 9);
 
    Serial.print("Device 0 Resolution: ");
    Serial.print(sensors.getResolution(insideThermometer), DEC); 
    Serial.println(); 
  }
  sensors.requestTemperatures(); // Send the command to get temperatures
  tempC = sensors.getTempC(insideThermometer);
  lcd.clear();
  printTemperature(tempC);

  pinMode (CLK,INPUT);
  pinMode (DT,INPUT);
  /* Read Pin A
     Whatever state it's in will reflect the last position
  */
  clkLast = digitalRead(CLK);
  encoderPosCount = (int) tempC;
  printRot(encoderPosCount);
  
}

int a = 100;
int delayVal = 100;
void loop() {
  //Serial.print("a: ");
  //Serial.println(a);
  //Serial.print("delayVal: ");
  //Serial.println(delayVal);
  if (a >= delayVal) {
    a = 0;
    delayVal = 100;
    sensors.requestTemperatures(); // Send the command to get temperatures
    tempC = sensors.getTempC(insideThermometer);
    printTemperature(tempC); 
 
  }
  readEnc();
  delay(5);
  a++;
}

void readEnc() {
  aVal = digitalRead(CLK);
  if (aVal != clkLast){ // Means the knob is rotating
    // if the knob is rotating, we need to determine direction
    // We do that by reading pin B.
    delayVal = 200;
    a=0;
    if (digitalRead(DT) != aVal) { // Means pin A Changed first - We're Rotating Clockwise
      encoderPosCount--;
      bCW = true;
    } else {// Otherwise B changed first and we're moving CCW
      bCW = false;
      encoderPosCount++;
    }
    Serial.print ("Rotated: ");
    if (bCW){
      Serial.println ("clockwise");
    }else{
      Serial.println("counterclockwise");
    }

    if (encoderPosCount < 1) encoderPosCount = 1;
    if (encoderPosCount > 99) encoderPosCount = 99;

    Serial.print("Encoder Position: ");
    Serial.println(encoderPosCount);
    printRot(encoderPosCount);
  }
  clkLast = aVal;
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
    {
      if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
    }
}

// function to print the temperature for a device
void printTemperature(float t)
{
  char text[50] = {0};
  int tmp;
  float tC = sensors.getTempC(insideThermometer);
  if(tC == DEVICE_DISCONNECTED_C) 
    {
      Serial.println("T error");
      return;
    }
  lcd.setCursor(0, 0);
  lcd.print("T=");
  lcd.setCursor(2, 0);
  lcd.print("    ");
  lcd.setCursor(2, 0);
  tmp = round(t * 10);
  sprintf(text,"%2d.%01d", tmp/10, abs(tmp%10));
  lcd.print(text);
  lcd.print((char) 223);
  lcd.print("C");
  //Serial.print("Temp C: ");
  //Serial.print(text);
  //Serial.print("\n");
}

void printRot(int v) {
  lcd.setCursor(10, 0);
  lcd.print("SET    ");
  lcd.setCursor(14, 0);
  lcd.print(v);
}
