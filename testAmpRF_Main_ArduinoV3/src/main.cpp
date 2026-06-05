/**
 * Especificación de Requerimientos de Firmware: Controlador de Amplificadores (I2C Slave)
 * Plataforma: STM32F103C6T6 (Framework Arduino / PlatformIO)
 */

#include <Arduino.h>
#include <Wire.h>

// ==============================================================================
// 1. DEFINICIONES DE HARDWARE Y MACROS
// ==============================================================================
#define I2C_ADDRESS              0x20

// Pines de Control Global
#define PIN_ON5V                 PB0
#define PIN_ON50V_GLOBAL         PB1
#define PIN_GLOBAL_I_SENSE       PA1

// Subsistema Amplificador 920 MHz
#define PIN_IN_AMP2_920          PA13
#define PIN_IN_AMP3_920          PA15
#define PIN_SELLNA_LNA_920       PB3
#define PIN_SELLNA_EXT_920       PB5
#define PIN_SELPWR_AMP_920       PB7
#define PIN_SELPWR_LNA_920       PB6
#define PIN_ON_LNA1_920          PB4
#define PIN_ON_LNA2_920          PB14
#define PIN_SEL_LNAOUT2_920      PA11
#define PIN_SEL_LNAOUT3_920      PB13

// Subsistema Amplificador 2.4 GHz
#define PIN_SEL_PWR_LNA_24       PA10
#define PIN_LNA_ON_24            PA8
#define PIN_LNA_IN1_24           PB12
#define PIN_LNA_IN2_24           PB15
#define PIN_ON_AMP_24            PA9

// Temporizaciones (ms)
#define CHANGEMODEDELAY          200
#define ONLNADELAY               100
#define POWEROFFDELAY            100
#define MONITORING_INTERVAL      200

// Límites de Seguridad
#define TEMPERATURE_LIMIT        70   // °C (Límite configurable)
#define CONSUMPTION_LIMIT        204  // Rango 0-255 (Aprox. 2.6V en ADC)

// Constantes I2C
#define CRC8_POLY                0x31

// ==============================================================================
// 2. ESTRUCTURAS DE DATOS Y ESTADOS
// ==============================================================================
enum AmpMode {
    MODE_OFF = 0,
    MODE_STANDBY,
    MODE_TX,
    MODE_RX,
    MODE_EX
};

volatile AmpMode currentMode920 = MODE_OFF;
volatile AmpMode targetMode920  = MODE_OFF;
volatile AmpMode currentMode24  = MODE_OFF;
volatile AmpMode targetMode24   = MODE_OFF;

volatile uint8_t currentArg920 = 0x00;
volatile uint8_t currentArg24  = 0x00;

// Variables de Telemetría y Fallas
uint8_t  sys_temperature_c = 0;
uint8_t  sys_consumption   = 0;
uint8_t  sys_alert_state   = 0x01; // 0x01=OK, 0xF0=Temp Falla, 0x08=Consumo Falla
bool     fault_active      = false;

unsigned long lastMonitoringTime = 0;

// Buffer de respuesta I2C (8 bytes)
volatile uint8_t tx_buffer[8] = {0};

// Máquina de estados para secuencias no bloqueantes
enum SeqStep { STEP_IDLE, STEP_DELAY_1, STEP_DELAY_2, STEP_FINISH };
SeqStep seqStep920 = STEP_IDLE;
SeqStep seqStep24  = STEP_IDLE;
unsigned long seqTimer920 = 0;
unsigned long seqTimer24  = 0;


