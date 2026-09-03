import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

CODEOWNERS = ["@remco"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["climate", "sensor", "binary_sensor", "text_sensor"]
MULTI_CONF = False

CONF_MHI_XY_ID = "mhi_xy_id"
CONF_PACKET_TIMEOUT = "packet_timeout"
CONF_ALLOW_CONTROL = "allow_control"

mhi_xy_ns = cg.esphome_ns.namespace("mhi_xy")
MhiXyComponent = mhi_xy_ns.class_("MhiXyComponent", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MhiXyComponent),
            cv.Optional(CONF_PACKET_TIMEOUT, default="12ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_ALLOW_CONTROL, default=False): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_packet_timeout(config[CONF_PACKET_TIMEOUT]))
    cg.add(var.set_allow_control(config[CONF_ALLOW_CONTROL]))
