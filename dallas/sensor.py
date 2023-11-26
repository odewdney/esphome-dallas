import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ADDRESS,
    CONF_DALLAS_ID,
    CONF_INDEX,
    CONF_RESOLUTION,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)
from . import DallasComponent, dallas_ns
import esphome.components.dallas as dallas

DallasTemperatureSensor = dallas_ns.class_("DallasTemperatureSensor", sensor.Sensor)

CONFIG_SCHEMA = cv.All(
    sensor.sensor_schema(
        DallasTemperatureSensor,
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Optional(CONF_RESOLUTION, default=12): cv.int_range(min=9, max=12),
        }
    )
    .extend(dallas.dallas_device_schema())
)

async def to_code(config):
    var = await sensor.new_sensor(config)

    if CONF_RESOLUTION in config:
        cg.add(var.set_resolution(config[CONF_RESOLUTION]))

    await dallas.register_dallas_device(var, config)
