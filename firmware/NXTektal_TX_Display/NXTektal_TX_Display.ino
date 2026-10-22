#include <Arduino.h>
#include <Wire.h>
#include <vl53l4cx_class.h>
#include "LoRaWan_APP.h"
#include "NXTektal_Display.h"

// NXTektal TX: VL53L4CX sensor + SX1262 LoRa + 5s logo animation
// Original, field-tested sensor wiring from sketch_aug15b.ino
#define SDA_PIN 45
#define SCL_PIN 46
#define XSHUT_PIN 44
#define LOW_BALL_THRESHOLD_MM 50   // Experimental only; NOT calibrated % fill

// SAME radio settings on transmitter and receiver.
#define RF_FREQUENCY 915000000
#define TX_OUTPUT_POWER 5
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

const uint32_t SEND_INTERVAL_MS = 3000;
const uint32_t SENSOR_POLL_MS = 200;
const uint32_t RANGE_STALE_MS = 10000;
const uint32_t DISPLAY_REFRESH_MS = 750;

VL53L4CX sensor(&Wire, XSHUT_PIN);
static RadioEvents_t RadioEvents;
bool sensorReady = false;
bool txReady = true;
int lastDistanceMM = -1, lastRangeStatus = -1;
uint32_t lastMeasureMs = 0, lastPollMs = 0, lastSendMs = 0;
uint32_t lastUiMs = 0;
unsigned long packetNumber = 0;
char txpacket[160];

void OnTxDone() {
  Serial.println("LoRa TX done");
  txReady = true;
}
void OnTxTimeout() {
  Radio.Sleep();
  Serial.println("LoRa TX timeout");
  txReady = true;
}

void initLoRa() {
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
  Serial.println("LoRa TX initialized");
}

void initSensor() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Wire.setBufferSize(512);
  Wire.setTimeOut(1000);
  sensor.begin();
  sensor.VL53L4CX_Off();
  delay(100);
  int status = sensor.InitSensor(0x12);
  Serial.printf("InitSensor = %d\n", status);
  if (status != 0) {
    sensorReady = false;
    return;
  }
  status = sensor.VL53L4CX_StartMeasurement();
  Serial.printf("StartMeasurement = %d\n", status);
  sensorReady = (status == 0);
}

void updateSensor() {
  if (!sensorReady) return;
  uint8_t ready = 0;
  int code = sensor.VL53L4CX_GetMeasurementDataReady(&ready);
  if (code != 0 || !ready) return;

  VL53L4CX_MultiRangingData_t data;
  code = sensor.VL53L4CX_GetMultiRangingData(&data);
  lastDistanceMM = -1;
  lastRangeStatus = -1;
  lastMeasureMs = millis();
  if (code == 0) {
    for (int i = 0; i < data.NumberOfObjectsFound; ++i) {
      int mm = data.RangeData[i].RangeMilliMeter;
      int rangeStatus = data.RangeData[i].RangeStatus;
      if (i == 0) lastRangeStatus = rangeStatus;
      if (rangeStatus == 0 && mm > 0 && lastDistanceMM < 0) {
        lastDistanceMM = mm;
        lastRangeStatus = 0;
      }
    }
    if (lastDistanceMM > 0) {
      Serial.printf("Selected distance: %d mm\n", lastDistanceMM);
      Serial.println(lastDistanceMM >= LOW_BALL_THRESHOLD_MM ?
                     ">>> WARNING: LOW BALL LEVEL <<<" : "Ball level OK");
    } else {
      Serial.println("No valid distance");
    }
  } else {
    Serial.printf("Sensor read error: %d\n", code);
  }
  code = sensor.VL53L4CX_ClearInterruptAndStartMeasurement();
  if (code != 0) Serial.printf("Restart error: %d\n", code);
}

const char *currentLevel(bool &valid) {
  bool fresh = lastMeasureMs != 0 &&
               (millis() - lastMeasureMs <= RANGE_STALE_MS);
  valid = sensorReady && fresh && lastDistanceMM > 0 && lastRangeStatus == 0;
  if (!sensorReady) return "SENSOR_ERROR";
  if (!valid) return "NO_VALID_RANGE";
  return lastDistanceMM >= LOW_BALL_THRESHOLD_MM ? "LOW" : "OK";
}

void sendData() {
  bool valid = false;
  const char *level = currentLevel(valid);
  ++packetNumber;
  snprintf(txpacket, sizeof(txpacket),
           "{\"box_id\":\"A01\",\"distance_mm\":%d,"
           "\"range_status\":%d,\"level\":\"%s\",\"seq\":%lu}",
           valid ? lastDistanceMM : -1,
           valid ? lastRangeStatus : -1,
           level, packetNumber);
  Serial.print("LoRa Sending: ");
  Serial.println(txpacket);
  txReady = false;
  Radio.Send((uint8_t *)txpacket, strlen(txpacket));
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  NXTUI::begin();
  initLoRa();
  initSensor();
  Serial.println("NXTektal TX system started");
}

void loop() {
  // CRITICAL: radio and sensor continue working during the 5s splash.
  Radio.IrqProcess();
  uint32_t now = millis();
  if (now - lastPollMs >= SENSOR_POLL_MS) {
    lastPollMs = now;
    updateSensor();
  }
  if (txReady && now - lastSendMs >= SEND_INTERVAL_MS) {
    lastSendMs = now;
    sendData();
  }
  if (NXTUI::tick() && now - lastUiMs >= DISPLAY_REFRESH_MS) {
    lastUiMs = now;
    bool valid = false;
    const char *level = currentLevel(valid);
    NXTUI::txStatus(valid ? lastDistanceMM : -1,
                    level, packetNumber, sensorReady);
  }
}
