#include <Arduino.h>
#include <SPI.h>
// Pines ESP32 Dev Kit V1
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23
#define NSS_PIN 5
#define RESET_PIN 16
#define BUSY_PIN 4

#define CMD_GET_STATUS 0x0100
#define CMD_GET_ERRORS 0x010D
#define CMD_CALIBRATE 0x010F
#define CMD_SET_REGULATOR_MODE 0x0110
#define CMD_CALIBRATE_IMAGE 0x0111
#define CMD_SET_DIO_AS_RF_SW 0x0112
#define CMD_SET_STANDBY 0x011C
#define CMD_SET_FS 0x011D
#define CMD_SET_PACKET_TYPE 0x020E
#define CMD_SET_PA_CONFIG 0x0215
#define CMD_SET_TX_PARAMS 0x0211

// Comandos para recepcion
#define CMD_READ_BUFFER 0x010A
#define CMD_GET_RX_BUFFER_STATUS 0x0203
#define CMD_CLEAR_IRQ_STATUS 0x0117
#define CMD_SET_RX 0x0209

// Comandos para transmisión
#define CMD_SET_RF_FREQUENCY 0x020B
#define CMD_SET_MODULATION_PARAMS 0x020F
#define CMD_SET_PACKET_PARAMS 0x0210
#define CMD_WRITE_BUFFER 0x0109
#define CMD_SET_TX 0x020A

bool toggleFreq = true;

void waitBusy() {
  // rutina que usa el pin BUSY para esperar a que el chip termine de procesar comandos o esté listo para el siguiente paso
  /*uint32_t timeout = millis();
  while (digitalRead(BUSY_PIN) == HIGH) {
    if (millis() - timeout > 1000)
      return;
    yield();
  }
  delayMicroseconds(100);*/ 

  //si no se utiliza el pin BUSY, se puede usar un delay fijo
  delayMicroseconds(500);
}

void ChipStatusStat1(byte s1) {
  byte cmdStat = (s1 & 0x0E) >> 1;
  Serial.print(" CmdStatus: ");
  switch(cmdStat) {
    case 0: Serial.print("CMD_FAIL"); break;
    case 1: Serial.print("CMD_PERR (ProcError)"); break;
    case 2: Serial.print("CMD_OK"); break;
    case 3: Serial.print("CMD_DAT (DataAvailable)"); break;
    default: Serial.print("Unknown"); break;
  }
  if (s1 & 0x01) Serial.print(" [IRQ Pending]");
}

void ChipStatus(byte s2) {
  byte mode = (s2 & 0b1110) >> 1;
  Serial.print(" Mode: ");
  switch (mode) {
  case 0x00: Serial.print(F("Sleep")); break;
  case 0x01: Serial.print(F("Standby RC")); break;
  case 0x02: Serial.print(F("Standby XOSC")); break;
  case 0x03: Serial.print(F("FS")); break;
  case 0x04: Serial.print(F("Rx")); break;
  case 0x05: Serial.print(F("Tx")); break;
  case 0x06: Serial.print(F("WiFi/GNSS")); break;
  default: Serial.print(F("Unknown")); break;
  }
}

void ChipErrors(byte e1, byte e2) {
  uint16_t errors = (e1 << 8) | e2;

  Serial.print(" Errors: ");
  if (errors == 0) {
    Serial.println("None");
    return;
  }
  Serial.print("0x");
  Serial.print(errors, HEX);
  Serial.println();
  
  if (errors & (1<<0)) Serial.println("\t- cal lf rc not done");
  if (errors & (1<<1)) Serial.println("\t- cal hf rc not done");
  if (errors & (1<<2)) Serial.println("\t- cal adc was not done");
  if (errors & (1<<3)) Serial.println("\t- cal of max and min freq not done");
  if (errors & (1<<4)) Serial.println("\t- cal of image rejection not done");
  if (errors & (1<<5)) Serial.println("\t- high freq xosc did not start");
  if (errors & (1<<6)) Serial.println("\t- lo freq xosc did not start");
  if (errors & (1<<7)) Serial.println("\t- pll did not lock");
  if (errors & (1<<8)) Serial.println("\t- cal adc offset was not done");
}

