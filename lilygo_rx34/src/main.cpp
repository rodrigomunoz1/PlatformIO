#include <RadioLib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Definición de pines fijos para la LilyGO T3-S3 con SX1280
#define LORA_MISO      3
#define LORA_MOSI      6
#define LORA_SCLK      5
#define LORA_CS        7
#define LORA_RST       8
#define LORA_BUSY      36
#define LORA_DIO1      9

// Pines para el control de antena en la versión con Amplificador (PA)
#define LORA_RX_EN     21
#define LORA_TX_EN     10

// Pines de la pantalla OLED integrada (SSD1306)
#define OLED_SDA       18
#define OLED_SCL       17
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64

// Inicialización de la pantalla OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Inicialización del módulo de Radio
SX1280 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);

// --- VARIABLES PARA INTERRUPCIÓN ---
volatile bool mensajeRecibidoFlag = false; // Bandera que se activa en la interrupción

// Función ISR (Interrupt Service Routine) que se ejecuta en la RAM del ESP32
void IRAM_ATTR lw_onReceive() {
  mensajeRecibidoFlag = true;
}
// ------------------------------------

void setup() {
  Serial.begin(115200);

  // 1. Configurar pines de control RF
  pinMode(LORA_RX_EN, OUTPUT);
  pinMode(LORA_TX_EN, OUTPUT);
  digitalWrite(LORA_RX_EN, HIGH);  // Activamos modo recepción
  digitalWrite(LORA_TX_EN, LOW);

  // 2. Inicializar la pantalla OLED integrada
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("[ERROR] No se pudo inicializar la pantalla OLED"));
    while(true);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();

  // 3. Configurar bus SPI
  SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI, LORA_CS);

  // 4. Inicializar el chip SX1280
  Serial.print(F("[SX1280] Configurando parámetros LoRa... "));
  int state = radio.begin(2450.0, 406.25,9, 8);

  if (state == RADIOLIB_ERR_NONE) {
    state = radio.setPreambleLength(8); // Preámbulo de 12 símbolos
  }
  if (state == RADIOLIB_ERR_NONE) {
    state = radio.setCRC(2);             // CRC Activado (2 bytes)
  }

  display.clearDisplay();
  display.setCursor(0, 0);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("¡Configuración Exitosa!"));
    display.println("LoRa 2.4GHz OK");
    display.println("Modo: Interrupciones");
    display.println("Freq: 2.45 GHz");
    display.println("SF: 7  |  CR: 4/5");
    display.println("BW: 203.12 kHz");
    display.display();

    // --- CONFIGURAR LA INTERRUPCIÓN Y ACTIVAR RECEPCIÓN ---
    // Decimos a RadioLib qué función ejecutar cuando DIO1 pase a HIGH
    radio.setPacketReceivedAction(lw_onReceive);

    // Activamos el modo de escucha constante (No bloqueante)
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
      Serial.print(F("Error al iniciar escucha: ")); Serial.println(state);
    }
    // ------------------------------------------------------

  } else {
    Serial.print(F("Fallo en inicialización: ")); Serial.println(state);
    display.println("ERROR LORA");
    display.display();
    while (true);
  }
}

void loop() {
  // Verificamos si la interrupción levantó la bandera
  if (mensajeRecibidoFlag) {
    mensajeRecibidoFlag = false; // Reseteamos la bandera de inmediato

    String mensaje;
    // Leemos el mensaje guardado en la memoria intermedia (FIFO) del SX1280
    int state = radio.readData(mensaje);

    if (state == RADIOLIB_ERR_NONE) {
      // Mensaje leído correctamente
      float rssi = radio.getRSSI();
      float snr = radio.getSNR();
      float freqErr = radio.getFrequencyError();


      // Mostrar en Monitor Serial
      Serial.print(F("[Datos]: ")); Serial.println(mensaje);
      Serial.print(F("[RSSI]: "));  Serial.print(rssi); Serial.println(F(" dBm"));
      Serial.print(F("[SNR]: "));   Serial.print(snr);  Serial.println(F(" dB"));
      Serial.print(F("[FreqErr]: "));   Serial.print(freqErr);  Serial.println(F(" Hz"));
      Serial.println();

      // Mostrar en Pantalla OLED
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("--- INTERRUPCION ---");
      display.println("");
      display.print("Msg: "); display.println(mensaje);
      display.println("");
      display.print("RSSI: "); display.print(rssi, 1); display.println(" dBm");
      display.print("SNR: ");  display.print(snr, 1);  display.println(" dB");
      display.print("FreqErr: ");  display.print(freqErr, 1);  display.println(" Hz");
      display.display();

    } else {
      // Error de CRC o paquete corrupto
      Serial.print(F("Error al leer datos, código: "));
      Serial.println(state);
    }

    // CRUCIAL: Volver a activar el modo escucha de fondo para el siguiente paquete
    radio.startReceive();
  }

  // Aquí el loop corre libre, puedes agregar otras tareas sin congelar el sistema
}