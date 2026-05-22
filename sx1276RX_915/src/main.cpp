#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -----------------------------------------
// Definición de Pines - LoRa SX1276
// -----------------------------------------
#define LORA_SCK  5  // <-- CORREGIDO PARA COINCIDIR CON EL TX
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_NSS  18
#define LORA_RST  14
#define LORA_DIO0 26
#define LORA_DIO1 33 

// -----------------------------------------
// Definición de Pines - Pantalla OLED I2C
// -----------------------------------------
#define I2C_SDA 21
#define I2C_SCL 22

// -----------------------------------------
// Configuración OLED
// -----------------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -----------------------------------------
// Prototipos
// -----------------------------------------
void updateDisplay(String incomingText, int rssi, float snr);

void setup() {
  Serial.begin(115200);
  // Pequeño retardo para estabilizar el puerto serie tras el boot
  delay(1500); 
  Serial.println("\n--- Iniciando diagnóstico de hardware ---");

  // 1. Diagnóstico de I2C y Pantalla
  Wire.begin(I2C_SDA, I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("ERROR CRÍTICO: No se detecta la pantalla OLED en la dirección 0x3C.");
    Serial.println("Revisa los pines SDA (21) y SCL (22) y la alimentación de la pantalla.");
    while(true) { 
      delay(1000); // El delay evita que el Watchdog resetee el ESP32
    }
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Iniciando LoRa...");
  display.display();
  Serial.println("Pantalla OLED inicializada OK.");

  // 2. Diagnóstico de SPI y LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, -1);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  while (!LoRa.begin(433E6)) {
    Serial.println(".");
      delay(1000);
  }

  // Configuración de parámetros de RF
  LoRa.setSpreadingFactor(7);           
  LoRa.setSignalBandwidth(125E3);       
  LoRa.setCodingRate4(5);               
  LoRa.enableCrc();
  
  Serial.println("Módulo LoRa inicializado OK.");
  display.println("LoRa OK. RX Mode");
  display.display();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    Serial.println("--- Paquete Recibido ---");
    Serial.print("Payload : ");
    Serial.println(incoming);
    Serial.print("RSSI    : ");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.print("SNR     : ");
    Serial.print(snr);
    Serial.println(" dB");
    Serial.println("------------------------\n");

    updateDisplay(incoming, rssi, snr);
  }
}

void updateDisplay(String incomingText, int rssi, float snr) {
  display.clearDisplay();
  display.setCursor(0, 0);
  
  display.println("== NUEVO PAQUETE ==");
  
  display.setCursor(0, 15);
  display.print("Data: ");
  display.println(incomingText.substring(0, 20)); 
  
  display.setCursor(0, 35);
  display.print("RSSI: ");
  display.print(rssi);
  display.println(" dBm");
  
  display.setCursor(0, 45);
  display.print("SNR:  ");
  display.print(snr);
  display.println(" dB");
  
  display.display();
}