# MHI XY Interface (ESPHome)

ESPHome external component for Mitsubishi Heavy Industries **X-Y bus** monitoring on a Wemos D1 Mini with a Homebus-to-TTL adapter on RX/TX.

Protocol decoding follows the P1P2MQTT `MHI_SERIES` mapping and the 3-byte-to-1-byte encoding described by HamdiOlgun / [P1P2Serial MHI branch](https://github.com/Arnold-n/P1P2Serial/blob/MHI/README.md).

## Hardware

- Wemos D1 Mini (ESP8266)
- Homebus / HBS transceiver (MAX22088, MM1192/XL1192, or similar) wired to:
  - **TX** → D1 Mini `GPIO1` (TX)
  - **RX** → D1 Mini `GPIO3` (RX)
- UART settings: **9600 8E1**
- Logger serial is disabled (`baud_rate: 0`) so hardware UART0 can be used for the bus

## Install on Home Assistant ESPHome

Use the published component from GitHub:

```yaml
external_components:
  - source: github://rwijbenga/mhi-xy-esphome@main
    components: [mhi_xy]
```

Or copy `components/mhi_xy/` into `/config/esphome/components/mhi_xy/` and switch the YAML to a local `external_components` path.

Ensure `secrets.yaml` has `wifi_ssid` / `wifi_password`, then compile and flash the D1 Mini (USB first, then OTA).

## Entities

Listen-only by default (`allow_control: false`):

- Climate (mode, setpoint, fan, swing, indoor temp)
- Power / Swing binary sensors
- Setpoint, indoor temperature, fan speed, vane
- Diagnostics: packet count, checksum errors, last raw / decoded packet hex

## Control (experimental)

Set `allow_control: true` only after listen-only decoding looks correct. Writing to the XY bus is reverse-engineered and can disturb the wired remote / indoor unit. Use at your own risk.

## Notes

- Official P1P2MQTT hardware uses an ATmega328 + MAX22088 for bit-accurate HBS timing. This project assumes your adapter already presents cleaned UART bytes (as with a Homebus-to-TTL converter).
- If you see raw frames but checksum failures, check parity (EVEN), wiring (TX/RX swap), and whether your adapter already performs the 3-to-1 decode.
- Your existing split-unit ESPHome devices (`MhiAcCtrl` / SPI CNS) are a **different** interface from this XY / Superlink bus.