void sendCommandVerified(const char *name, uint16_t opcode, byte *params, int len) {
  waitBusy();
  digitalWrite(NSS_PIN, LOW);
  SPI.transfer((opcode >> 8) & 0xFF);
  SPI.transfer(opcode & 0xFF);
  for (int i = 0; i < len; i++) {
    SPI.transfer(params[i]);
  }
  digitalWrite(NSS_PIN, HIGH);

  // Esperar a que el chip termine de procesar el comando
  waitBusy();

  // Leer Stat1 y Stat2 (Status Registers)
  digitalWrite(NSS_PIN, LOW);
  byte s1 = SPI.transfer(0x00);
  byte s2 = SPI.transfer(0x00);
  digitalWrite(NSS_PIN, HIGH);

  byte cmdStat = (s1 & 0x0E) >> 1;

  Serial.print(name);
  if (cmdStat == 0x02 || cmdStat == 0x03) {
    Serial.print(F(": OK"));
  } else {
    Serial.print(F(": ERROR"));
  }
  
  Serial.print(F(" [Stat1:0x")); Serial.print(s1, HEX);
  Serial.print(F(" Stat2:0x")); Serial.print(s2, HEX);
  Serial.println(F("]"));

  Serial.print("  ->");
  ChipStatusStat1(s1);
  ChipStatus(s2);
  Serial.println();

  // Obtener Errores internos (CMD_GET_ERRORS)
  waitBusy();
  digitalWrite(NSS_PIN, LOW);
  SPI.transfer((CMD_GET_ERRORS >> 8) & 0xFF);
  SPI.transfer(CMD_GET_ERRORS & 0xFF);
  digitalWrite(NSS_PIN, HIGH);

  waitBusy();

  // Leer 4 bytes de respuesta de GET_ERRORS: Stat1, Stat2, Err_MSB, Err_LSB
  digitalWrite(NSS_PIN, LOW);
  SPI.transfer(0x00); // Stat1
  SPI.transfer(0x00); // Stat2
  byte e1 = SPI.transfer(0x00); // Error MSB
  byte e2 = SPI.transfer(0x00); // Error LSB
  digitalWrite(NSS_PIN, HIGH);

  Serial.print("  ->");
  ChipErrors(e1, e2);
  Serial.println("----------------------------------------");
}

