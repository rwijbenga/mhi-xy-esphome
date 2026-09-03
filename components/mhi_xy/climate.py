import esphome.codegen as cg
from esphome.components import climate
import esphome.config_validation as cv

from . import CONF_MHI_XY_ID, MhiXyComponent, mhi_xy_ns

MhiXyClimate = mhi_xy_ns.class_("MhiXyClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.climate_schema(MhiXyClimate).extend(
    {
        cv.GenerateID(CONF_MHI_XY_ID): cv.use_id(MhiXyComponent),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)
    hub = await cg.get_variable(config[CONF_MHI_XY_ID])
    cg.add(var.set_parent(hub))
    cg.add(hub.set_climate(var))
