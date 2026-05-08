#include <Arduino.h>
#include <Wire.h>

/**
 * CONFIGURACIÓN DE PINES (Basado en amplRF_SuchaiIV.txt)
 */
// Comunes
#define PIN_ON5V PB0
#define PIN_ON50V_GLOBAL PB1
#define PIN_GLOBAL_I_SENSE PA1
#define PIN_TEMP_SENSE PA3

// Amplificador 920
#define PIN_IN_AMP2_920 PA15
#define PIN_IN_AMP3_920 PA13
#define PIN_SELLNA_LNA_920 PB3
#define PIN_SELLNA_EXT_920 PB5
#define PIN_SELPWR_AMP_920 PB7
#define PIN_SELPWR_LNA_920 PB6
#define PIN_ON_LNA1_920 PB4
#define PIN_ON_LNA2_920 PB14
#define PIN_SEL_LNAOUT2_920 PA11
#define PIN_SEL_LNAOUT3_920 PB13

// Amplificador 24
#define PIN_SEL_PWR_LNA_24 PB15
#define PIN_LNA_ON_24 PA10
#define PIN_LNA_IN1_24 PB11
#define PIN_LNA_IN2_24 PA8
#define PIN_ON_AMP_24 PB12

// I2C
#define PIN_I2C1_SCL PB8
#define PIN_I2C1_SDA PB9

/**
 * CONSTANTES DE REQUISITOS
 */
#define I2C_ADDRESS 0x20
#define TEMPERATURE_LIMIT 80.0
#define CONSUMPTION_LIMIT 4000
#define MONITOR_INTERVAL 200

#define CHANGEMODEDELAY 50
#define ONLNADELAY 200
#define POWEROFFDELAY 100

// Códigos de Dispositivo y Comandos
#define DEV_CONTROL 0x11
#define DEV_AMP920 0x22
#define DEV_AMP24 0x33

#define CMD_MONITOR 0x01
#define CMD_STATUS 0x02
#define CMD_ON5V 0x0A
#define CMD_ON50V 0x0F
#define CMD_POWER_ON 0x10
#define CMD_POWER_OFF 0x20
#define CMD_TX 0x40
#define CMD_RX 0x80
#define CMD_EX 0xF0

enum Mode { OFF = 0, STANDBY = 1, TX = 2, RX = 3, EX = 4 };
Mode mode920 = OFF;
Mode mode24 = OFF;

#define RES_OK 0x01
#define RES_FAIL 0xFF
#define RES_UNKNOWN 0x0A

struct Response {
  uint8_t device;
  uint8_t command;
  uint8_t result;
  uint8_t data[4];
  uint8_t crc;
} currentRes;

float currentTemp = 0;
uint8_t currentConsRaw = 0;
bool faultActive = false;
uint8_t faultType = 0x01; // 0x01=OK, 0xF0=Temp, 0x08=Cons
unsigned long lastMonitorTime = 0;

/**
 * FUNCIONES DE APOYO
 */
uint8_t calculateCRC8(uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x31;
      else
        crc <<= 1;
    }
  }
  return crc;
}

void amp920_toStandby() {
  digitalWrite(PIN_SELPWR_LNA_920, HIGH);
  digitalWrite(PIN_SELPWR_AMP_920, LOW);
  digitalWrite(PIN_SELLNA_LNA_920, HIGH);
  digitalWrite(PIN_SELLNA_EXT_920, LOW);
  digitalWrite(PIN_IN_AMP2_920, HIGH);
  digitalWrite(PIN_IN_AMP3_920, HIGH);
  digitalWrite(PIN_SEL_LNAOUT2_920, HIGH);
  digitalWrite(PIN_SEL_LNAOUT3_920, LOW);
  digitalWrite(PIN_ON_LNA1_920, HIGH);
  digitalWrite(PIN_ON_LNA2_920, HIGH);
  digitalWrite(PIN_ON5V, HIGH);
  mode920 = STANDBY;
  delay(CHANGEMODEDELAY);
}

