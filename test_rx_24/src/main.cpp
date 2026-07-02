/*
   RadioLib Receive with Interrupts Example for SX1280

   This example listens for LoRa transmissions and tries to
   receive them. Once a packet is received, an interrupt is
   triggered. To successfully receive data, the following
   settings have to be the same on both transmitter
   and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>
#include "LoRaBoards.h"

// SX1280 Configuration
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           2450.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   13
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             203.125
#endif

SX1280 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// flag to indicate that a packet was received
static volatile bool receivedFlag = false;
static String rssi = "0dBm";
static String snr = "0dB";
static String payload = "0";

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void)
{
    // we got a packet, set the flag
    receivedFlag = true;
}

void drawMain();

void setup()
{
    setupBoards();

    // When the power is turned on, a delay is required.
    delay(1500);

#ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
#endif

    // initialize radio with default settings
    int state = radio.begin();

    printResult(state == RADIOLIB_ERR_NONE);

    Serial.printf("[%s]:", RADIO_TYPE_STR);
    Serial.print(F("Radio Initializing ... "));
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    // set the function that will be called
    // when new packet is received
    radio.setPacketReceivedAction(setFlag);

    /*
    *   Sets carrier frequency.
    *   SX1278/SX1276 : Allowed values range from 137.0 MHz to 525.0 MHz.
    *   SX1268/SX1262 : Allowed values are in range from 150.0 to 960.0 MHz.
    *   SX1280        : Allowed values are in range from 2400.0 to 2500.0 MHz.
    *   LR1121        : Allowed values are in range from 150.0 to 960.0 MHz, 1900 - 2200 MHz and 2400 - 2500 MHz. Will also perform calibrations.
    * * * */

    Serial.print(F("Setting carrier frequency to:"));
    Serial.println(CONFIG_RADIO_FREQ);
    if (radio.setFrequency(CONFIG_RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        while (true);
    }

    /*
    *   Sets LoRa link bandwidth.
    *   SX1278/SX1276 : Allowed values are 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250 and 500 kHz. Only available in %LoRa mode.
    *   SX1268/SX1262 : Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
    *   SX1280        : Allowed values are 203.125, 406.25, 812.5 and 1625.0 kHz.
    *   LR1121        : Allowed values are 62.5, 125.0, 250.0 and 500.0 kHz.
    * * * */
    Serial.print(F("Setting bandwidth to:"));
    Serial.println(CONFIG_RADIO_BW);  
    if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        while (true);
    }


    /*
    * Sets LoRa link spreading factor.
    * SX1278/SX1276 :  Allowed values range from 6 to 12. Only available in LoRa mode.
    * SX1262        :  Allowed values range from 5 to 12.
    * SX1280        :  Allowed values range from 5 to 12.
    * LR1121        :  Allowed values range from 5 to 12.
    * * * */
    if (radio.setSpreadingFactor(7) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        while (true);
    }

    /*
    * Sets LoRa coding rate denominator.
    * SX1278/SX1276/SX1268/SX1262 : Allowed values range from 5 to 8. Only available in LoRa mode.
    * SX1280        :  Allowed values range from 5 to 8.
    * LR1121        :  Allowed values range from 5 to 8.
    * * * */
    if (radio.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        while (true);
    }

    /*
    * Sets LoRa sync word.
    * SX1278/SX1276/SX1268/SX1262/SX1280 : Sets LoRa sync word. Only available in LoRa mode.
    * * */
    if (radio.setSyncWord(0x00) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        while (true);
    }

    /*
    * Sets transmission output power.
    * SX1280        :  Allowed values are in range from -18 to 13 dBm.
    * * * */
    if (radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true);
    }

    /*
    * Sets preamble length for LoRa or FSK modem.
    * SX1278/SX1276 : Allowed values range from 6 to 65535 in %LoRa mode or 0 to 65535 in FSK mode.
    * SX1262/SX1268 : Allowed values range from 1 to 65535.
    * SX1280        : Allowed values range from 1 to 65535. preamble length is multiple of 4
    * LR1121        : Allowed values range from 1 to 65535.
    * * */
    if (radio.setPreambleLength(12) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        while (true);
    }

    // Enables or disables CRC check of received packets.
    if (radio.setCRC(2) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        while (true);
    }

    delay(1000);

    // start listening for LoRa packets
    Serial.print(F("Radio Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
    }

    drawMain();
}

void loop()
{
    // check if the flag is set
    if (receivedFlag) {

        // reset flag
        receivedFlag = false;

        // you can read received data as an Arduino String
        int state = radio.readData(payload);

        // you can also read received data as byte array
        /*
          byte byteArr[8];
          int state = radio.readData(byteArr, 8);
        */

        flashLed();

        if (state == RADIOLIB_ERR_NONE) {

            rssi = String(radio.getRSSI()) + "dBm";
            snr = String(radio.getSNR()) + "dB";

            drawMain();

            // packet was successfully received
            Serial.println(F("Radio Received packet!"));

            // print data of the packet
            Serial.print(F("Radio Data:\t\t"));
            Serial.println(payload);

            // print RSSI (Received Signal Strength Indicator)
            Serial.print(F("Radio RSSI:\t\t"));
            Serial.println(rssi);

            // print SNR (Signal-to-Noise Ratio)
            Serial.print(F("Radio SNR:\t\t"));
            Serial.println(snr);

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));
        } else {
            // some other error occurred
            Serial.print(F("failed, code "));
            Serial.println(state);
        }

        // put module back to listen mode
        radio.startReceive();

    }
}

void drawMain()
{
    if (disp) {
        disp->clearBuffer();
        disp->drawRFrame(0, 0, 128, 64, 5);
        disp->setFont(u8g2_font_pxplusibmvga8_mr);
        disp->setCursor(15, 20);
        disp->print("RX:");
        disp->setCursor(15, 35);
        disp->print("SNR:");
        disp->setCursor(15, 50);
        disp->print("RSSI:");

        disp->setFont(u8g2_font_crox1h_tr);
        disp->setCursor( U8G2_HOR_ALIGN_RIGHT(payload.c_str()) - 21, 20 );
        disp->print(payload);
        disp->setCursor( U8G2_HOR_ALIGN_RIGHT(snr.c_str()) - 21, 35 );
        disp->print(snr);
        disp->setCursor( U8G2_HOR_ALIGN_RIGHT(rssi.c_str()) - 21, 50 );
        disp->print(rssi);
        disp->sendBuffer();
    }
}