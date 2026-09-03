import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import DEVICE_CLASS_RUNNING, ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_MHI_XY_ID, MhiXyComponent

CONF_POWER = "power"
CONF_SWING = "swing"
CONF_CHECKSUM_OK = "checksum_ok"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MHI_XY_ID): cv.use_id(MhiXyComponent),
        cv.Optional(CONF_POWER): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_SWING): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_CHECKSUM_OK): binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_RUNNING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_MHI_XY_ID])
    if CONF_POWER in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_POWER])
        cg.add(hub.set_power_binary_sensor(sens))
    if CONF_SWING in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_SWING])
        cg.add(hub.set_swing_binary_sensor(sens))
    if CONF_CHECKSUM_OK in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CHECKSUM_OK])
        cg.add(hub.set_checksum_ok_binary_sensor(sens))
