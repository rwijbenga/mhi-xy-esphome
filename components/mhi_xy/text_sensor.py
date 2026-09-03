import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_MHI_XY_ID, MhiXyComponent

CONF_SOURCE = "source"
CONF_RC_MODE = "rc_mode"
CONF_LAST_PACKET = "last_packet"
CONF_LAST_RAW = "last_raw"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MHI_XY_ID): cv.use_id(MhiXyComponent),
        cv.Optional(CONF_SOURCE): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_RC_MODE): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_LAST_PACKET): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_LAST_RAW): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_MHI_XY_ID])
    if CONF_SOURCE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_SOURCE])
        cg.add(hub.set_source_text_sensor(sens))
    if CONF_RC_MODE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_RC_MODE])
        cg.add(hub.set_rc_mode_text_sensor(sens))
    if CONF_LAST_PACKET in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_PACKET])
        cg.add(hub.set_last_packet_text_sensor(sens))
    if CONF_LAST_RAW in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_RAW])
        cg.add(hub.set_last_raw_text_sensor(sens))