void setup() {
  Serial.begin(115200);
  pinMode(NSS_PIN, OUTPUT);
  pinMode(BUSY_PIN, INPUT);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(NSS_PIN, HIGH);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, NSS_PIN);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  // Reset Físico
  digitalWrite(RESET_PIN, LOW);
  delay(20);
  digitalWrite(RESET_PIN, HIGH);
  delay(400); //retardo necesario para que el chip se inicialice y pueda responder a comandos (sin usar el pin BUSY)
  waitBusy();

  Serial.println(F("\n--- INICIALIZACIÓN ESTILO RANGING DEMO ---"));
  // 2. Definir Regulador (Usamos LDO para asegurar
  // arranque)
  byte regMode[] = {0x00}; // 0x00 = LDO, 0x01 = DC-DC (recomendada para consumo óptimo, pero puede requerir ajustes en la configuración de potencia)
  sendCommandVerified("SetRegulator(LDO)", CMD_SET_REGULATOR_MODE, regMode, 1);
  //sendCommandVerified("SetFSMode", CMD_SET_FS, regMode, 0);

  // 1. Establecer Standby RC (Estado base para calibrar)
  byte stbyRC[] = {0x00}; // 0x00 = Standby RC, 0x01 = Standby XOSC
  sendCommandVerified("SetStandbyRC", CMD_SET_STANDBY, stbyRC, 1);

  // 4. Calibración de Imagen (900MHz
  // y 2.4GHz) Necesario para que el chip
  // "desbloquee" el sintetizador en estas bandas
  //byte calImg[] = {0xE1, 0xE2}; // Banda 900MHz
  //sendCommandVerified("CalImg900", CMD_CALIBRATE_IMAGE, calImg, 2);
  
    // 4. Calibración de Imagen (Banda 433MHz)
  // Fórmula de Semtech: Freq(MHz) / 4. Ej: 428MHz/4 = 0x6B, 444MHz/4 = 0x6F
  byte calImg[] = {0x6B, 0x6F}; 
  sendCommandVerified("CalImg433", CMD_CALIBRATE_IMAGE, calImg, 2);
  
  byte calImg2[] = {0xF7, 0xF8}; // Banda 2.4GHz
  sendCommandVerified("CalImg2.4", CMD_CALIBRATE_IMAGE, calImg2, 2);


  // 3. Configurar Switches de RF
  // Activa el control de switches de antena.
  // El LR1120 requiere 8 bytes: en, stby, rx, tx, tx_hp, tx_hf, gnss, wifi
  // Si usamos los bits 0x02, 0x04, 0x08 y 0x10 en las configs, el bitmask "enable" (byte 0) debe incluir esos bits (0x1E).
  byte rfSw[] = {0x1E, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00, 0x00};
  sendCommandVerified("SetDioAsRfSwitch", CMD_SET_DIO_AS_RF_SW, rfSw, 8); //comando solo funciona si el chip está en modo Standby RC.

  // 5. Definir Packet Type LoRa
  byte pType[] = {0x02}; // 0x00 = none; 0x01 = GFSK; 0x02 = LoRa; 0x03 = sigfox uplink; 0x04 = LR-FHSS; 0x05 = ranging; 0x06 = bluetooth; 0x07 = other values are RFU
  sendCommandVerified("SetPacketType", CMD_SET_PACKET_TYPE, pType, 1);

    // 2. Parámetros de Modulación (SF7, BW125, CR4/5, LDR off)
  // Valores típicos: SF=0x07, BW=0x04 (125kHz), CR=0x01 (4/5), LDRO=0x00
  byte modParams[] = {0x07, 0x04, 0x01, 0x00};
  sendCommandVerified("SetModParams", CMD_SET_MODULATION_PARAMS, modParams, 4);

    // 3. Parámetros de Paquete
  // Preamble(12), Header(0=Explicit), PayloadLen(FF), CRC(1=On), InvertIQ(0)
  byte pktParams[] = {0x00, 0x08, 0x00, 0xFF, 0x01, 0x00};
  sendCommandVerified("SetPktParams", CMD_SET_PACKET_PARAMS, pktParams, 6);

    // 1. Configurar Frecuencia (915 MHz = 915,000,000 Hz)
  //uint32_t freqHz = 915000000;
  uint32_t freqHz = 433000000;
  byte rfFreq[] = {
    (byte)((freqHz >> 24) & 0xFF),
    (byte)((freqHz >> 16) & 0xFF),
    (byte)((freqHz >> 8) & 0xFF),
    (byte)(freqHz & 0xFF)
  };
  sendCommandVerified("SetRfFreq(915MHz)", CMD_SET_RF_FREQUENCY, rfFreq, 4);

  // 6. Calibración General
  // | Bits | (7:6) | 5    | 4   | 3 | 2  | 1    | 0    |
  // | Name | RFU   |PLL_TX| IMG |ADC| PLL| HF_RC| LF_RC|
  byte calAll[] = {0x3F};
  sendCommandVerified("CalibrateAll", CMD_CALIBRATE, calAll, 1);
  delay(50);

  // 7. Activar Cristal 32MHz (Standby XOSC)
  byte stbyXosc[] = {0x01};
  sendCommandVerified("SetStandbyXOSC", CMD_SET_STANDBY, stbyXosc, 1);

  // 8. Configuración de Potencia (PA LP LF para 14dBm)
  // PA_LP requiere regPaSupply = 0 (regulator interno), paDutyCycle = 4, paHpSel = 7
  //byte paCfg[] = {0x00, 0x00, 0x04, 0x07};
  //sendCommandVerified("SetPaConfig", CMD_SET_PA_CONFIG, paCfg, 4);

  //byte txParams[] = {0x00, 0x04}; // -17dBm (0xEF) y ramp time 48us (0x04)
  //sendCommandVerified("SetTxParams", CMD_SET_TX_PARAMS, txParams, 2);
  //while(1);

  byte clrIrqRxDone[] = {0xFF,0xFF,0xFF,0xFF}; // Limpiar solo IRQ de RxDone
  sendCommandVerified("ClearIRQ", CMD_CLEAR_IRQ_STATUS, clrIrqRxDone, 4);

  byte rxTimeout[] = {0x00, 0x00, 0x00}; //disable timeout
  sendCommandVerified("SetRx", CMD_SET_RX, rxTimeout, 3);

}