void amp24_toStandby() {
  digitalWrite(PIN_SEL_PWR_LNA_24, HIGH);
  digitalWrite(PIN_LNA_ON_24, HIGH);
  digitalWrite(PIN_LNA_IN1_24, HIGH);
  digitalWrite(PIN_LNA_IN2_24, LOW);
  digitalWrite(PIN_ON_AMP_24, LOW);
  digitalWrite(PIN_ON5V, HIGH);
  mode24 = STANDBY;
  delay(CHANGEMODEDELAY);
}

void shutdownAll() {
  digitalWrite(PIN_ON5V, LOW);
  delay(POWEROFFDELAY);
  // Pines 920 a 0
  digitalWrite(PIN_IN_AMP2_920, LOW);
  digitalWrite(PIN_IN_AMP3_920, LOW);
  digitalWrite(PIN_SELLNA_LNA_920, LOW);
  digitalWrite(PIN_SELLNA_EXT_920, LOW);
  digitalWrite(PIN_SELPWR_AMP_920, LOW);
  digitalWrite(PIN_SELPWR_LNA_920, LOW);
  digitalWrite(PIN_ON_LNA1_920, LOW);
  digitalWrite(PIN_ON_LNA2_920, LOW);
  digitalWrite(PIN_SEL_LNAOUT2_920, LOW);
  digitalWrite(PIN_SEL_LNAOUT3_920, LOW);
  // Pines 24 a 0
  digitalWrite(PIN_SEL_PWR_LNA_24, LOW);
  digitalWrite(PIN_LNA_ON_24, LOW);
  digitalWrite(PIN_LNA_IN1_24, LOW);
  digitalWrite(PIN_LNA_IN2_24, LOW);
  digitalWrite(PIN_ON_AMP_24, LOW);
  mode920 = OFF;
  mode24 = OFF;
}

/**
 * MANEJO DE EVENTOS I2C
 */
void receiveEvent(int howMany) {
  if (howMany != 4)
    return;
  uint8_t buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = Wire.read();
  if (calculateCRC8(buf, 3) != buf[3])
    return;

  uint8_t dev = buf[0], cmd = buf[1], arg = buf[2];
  currentRes = {dev, cmd, RES_OK, {0, 0, 0, 0}, 0};

  if (faultActive && cmd != CMD_MONITOR && cmd != CMD_STATUS) {
    currentRes.result = RES_FAIL;
  } else {
    if (dev == DEV_CONTROL) 
    { /* DEVICE COMMANDS*/
      if (cmd == CMD_MONITOR) {
        currentRes.data[0] = (uint8_t)currentTemp;
        currentRes.data[1] = currentConsRaw;
      } else if (cmd == CMD_STATUS) {
        currentRes.data[0] = faultType;
        currentRes.data[1] = (uint8_t)mode920;
        currentRes.data[2] = (uint8_t)mode24;
      } else if (cmd == CMD_ON5V)
        digitalWrite(PIN_ON5V, arg);
      else if (cmd == CMD_ON50V)
        digitalWrite(PIN_ON50V_GLOBAL, arg);
    } else if (dev == DEV_AMP920) 
    { /* AMP920 COMMANDS */
      if (cmd == CMD_POWER_ON)
        amp920_toStandby();
      else if (cmd == CMD_POWER_OFF) {
        digitalWrite(PIN_ON5V, LOW);
        delay(POWEROFFDELAY);
        mode920 = OFF;
      } else {
        if (mode920 != STANDBY)
          amp920_toStandby();
        if (cmd == CMD_TX) {
          digitalWrite(PIN_SELPWR_LNA_920, LOW);
          digitalWrite(PIN_SELPWR_AMP_920, HIGH);
          digitalWrite(PIN_IN_AMP2_920, arg & 0x01);
          digitalWrite(PIN_IN_AMP3_920, (arg >> 1) & 0x01);
          mode920 = TX;
        } else if (cmd == CMD_RX) {
          digitalWrite(PIN_ON_LNA2_920, LOW);
          delay(ONLNADELAY);
          digitalWrite(PIN_ON_LNA1_920, LOW);
          delay(ONLNADELAY);
          digitalWrite(PIN_SEL_LNAOUT2_920, arg & 0x01);
          digitalWrite(PIN_SEL_LNAOUT3_920, (arg >> 1) & 0x01);
          mode920 = RX;
        } else if (cmd == CMD_EX) {
          digitalWrite(PIN_SELLNA_LNA_920, LOW);
          digitalWrite(PIN_SELLNA_EXT_920, HIGH);
          mode920 = EX;
        }
      }
    } else if (dev == DEV_AMP24) 
    { /* AMP24 COMMANDS */
      if (cmd == CMD_POWER_ON)
        amp24_toStandby();
      else if (cmd == CMD_POWER_OFF) {
        digitalWrite(PIN_ON5V, LOW);
        delay(POWEROFFDELAY);
        mode24 = OFF;
      } else {
        if (mode24 != STANDBY)
          amp24_toStandby();
        if (cmd == CMD_TX) {
          digitalWrite(PIN_SEL_PWR_LNA_24, LOW);
          digitalWrite(PIN_LNA_IN1_24, arg & 0x01);
          digitalWrite(PIN_LNA_IN2_24, (arg >> 1) & 0x01);
          digitalWrite(PIN_ON_AMP_24, HIGH);
          mode24 = TX;
        } else if (cmd == CMD_RX) {
          digitalWrite(PIN_LNA_ON_24, LOW);
          delay(ONLNADELAY);
          digitalWrite(PIN_LNA_IN1_24, arg & 0x01);
          digitalWrite(PIN_LNA_IN2_24, (arg >> 1) & 0x01);
          mode24 = RX;
        }
      }
    } else /* UNKNOWN COMMANDS */
      currentRes.result = RES_UNKNOWN;
  }
  uint8_t resBuf[7] = {currentRes.device,  currentRes.command,
                       currentRes.result,  currentRes.data[0],
                       currentRes.data[1], currentRes.data[2],
                       currentRes.data[3]};
  currentRes.crc = calculateCRC8(resBuf, 7);
}

