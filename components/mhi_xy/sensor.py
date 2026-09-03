import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

from . import CONF_MHI_XY_ID, MhiXyComponent

CONF_SETPOINT = "setpoint"
CONF_INDOOR_TEMPERATURE = "indoor_temperature"
CONF_FAN_SPEED = "fan_speed"
CONF_VANE = "vane"
CONF_PACKET_COUNT = "packet_count"
CONF_CHECKSUM_ERRORS = "checksum_errors"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MHI_XY_ID): cv.use_id(MhiXyComponent),
        cv.Optional(CONF_SETPOINT): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_INDOOR_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FAN_SPEED): sensor.sensor_schema(
            icon="mdi:fan",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_VANE): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_PACKET_COUNT): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_CHECKSUM_ERRORS): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_MHI_XY_ID])
    if CONF_SETPOINT in config:
        sens = await sensor.new_sensor(config[CONF_SETPOINT])
        cg.add(hub.set_setpoint_sensor(sens))
    if CONF_INDOOR_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_INDOOR_TEMPERATURE])
        cg.add(hub.set_indoor_temperature_sensor(sens))
    if CONF_FAN_SPEED in config:
        sens = await sensor.new_sensor(config[CONF_FAN_SPEED])
        cg.add(hub.set_fan_speed_sensor(sens))
    if CONF_VANE in config:
        sens = await sensor.new_sensor(config[CONF_VANE])
        cg.add(hub.set_vane_sensor(sens))
    if CONF_PACKET_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_PACKET_COUNT])
        cg.add(hub.set_packet_count_sensor(sens))
    if CONF_CHECKSUM_ERRORS in config:
        sens = await sensor.new_sensor(config[CONF_CHECKSUM_ERRORS])
        cg.add(hub.set_checksum_errors_sensor(sens))