// ==============================================================================
// 3. FUNCIONES UTILITARIAS Y DE PROTECCIÓN
// ==============================================================================
uint8_t calculateCRC8(volatile uint8_t *data, uint8_t len) {
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

int readInternalTemperature() {
    // Lectura del sensor de temperatura interno del STM32 (ADC1_IN16)
    // Nota: Dependiendo del core de Arduino específico (stm32duino), puede usarse ATEMP o cálculo manual.
    // Aquí implementamos una lectura genérica. Asumiendo ATEMP disponible o un pin analógico representativo.
#ifdef ATEMP
    int val = analogRead(ATEMP);
    // Fórmula genérica STM32: Temp = ((V25 - V_sense) / Avg_Slope) + 25
    float voltage = (val * 3.3) / 1023.0; // Asumiendo ADC 10-bit
    float temp = ((1.43 - voltage) / 0.0043) + 25.0;
    return (int)temp;
#else
    return 35; // Placeholder si no está habilitado en el core
#endif
}

void triggerEmergencyShutdown() {
    // Forzar pinajes a 0 de forma inmediata
    digitalWrite(PIN_ON5V, LOW);
    digitalWrite(PIN_ON50V_GLOBAL, LOW);
    
    // Apagado 920
    digitalWrite(PIN_ON_LNA1_920, LOW);
    digitalWrite(PIN_ON_LNA2_920, LOW);
    digitalWrite(PIN_SELPWR_AMP_920, LOW);
    digitalWrite(PIN_SELPWR_LNA_920, LOW);
    
    // Apagado 24
    digitalWrite(PIN_LNA_ON_24, LOW);
    digitalWrite(PIN_ON_AMP_24, LOW);
    digitalWrite(PIN_SEL_PWR_LNA_24, LOW);

    currentMode920 = MODE_OFF;
    targetMode920  = MODE_OFF;
    currentMode24  = MODE_OFF;
    targetMode24   = MODE_OFF;
    seqStep920     = STEP_IDLE;
    seqStep24      = STEP_IDLE;
}

void processMonitoring() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastMonitoringTime >= MONITORING_INTERVAL) {
        lastMonitoringTime = currentMillis;

        sys_temperature_c = readInternalTemperature();
        uint16_t adcConsumo = analogRead(PIN_GLOBAL_I_SENSE); // 0-1023 (10-bit ADC standard Arduino)
        sys_consumption = map(adcConsumo, 0, 1023, 0, 255);

        // Verificación de límites de seguridad
        if (sys_temperature_c >= TEMPERATURE_LIMIT) {
            sys_alert_state = 0xF0;
            fault_active = true;
        } else if (sys_consumption >= CONSUMPTION_LIMIT) {
            sys_alert_state = 0x08;
            fault_active = true;
        } else {
            // Recuperación automática si las lecturas vuelven a rangos seguros
            if (fault_active) {
                sys_alert_state = 0x01;
                fault_active = false;
            }
        }

        if (fault_active) {
            triggerEmergencyShutdown();
        }
    }
}

// ==============================================================================
// 4. MÁQUINAS DE ESTADOS PARA SECUENCIAS RF NO BLOQUEANTES
// ==============================================================================
bool isAmp920Active() { return (currentMode920 == MODE_TX || currentMode920 == MODE_RX || currentMode920 == MODE_EX); }
bool isAmp24Active()  { return (currentMode24 == MODE_TX || currentMode24 == MODE_RX); }

