/**
 * I2C Commander - Controlador Maestro para Amplificadores RF
 * Plataforma: ESP32 Dev Kit V1 (PlatformIO / Arduino)
 */

#include <Arduino.h>
#include <Wire.h>

#define I2C_SLAVE_ADDR 0x20
#define I2C_FREQ       100000
#define CRC8_POLY      0x31

//ESP32 (SDA: 21, SCL: 22),

// ==============================================================================
// FUNCIONES UTILITARIAS
// ==============================================================================
uint8_t calculateCRC8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ CRC8_POLY;
            else crc <<= 1;
        }
    }
    return crc;
}

const char* getModeName(uint8_t modeCode) {
    switch(modeCode) {
        case 0: return "OFF";
        case 1: return "STANDBY";
        case 2: return "TX";
        case 3: return "RX";
        case 4: return "EXT";
        default: return "DESCONOCIDO";
    }
}

// ==============================================================================
// PROCESAMIENTO DE RESPUESTAS I2C
// ==============================================================================
void parseResponse(uint8_t *rx) {
    // Validar el CRC de la respuesta del esclavo
    if (calculateCRC8(rx, 7) != rx[7]) {
        Serial.println("[ERROR] CRC8 de la respuesta I2C es incorrecto. Trama corrupta.");
        return;
    }

    uint8_t disp   = rx[0];
    uint8_t cmd    = rx[1];
    uint8_t result = rx[2];
    uint8_t b3 = rx[3], b4 = rx[4], b5 = rx[5];

    Serial.println("\n=== RESPUESTA DEL ESCLAVO ===");
    Serial.printf("Dispositivo: 0x%02X | Comando: 0x%02X\n", disp, cmd);
    
    Serial.print("Resultado de Ejecución: ");
    if (result == 0x01) {
        Serial.println("OK (Exito)");
    } else if (result == 0xFF) {
        Serial.println("FAIL (Comando rechazado / Alarma Activa)");
    } else if (result == 0x0A) {
        Serial.println("UNKNOWN (Comando no reconocido)");
    } else {
        Serial.printf("DESCONOCIDO (0x%02X)\n", result);
    }

    // Desglose de telemetría si corresponde
    if (cmd == 0x01) { // Comando monitoreo
        Serial.printf("Temperatura Interna : %d °C\n", b3);
        Serial.printf("Consumo ADC         : %d / 255\n", b4);
        Serial.print("Estado de Alertas   : ");
        if (b5 == 0x01) Serial.println("OK (Seguro)");
        else if (b5 == 0xF0) Serial.println("¡FALLA POR TEMPERATURA!");
        else if (b5 == 0x08) Serial.println("¡FALLA POR CONSUMO!");
        else Serial.printf("Estado inusual (0x%02X)\n", b5);
    } 
    else if (cmd == 0x02) { // Comando estado
        Serial.print("Estado de Alertas   : ");
        if (b3 == 0x01) Serial.println("OK (Seguro)");
        else if (b3 == 0xF0) Serial.println("¡FALLA POR TEMPERATURA!");
        else if (b3 == 0x08) Serial.println("¡FALLA POR CONSUMO!");
        
        Serial.printf("Estado Amp 920 MHz  : %s\n", getModeName(b4));
        Serial.printf("Estado Amp 2.4 GHz  : %s\n", getModeName(b5));
    }
    Serial.println("=============================\n");
}

void sendI2CCommand(uint8_t disp, uint8_t cmd, uint8_t arg) {
    uint8_t tx_frame[4];
    tx_frame[0] = disp;
    tx_frame[1] = cmd;
    tx_frame[2] = arg;
    tx_frame[3] = calculateCRC8(tx_frame, 3);

    // 1. Enviar el comando al esclavo
    Wire.beginTransmission(I2C_SLAVE_ADDR);
    Wire.write(tx_frame, 4);
    uint8_t busStatus = Wire.endTransmission();

    if (busStatus != 0) {
        Serial.printf("[ERROR] Fallo en el bus I2C (Código: %d). Revisa las conexiones físicas.\n", busStatus);
        return;
    }

    // Breve pausa para asegurar el tiempo de reacción en la FSM del esclavo
    delay(15);

    // 2. Solicitar los 8 bytes de respuesta obligatoria
    uint8_t rx_frame[8];
    uint8_t bytesRead = Wire.requestFrom((uint16_t)I2C_SLAVE_ADDR, (uint8_t)8);
    
    if (bytesRead == 8) {
        for (int i = 0; i < 8; i++) {
            rx_frame[i] = Wire.read();
        }
        parseResponse(rx_frame);
    } else {
        Serial.printf("[ERROR] El esclavo retornó %d bytes en lugar de los 8 esperados.\n", bytesRead);
    }
}

