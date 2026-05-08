#include <Arduino.h>
#include <Wire.h>

/**
 * CONFIGURACIÓN Y CONSTANTES
 */
#define I2C_SLAVE_ADDR 0x20
#define SDA_PIN 21
#define SCL_PIN 22

// Códigos de Dispositivo
#define DEV_CONTROL  0x11
#define DEV_AMP920   0x22
#define DEV_AMP24    0x33

// Códigos de Comando
#define CMD_MONITOR  0x01
#define CMD_STATUS   0x02
#define CMD_ON5V     0x0A
#define CMD_ON50V    0x0F
#define CMD_PWR_ON   0x10
#define CMD_PWR_OFF  0x20
#define CMD_TX       0x40
#define CMD_RX       0x80
#define CMD_EX       0xF0

/**
 * LÓGICA DE COMUNICACIÓN
 */

// Cálculo de CRC8 Polinomio 0x31
uint8_t calculateCRC8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x31;
            else crc <<= 1;
        }
    }
    return crc;
}

const char* getModeName(uint8_t mode) {
    switch(mode) {
        case 0: return "Apagado (OFF)";
        case 1: return "Standby";
        case 2: return "TX";
        case 3: return "RX";
        case 4: return "EX";
        default: return "Desconocido";
    }
}

void sendAndPrint(uint8_t device, uint8_t command, uint8_t arg) {
    uint8_t txBuf[4] = {device, command, arg, 0};
    txBuf[3] = calculateCRC8(txBuf, 3);

    // Enviar comando
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(txBuf, 4);
    if (Wire.endTransmission() != 0) {
        Serial.println("Error: No se pudo contactar con el STM32 (I2C NACK)");
        return;
    }

    // Solicitar respuesta (8 bytes)
    delay(50); 
    Wire.requestFrom(I2C_SLAVE_ADDR, 8);
    
    if (Wire.available() == 8) {
        uint8_t rxBuf[8];
        for (int i = 0; i < 8; i++) rxBuf[i] = Wire.read();

        // Validar CRC de respuesta
        if (calculateCRC8(rxBuf, 7) != rxBuf[7]) {
            Serial.println("Error: CRC de respuesta inválido");
            return;
        }

        uint8_t res = rxBuf[2];
        if (res == 0xFF) Serial.println(">>> RESULTADO: FALLO (Protección activa o comando inválido)");
        else if (res == 0x0A) Serial.println(">>> RESULTADO: COMANDO DESCONOCIDO");
        else {
            Serial.println(">>> RESULTADO: OK");
            
            // Procesar datos específicos según comando
            if (command == CMD_MONITOR) {
                Serial.printf("    Temperatura: %d °C\n", rxBuf[3]);
                // Mapeo aproximado: 0-255 raw -> 0-1000mA basado en límites
                float mA = (rxBuf[4] / 255.0) * 1000.0; 
                Serial.printf("    Consumo: %.1f mA\n", mA);
            } 
            else if (command == CMD_STATUS) {
                Serial.print("    Estado Global: ");
                if (rxBuf[3] == 0x01) Serial.println("Todo OK");
                else if (rxBuf[3] == 0xF0) Serial.println("ALERTA: Falla por Temperatura");
                else if (rxBuf[3] == 0x08) Serial.println("ALERTA: Falla por Consumo");
                
                Serial.printf("    Modo Amp 920: %s\n", getModeName(rxBuf[4]));
                Serial.printf("    Modo Amp 24:  %s\n", getModeName(rxBuf[5]));
            }
        }
    }
}

/**
 * PARSER DE TERMINAL
 */
void processTerminal(String input) {
    input.trim();
    if (input == "monitor") {
        sendAndPrint(DEV_CONTROL, CMD_MONITOR, 0);
    } 
    else if (input == "status") {
        sendAndPrint(DEV_CONTROL, CMD_STATUS, 0);
    } 
    else if (input.startsWith("power ")) {
        // Formato: power <target> <on/off>
        if (input.indexOf("920") > 0) {
            uint8_t cmd = (input.indexOf("on") > 0) ? CMD_PWR_ON : CMD_PWR_OFF;
            sendAndPrint(DEV_AMP920, cmd, 0);
        } else if (input.indexOf("24") > 0) {
            uint8_t cmd = (input.indexOf("on") > 0) ? CMD_PWR_ON : CMD_PWR_OFF;
            sendAndPrint(DEV_AMP24, cmd, 0);
        } else if (input.indexOf("5v") > 0) {
            uint8_t arg = (input.indexOf("on") > 0) ? 1 : 0;
            sendAndPrint(DEV_CONTROL, CMD_ON5V, arg);
        } else if (input.indexOf("50v") > 0) {
            uint8_t arg = (input.indexOf("on") > 0) ? 1 : 0;
            sendAndPrint(DEV_CONTROL, CMD_ON50V, arg);
        }
    } 
    else if (input.startsWith("mode ")) {
        // Formato: mode <920/24> <tx/rx/ex/standby> [arg_hex]
        uint8_t device = (input.indexOf("920") > 0) ? DEV_AMP920 : DEV_AMP24;
        uint8_t command = 0;
        uint8_t arg = 0;

        if (input.indexOf("tx") > 0) command = CMD_TX;
        else if (input.indexOf("rx") > 0) command = CMD_RX;
        else if (input.indexOf("ex") > 0) command = CMD_EX;
        else if (input.indexOf("standby") > 0) command = CMD_PWR_ON;

        // Extraer argumento hex si existe (ej: 0x01)
        int hexIdx = input.indexOf("0x");
        if (hexIdx > 0) {
            String hexVal = input.substring(hexIdx + 2, hexIdx + 4);
            arg = (uint8_t) strtol(hexVal.c_str(), NULL, 16);
        }
        
        if (command != 0) sendAndPrint(device, command, arg);
        else Serial.println("Modo no reconocido.");
    }
    else if (input != "") {
        Serial.println("Comando desconocido. Usa: monitor, status, power, mode.");
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN, 100000); // Master I2C a 100KHz
    
    Serial.println("\n--- SUCHAI IV: Terminal Control ESP32 ---");
    Serial.println("Comandos disponibles:");
    Serial.println("- monitor");
    Serial.println("- status");
    Serial.println("- power <920 | 24 | 5v | 50v> <on | off>");
    Serial.println("- mode <920 | 24> <tx | rx | ex | standby> [0xarg]");
    Serial.print("> ");
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        Serial.println(input); // Echo
        processTerminal(input);
        Serial.print("\n> ");
    }
}