void processFSM_920() {
    if (currentMode920 == targetMode920) return;
    unsigned long currentMillis = millis();

    // Regla global: Forzar retorno a Standby primero si estamos en modo activo y se pidió otro activo u OFF
    if (currentMode920 != MODE_STANDBY && currentMode920 != MODE_OFF && targetMode920 != MODE_STANDBY) {
        targetMode920 = MODE_STANDBY; // Forzamos el paso intermedio
    }

    // Ejecución de Transiciones
    if (currentMode920 == MODE_OFF && targetMode920 == MODE_STANDBY) {
        digitalWrite(PIN_SELPWR_LNA_920, HIGH); digitalWrite(PIN_SELPWR_AMP_920, LOW);
        digitalWrite(PIN_SELLNA_LNA_920, HIGH); digitalWrite(PIN_SELLNA_EXT_920, LOW);
        digitalWrite(PIN_IN_AMP2_920, HIGH);    digitalWrite(PIN_IN_AMP3_920, HIGH);
        digitalWrite(PIN_SEL_LNAOUT2_920, HIGH);digitalWrite(PIN_SEL_LNAOUT3_920, LOW);
        digitalWrite(PIN_ON_LNA1_920, HIGH);    digitalWrite(PIN_ON_LNA2_920, HIGH);
        digitalWrite(PIN_ON5V, HIGH);
        currentMode920 = MODE_STANDBY;
    }
    
    else if (currentMode920 == MODE_STANDBY && targetMode920 == MODE_TX) {
        if (seqStep920 == STEP_IDLE) {
            seqTimer920 = currentMillis;
            seqStep920 = STEP_DELAY_1;
        } else if (seqStep920 == STEP_DELAY_1 && currentMillis - seqTimer920 >= CHANGEMODEDELAY) {
            digitalWrite(PIN_SELPWR_LNA_920, LOW);
            digitalWrite(PIN_SELPWR_AMP_920, HIGH);
            digitalWrite(PIN_IN_AMP2_920, (currentArg920 & 0x01) ? HIGH : LOW);
            digitalWrite(PIN_IN_AMP3_920, (currentArg920 & 0x02) ? HIGH : LOW);
            currentMode920 = MODE_TX;
            seqStep920 = STEP_IDLE;
        }
    }
    
    else if (currentMode920 == MODE_TX && targetMode920 == MODE_STANDBY) {
        digitalWrite(PIN_SELPWR_LNA_920, HIGH);
        digitalWrite(PIN_SELPWR_AMP_920, LOW);
        digitalWrite(PIN_IN_AMP2_920, HIGH);
        digitalWrite(PIN_IN_AMP3_920, HIGH);
        currentMode920 = MODE_STANDBY;
    }
    
    else if (currentMode920 == MODE_STANDBY && targetMode920 == MODE_RX) {
        if (seqStep920 == STEP_IDLE) {
            seqTimer920 = currentMillis;
            seqStep920 = STEP_DELAY_1;
        } else if (seqStep920 == STEP_DELAY_1 && currentMillis - seqTimer920 >= CHANGEMODEDELAY) {
            digitalWrite(PIN_ON_LNA2_920, LOW);
            seqTimer920 = currentMillis;
            seqStep920 = STEP_DELAY_2;
        } else if (seqStep920 == STEP_DELAY_2 && currentMillis - seqTimer920 >= ONLNADELAY) {
            digitalWrite(PIN_ON_LNA1_920, LOW);
            seqTimer920 = currentMillis;
            seqStep920 = STEP_FINISH;
        } else if (seqStep920 == STEP_FINISH && currentMillis - seqTimer920 >= ONLNADELAY) {
            digitalWrite(PIN_SEL_LNAOUT2_920, (currentArg920 & 0x01) ? HIGH : LOW);
            digitalWrite(PIN_SEL_LNAOUT3_920, (currentArg920 & 0x02) ? HIGH : LOW);
            currentMode920 = MODE_RX;
            seqStep920 = STEP_IDLE;
        }
    }

    else if (currentMode920 == MODE_RX && targetMode920 == MODE_STANDBY) {
        if (seqStep920 == STEP_IDLE) {
            digitalWrite(PIN_ON_LNA1_920, HIGH);
            seqTimer920 = currentMillis;
            seqStep920 = STEP_DELAY_1;
        } else if (seqStep920 == STEP_DELAY_1 && currentMillis - seqTimer920 >= ONLNADELAY) {
            digitalWrite(PIN_ON_LNA2_920, HIGH);
            digitalWrite(PIN_SEL_LNAOUT2_920, HIGH);
            digitalWrite(PIN_SEL_LNAOUT3_920, LOW);
            currentMode920 = MODE_STANDBY;
            seqStep920 = STEP_IDLE;
        }
    }

    else if (currentMode920 == MODE_STANDBY && targetMode920 == MODE_EX) {
        if (seqStep920 == STEP_IDLE) {
            seqTimer920 = currentMillis;
            seqStep920 = STEP_DELAY_1;
        } else if (seqStep920 == STEP_DELAY_1 && currentMillis - seqTimer920 >= CHANGEMODEDELAY) {
            digitalWrite(PIN_SELLNA_LNA_920, LOW);
            digitalWrite(PIN_SELLNA_EXT_920, HIGH);
            currentMode920 = MODE_EX;
            seqStep920 = STEP_IDLE;
        }
    }

    else if (currentMode920 == MODE_EX && targetMode920 == MODE_STANDBY) {
        digitalWrite(PIN_SELLNA_LNA_920, HIGH);
        digitalWrite(PIN_SELLNA_EXT_920, LOW);
        currentMode920 = MODE_STANDBY;
    }

    else if (currentMode920 == MODE_STANDBY && targetMode920 == MODE_OFF) {
        if (seqStep920 == STEP_IDLE) {
            if (currentMode24 == MODE_OFF) digitalWrite(PIN_ON5V, LOW); // Apaga bus si el 24 tampoco lo usa
            seqTimer920 = currentMillis;
            seqStep920 = STEP_DELAY_1;
        } else if (seqStep920 == STEP_DELAY_1 && currentMillis - seqTimer920 >= POWEROFFDELAY) {
            // Forzar a 0 salidas
            digitalWrite(PIN_IN_AMP2_920, LOW); digitalWrite(PIN_IN_AMP3_920, LOW);
            digitalWrite(PIN_SELLNA_LNA_920, LOW); digitalWrite(PIN_SELLNA_EXT_920, LOW);
            digitalWrite(PIN_SELPWR_AMP_920, LOW); digitalWrite(PIN_SELPWR_LNA_920, LOW);
            digitalWrite(PIN_ON_LNA1_920, LOW); digitalWrite(PIN_ON_LNA2_920, LOW);
            digitalWrite(PIN_SEL_LNAOUT2_920, LOW); digitalWrite(PIN_SEL_LNAOUT3_920, LOW);
            currentMode920 = MODE_OFF;
            seqStep920 = STEP_IDLE;
        }
    }
}

