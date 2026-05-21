#include <Arduino.h>
#include <Wire.h>

#define SLAVE_ADDR 0x20
#define I2C_FREQ 100000

// Prototipos
uint8_t calculate_crc8_from_32(uint8_t *data, size_t len);
void sendI2C(uint8_t cmd, uint8_t amp, uint8_t arg);
void handleResponse(uint8_t cmd, uint8_t amp);
void parsePower(String input);
void parseMode(String input);

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.setClock(I2C_FREQ);
  
  Serial.println("\n--- SUCHAI IV: Terminal de Control RF ---");
  Serial.println("Comandos disponibles:");
  Serial.println(" - monitor");
  Serial.println(" - power <920 | 24 | 5v | 50v> <on | off>");
  Serial.println(" - mode <920 | 24> <tx | rx | ex | standby> [args_hex]");
  Serial.println("------------------------------------------");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();

    if (input == "monitor") {
      sendI2C(0x01, 0xFF, 0x00); // CMD Monitoreo, AMP Controlador
    } 
    else if (input.startsWith("power")) {
      parsePower(input);
    } 
    else if (input.startsWith("mode")) {
      parseMode(input);
    }
  }
}

// Lógica de parsing para comandos de encendido
void parsePower(String input) {
  uint8_t amp = 0, cmd = 0, arg = 0;
  
  if (input.indexOf("920") > 0) {
    amp = 0x01;
    cmd = (input.indexOf("on") > 0) ? 0x10 : 0x11; // 0x10= StandBy (Encendido), 0x11=Apagado
  } else if (input.indexOf("24") > 0) {
    amp = 0x02;
    cmd = (input.indexOf("on") > 0) ? 0x10 : 0x11;
  } else if (input.indexOf("5v") > 0) {
    amp = 0xFF;
    cmd = 0x04; // Pin ON5V
    arg = (input.indexOf("on") > 0) ? 0x01 : 0x00;
  } else if (input.indexOf("50v") > 0) {
    amp = 0xFF;
    cmd = 0x05; // Pin ON50V
    arg = (input.indexOf("on") > 0) ? 0x01 : 0x00;
  }
  
  if (cmd != 0) sendI2C(cmd, amp, arg);
}

// Lógica para modos de operación
void parseMode(String input) {
  uint8_t amp = (input.indexOf("920") > 0) ? 0x01 : 0x02;
  uint8_t cmd = 0, arg = 0;

  if (input.indexOf("tx") > 0) cmd = 0x12;
  else if (input.indexOf("rx") > 0) cmd = 0x13;
  else if (input.indexOf("ex") > 0) cmd = 0x14;
  else if (input.indexOf("standby") > 0) cmd = 0x10;

  // Si hay un valor hex al final para los bits de RX/TX
  int lastSpace = input.lastIndexOf(' ');
  if (lastSpace > 8) { 
    arg = (uint8_t)strtol(input.substring(lastSpace + 1).c_str(), NULL, 16);
  }

  if (cmd != 0) sendI2C(cmd, amp, arg);
}

// --- Funciones de comunicación (Misma lógica técnica) ---

void sendI2C(uint8_t cmd, uint8_t amp, uint8_t arg) {
  uint8_t pkt[3] = {cmd, amp, arg};
  uint8_t crc = calculate_crc8_from_32(pkt, 3);
  
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(cmd); Wire.write(amp); Wire.write(arg); Wire.write(crc);
  
  if (Wire.endTransmission() == 0) {
    delay(20);
    handleResponse(cmd, amp);
  } else {
    Serial.println(">> Error: No se pudo contactar al esclavo 0x20.");
  }
}

uint8_t calculate_crc8_from_32(uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF; // Valor inicial fijo en F103
    const uint32_t polynomial = 0x04C11DB7;
    // Procesamos en bloques de 4 bytes (como palabras de 32 bits)
    for (size_t i = 0; i < len; i += 4) {
        uint32_t word = 0;
        // Empaquetado: El F103 al recibir bytes por puntero (uint32_t*)
        // interpreta el primer byte como el LSB de la palabra (Little Endian).
        // Si len_bytes no es múltiplo de 4, rellenamos con ceros (Padding).
        word |= ((uint32_t)(i + 0 < len ? data[i + 0] : 0) << 0);
        word |= ((uint32_t)(i + 1 < len ? data[i + 1] : 0) << 8);
        word |= ((uint32_t)(i + 2 < len ? data[i + 2] : 0) << 16);
        word |= ((uint32_t)(i + 3 < len ? data[i + 3] : 0) << 24);
        crc ^= word;
        for (uint8_t j = 0; j < 32; j++) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return (uint8_t)(crc & 0xFF);
}

void handleResponse(uint8_t cmd, uint8_t amp) {
  Wire.requestFrom(SLAVE_ADDR, 14);
  if (Wire.available() >= 3) {
    uint8_t status = Wire.read();
    uint8_t len = Wire.read();
    uint8_t data[10];
    for (int i = 0; i < len && i < 10; i++) data[i] = Wire.read();
    uint8_t r_crc = Wire.read();

    if (status != 0) { Serial.println(">> ESCLAVO REPORTA ERROR"); return; }

    if (cmd == 0x01) { // Monitoreo
      float temp = ((data[0] << 8) | data[1]) / 100.0;
      int cons = (data[2] << 8) | data[3];
      Serial.printf(">> [TELEMETRÍA] Temp: %.2f C | Consumo: %d mA\n", temp, cons);
    } else {
      Serial.println(">> Comando ejecutado con éxito (OK).");
    }
  }
}