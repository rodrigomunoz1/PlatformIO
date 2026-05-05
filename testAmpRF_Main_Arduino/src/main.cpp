#include <Arduino.h>
#include <Wire.h>

/**
 * PROYECTO: ampIRF_SuchailV (STM32F103C6T6)
 * ROL: Esclavo I2C (Dirección 0x20)
 * FRAMEWORK: Arduino / PlatformIO
 */

// --- DEFINICIONES DE HARDWARE (Basado en CubeMX Report) ---
#define I2C_ADDRESS         0x20
#define PIN_GlobalSWIsense  PA1   // ADC1_IN1[span_5](end_span)
#define PIN_ON5V            PB0   // Control 5V[span_6](end_span)
#define PIN_ON50VGlobal     PB1   // Control 50V[span_7](end_span)

// Pines Amplificador 920[span_8](end_span)[span_9](end_span)
#define PIN_IN_AMP2_920      PA15
#define PIN_IN_AMP3_920      PA13
#define PIN_SEL_LNAOUT2_920  PA11
#define PIN_SEL_LNAOUT3_920  PB13
#define PIN_SELLNA_LNA_920   PB3
#define PIN_ON_LNA1_920      PB4
#define PIN_SELLNA_EXT_920   PB5
#define PIN_SELPWR_LNA_920   PB6
#define PIN_SELPWR_AMP_920   PB7
#define PIN_ON_LNA2_920      PB14

// Pines Amplificador 24[span_10](end_span)[span_11](end_span)
#define PIN_LNA_IN1_24       PB11
#define PIN_LNA_IN2_24       PA8
#define PIN_LNA_ON_24        PA10
#define PIN_SEL_PWR_LNA_24   PB15
#define PIN_ON_AMP_24        PB12

// --- PARÁMETROS CRÍTICOS ---
#define TEMPERATURE_LIMIT   90.0f  //[span_12](end_span)
#define CONSUMPTION_LIMIT   4500.0f // mA[span_13](end_span)
#define CHANGEMODEDELAY     100
#define ONLNADELAY          50
#define POWEROFFDELAY       100

// --- VARIABLES GLOBALES ---
enum Mode { STANDBY = 0x01, TX = 0x02, RX = 0x03, EX = 0x04, OFF = 0x05 };

struct AmpState {
    Mode currentMode = OFF;
};

AmpState amp920, amp24;
float currentTemp = 0;
float currentCons = 0;
bool globalFault = false;

// Buffer de respuesta fija para el maestro (13 bytes)[span_14](end_span)
// [STATUS] [DATA_LEN] [DATA0...DATA9] [CRC]
uint8_t i2c_tx_buffer[13];

// =============================================================================
// 1. FUNCIONES DE MONITOREO Y PROTECCIÓN
// =============================================================================

float readInternalTemp() {
    // Lectura del sensor interno (Canal 16 en F103)[span_15](end_span)
    int raw = analogRead(ATEMP); 
    float v_sense = (raw * 3.3f) / 4095.0f;
    // Fórmula estándar: ((V25 - Vsense) / Avg_Slope) + 25
    return ((1.43f - v_sense) / 0.0043f) + 25.0f;
}

void shutdownAll() {
    // Apagar líneas de potencia[span_16](end_span)
    digitalWrite(PIN_ON5V, LOW);
    digitalWrite(PIN_ON50VGlobal, LOW);
    
    // Resetear estados y pines de control
    digitalWrite(PIN_ON_AMP_24, LOW);
    digitalWrite(PIN_SELPWR_AMP_920, LOW);
    amp920.currentMode = OFF;
    amp24.currentMode = OFF;
}

void updateSensors() {
    currentTemp = readInternalTemp();
    int rawCons = analogRead(PIN_GlobalSWIsense);
    currentCons = (rawCons * 5000.0f) / 4095.0f;

    // Lógica de fallo: apagar si se exceden los límites[span_17](end_span)
    if (currentTemp > TEMPERATURE_LIMIT || currentCons > CONSUMPTION_LIMIT) {
        if (!globalFault) {
            globalFault = true;
            shutdownAll();
        }
    } else {
        globalFault = false;
    }
}

// =============================================================================
// 2. SECUENCIAS DE CONTROL DE AMPLIFICADORES[span_18](end_span)
// =============================================================================

void setStandby920() {
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
    amp920.currentMode = STANDBY;
}