void requestEvent() {
  uint8_t resBuf[8] = {currentRes.device,  currentRes.command,
                       currentRes.result,  currentRes.data[0],
                       currentRes.data[1], currentRes.data[2],
                       currentRes.data[3], currentRes.crc};
  Wire.write(resBuf, 8);
}

void setup() {
  // Configuración de salidas
  int pins[] = {PIN_ON5V,           PIN_ON50V_GLOBAL,    PIN_IN_AMP2_920,
                PIN_IN_AMP3_920,    PIN_SELLNA_LNA_920,  PIN_SELLNA_EXT_920,
                PIN_SELPWR_AMP_920, PIN_SELPWR_LNA_920,  PIN_ON_LNA1_920,
                PIN_ON_LNA2_920,    PIN_SEL_LNAOUT2_920, PIN_SEL_LNAOUT3_920,
                PIN_SEL_PWR_LNA_24, PIN_LNA_ON_24,       PIN_LNA_IN1_24,
                PIN_LNA_IN2_24,     PIN_ON_AMP_24};
  for (int p : pins)
    pinMode(p, OUTPUT);

  pinMode(PIN_GLOBAL_I_SENSE, INPUT_ANALOG);
  pinMode(PIN_TEMP_SENSE, INPUT_ANALOG);

  Wire.setSCL(PIN_I2C1_SCL);
  Wire.setSDA(PIN_I2C1_SDA);
  Wire.begin(I2C_ADDRESS);
  Wire.setClock(100000);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  shutdownAll();
}

void loop() {
  if (millis() - lastMonitorTime >= MONITOR_INTERVAL) {
    lastMonitorTime = millis();
    // Sensor interno STM32
    int tempRaw = analogRead(ATEMP);
    currentTemp = ((1.43 - (tempRaw * 3.3 / 4095.0)) / 0.0043) + 25.0;

    int consRaw = analogRead(PIN_GLOBAL_I_SENSE);
    currentConsRaw = map(consRaw, 0, 4095, 0, 255);

    if (currentTemp > TEMPERATURE_LIMIT ||
        currentConsRaw > 200) { // 200 raw ~ 800mA
      if (!faultActive) {
        shutdownAll();
        faultActive = true;
        faultType = (currentTemp > TEMPERATURE_LIMIT) ? 0xF0 : 0x08;
      }
    } else {
      faultActive = false;
      faultType = 0x01;
    }
  }
}