void processFSM_24() {
    if (currentMode24 == targetMode24) return;
    unsigned long currentMillis = millis();

    if (currentMode24 != MODE_STANDBY && currentMode24 != MODE_OFF && targetMode24 != MODE_STANDBY) {
        targetMode24 = MODE_STANDBY; 
    }

    if (currentMode24 == MODE_OFF && targetMode24 == MODE_STANDBY) {
        digitalWrite(PIN_SEL_PWR_LNA_24, HIGH);
        digitalWrite(PIN_LNA_ON_24, HIGH);
        digitalWrite(PIN_LNA_IN1_24, HIGH);
        digitalWrite(PIN_LNA_IN2_24, LOW);
        digitalWrite(PIN_ON_AMP_24, LOW);
        digitalWrite(PIN_ON5V, HIGH);
        currentMode24 = MODE_STANDBY;
    }

    else if (currentMode24 == MODE_STANDBY && targetMode24 == MODE_TX) {
        if (seqStep24 == STEP_IDLE) {
            seqTimer24 = currentMillis;
            seqStep24 = STEP_DELAY_1;
        } else if (seqStep24 == STEP_DELAY_1 && currentMillis - seqTimer24 >= CHANGEMODEDELAY) {
            digitalWrite(PIN_SEL_PWR_LNA_24, LOW);
            digitalWrite(PIN_LNA_IN1_24, (currentArg24 & 0x01) ? HIGH : LOW);
            digitalWrite(PIN_LNA_IN2_24, (currentArg24 & 0x02) ? HIGH : LOW);
            digitalWrite(PIN_ON_AMP_24, HIGH);
            currentMode24 = MODE_TX;
            seqStep24 = STEP_IDLE;
        }
    }

    else if (currentMode24 == MODE_TX && targetMode24 == MODE_STANDBY) {
        digitalWrite(PIN_ON_AMP_24, LOW);
        digitalWrite(PIN_SEL_PWR_LNA_24, HIGH);
        digitalWrite(PIN_LNA_IN1_24, HIGH);
        digitalWrite(PIN_LNA_IN2_24, LOW);
        currentMode24 = MODE_STANDBY;
    }

    else if (currentMode24 == MODE_STANDBY && targetMode24 == MODE_RX) {
        if (seqStep24 == STEP_IDLE) {
            seqTimer24 = currentMillis;
            seqStep24 = STEP_DELAY_1;
        } else if (seqStep24 == STEP_DELAY_1 && currentMillis - seqTimer24 >= CHANGEMODEDELAY) {
            digitalWrite(PIN_LNA_ON_24, LOW);
            seqTimer24 = currentMillis;
            seqStep24 = STEP_DELAY_2;
        } else if (seqStep24 == STEP_DELAY_2 && currentMillis - seqTimer24 >= ONLNADELAY) {
            digitalWrite(PIN_LNA_IN1_24, (currentArg24 & 0x01) ? HIGH : LOW);
            digitalWrite(PIN_LNA_IN2_24, (currentArg24 & 0x02) ? HIGH : LOW);
            currentMode24 = MODE_RX;
            seqStep24 = STEP_IDLE;
        }
    }

    else if (currentMode24 == MODE_RX && targetMode24 == MODE_STANDBY) {
        if (seqStep24 == STEP_IDLE) {
            digitalWrite(PIN_LNA_ON_24, HIGH);
            seqTimer24 = currentMillis;
            seqStep24 = STEP_DELAY_1;
        } else if (seqStep24 == STEP_DELAY_1 && currentMillis - seqTimer24 >= ONLNADELAY) {
            digitalWrite(PIN_LNA_IN1_24, HIGH);
            digitalWrite(PIN_LNA_IN2_24, LOW);
            currentMode24 = MODE_STANDBY;
            seqStep24 = STEP_IDLE;
        }
    }

    else if (currentMode24 == MODE_STANDBY && targetMode24 == MODE_OFF) {
        if (seqStep24 == STEP_IDLE) {
            if (currentMode920 == MODE_OFF) digitalWrite(PIN_ON5V, LOW);
            seqTimer24 = currentMillis;
            seqStep24 = STEP_DELAY_1;
        } else if (seqStep24 == STEP_DELAY_1 && currentMillis - seqTimer24 >= POWEROFFDELAY) {
            digitalWrite(PIN_SEL_PWR_LNA_24, LOW);
            digitalWrite(PIN_LNA_ON_24, LOW);
            digitalWrite(PIN_LNA_IN1_24, LOW);
            digitalWrite(PIN_LNA_IN2_24, LOW);
            digitalWrite(PIN_ON_AMP_24, LOW);
            currentMode24 = MODE_OFF;
            seqStep24 = STEP_IDLE;
        }
    }
}

