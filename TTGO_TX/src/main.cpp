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
#define OLED_RST -1 // En la v2.0 generalmente no se usa el pin RST
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Inicializar el objeto de la pantalla
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

int msgCount = 0; // Contador de mensajes

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
  display.println("Iniciando TTGO TX...");
  display.display();

  // 2. Configurar pines del bus SPI para el módulo LoRa
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);

  // 3. Inicializar LoRa a 924.5 MHz (Misma frecuencia que el RX)
  if (!LoRa.begin(924.5E6)) {
    Serial.println("¡Fallo al iniciar el modulo LoRa!");
    display.println("Fallo Radio LoRa!");
    display.display();
    while (1);
  }

  // 4. Configurar los parámetros idénticos al RX
  LoRa.setSpreadingFactor(7);           // SF 7
  LoRa.setSignalBandwidth(125E3);       // BW 125 KHz
  LoRa.setCodingRate4(5);               // CR 4/5
  LoRa.enableCrc();                     // CRC On

  // Mensaje de éxito inicial
  display.println("LoRa TX OK!");
  display.print("Freq: 924.5 MHz");
  display.display();
  Serial.println("Módulo LoRa TX iniciado. Listo para enviar.");
  
  delay(2000); // Pausa breve para leer el mensaje de inicio
}

void loop() {
  // Construir el mensaje con el contador
  String outgoingMessage = "hola desde TTGO 915! " + String(msgCount);

  // --- Imprimir en el puerto Serial ---
  Serial.print("Enviando: ");
  Serial.println(outgoingMessage);

  // --- Transmitir por LoRa ---
  LoRa.beginPacket();
  LoRa.print(outgoingMessage);
  LoRa.endPacket();

  // --- Mostrar en la pantalla OLED ---
  display.clearDisplay();
  display.setTextSize(1);
  
  // Encabezado
  display.setCursor(0, 0);
  display.println(">> ENVIANDO LORA <<");
  display.drawLine(0, 10, 128, 10, WHITE); // Línea separadora
  
  // Información de estado
  display.setCursor(0, 15);
  display.print("Msg Num: ");
  display.print(msgCount);

  // Separador secundario
  display.drawLine(0, 25, 128, 25, WHITE);
  
  // Mostrar el mensaje enviado
  display.setTextWrap(true); 
  display.setCursor(0, 30);
  display.println(outgoingMessage); 
  
  // Actualizar la pantalla
  display.display();

  // Incrementar contador y esperar 5 segundos
  msgCount++;
  delay(5000);
}