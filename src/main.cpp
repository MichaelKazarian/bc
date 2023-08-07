#include <EEPROM.h>
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
void readTempSettings();
void printRot(int v);
void printState();
void savedSetup();
void saveSettingsTemp(int temp);
void process();
void heatStart();
void heatStop();
void printTempReady();
void printStart();
void printStop();
int isBtnLow(int btn);
int isTempReady();

const int SAVED_TEMP_ADDR       = 4;
const int STATUS_MON_UNDEFINED  = -1;
const int STATUS_MON_READY      = 0;
const int STATUS_MON_STOP       = 1;
const int STATUS_MON_START      = 2;
const float DEFAULT_TEMP        = 36;
int const BTN_TEMP_SETTING_DOWN = 10; 
int const BTN_TEMP_SETTING_UP   = 11;

const int rs = 3, en = 4, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


int const MAIN_BTN = 12;
int const HEAT_PIN = 13;
int settingTemp = 0;
float prevTemp= -99.9;
float tempC = -99.9;
int savedTemp = 0;
int statusMon = STATUS_MON_UNDEFINED;
bool heatStatus = false;
bool btnMainPressed = false;
unsigned long last_run = 0;
void setup() {
  pinMode(MAIN_BTN, INPUT);
  pinMode(BTN_TEMP_SETTING_UP, INPUT);
  pinMode(BTN_TEMP_SETTING_DOWN, INPUT);
  pinMode(MAIN_BTN, INPUT_PULLUP);
  pinMode(BTN_TEMP_SETTING_UP, INPUT_PULLUP);
  pinMode(BTN_TEMP_SETTING_DOWN, INPUT_PULLUP);
  pinMode(HEAT_PIN, OUTPUT);
  heatStop();
  
  // initializes the LCD with the size in chars (16x2)
  lcd.begin(16, 2);
  lcd.print("Loading...");
  delay(500);
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

  savedSetup();

  settingTemp = (int) savedTemp;
  printRot(settingTemp);
  // saveSettingsTemp(29);
}

int a = 100;
int delayVal = 100;


void loop() {
  if (a >= delayVal) {
    a = 0;
    delayVal = 10;
    sensors.requestTemperatures(); // Send the command to get temperatures
    tempC = sensors.getTempC(insideThermometer);
    if (tempC != prevTemp) printTemperature(tempC);
    prevTemp = tempC;
  }
  readTempSettings();
  printState();
  process();
  delay(50);
  a++;
}

void process() {
  if (isTempReady() == true) {
    if (heatStatus == true) {
      heatStop();
      return;
    }
    printTempReady();
  }
  int bs = isBtnLow(MAIN_BTN);
  if (bs == HIGH && btnMainPressed == false) return;
  // Run if pressed once. (Avoid STOP/START switching if btn is long pressed)
  if (bs == LOW && btnMainPressed == false) {
    if (heatStatus == false) heatStart();
    else heatStop();
    btnMainPressed = true;
  }
  // Long pressed ended
  if (bs == HIGH && btnMainPressed == true) btnMainPressed = false;
}

int isTempReady() {
  return (tempC > savedTemp);
}

int isBtnLow(int btn) {
  if (digitalRead(btn) == HIGH) return HIGH;
  delay(200);
  return digitalRead(btn);
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
void printTemperature(float t) {
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
  Serial.print("Temp C: ");
  Serial.println(text);
}

void printRot(int v) {
  lcd.setCursor(10, 0);
  lcd.print("SET    ");
  lcd.setCursor(14, 0);
  lcd.print(v);
}

void printStop() {
  if (statusMon == STATUS_MON_STOP) return;
  lcd.setCursor(10, 1);
  lcd.print("     ");
  lcd.setCursor(10, 1);
  lcd.print("STOP");
  statusMon = STATUS_MON_STOP;
}

void printStart() {
  if (statusMon == STATUS_MON_START) return;
  lcd.setCursor(10, 1);
  lcd.print("     ");
  lcd.setCursor(10, 1);
  lcd.print("START");
  statusMon = STATUS_MON_START;
}

void printState() {
  if (isTempReady() == true) printTempReady();
  else {
    if (heatStatus == true) printStop();
    else printStart();
  }
}

void printTempReady() {
  if (statusMon == STATUS_MON_READY) return;
  lcd.setCursor(10, 1);
  Serial.println(statusMon);
  Serial.println("READY");
  lcd.print("READY");
  statusMon = STATUS_MON_READY;
}

void savedSetup() {
  EEPROM.get(SAVED_TEMP_ADDR, savedTemp);    
  Serial.print("Saved value ");
  Serial.println(savedTemp);
  if (savedTemp == -1) saveSettingsTemp(DEFAULT_TEMP);
}

void saveSettingsTemp(int temp) {
  Serial.print("Save value ");
  EEPROM.put(SAVED_TEMP_ADDR, temp);
  savedTemp = temp;
}

void heatStart() {
  digitalWrite(HEAT_PIN, HIGH);
  heatStatus = true;
  lcd.setCursor(0, 1);
  lcd.print("          ");
  lcd.setCursor(0, 1);
  lcd.print("HEATING   ");
}

void heatStop() {
  digitalWrite(HEAT_PIN, LOW);
  heatStatus = false;
  lcd.setCursor(0, 1);
  lcd.print("          ");
}

void readTempSettings() {
  if (isBtnLow(BTN_TEMP_SETTING_UP))   settingTemp++;
  if (isBtnLow(BTN_TEMP_SETTING_DOWN)) settingTemp--;
  if (settingTemp < 10) settingTemp = 10;
  if (settingTemp > 70) settingTemp = 70;
  if (settingTemp != savedTemp) {
    saveSettingsTemp(settingTemp);
    printRot(settingTemp);
  }
}

