#include <Arduino.h>
#include "LoRaWan_APP.h"
#include "NXTektal_Display.h"

// NXTektal RX: SX1262 LoRa receiver + 5s logo animation
#define RF_FREQUENCY 915000000
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

const uint32_t DISPLAY_REFRESH_MS = 750;
const uint32_t LINK_STALE_MS = 12000;
static RadioEvents_t RadioEvents;
bool startRx = true;
char rxpacket[192];
char latestLevel[20] = "WAITING";
int latestDistanceMM = -1, latestRSSI = 0, latestSNR = 0;
unsigned long latestSeq = 0;
uint32_t lastPacketMs = 0, lastUiMs = 0;

void parsePacket(const char *str) {
  const char *distanceField = strstr(str, "\"distance_mm\":");
  if (distanceField) {
    latestDistanceMM = atoi(distanceField + strlen("\"distance_mm\":"));
  }
  const char *seqField = strstr(str, "\"seq\":");
  if (seqField) {
    latestSeq = strtoul(seqField + strlen("\"seq\":"), nullptr, 10);
  }
  const char *levelField = strstr(str, "\"level\":\"");
  if (levelField) {
    levelField += strlen("\"level\":\"");
    size_t j = 0;
    while (levelField[j] && levelField[j] != '"' &&
           j < sizeof(latestLevel) - 1) {
      latestLevel[j] = levelField[j];
      ++j;
    }
    latestLevel[j] = '\0';
  }
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  size_t n = size < sizeof(rxpacket) ? size : sizeof(rxpacket) - 1;
  memcpy(rxpacket, payload, n);
  rxpacket[n] = '\0';
  latestRSSI = rssi;
  latestSNR = snr;
  lastPacketMs = millis();
  parsePacket(rxpacket);
  Serial.println("---- PACKET RECEIVED ----");
  Serial.print("Data: "); Serial.println(rxpacket);
  Serial.printf("RSSI: %d dBm\nSNR: %d dB\n", rssi, snr);
  Radio.Sleep();
  startRx = true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  NXTUI::begin();
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  Serial.println("NXTektal Receiver READY");
}

void loop() {
  if (startRx) {
    startRx = false;
    Radio.Rx(0);
  }
  Radio.IrqProcess();
  uint32_t now = millis();
  if (NXTUI::tick() && now - lastUiMs >= DISPLAY_REFRESH_MS) {
    lastUiMs = now;
    bool linkGood = lastPacketMs != 0 &&
                    (now - lastPacketMs < LINK_STALE_MS);
    NXTUI::rxStatus(latestDistanceMM, latestLevel,
                    latestRSSI, latestSNR, latestSeq, linkGood);
  }
}
