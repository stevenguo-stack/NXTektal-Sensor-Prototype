#include <Arduino.h>
#include <Wire.h>
#include <vl53l4cx_class.h>

#define SDA_PIN 45
#define SCL_PIN 46
#define XSHUT_PIN 44

VL53L4CX sensor(&Wire, XSHUT_PIN);

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("START");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Wire.setBufferSize(512);
  Wire.setTimeOut(1000);

  Serial.println("I2C OK");

  sensor.begin();
  Serial.println("sensor.begin OK");

  sensor.VL53L4CX_Off();
  delay(100);

  int status = sensor.InitSensor(0x12);
  Serial.print("InitSensor = ");
  Serial.println(status);

  status = sensor.VL53L4CX_StartMeasurement();
  Serial.print("StartMeasurement = ");
  Serial.println(status);
}

void loop() {
  uint8_t ready = 0;

  int status = sensor.VL53L4CX_GetMeasurementDataReady(&ready);

  Serial.print("ready status=");
  Serial.print(status);
  Serial.print(" ready=");
  Serial.println(ready);

  if (status != 0) {
    delay(500);
    return;
  }

  if (ready) {
    VL53L4CX_MultiRangingData_t data;

    status = sensor.VL53L4CX_GetMultiRangingData(&data);

    Serial.print("read status=");
    Serial.println(status);

    if (status == 0) {
      Serial.print("objects=");
      Serial.println(data.NumberOfObjectsFound);

      for (int i = 0; i < data.NumberOfObjectsFound; i++) {
        Serial.print("distance=");
        Serial.print(data.RangeData[i].RangeMilliMeter);
        Serial.print(" mm status=");
        Serial.println(data.RangeData[i].RangeStatus);
      }
    }

    status = sensor.VL53L4CX_ClearInterruptAndStartMeasurement();

    Serial.print("restart status=");
    Serial.println(status);
  }

  delay(200);
}