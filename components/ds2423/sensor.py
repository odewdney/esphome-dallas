import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_DALLAS_ID,
	UNIT_PULSES,
	ICON_COUNTER,
    STATE_CLASS_TOTAL_INCREASING,
)
import esphome.components.dallas as dallas

DallasCounterComponent = dallas.dallas_ns.class_("DallasCounterComponent", cg.PollingComponent)

DEPENDENCIES = ["dallas"]

CONF_COUNTER_A = "counter_a"
CONF_COUNTER_B = "counter_b"
CONF_COUNTER_C = "counter_c"
CONF_COUNTER_D = "counter_d"


counter_schema = sensor.sensor_schema(
    unit_of_measurement=UNIT_PULSES,
    icon=ICON_COUNTER,
    accuracy_decimals=0,
	state_class=STATE_CLASS_TOTAL_INCREASING,
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DallasCounterComponent),

            cv.Optional(CONF_COUNTER_A): counter_schema,
            cv.Optional(CONF_COUNTER_B): counter_schema,
            cv.Optional(CONF_COUNTER_C): counter_schema,
            cv.Optional(CONF_COUNTER_D): counter_schema,
		}
	)
    .extend(cv.polling_component_schema("60s"))
	.extend(dallas.dallas_device_schema())
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_COUNTER_A in config:
        sens = await sensor.new_sensor(config[CONF_COUNTER_A])
        cg.add(var.set_counter_a_sensor(sens))
    if CONF_COUNTER_B in config:
        sens = await sensor.new_sensor(config[CONF_COUNTER_B])
        cg.add(var.set_counter_b_sensor(sens))
    if CONF_COUNTER_C in config:
        sens = await sensor.new_sensor(config[CONF_COUNTER_C])
        cg.add(var.set_counter_c_sensor(sens))
    if CONF_COUNTER_D in config:
        sens = await sensor.new_sensor(config[CONF_COUNTER_D])
        cg.add(var.set_counter_d_sensor(sens))

    await dallas.register_dallas_device(var, config)

