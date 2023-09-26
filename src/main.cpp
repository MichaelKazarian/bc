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
void loadSettings();
void tempSetToDesired(int temp);
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
void settingsSaveDesiredTemp();
void checkTempSettings();
void lcdBlinkHeating();

const bool USE_ENCODER           = true;
const int MAIN_BTN               = 12;
const int HEAT_PIN               = 13;
const int SAVED_TEMP_ADDR        = 4;
const int STATUS_MON_UNDEFINED   = -1;
const int STATUS_MON_READY       = 0;
const int STATUS_MON_STOP        = 1;
const int STATUS_MON_START       = 2;
const float DEFAULT_TEMP         = 36;
const int BTN_TEMP_SETTING_DOWN  = 3; // Encoder DT too
const int BTN_TEMP_SETTING_UP    = 4; // Encoder CLK too
const int TEMP_READ_PERIOD       = 2000;
const int ENCODER_CHANGE_PERIOD  = 5000;
const int HEATING_BLINK_PERIOD   = 1000;
const int BTN_PRESS_PERIOD       = 200;
const int TEMP_MIN               = 10;
const int TEMP_MAX               = 60;
const int rs = 10, en = 11, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int settingTemp = 0;
float tempPrev= -99.9;
float tempCurrent = -99.9;
int tempDesired = 0;
int statusMon = STATUS_MON_UNDEFINED;
int encoderClkLast;
bool heatStatus = false;
bool heatBlinkStatus = false;
bool btnMainPressed = false;
bool tempIsSaved = false;
unsigned long tempTimeLastRead = 0;
unsigned long settingsLastSave = 0;
unsigned long heatingBlinkLastSave = 0;

void setup() {
  btnsSetup();
  heatStop();

  lcd.begin(16, 2); // (16x2) LCD initialize
  lcd.print("Loading...");
  delay(500);
  Serial.begin(9600);
  tempSensorSetup();
  lcd.clear();
  lcdprintTemperature(tempCurrent);
  loadSettings();

  settingTemp = (int) tempDesired;
  lcdprintTempSetting(settingTemp);
}

void loop() {
  tempSensorRead();
  if (USE_ENCODER == true) encoderReadValue();
  else readTempSettings();
  lcdprintState();
  tempObserve();
  lcdBlinkHeating();
  settingsSaveDesiredTemp();
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
    tempCurrent = sensors.getTempC(insideThermometer);
    if (tempCurrent != tempPrev) lcdprintTemperature(tempCurrent);
    tempPrev = tempCurrent;
  }
}

int isTempReady() {
  return (tempCurrent > tempDesired);
}

int isBtnLow(int btn) {
  if (digitalRead(btn) == HIGH) return HIGH;
  delay(200);
  return digitalRead(btn);
}

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
  lcd.setCursor(9, 0);
  lcd.print("|SET    ");
  lcd.setCursor(14, 0);
  lcd.print(v);
}

void lcdprintStop() {
  if (statusMon == STATUS_MON_STOP) return;
  lcd.setCursor(9, 1);
  lcd.print("       ");
  lcd.setCursor(9, 1);
  lcd.print("|  STOP");
  statusMon = STATUS_MON_STOP;
}

void lcdprintStart() {
  if (statusMon == STATUS_MON_START) return;
  lcd.setCursor(9, 1);
  lcd.print("       ");
  lcd.setCursor(9, 1);
  lcd.print("| START");
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
  lcd.setCursor(9, 1);
  lcd.print("| READY");
  statusMon = STATUS_MON_READY;
}

void loadSettings() {
  EEPROM.get(SAVED_TEMP_ADDR, tempDesired);
  if (tempDesired == -1) tempSetToDesired(DEFAULT_TEMP);
  tempIsSaved = true;
}

/**
 * \brief Set desired temperature.
 *
 * \details To avoid freezing while value saving, you need to check encoder
 * is stopped and save a value in the main loop.
 */
void tempSetToDesired(int temp) {
  tempDesired = temp;
  tempIsSaved = false;
}

/**
 * \brief Save temperature.
 *
 * \details Periodically check temperature changes and save if found one.
 */
void settingsSaveDesiredTemp() {
  if (tempIsSaved == true) return;
  if ((unsigned long)(millis() - settingsLastSave) > ENCODER_CHANGE_PERIOD) {
    settingsLastSave = millis();
    EEPROM.put(SAVED_TEMP_ADDR, tempDesired);
    tempIsSaved = true;
  }
}

void heatStart() {
  digitalWrite(HEAT_PIN, HIGH);
  heatStatus = true;
  lcdBlinkHeating();
}

void heatStop() {
  digitalWrite(HEAT_PIN, LOW);
  heatStatus = false;
  lcd.setCursor(0, 1);
  lcd.print("         |");
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
    checkTempSettings();
    lcdprintTempSetting(settingTemp);
    tempSetToDesired(settingTemp);
  }
  encoderClkLast = val;
}

void readTempSettings() {
  if (isBtnLow(BTN_TEMP_SETTING_UP))   settingTemp++;
  if (isBtnLow(BTN_TEMP_SETTING_DOWN)) settingTemp--;
  checkTempSettings();
  if (settingTemp != tempDesired) {
    tempSetToDesired(settingTemp);
    lcdprintTempSetting(settingTemp);
  }
}

void checkTempSettings() {
  if (settingTemp < TEMP_MIN) settingTemp = TEMP_MIN;
  if (settingTemp > TEMP_MAX) settingTemp = TEMP_MAX;
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
    Serial.println("Unable to find address for Device 0");
    delay(5000);
  } else {
    sensors.setResolution(insideThermometer, 12);
    sensors.requestTemperatures();
    tempCurrent = sensors.getTempC(insideThermometer);
  }
}

void lcdBlinkHeating() {
  if (heatStatus == false) {
    heatBlinkStatus = false;
    return;
  }
  if ((unsigned long)(millis() - heatingBlinkLastSave) > HEATING_BLINK_PERIOD) {
    heatingBlinkLastSave = millis();
    if (heatBlinkStatus == true) {
      lcd.setCursor(0, 1);
      lcd.print("         ");
      heatBlinkStatus = false;
    } else {
      lcd.setCursor(0, 1);
      lcd.print("HEATING  ");
      heatBlinkStatus = true;
    }
  }
}