void loop() {
  waitBusy();
  digitalWrite(NSS_PIN, LOW);
  SPI.transfer((CMD_GET_STATUS >> 8) & 0xFF);
  SPI.transfer(CMD_GET_STATUS & 0xFF);
  //SPI.transfer(0x00); // Byte Dummy de Status requerido por el protocolo
  byte irq3 = SPI.transfer(0x00);
  byte irq2 = SPI.transfer(0x00);
  byte irq1 = SPI.transfer(0x00);
  byte irq0 = SPI.transfer(0x00);
  digitalWrite(NSS_PIN, HIGH);

Serial.print("IRQ0' Status: 0x");
Serial.println(irq0, HEX);

  waitBusy();
  uint32_t irqStatus = ((uint32_t)irq3 << 24) | ((uint32_t)irq2 << 16) | ((uint32_t)irq1 << 8) | irq0;

  Serial.print("IRQstatus' Status: 0x");
Serial.println(irqStatus, HEX);

  if (irqStatus & 0x08) { // RXDone interrupt
    Serial.println("\n========================================");
    Serial.println("[!] ¡PAQUETE LORA CAPTURADO EN AIRE!");
    if (irqStatus & 0x80) {
      Serial.println("-> [ERROR]: Falla de CRC. Los datos se corrompieron en el trayecto.");
    } else {
      // Extraer propiedades del paquete del módem (GetRxBufferStatus)
      waitBusy();
      digitalWrite(NSS_PIN, LOW);
      SPI.transfer((CMD_GET_RX_BUFFER_STATUS >> 8) & 0xFF);
      SPI.transfer(CMD_GET_RX_BUFFER_STATUS & 0xFF);
      SPI.transfer(0x00); // Byte Dummy de lectura
      byte payloadLength = SPI.transfer(0x00);
      byte bufferStartPointer = SPI.transfer(0x00);
      digitalWrite(NSS_PIN, HIGH);

      Serial.print("-> Tamaño detectado: "); Serial.print(payloadLength); Serial.println(" bytes.");
      Serial.print("-> offset bufferRX: "); Serial.print(bufferStartPointer); Serial.println(" bytes.");
      // Extraer datos desde la memoria de la radio (ReadBuffer8)
      waitBusy();
      digitalWrite(NSS_PIN, LOW);
      SPI.transfer((CMD_READ_BUFFER >> 8) & 0xFF);
      SPI.transfer(CMD_READ_BUFFER & 0xFF);
      SPI.transfer(bufferStartPointer); // Offset inicial de lectura en la RAM del chip
      SPI.transfer(payloadLength);               // Byte Dummy mandatorio para sincronizar bus de datos

      char rxBuffer[256];
      //memset(rxBuffer, 0, sizeof(rxBuffer));
      
      SPI.transfer(0x00); // Dummy read requerido por el protocolo antes de leer datos
      for (int i = 0; i < payloadLength; i++) {
        rxBuffer[i] = (char)SPI.transfer(0x00);
      }
      digitalWrite(NSS_PIN, HIGH);
      waitBusy();
      // Despliegue limpio del payload en consola
      Serial.print("-> Mensaje decodificado: ");
      Serial.println(rxBuffer);
    }

    //delay(5000);
    // Limpieza selectiva de flags procesadas
    byte clrIrq[] = {0xFF, 0xFF, 0xFF, 0xFF};
    sendCommandVerified("ClearIrqDone", CMD_CLEAR_IRQ_STATUS, clrIrq, 4);

    // Forzar re-entrada al bucle de escucha continuo
    byte rxTimeout[] = {0xFF, 0xFF, 0xFF};
    sendCommandVerified("Re-enclavar RX", CMD_SET_RX, rxTimeout, 3);
    Serial.println("========================================");
  }

delay(500);

}