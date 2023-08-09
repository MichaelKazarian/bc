#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);       // OneWire instance setup
DallasTemperature sensors(&oneWire); //Pass 1Wire reference to DallasTemperature
DeviceAddress insideThermometer;     // arrays to hold device address

void lcdprintTemperature(float t);
void readTempSettings();
void lcdprintTempSetting(int v);
void lcdprintState();
void loadSavedSettings();
void settingsSaveTemp(int temp);
void tempObserve();
void heatStart();
void heatStop();
void lcdprintTempReady();
void lcdprintStart();
void lcdprintStop();
void btnsSetup();
void tempSensorSetup();
int isBtnLow(int btn);
int isTempReady();
void tempSensorRead();
void encoderReadValue();

const bool USE_ENCODER           = true;
const int MAIN_BTN               = 12;
const int HEAT_PIN               = 13;
const int SAVED_TEMP_ADDR        = 4;
const int STATUS_MON_UNDEFINED   = -1;
const int STATUS_MON_READY       = 0;
const int STATUS_MON_STOP        = 1;
const int STATUS_MON_START       = 2;
const float DEFAULT_TEMP         = 36;
const int  BTN_TEMP_SETTING_DOWN = 10; // Encoder DT
const int BTN_TEMP_SETTING_UP    = 11; // Encoder CLK
const int TEMP_READ_PERIOD       = 2000;
const int BTN_PRESS_PERIOD       = 200;
const int rs = 3, en = 4, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int settingTemp = 0;
float tempPrev= -99.9;
float tempC = -99.9;
int savedTemp = 0;
int statusMon = STATUS_MON_UNDEFINED;
int encoderClkLast;
bool heatStatus = false;
bool btnMainPressed = false;
unsigned long tempTimeLastRead = 0;

void setup() {
  btnsSetup();
  heatStop();
  
  lcd.begin(16, 2); // (16x2) LCD initialize
  lcd.print("Loading...");
  delay(500);
  Serial.begin(9600);
  tempSensorSetup();
  lcd.clear();
  lcdprintTemperature(tempC);
  loadSavedSettings();

  settingTemp = (int) savedTemp;
  lcdprintTempSetting(settingTemp);
  // settingsSaveTemp(29);
}

void loop() {
  tempSensorRead();
  if (USE_ENCODER == true) encoderReadValue();
  else readTempSettings();
  lcdprintState();
  tempObserve();
}

void tempObserve() {
  if (isTempReady() == true) {
    if (heatStatus == true) {
      heatStop();
      return;
    }
    lcdprintTempReady();
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

void tempSensorRead() {
  if ((unsigned long)(millis() - tempTimeLastRead) > TEMP_READ_PERIOD) {
    tempTimeLastRead = millis();
    sensors.requestTemperatures();
    tempC = sensors.getTempC(insideThermometer);
    Serial.println(millis() - tempTimeLastRead);
    Serial.println(tempC);
    if (tempC != tempPrev) lcdprintTemperature(tempC);
    tempPrev = tempC;
  }
}

int isTempReady() {
  return (tempC > savedTemp);
}

int isBtnLow(int btn) {
  if (digitalRead(btn) == HIGH) return HIGH;
  delay(200);
  return digitalRead(btn);
}

// function to print the temperature for a device
void lcdprintTemperature(float t) {
  char text[50] = {0};
  int tmp;
  float tC = sensors.getTempC(insideThermometer);
  if (tC == DEVICE_DISCONNECTED_C) {
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

void lcdprintTempSetting(int v) {
  lcd.setCursor(10, 0);
  lcd.print("SET    ");
  lcd.setCursor(14, 0);
  lcd.print(v);
}

void lcdprintStop() {
  if (statusMon == STATUS_MON_STOP) return;
  lcd.setCursor(10, 1);
  lcd.print("     ");
  lcd.setCursor(10, 1);
  lcd.print("STOP");
  statusMon = STATUS_MON_STOP;
}

void lcdprintStart() {
  if (statusMon == STATUS_MON_START) return;
  lcd.setCursor(10, 1);
  lcd.print("     ");
  lcd.setCursor(10, 1);
  lcd.print("START");
  statusMon = STATUS_MON_START;
}

void lcdprintState() {
  if (isTempReady() == true) lcdprintTempReady();
  else {
    if (heatStatus == true) lcdprintStop();
    else lcdprintStart();
  }
}

void lcdprintTempReady() {
  if (statusMon == STATUS_MON_READY) return;
  lcd.setCursor(10, 1);
  lcd.print("READY");
  statusMon = STATUS_MON_READY;
}

void loadSavedSettings() {
  EEPROM.get(SAVED_TEMP_ADDR, savedTemp);    
  if (savedTemp == -1) settingsSaveTemp(DEFAULT_TEMP);
}

void settingsSaveTemp(int temp) {
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

void encoderReadValue() {
  int val = digitalRead(BTN_TEMP_SETTING_UP);
  if (val != encoderClkLast){ // Means the knob is rotating
    // if the knob is rotating, we need to determine direction
    // We do that by reading pin B.
    if (digitalRead(BTN_TEMP_SETTING_DOWN) != val) {
      settingTemp--; // Means pin A Changed first - We're Rotating Clockwise
    } else {         // Otherwise B changed first and we're moving CCW
      settingTemp++;
    }
    if (settingTemp < 10) settingTemp = 10;
    if (settingTemp > 60) settingTemp = 60;
    lcdprintTempSetting(settingTemp);
    settingsSaveTemp(settingTemp);
  }
  encoderClkLast = val;
}

void readTempSettings() {
  if (isBtnLow(BTN_TEMP_SETTING_UP))   settingTemp++;
  if (isBtnLow(BTN_TEMP_SETTING_DOWN)) settingTemp--;
  if (settingTemp < 10) settingTemp = 10;
  if (settingTemp > 60) settingTemp = 60;
  if (settingTemp != savedTemp) {
    settingsSaveTemp(settingTemp);
    lcdprintTempSetting(settingTemp);
  }
}

void btnsSetup() {
  pinMode(MAIN_BTN, INPUT);
  pinMode(BTN_TEMP_SETTING_UP, INPUT);
  pinMode(BTN_TEMP_SETTING_DOWN, INPUT);
  pinMode(MAIN_BTN, INPUT_PULLUP);
  pinMode(BTN_TEMP_SETTING_UP, INPUT_PULLUP);
  pinMode(BTN_TEMP_SETTING_DOWN, INPUT_PULLUP);
  pinMode(HEAT_PIN, OUTPUT);
}

void tempSensorSetup() {
  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) {
    lcd.setCursor(0, 0);
    lcd.print("No sensor");
    delay(10000);
    Serial.println("Unable to find address for Device 0");
  } else {
    sensors.setResolution(insideThermometer, 9);
    sensors.requestTemperatures();
    tempC = sensors.getTempC(insideThermometer);
  }
}
