# NXTektal Sensor Prototype

Working source for NXTektal's golf ball-bin ToF distance sensor and SX1262 LoRa link. The original standalone sensor sketch is preserved as [sketch_aug15b.ino](sketch_aug15b.ino).

## Latest firmware: v5 (October 2026)

- **TX (transmitter):** VL53L4CX distance sampling on a Heltec Wireless Tracker, validates the sensor reading and sends a JSON LoRa packet with `box_id`, `distance_mm`, `range_status`, `level`, and `seq` approximately every 3 seconds during bench testing.
- **RX (receiver):** Heltec Wireless Tracker listens for packets, reports RSSI/SNR over USB serial, and displays distance, ball level and link status.
- **Display:** official NXTektal NX mark and green italic NXTektal wordmark in a non-blocking five-second startup animation, then TX/RX status screens; `LOW` is yellow on both screens, `OK` green, errors red.
- The radio parameters currently match on TX/RX: 915 MHz, 125 kHz bandwidth, spreading factor 7 and coding rate 4/5.
- **Field validation:** The earlier integrated sensor/LoRa firmware successfully demonstrated live distance changes sent from TX to RX in a desktop test. This v5 display revision still requires final on-device Verify/Upload and physical regression testing.

## Sketch layout

```text
firmware/
  NXTektal_TX_Display/
    NXTektal_TX_Display.ino
    NXTektal_Display.h
  NXTektal_RX_Display/
    NXTektal_RX_Display.ino
    NXTektal_Display.h
```

Keep each `.ino` in the same folder as its matching `NXTektal_Display.h`. In Arduino IDE, select **Heltec Wireless Tracker** (first-generation option) and the actual COM port for the connected board.

**Dependencies:** Heltec ESP32 Arduino board package, Heltec ESP32 Dev-Boards library (`LoRaWan_APP.h`, `HT_st7735.h`), and STM32duino VL53L4CX library for the transmitter.

**Bench wiring (existing prototype, check actual board pin labeling):** SDA GPIO45, SCL GPIO46, XSHUT GPIO44. The ESP32-S3 has special-purpose boot-related pins, so do not assume this wiring generalizes to other boards.

## Testing and deployment limits

- `LOW_BALL_THRESHOLD_MM = 50` is a **demonstration distance threshold**, not a calibrated 20% fill-level rule; use real empty/half/full bin measurements before making operational decisions.
- 3-second packets are for bench testing. An operational 2–3 minute interval, filtering, calibration, packet-delivery checks and gateway integration are still pending.
- A standalone LoRa receiver only prints data to its serial connection. Sending data onward to Insite/Agent or Dashboard needs a gateway bridge (e.g. serial-to-HTTP/MQTT).
- Current 915 MHz RF settings are for the US test hardware; recheck RF-band rules and hardware before deployments elsewhere.
- Attach the LoRa antenna before transmitting. TX and RX boards can run from independent suitable 5 V USB supplies.
- The branded screen images are encoded in the shared header; no image files are required for flashing.

## Handoff

For company integration, preserve the same two sketch directories and the shared display header. Update the upstream system only after interface ownership, wire format and integration tests are confirmed.
