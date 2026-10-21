import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor"]
MULTI_CONF = False

aldes_ibus_ns = cg.esphome_ns.namespace("aldes_ibus")
AldesIBus = aldes_ibus_ns.class_("AldesIBus", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(AldesIBus),
    cv.Optional("frames_ok"): sensor.sensor_schema(accuracy_decimals=0, entity_category="diagnostic"),
    cv.Optional("checksum_bad"): sensor.sensor_schema(accuracy_decimals=0, entity_category="diagnostic"),
    cv.Optional("sync_errors"): sensor.sensor_schema(accuracy_decimals=0, entity_category="diagnostic"),
    cv.Optional("rx_bytes"): sensor.sensor_schema(accuracy_decimals=0, entity_category="diagnostic"),
    cv.Optional("last_frame"): text_sensor.text_sensor_schema(entity_category="diagnostic"),
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "aldes_ibus",
    baud_rate=2400,
    require_rx=True,
    data_bits=8,
    parity="EVEN",
    stop_bits=1,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    for key, setter in (("frames_ok", "set_frames_ok_sensor"), ("checksum_bad", "set_checksum_bad_sensor"), ("sync_errors", "set_sync_errors_sensor"), ("rx_bytes", "set_rx_bytes_sensor")):
        if key in config:
            s = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(s))
    if "last_frame" in config:
        s = await text_sensor.new_text_sensor(config["last_frame"])
        cg.add(var.set_last_frame_sensor(s))