void control920(uint8_t cmd, uint8_t args) {
    if (globalFault) return;
    if (amp920.currentMode != STANDBY && cmd != 0x10 && cmd != 0x11) {
        setStandby920();
        delay(CHANGEMODEDELAY);
    }
    switch(cmd) {
        case 0x10: setStandby920(); break;
        case 0x11: digitalWrite(PIN_ON5V, LOW); delay(POWEROFFDELAY); amp920.currentMode = OFF; break;
        case 0x12: // TX
            digitalWrite(PIN_SELPWR_LNA_920, LOW);
            digitalWrite(PIN_SELPWR_AMP_920, HIGH);
            digitalWrite(PIN_IN_AMP2_920, (args & 0x01));
            digitalWrite(PIN_IN_AMP3_920, (args & 0x02) >> 1);
            amp920.currentMode = TX;
            break;
    }
}

void setStandby24() {
    digitalWrite(PIN_SEL_PWR_LNA_24, HIGH);
    digitalWrite(PIN_LNA_ON_24, HIGH);
    digitalWrite(PIN_LNA_IN1_24, HIGH);
    digitalWrite(PIN_LNA_IN2_24, LOW);
    digitalWrite(PIN_ON_AMP_24, LOW);
    digitalWrite(PIN_ON5V, HIGH);
    amp24.currentMode = STANDBY;
}

void control24(uint8_t cmd, uint8_t args) {
    if (globalFault) return;
    if (amp24.currentMode != STANDBY && cmd != 0x10 && cmd != 0x11) {
        setStandby24();
        delay(CHANGEMODEDELAY);
    }
    switch(cmd) {
        case 0x10: setStandby24(); break;
        case 0x11: digitalWrite(PIN_ON5V, LOW); delay(POWEROFFDELAY); amp24.currentMode = OFF; break;
        case 0x12: // TX
            digitalWrite(PIN_SEL_PWR_LNA_24, LOW);
            digitalWrite(PIN_LNA_IN1_24, (args & 0x01));
            digitalWrite(PIN_LNA_IN2_24, (args & 0x02) >> 1);
            digitalWrite(PIN_ON_AMP_24, HIGH);
            amp24.currentMode = TX;
            break;
    }
}

// =============================================================================
// 3. COMUNICACIÓN I2C (RESPUESTA FIJA DE 13 BYTES)
// =============================================================================

void receiveEvent(int howMany) {
    if (howMany < 4) return;
    uint8_t cmd = Wire.read();
    uint8_t amp = Wire.read();
    uint8_t arg = Wire.read();
    uint8_t crc = Wire.read(); 

    memset(i2c_tx_buffer, 0, 13);
    i2c_tx_buffer[0] = globalFault ? 0x01 : 0x00;

    if (amp == 0xFF) {
        if (cmd == 0x01) { // Monitoreo
            i2c_tx_buffer[1] = 0x04; // DATA_LEN
            uint16_t t = (uint16_t)(currentTemp * 100);
            uint16_t c = (uint16_t)currentCons;
            // Mapeo para que el maestro lea data[0]...data[3]
            i2c_tx_buffer[2] = (t >> 8); i2c_tx_buffer[3] = (t & 0xFF);
            i2c_tx_buffer[4] = (c >> 8); i2c_tx_buffer[5] = (c & 0xFF);
        } else if (cmd == 0x04) digitalWrite(PIN_ON5V, arg);
          else if (cmd == 0x05) digitalWrite(PIN_ON50VGlobal, arg);
    } else if (amp == 0x01) control920(cmd, arg);
      else if (amp == 0x02) control24(cmd, arg);

    i2c_tx_buffer[12] = 0; // Placeholder para CRC (Ignorado por ahora)
}

void requestEvent() {
    Wire.write(i2c_tx_buffer, 13);// Siempre enviar 13 bytes[span_19](end_span)
}

// =============================================================================
// 4. CONFIGURACIÓN INICIAL
// =============================================================================

void setup() {
    // Configuración de pines de salida[span_20](end_span)
    int outputs[] = {PIN_ON5V, PIN_ON50VGlobal, PIN_IN_AMP2_920, PIN_IN_AMP3_920, 
                     PIN_SEL_LNAOUT2_920, PIN_SEL_LNAOUT3_920, PIN_SELLNA_LNA_920, 
                     PIN_ON_LNA1_920, PIN_SELLNA_EXT_920, PIN_SELPWR_LNA_920, 
                     PIN_SELPWR_AMP_920, PIN_ON_LNA2_920, PIN_LNA_IN1_24, 
                     PIN_LNA_IN2_24, PIN_LNA_ON_24, PIN_SEL_PWR_LNA_24, PIN_ON_AMP_24};
    
    for(int p : outputs) pinMode(p, OUTPUT);

    analogReadResolution(12);
    ADC1->CR2 |= ADC_CR2_TSVREFE;// Habilitar sensor de temperatura interno[span_22](end_span)

    Wire.begin(I2C_ADDRESS);
    Wire.setClock(100000);// 100kHz[span_23](end_span)
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    
    shutdownAll(); 
}

void loop() {
    updateSensors();
    delay(50);
}