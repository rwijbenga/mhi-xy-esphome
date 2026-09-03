#include "mhi_xy.h"
#include "esphome/core/log.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace esphome {
namespace mhi_xy {

static const char *const TAG = "mhi_xy";

void MhiXyComponent::setup() {
  this->rx_len_ = 0;
  this->receiving_ = false;
  this->last_byte_ms_ = millis();
}

void MhiXyComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MHI XY bus:");
  ESP_LOGCONFIG(TAG, "  Packet timeout: %u ms", this->packet_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Allow control: %s", YESNO(this->allow_control_));
}

void MhiXyComponent::loop() {
  while (this->available()) {
    uint8_t byte = 0;
    if (!this->read_byte(&byte)) {
      break;
    }
    if (this->rx_len_ < RAW_MAX) {
      this->rx_buf_[this->rx_len_++] = byte;
    }
    this->last_byte_ms_ = millis();
    this->receiving_ = true;
  }

  if (this->receiving_ && this->rx_len_ > 0 && (millis() - this->last_byte_ms_) >= this->packet_timeout_ms_) {
    this->process_raw_(this->rx_buf_, this->rx_len_);
    this->rx_len_ = 0;
    this->receiving_ = false;
  }
}

int MhiXyComponent::symbol_value_(uint8_t encoded) {
  // Each encoded UART byte has a single 0-bit; its position (0-7) is the 3-bit value.
  for (int i = 0; i < 8; i++) {
    if ((encoded & (1 << i)) == 0) {
      uint8_t inverted = static_cast<uint8_t>(~encoded);
      if ((inverted & (inverted - 1)) == 0) {
        return i;
      }
      return -1;
    }
  }
  return -1;
}

bool MhiXyComponent::looks_encoded_(const uint8_t *data, size_t len) {
  if (len < 3) {
    return false;
  }
  size_t ok = 0;
  for (size_t i = 0; i < len; i++) {
    if (symbol_value_(data[i]) >= 0) {
      ok++;
    }
  }
  return ok * 4 >= len * 3;  // >= 75%
}

bool MhiXyComponent::decode_3to1_(const uint8_t *raw, size_t raw_len, uint8_t *out, size_t *out_len) {
  if (raw_len < 3 || (raw_len % 3) != 0) {
    return false;
  }
  size_t n = raw_len / 3;
  if (n > DECODED_LEN) {
    n = DECODED_LEN;
  }
  for (size_t i = 0; i < n; i++) {
    int v0 = symbol_value_(raw[i * 3]);
    int v1 = symbol_value_(raw[i * 3 + 1]);
    int v2 = symbol_value_(raw[i * 3 + 2]);
    if (v0 < 0 || v1 < 0 || v2 < 0) {
      return false;
    }
    // 9 bits, LSB first; bit 8 is always 0 and discarded.
    out[i] = static_cast<uint8_t>((v0 & 0x07) | ((v1 & 0x07) << 3) | ((v2 & 0x03) << 6));
  }
  *out_len = n;
  return true;
}

uint8_t MhiXyComponent::checksum_(const uint8_t *decoded, size_t len) {
  uint16_t sum = 0;
  size_t n = len > 0 ? len - 1 : 0;
  for (size_t i = 0; i < n; i++) {
    sum += decoded[i];
  }
  return static_cast<uint8_t>(sum & 0xFF);
}

void MhiXyComponent::publish_hex_(text_sensor::TextSensor *sensor, const uint8_t *data, size_t len) {
  if (sensor == nullptr || data == nullptr || len == 0) {
    return;
  }
  char buf[3 * RAW_MAX + 1];
  size_t pos = 0;
  for (size_t i = 0; i < len && pos + 3 < sizeof(buf); i++) {
    pos += snprintf(buf + pos, sizeof(buf) - pos, i ? " %02X" : "%02X", data[i]);
  }
  buf[pos] = '\0';
  sensor->publish_state(buf);
}

void MhiXyComponent::process_raw_(const uint8_t *data, size_t len) {
  if (len == 0) {
    return;
  }
  this->publish_hex_(this->last_raw_text_sensor_, data, len);

  uint8_t decoded[DECODED_LEN];
  size_t decoded_len = 0;
  bool decoded_ok = false;

  if (looks_encoded_(data, len)) {
    decoded_ok = this->decode_3to1_(data, len, decoded, &decoded_len);
  } else if (len == DECODED_LEN || len == 15 || len == 29) {
    decoded_len = len > DECODED_LEN ? DECODED_LEN : len;
    memcpy(decoded, data, decoded_len);
    decoded_ok = true;
  } else if (len > 16 && (len % 3) == 0) {
    decoded_ok = this->decode_3to1_(data, len, decoded, &decoded_len);
  }

  if (!decoded_ok || decoded_len < 8) {
    ESP_LOGD(TAG, "Ignored UART frame (%u bytes, encoded=%s)", (unsigned) len, YESNO(looks_encoded_(data, len)));
    return;
  }

  this->publish_hex_(this->last_packet_text_sensor_, decoded, decoded_len);

  bool crc_ok = true;
  if (decoded_len >= 2) {
    crc_ok = this->checksum_(decoded, decoded_len) == decoded[decoded_len - 1];
  }
  if (this->checksum_ok_binary_sensor_ != nullptr) {
    this->checksum_ok_binary_sensor_->publish_state(crc_ok);
  }
  if (!crc_ok) {
    this->checksum_errors_++;
    if (this->checksum_errors_sensor_ != nullptr) {
      this->checksum_errors_sensor_->publish_state(this->checksum_errors_);
    }
    ESP_LOGW(TAG, "Checksum mismatch (got 0x%02X expected 0x%02X)", decoded[decoded_len - 1],
             this->checksum_(decoded, decoded_len));
    return;
  }

  this->packet_count_++;
  if (this->packet_count_sensor_ != nullptr) {
    this->packet_count_sensor_->publish_state(this->packet_count_);
  }
  this->parse_decoded_(decoded, decoded_len);
}

void MhiXyComponent::parse_decoded_(const uint8_t *decoded, size_t len) {
  if (len < 8) {
    return;
  }

  memcpy(this->last_decoded_, decoded, len < DECODED_LEN ? len : DECODED_LEN);
  this->have_decoded_ = true;

  const uint8_t src = decoded[0];
  const char *source = "unknown";
  if (src <= 0x01) {
    source = "indoor";
  } else if (src >= 0x02 && src <= 0x07) {
    source = "remote";
  } else if (src == 0x80 || src == 0x81) {
    source = "outdoor";
  }
  if (this->source_text_sensor_ != nullptr) {
    this->source_text_sensor_->publish_state(source);
  }

  // Decoded layout after 3-to-1 conversion (P1P2MQTT MHI_SERIES):
  // [0] address / packet source
  // [1] status
  // [2] power (bit0), mode (bits 2-4), swing (bit6)
  // [3] fan (bits 0-2), vane (bits 3-4)
  // [4] setpoint = (byte - 0x80) * 0.5 °C
  // [5] indoor temp = (byte - 0x3D) * 0.25 °C
  // [8] RC mode
  const uint8_t mode_byte = decoded[2];
  const uint8_t fan_byte = decoded[3];
  const bool power = (mode_byte & 0x01) != 0;
  const bool swing = (mode_byte & 0x40) != 0;
  const uint8_t mode_bits = (mode_byte >> 2) & 0x07;
  const uint8_t fan_level = (fan_byte & 0x07) + 1;
  const uint8_t vane = ((fan_byte & 0x30) >> 4) + 1;
  const float setpoint = (decoded[4] - 0x80) * 0.5f;
  const float indoor = (decoded[5] - 0x3D) * 0.25f;

  climate::ClimateMode climate_mode = climate::CLIMATE_MODE_OFF;
  if (power) {
    switch (mode_bits) {
      case 0:
        climate_mode = climate::CLIMATE_MODE_AUTO;
        break;
      case 1:
        climate_mode = climate::CLIMATE_MODE_DRY;
        break;
      case 2:
        climate_mode = climate::CLIMATE_MODE_COOL;
        break;
      case 3:
        climate_mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case 4:
        climate_mode = climate::CLIMATE_MODE_HEAT;
        break;
      default:
        climate_mode = climate::CLIMATE_MODE_AUTO;
        break;
    }
  }

  climate::ClimateFanMode fan_mode = climate::CLIMATE_FAN_AUTO;
  switch (fan_level) {
    case 1:
      fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case 2:
      fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case 3:
      fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
    default:
      fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
  }

  auto swing_mode = swing ? climate::CLIMATE_SWING_VERTICAL : climate::CLIMATE_SWING_OFF;

  if (this->power_binary_sensor_ != nullptr) {
    this->power_binary_sensor_->publish_state(power);
  }
  if (this->swing_binary_sensor_ != nullptr) {
    this->swing_binary_sensor_->publish_state(swing);
  }
  if (this->setpoint_sensor_ != nullptr && setpoint >= 10.0f && setpoint <= 35.0f) {
    this->setpoint_sensor_->publish_state(setpoint);
  }
  if (this->indoor_temperature_sensor_ != nullptr && indoor >= -10.0f && indoor <= 50.0f) {
    this->indoor_temperature_sensor_->publish_state(indoor);
  }
  if (this->fan_speed_sensor_ != nullptr) {
    this->fan_speed_sensor_->publish_state(fan_level);
  }
  if (this->vane_sensor_ != nullptr) {
    this->vane_sensor_->publish_state(vane);
  }
  if (this->rc_mode_text_sensor_ != nullptr && len > 8) {
    const uint8_t rc = decoded[8];
    const char *rc_mode = "unknown";
    if (rc == 0x80) {
      rc_mode = "standard";
    } else if (rc == 0x40) {
      rc_mode = "indoor data";
    } else if (rc == 0xC0) {
      rc_mode = "outdoor data";
    } else {
      static char rc_buf[8];
      snprintf(rc_buf, sizeof(rc_buf), "0x%02X", rc);
      rc_mode = rc_buf;
    }
    this->rc_mode_text_sensor_->publish_state(rc_mode);
  }

  if (this->climate_ != nullptr) {
    this->climate_->publish_from_bus(power, climate_mode, setpoint, indoor, fan_mode, swing_mode);
  }

  ESP_LOGD(TAG, "src=0x%02X %s power=%d mode=%u set=%.1f indoor=%.2f fan=%u vane=%u swing=%d", src, source, power,
           mode_bits, setpoint, indoor, fan_level, vane, swing);
}

bool MhiXyComponent::encode_and_write_(const uint8_t *decoded) {
  uint8_t packet[DECODED_LEN];
  memcpy(packet, decoded, DECODED_LEN);
  packet[DECODED_LEN - 1] = this->checksum_(packet, DECODED_LEN);

  uint8_t raw[DECODED_LEN * 3];
  for (size_t i = 0; i < DECODED_LEN; i++) {
    uint8_t b = packet[i];
    raw[i * 3] = static_cast<uint8_t>(~(1 << (b & 0x07)));
    b >>= 3;
    raw[i * 3 + 1] = static_cast<uint8_t>(~(1 << (b & 0x07)));
    b >>= 3;
    raw[i * 3 + 2] = static_cast<uint8_t>(~(1 << (b & 0x07)));
  }
  this->write_array(raw, sizeof(raw));
  return true;
}

bool MhiXyComponent::apply_climate_call(const climate::ClimateCall &call) {
  if (!this->allow_control_) {
    ESP_LOGW(TAG, "Climate control is disabled (listen-only). Set allow_control: true to write to the XY bus.");
    return false;
  }
  if (!this->have_decoded_) {
    ESP_LOGW(TAG, "No XY packet seen yet; cannot write.");
    return false;
  }

  uint8_t packet[DECODED_LEN];
  memcpy(packet, this->last_decoded_, DECODED_LEN);

  uint8_t mode_byte = packet[2];
  uint8_t fan_byte = packet[3];

  if (call.get_mode().has_value()) {
    auto mode = *call.get_mode();
    if (mode == climate::CLIMATE_MODE_OFF) {
      mode_byte &= ~0x01;
    } else {
      mode_byte |= 0x01;
      uint8_t bits = 0;
      switch (mode) {
        case climate::CLIMATE_MODE_AUTO:
          bits = 0;
          break;
        case climate::CLIMATE_MODE_DRY:
          bits = 1;
          break;
        case climate::CLIMATE_MODE_COOL:
          bits = 2;
          break;
        case climate::CLIMATE_MODE_FAN_ONLY:
          bits = 3;
          break;
        case climate::CLIMATE_MODE_HEAT:
          bits = 4;
          break;
        default:
          break;
      }
      mode_byte = static_cast<uint8_t>((mode_byte & ~0x1C) | ((bits & 0x07) << 2));
    }
  }

  if (call.get_swing_mode().has_value()) {
    if (*call.get_swing_mode() == climate::CLIMATE_SWING_OFF) {
      mode_byte &= ~0x40;
    } else {
      mode_byte |= 0x40;
    }
  }

  if (call.get_fan_mode().has_value()) {
    uint8_t level = 3;  // auto-ish
    switch (*call.get_fan_mode()) {
      case climate::CLIMATE_FAN_LOW:
        level = 0;
        break;
      case climate::CLIMATE_FAN_MEDIUM:
        level = 1;
        break;
      case climate::CLIMATE_FAN_HIGH:
        level = 2;
        break;
      default:
        level = 3;
        break;
    }
    fan_byte = static_cast<uint8_t>((fan_byte & ~0x07) | (level & 0x07));
  }

  if (call.get_target_temperature().has_value()) {
    float t = clamp(*call.get_target_temperature(), 16.0f, 30.0f);
    packet[4] = static_cast<uint8_t>(0x80 + static_cast<int>(roundf(t * 2.0f)));
  }

  packet[2] = mode_byte;
  packet[3] = fan_byte;
  ESP_LOGI(TAG, "Writing XY control packet");
  return this->encode_and_write_(packet);
}

void MhiXyClimate::dump_config() { LOG_CLIMATE("", "MHI XY Climate", this); }

void MhiXyClimate::publish_from_bus(bool power, climate::ClimateMode mode, float setpoint, float current,
                                    climate::ClimateFanMode fan, climate::ClimateSwingMode swing) {
  this->mode = mode;
  if (setpoint >= 10.0f && setpoint <= 35.0f) {
    this->target_temperature = setpoint;
  }
  if (current >= -10.0f && current <= 50.0f) {
    this->current_temperature = current;
  }
  this->fan_mode = fan;
  this->swing_mode = swing;
  this->publish_state();
  (void) power;
}

void MhiXyClimate::control(const climate::ClimateCall &call) {
  if (this->parent_ != nullptr) {
    this->parent_->apply_climate_call(call);
  }
}

climate::ClimateTraits MhiXyClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_AUTO,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_DRY,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
  });
  traits.set_supported_swing_modes({
      climate::CLIMATE_SWING_OFF,
      climate::CLIMATE_SWING_VERTICAL,
  });
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(16.0f);
  traits.set_visual_max_temperature(30.0f);
  traits.set_visual_temperature_step(0.5f);
  return traits;
}

}  // namespace mhi_xy
}  // namespace esphome
