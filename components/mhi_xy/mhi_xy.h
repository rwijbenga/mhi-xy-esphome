#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace mhi_xy {

static const uint8_t DECODED_LEN = 16;
static const size_t RAW_MAX = 64;

class MhiXyClimate;

class MhiXyComponent : public Component, public uart::UARTDevice {
 public:
  void set_packet_timeout(uint32_t timeout_ms) { this->packet_timeout_ms_ = timeout_ms; }
  void set_allow_control(bool allow) { this->allow_control_ = allow; }
  void set_climate(MhiXyClimate *climate) { this->climate_ = climate; }

  void set_setpoint_sensor(sensor::Sensor *s) { this->setpoint_sensor_ = s; }
  void set_indoor_temperature_sensor(sensor::Sensor *s) { this->indoor_temperature_sensor_ = s; }
  void set_fan_speed_sensor(sensor::Sensor *s) { this->fan_speed_sensor_ = s; }
  void set_vane_sensor(sensor::Sensor *s) { this->vane_sensor_ = s; }
  void set_packet_count_sensor(sensor::Sensor *s) { this->packet_count_sensor_ = s; }
  void set_checksum_errors_sensor(sensor::Sensor *s) { this->checksum_errors_sensor_ = s; }

  void set_power_binary_sensor(binary_sensor::BinarySensor *s) { this->power_binary_sensor_ = s; }
  void set_swing_binary_sensor(binary_sensor::BinarySensor *s) { this->swing_binary_sensor_ = s; }
  void set_checksum_ok_binary_sensor(binary_sensor::BinarySensor *s) { this->checksum_ok_binary_sensor_ = s; }

  void set_source_text_sensor(text_sensor::TextSensor *s) { this->source_text_sensor_ = s; }
  void set_rc_mode_text_sensor(text_sensor::TextSensor *s) { this->rc_mode_text_sensor_ = s; }
  void set_last_packet_text_sensor(text_sensor::TextSensor *s) { this->last_packet_text_sensor_ = s; }
  void set_last_raw_text_sensor(text_sensor::TextSensor *s) { this->last_raw_text_sensor_ = s; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  bool allow_control() const { return this->allow_control_; }
  bool apply_climate_call(const climate::ClimateCall &call);

 protected:
  void process_raw_(const uint8_t *data, size_t len);
  bool decode_3to1_(const uint8_t *raw, size_t raw_len, uint8_t *out, size_t *out_len);
  static int symbol_value_(uint8_t encoded);
  static bool looks_encoded_(const uint8_t *data, size_t len);
  static uint8_t checksum_(const uint8_t *decoded, size_t len);
  void parse_decoded_(const uint8_t *decoded, size_t len);
  void publish_hex_(text_sensor::TextSensor *sensor, const uint8_t *data, size_t len);
  bool encode_and_write_(const uint8_t *decoded);

  uint32_t packet_timeout_ms_{12};
  bool allow_control_{false};

  uint8_t rx_buf_[RAW_MAX];
  size_t rx_len_{0};
  uint32_t last_byte_ms_{0};
  bool receiving_{false};

  uint8_t last_decoded_[DECODED_LEN]{};
  bool have_decoded_{false};
  uint32_t packet_count_{0};
  uint32_t checksum_errors_{0};

  MhiXyClimate *climate_{nullptr};
  sensor::Sensor *setpoint_sensor_{nullptr};
  sensor::Sensor *indoor_temperature_sensor_{nullptr};
  sensor::Sensor *fan_speed_sensor_{nullptr};
  sensor::Sensor *vane_sensor_{nullptr};
  sensor::Sensor *packet_count_sensor_{nullptr};
  sensor::Sensor *checksum_errors_sensor_{nullptr};
  binary_sensor::BinarySensor *power_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *swing_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *checksum_ok_binary_sensor_{nullptr};
  text_sensor::TextSensor *source_text_sensor_{nullptr};
  text_sensor::TextSensor *rc_mode_text_sensor_{nullptr};
  text_sensor::TextSensor *last_packet_text_sensor_{nullptr};
  text_sensor::TextSensor *last_raw_text_sensor_{nullptr};
};

class MhiXyClimate : public climate::Climate, public Component {
 public:
  void set_parent(MhiXyComponent *parent) { this->parent_ = parent; }
  void setup() override {}
  void dump_config() override;
  void publish_from_bus(bool power, climate::ClimateMode mode, float setpoint, float current, climate::ClimateFanMode fan,
                        climate::ClimateSwingMode swing);

 protected:
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;
  MhiXyComponent *parent_{nullptr};
};

}  // namespace mhi_xy
}  // namespace esphome
