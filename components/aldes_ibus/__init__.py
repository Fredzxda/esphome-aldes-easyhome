import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor"]
MULTI_CONF = False

aldes_ibus_ns = cg.esphome_ns.namespace("aldes_ibus")
AldesIBus = aldes_ibus_ns.class_("AldesIBus", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(AldesIBus),
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
