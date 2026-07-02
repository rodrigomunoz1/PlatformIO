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

// Potencia ajustable al mínimo
int8_t txPower = -18; 

// Variables de tiempo y conteo
unsigned long lastTransmitTime = 0;
const unsigned long transmitInterval = 10000; // 10 segundos
uint32_t msgCounter = 0;

// Nueva función de pantalla optimizada
void mostrarPantalla(String estado, String msg, bool invierteColor);

void setup() {
  Serial.begin(115200);

  pinMode(LORA_RX_EN, OUTPUT);
  pinMode(LORA_TX_EN, OUTPUT);
  digitalWrite(LORA_RX_EN, LOW);  
  digitalWrite(LORA_TX_EN, LOW);  

  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("[ERROR] No se pudo inicializar la pantalla OLED"));
    while(true);
  }
  
  // Configuración inicial de texto en la pantalla
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Iniciando TX...");
  display.display();

  SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI, LORA_CS);

  // Inicialización con tus parámetros actuales
  Serial.println(F("\n======================================="));
  Serial.println(F("[SX1280] CONFIGURACIÓN INICIAL DE HARDWARE"));
  int state = radio.begin(2450.0, 406.25, 9, 8);

  if (state == RADIOLIB_ERR_NONE) state = radio.setPreambleLength(8); 
  if (state == RADIOLIB_ERR_NONE) state = radio.setCRC(2);             
  if (state == RADIOLIB_ERR_NONE) state = radio.setOutputPower(txPower); 

  if (state == RADIOLIB_ERR_NONE) {
    // Imprimir el reporte completo en la consola una sola vez
    Serial.println(F("-> Sintonía: OK"));
    Serial.println(F("-> Frecuencia: 2.45 GHz"));
    Serial.println(F("-> Ancho de banda: 406.25 kHz"));
    Serial.println(F("-> Spreading Factor: SF9"));
    Serial.println(F("-> Coding Rate: 4/8 (Long Interleaver)"));
    Serial.print(F("-> Potencia configurada: ")); Serial.print(txPower); Serial.println(F(" dBm"));
    Serial.println(F("=======================================\n"));
    
    mostrarPantalla("STANDBY", "hola spel 0", false);
  } else {
    Serial.print(F("Fallo en inicialización, código: ")); Serial.println(state);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ERROR CONFIG LORA");
    display.display();
    while (true);
  }
}

void loop() {
  if (millis() - lastTransmitTime >= transmitInterval) {
    lastTransmitTime = millis();

    msgCounter++;
    String 
     = "hola spel " + String(msgCounter);
    
    Serial.print(F("Transmitiendo paquete: ")); Serial.println(mensaje);
    
    // 1. Mostrar estado de transmisión en pantalla grande e invertir el color de fondo (ALTA VISIBILIDAD)
    mostrarPantalla("ENVIANDO", mensaje, true);

    // Conmutar antena a modo TX
    digitalWrite(LORA_RX_EN, LOW);
    digitalWrite(LORA_TX_EN, HIGH); 

    // Enviar el paquete
    int state = radio.transmit(mensaje);

    // Apagar amplificador de antena
    digitalWrite(LORA_TX_EN, LOW);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("¡Transmisión Exitosa!"));
      // 2. Volver al estado de espera normal
      mostrarPantalla("ENVIADO OK", mensaje, false);
    } else {
      Serial.print(F("Fallo al transmitir, código: ")); Serial.println(state);
      mostrarPantalla("ERR: " + String(state), mensaje, false);
    }
  }
}

// Nueva función de dibujo ultra-clara
void mostrarPantalla(String estado, String msg, bool invierteColor) {
  display.clearDisplay();
  
  if (invierteColor) {
    // Dibuja una barra brillante en la parte superior si está transmitiendo
    display.fillRect(0, 0, 128, 28, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK); 
    display.setCursor(16, 6);
  } else {
    // Texto normal sobre fondo negro si está esperando
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(6, 6);
  }
  
  // IMPRESIÓN DEL ESTADO EN TEXTO GRANDE (Tamaño 2)
  display.setTextSize(2);
  display.println(estado);
  
  // DETALLES DEL PAQUETE ABAJO EN TEXTO PEQUEÑO (Tamaño 1)
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 38);
  display.print("Payload: "); display.println(msg);
  
  display.setCursor(0, 52);
  display.print("Intervalo: 10 Segundos");
  
  display.display();
}