// ==============================================================================
// PARSEO DE PUERTO SERIAL
// ==============================================================================
void parseSerialInput(String input) {
    input.trim();
    if (input.length() == 0) return;

    uint8_t targetDisp = 0x00;
    uint8_t targetCmd  = 0x00;
    uint8_t targetArg  = 0x00;

    // Comandos Globales
    if (input.equalsIgnoreCase("mon")) {
        targetDisp = 0x11; targetCmd = 0x01; targetArg = 0xFF;
    } 
    else if (input.equalsIgnoreCase("stat")) {
        targetDisp = 0x11; targetCmd = 0x02; targetArg = 0xFF;
    } 
    // Subsistema 920 MHz
    else if (input.startsWith("amp920 ")) {
        targetDisp = 0x22;
        String action = input.substring(7);
        action.trim();
        
        if (action.equalsIgnoreCase("on")) { targetCmd = 0x10; }
        else if (action.equalsIgnoreCase("off")) { targetCmd = 0x20; }
        else if (action.equalsIgnoreCase("EXT")) { targetCmd = 0xF0; }
        else if (action.startsWith("TX ") || action.startsWith("tx ")) {
            targetCmd = 0x40; targetArg = (uint8_t)action.substring(3).toInt();
        }
        else if (action.startsWith("RX ") || action.startsWith("rx ")) {
            targetCmd = 0x80; targetArg = (uint8_t)action.substring(3).toInt();
        } else {
            Serial.println("[ERROR] Sintaxis de amp920 inválida."); return;
        }
    } 
    // Subsistema 2.4 GHz
    else if (input.startsWith("amp24 ")) {
        targetDisp = 0x33;
        String action = input.substring(6);
        action.trim();
        
        if (action.equalsIgnoreCase("on")) { targetCmd = 0x10; }
        else if (action.equalsIgnoreCase("off")) { targetCmd = 0x20; }
        else if (action.startsWith("TX ") || action.startsWith("tx ")) {
            targetCmd = 0x40; targetArg = (uint8_t)action.substring(3).toInt();
        }
        else if (action.startsWith("RX ") || action.startsWith("rx ")) {
            targetCmd = 0x80; targetArg = (uint8_t)action.substring(3).toInt();
        } else {
            Serial.println("[ERROR] Sintaxis de amp24 inválida."); return;
        }
    } 
    else {
        Serial.println("[ERROR] Comando no reconocido."); return;
    }

    // Ejecutar transmisión
    sendI2CCommand(targetDisp, targetCmd, targetArg);
}

// ==============================================================================
// SETUP Y LOOP
// ==============================================================================
void setup() {
    Serial.begin(115200);
    while (!Serial) { ; } // Esperar estabilización del puerto serial

    // Iniciar I2C en pines por defecto (SDA = 21, SCL = 22 para ESP32 Dev Kit V1)
    Wire.begin();
    Wire.setClock(I2C_FREQ); 

    Serial.println("\n=======================================================");
    Serial.println("  ESP32 I2C Commander - Controlador Amplificadores RF");
    Serial.println("=======================================================");
    Serial.println("Comandos disponibles:");
    Serial.println("  - mon                     (Imprime telemetría analógica)");
    Serial.println("  - stat                    (Imprime estado de los modos)");
    Serial.println("  - amp920 on / off / EXT");
    Serial.println("  - amp920 TX <arg> / RX <arg>");
    Serial.println("  - amp24 on / off");
    Serial.println("  - amp24 TX <arg> / RX <arg>");
    Serial.println("=======================================================\n");
}

void loop() {
    if (Serial.available() > 0) {
        String incomingData = Serial.readStringUntil('\n');
        parseSerialInput(incomingData);
    }
    delay(2000);
}