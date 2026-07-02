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

// --- POTENCIA DE TRANSMISIÓN AJUSTABLE POR EL USUARIO ---
// El rango del SX1280 va desde -18 dBm hasta +13 dBm.
// Se predetermina al mínimo absoluto (-18) según lo solicitado.
int8_t txPower = 0; 

// Variables para el temporizador y el contador de mensajes
unsigned long lastTransmitTime = 0;
const unsigned long transmitInterval = 10000; // 10,000 ms = 10 segundos
uint32_t msgCounter = 0;

// Prototipo de función para actualizar los textos en la pantalla OLED
void actualizarPantalla(String msg, String estado);

void setup() {
  Serial.begin(115200);

  // 1. Configurar pines de control de la antena RF
  pinMode(LORA_RX_EN, OUTPUT);
  pinMode(LORA_TX_EN, OUTPUT);
  digitalWrite(LORA_RX_EN, LOW);  // Modo recepción apagado
  digitalWrite(LORA_TX_EN, LOW);  // Modo transmisión apagado de momento

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
  display.println("Iniciando TX...");
  display.display();

  // 3. Configurar bus SPI
  SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI, LORA_CS);

  // 4. Inicializar el chip SX1280 con tus parámetros base
  // Parámetros: Frecuencia (2450.0 MHz), BW (203.125 kHz), SF (7), CR (8)
  // NOTA: El valor '8' en RadioLib configura automáticamente CR 4/8 con Long Interleaving.
  Serial.print(F("[SX1280] Configurando parámetros LoRa TX... "));
  int state = radio.begin(2450.0, 203.125, 7, 8);

  if (state == RADIOLIB_ERR_NONE) {
    state = radio.setPreambleLength(12); // Preámbulo de 12 símbolos
  }
  if (state == RADIOLIB_ERR_NONE) {
    state = radio.setCRC(2);             // CRC Activado (2 bytes)
  }
  if (state == RADIOLIB_ERR_NONE) {
    state = radio.setOutputPower(txPower); // Asignar potencia mínima (-18 dBm)
  }

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("¡Configuración de Transmisor Exitosa!"));
    actualizarPantalla("Ninguno", "Listo para TX");
  } else {
    Serial.print(F("Fallo en inicialización, código: ")); Serial.println(state);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ERROR CONFIG LORA");
    display.print("Codigo: "); display.println(state);
    display.display();
    while (true);
  }
}

void loop() {
  // Temporizador no bloqueante basado en millis() cada 10 segundos
  if (millis() - lastTransmitTime >= transmitInterval) {
    lastTransmitTime = millis();

    // Incrementar conteo y formatear payload
    msgCounter++;
    String mensaje = "hola spel " + String(msgCounter);
    
    Serial.print(F("Transmitiendo paquete: ")); Serial.println(mensaje);
    actualizarPantalla(mensaje, "Enviando...");

    // --- CRUCIAL EN LILYGO: Conmutar la antena al amplificador de transmisión (PA) ---
    digitalWrite(LORA_RX_EN, LOW);
    digitalWrite(LORA_TX_EN, HIGH); 

    // Enviar el paquete de datos (Bloqueante controlado)
    int state = radio.transmit(mensaje);

    // --- Apagar la ruta de transmisión de la antena al terminar ---
    digitalWrite(LORA_TX_EN, LOW);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("¡Transmisión Exitosa!"));
      actualizarPantalla(mensaje, "Enviado OK");
    } else {
      Serial.print(F("Fallo al transmitir, código: ")); Serial.println(state);
      actualizarPantalla(mensaje, "Error: " + String(state));
    }
  }
}

// Función auxiliar para imprimir el estado actual en tu pantalla SSD1306
void actualizarPantalla(String msg, String estado) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("--- LORA 2.4G TX ---");
  display.println("");
  display.print("Frec: 2.45 GHz | SF: 7");
  display.println("");
  display.print("BW: 203.12 kHz | CR: 4/8");
  display.println("");
  display.print("Potencia: "); display.print(txPower); display.println(" dBm");
  display.println("--------------------");
  display.print("Msg: "); display.println(msg);
  display.print("Est: "); display.println(estado);
  display.display();
}