// ==============================================================================
// 5. EVENTOS I2C
// ==============================================================================
void prepareResponse(uint8_t disp, uint8_t cmd, uint8_t result, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6) {
    tx_buffer[0] = disp;
    tx_buffer[1] = cmd;
    tx_buffer[2] = result;
    tx_buffer[3] = b3;
    tx_buffer[4] = b4;
    tx_buffer[5] = b5;
    tx_buffer[6] = b6;
    tx_buffer[7] = calculateCRC8(tx_buffer, 7);
}

void onI2CReceive(int numBytes) {
    if (numBytes != 4) {
        while (Wire.available()) Wire.read(); // Vaciar buffer si la trama es inválida
        return; 
    }

    uint8_t rx[4];
    for (int i = 0; i < 4; i++) {
        rx[i] = Wire.read();
    }

    // Verificar CRC8
    if (calculateCRC8(rx, 3) != rx[3]) return; // Trama descartada

    uint8_t disp = rx[0];
    uint8_t cmd  = rx[1];
    uint8_t arg  = rx[2];
    uint8_t result = 0x01; // Asumimos OK por defecto

    // Bloqueo de comandos de potencia si hay falla
    if (fault_active && cmd != 0x01 && cmd != 0x02) {
        prepareResponse(disp, cmd, 0xFF, 0, 0, 0, 0);
        return;
    }

    if (disp == 0x11) {
        // Controlador Global
        if (cmd == 0x01) {
            prepareResponse(disp, cmd, 0x01, sys_temperature_c, sys_consumption, sys_alert_state, 0x00);
            return;
        } else if (cmd == 0x02) {
            prepareResponse(disp, cmd, 0x01, sys_alert_state, currentMode920, currentMode24, 0x00);
            return;
        } else if (cmd == 0x0A) {
            digitalWrite(PIN_ON5V, arg ? HIGH : LOW);
        } else if (cmd == 0x0F) {
            digitalWrite(PIN_ON50V_GLOBAL, arg ? HIGH : LOW);
        } else {
            result = 0x0A; // UNKNOWN
        }
    } 
    else if (disp == 0x22) {
        // Amplificador 920 MHz
        if (cmd == 0x10) { targetMode920 = MODE_STANDBY; }
        else if (cmd == 0x20) { targetMode920 = MODE_OFF; }
        else if (cmd == 0x40) {
            if (currentMode920 == MODE_OFF) result = 0xFF; // Debe encenderse primero a Standby
            else { targetMode920 = MODE_TX; currentArg920 = arg; targetMode24 = MODE_STANDBY; } // Exclusividad cruzada
        }
        else if (cmd == 0x80) {
            if (currentMode920 == MODE_OFF) result = 0xFF;
            else { targetMode920 = MODE_RX; currentArg920 = arg; targetMode24 = MODE_STANDBY; }
        }
        else if (cmd == 0xF0) {
            if (currentMode920 == MODE_OFF) result = 0xFF;
            else { targetMode920 = MODE_EX; currentArg920 = arg; targetMode24 = MODE_STANDBY; }
        } else {
            result = 0x0A;
        }
    } 
    else if (disp == 0x33) {
        // Amplificador 2.4 GHz
        if (cmd == 0x10) { targetMode24 = MODE_STANDBY; }
        else if (cmd == 0x20) { targetMode24 = MODE_OFF; }
        else if (cmd == 0x40) {
            if (currentMode24 == MODE_OFF) result = 0xFF;
            else { targetMode24 = MODE_TX; currentArg24 = arg; targetMode920 = MODE_STANDBY; }
        }
        else if (cmd == 0x80) {
            if (currentMode24 == MODE_OFF) result = 0xFF;
            else { targetMode24 = MODE_RX; currentArg24 = arg; targetMode920 = MODE_STANDBY; }
        } else {
            result = 0x0A;
        }
    } else {
        result = 0x0A; // Dispositivo no reconocido
    }

    prepareResponse(disp, cmd, result, 0, 0, 0, 0);
}

