#include <Arduino.h>
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

// Inicialización del módulo de Radio (Usa el bus SPI asignado internamente)
SX1280 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);

void setup() {
  Serial.begin(115200);

  // 1. Configurar pines de control RF (Crucial si tu placa incluye amplificador PA)
  pinMode(LORA_RX_EN, OUTPUT);
  pinMode(LORA_TX_EN, OUTPUT);
  digitalWrite(LORA_RX_EN, HIGH);  // Activamos modo recepción
  digitalWrite(LORA_TX_EN, LOW);   // Desactivamos transmisión

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

  // 3. Configurar y remapear el bus SPI para el ESP32-S3 de LilyGO
  SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI, LORA_CS);

  // 4. Inicializar el chip SX1280 con los parámetros solicitados
  // Parámetros de begin(): Frecuencia (MHz), BW (kHz), SF, CR (Denominador)
  // Nota: Frecuencia = 2450.0 MHz (2.45GHz), BW = 203.125 kHz, SF = 7, CR = 5 (para 4/5)
  Serial.print(F("[SX1280] Configurando parámetros LoRa... "));
  int state = radio.begin(2450.0, 125, 7, 5);

  display.clearDisplay();
  display.setCursor(0, 0);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("¡Configuración Exitosa!"));
    display.println("LoRa 2.4GHz OK");
    display.println("Freq: 2.45 GHz");
    display.println("SF: 7  |  CR: 4/5");
    display.println("BW: 125 kHz");
    display.println("");
    display.println("Esperando mensajes...");
  } else {
    Serial.print(F("Fallo en la inicialización, código: "));
    Serial.println(state);
    display.println("ERROR LORA");
    display.print("Codigo: ");
    display.println(state);
    while (true);
  }
  display.display();
}

void loop() {
  String mensajeRecibido;
  
  Serial.print(F("[SX1280] Escuchando... "));
  
  // Llamada bloqueante para recibir datos (espera hasta que llegue un paquete)
  int state = radio.receive(mensajeRecibido);

  if (state == RADIOLIB_ERR_NONE) {
    // ¡Mensaje recibido con éxito!
    Serial.println(F("¡Mensaje Recibido!"));

    // Extraer métricas de calidad de señal
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();

    // Imprimir en el monitor Serial
    Serial.print(F("[Datos]: "));   Serial.println(mensajeRecibido);
    Serial.print(F("[RSSI]: "));    Serial.print(rssi); Serial.println(F(" dBm"));
    Serial.print(F("[SNR]: "));     Serial.print(snr);  Serial.println(F(" dB"));

    // Mostrar la información en la pantalla OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("--- NUEVO MENSAJE ---");
    display.println("");
    
    // Mostrar el texto del mensaje
    display.print("Texto: ");
    display.println(mensajeRecibido);
    display.println("");
    
    // Mostrar métricas de red
    display.print("RSSI: "); display.print(rssi, 1); display.println(" dBm");
    display.print("SNR: ");  display.print(snr, 1);  display.println(" dB");
    display.display();

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Si configuras un tiempo límite y se agota (en este script es infinito por defecto)
    Serial.println(F("Timeout sin datos."));
  } else {
    // En caso de que el paquete se corrompa (Error de CRC, etc.)
    Serial.print(F("Error en recepción, código: "));
    Serial.println(state);
  }
}