#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Pines del módulo LoRa (TTGO v2.0) ---
#define SCK_PIN 5
#define MISO_PIN 19
#define MOSI_PIN 27
#define SS_PIN 18
#define RST_PIN 14
#define DIO0_PIN 26

// --- Pines de la pantalla OLED (TTGO v2.0) ---
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST -1 // En la v2.0 generalmente no se usa el pin RST para la pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Inicializar el objeto de la pantalla
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // 1. Inicializar I2C para la pantalla OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) { 
    Serial.println(F("Fallo al iniciar la pantalla OLED SSD1306"));
    for(;;); // Detener ejecución si falla la pantalla
  }
  
  // Limpiar y configurar texto inicial en pantalla
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Iniciando TTGO...");
  display.display();

  // 2. Configurar pines del bus SPI para el módulo LoRa
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);

  // 3. Inicializar LoRa a 924.5 MHz
  if (!LoRa.begin(924.5E6)) {
    Serial.println("¡Fallo al iniciar el modulo LoRa!");
    display.println("Fallo Radio LoRa!");
    display.display();
    while (1);
  }

  // 4. Configurar los parámetros solicitados
  LoRa.setSpreadingFactor(7);           // SF 7
  LoRa.setSignalBandwidth(125E3);       // BW 125 KHz
  LoRa.setCodingRate4(5);               // CR 4/5
  LoRa.enableCrc();                     // CRC On
  
  /* Nota sobre LDR (Low Data Rate Optimize):
   * Por defecto, el chip SX1276 desactiva el LDR. La librería de Sandeep Mistry 
   * lo activa automáticamente SOLO si el SF es 11 o 12. Dado que estamos usando 
   * SF7, el LDR está implícitamente en "Off" como solicitaste.
   */

  // Mensaje de éxito inicial
  display.println("LoRa OK!");
  display.print("Freq: 924.5 MHz");
  display.display();
  Serial.println("Módulo LoRa iniciado y escuchando...");
}
void loop() {
  // LoRa.parsePacket() devuelve el tamaño del payload en bytes. 
  // Si es 0, no hay mensaje.
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    String incomingMessage = "";
    
    // Leer el paquete byte a byte
    while (LoRa.available()) {
      incomingMessage += (char)LoRa.read();
    }

    // Obtener la intensidad de la señal (RSSI)
    int rssi = LoRa.packetRssi();

    // --- Imprimir en el puerto Serial ---
    Serial.print("Recibido: ");
    Serial.println(incomingMessage);
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.print("Longitud: ");
    Serial.print(packetSize);
    Serial.println(" bytes");
    Serial.println("-----------------------");

    // --- Mostrar en la pantalla OLED ---
    display.clearDisplay();
    display.setTextSize(1);
    
    // Encabezado
    display.setCursor(0, 0);
    display.println(">> MENSAJE LORA <<");
    display.drawLine(0, 10, 128, 10, WHITE); // Línea separadora
    
    // Fila de datos técnicos (Longitud y RSSI juntos para ahorrar espacio)
    display.setCursor(0, 15);
    display.print("L:");
    display.print(packetSize);
    display.print("b");
    
    display.setCursor(64, 15); // Mitad derecha de la pantalla
    display.print("R:");
    display.print(rssi);
    display.print("dBm");

    // Separador secundario
    display.drawLine(0, 25, 128, 25, WHITE);
    
    // Mostrar el mensaje recibido
    // Activamos explícitamente el ajuste de línea por si acaso
    display.setTextWrap(true); 
    display.setCursor(0, 30);
    
    // Imprimimos el mensaje. Tiene desde el pixel Y=30 hasta el 64 para expandirse.
    display.println(incomingMessage); 
    
    // Actualizar la pantalla con los nuevos datos
    display.display();
  }
}