void onI2CRequest() {
    Wire.write((uint8_t*)tx_buffer, 8);
}

// ==============================================================================
// 6. SETUP PRINCIPAL
// ==============================================================================
void setup() {
    // Inicialización de pines Globales
    pinMode(PIN_ON5V, OUTPUT);
    pinMode(PIN_ON50V_GLOBAL, OUTPUT);
    digitalWrite(PIN_ON5V, LOW);
    digitalWrite(PIN_ON50V_GLOBAL, LOW);
    
    // Inicialización 920 MHz
    pinMode(PIN_IN_AMP2_920, OUTPUT);
    pinMode(PIN_IN_AMP3_920, OUTPUT);
    pinMode(PIN_SELLNA_LNA_920, OUTPUT);
    pinMode(PIN_SELLNA_EXT_920, OUTPUT);
    pinMode(PIN_SELPWR_AMP_920, OUTPUT);
    pinMode(PIN_SELPWR_LNA_920, OUTPUT);
    pinMode(PIN_ON_LNA1_920, OUTPUT);
    pinMode(PIN_ON_LNA2_920, OUTPUT);
    pinMode(PIN_SEL_LNAOUT2_920, OUTPUT);
    pinMode(PIN_SEL_LNAOUT3_920, OUTPUT);

    // Inicialización 2.4 GHz
    pinMode(PIN_SEL_PWR_LNA_24, OUTPUT);
    pinMode(PIN_LNA_ON_24, OUTPUT);
    pinMode(PIN_LNA_IN1_24, OUTPUT);
    pinMode(PIN_LNA_IN2_24, OUTPUT);
    pinMode(PIN_ON_AMP_24, OUTPUT);
    
    triggerEmergencyShutdown(); // Fuerza condiciones seguras iniciales

    // Inicialización I2C Slave
    Wire.begin(I2C_ADDRESS);
    Wire.setClock(100000); // 100 kHz Standard Mode
    Wire.onReceive(onI2CReceive);
    Wire.onRequest(onI2CRequest);

    // Preparar buffer por defecto ante primer Request fantasma
    prepareResponse(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
}

// ==============================================================================
// 7. LOOP PRINCIPAL
// ==============================================================================
void loop() {
    // 1. Monitoreo analógico de temperatura y consumo de corriente
    processMonitoring();

    // 2. Procesamiento de las FSM concurrentes para conmutación de pines RF
    if (!fault_active) {
        // Exclusividad estricta de ejecución
        // Si el Amp 920 requiere transicionar a activo, el Amp 24 DEBE estar en Standby (u OFF) primero.
        if (targetMode920 != MODE_OFF && targetMode920 != MODE_STANDBY) {
            if (currentMode24 == MODE_STANDBY || currentMode24 == MODE_OFF) {
                processFSM_920();
            } else {
                targetMode24 = MODE_STANDBY;
                processFSM_24();
            }
        } 
        else if (targetMode24 != MODE_OFF && targetMode24 != MODE_STANDBY) {
            if (currentMode920 == MODE_STANDBY || currentMode920 == MODE_OFF) {
                processFSM_24();
            } else {
                targetMode920 = MODE_STANDBY;
                processFSM_920();
            }
        } 
        else {
            // Flujo libre para transiciones hacia Standby o Apagado
            processFSM_920();
            processFSM_24();
        }
    }
}