import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.components.dallas as dallas
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_RESOLUTION,
    CONF_SHUNT_RESISTANCE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_VOLT,
    UNIT_AMPERE,
 )

DEPENDENCIES = ["dallas"]
MULTI_CONF = True

DS2438Device = dallas.dallas_ns.class_("DS2438Device", dallas.DallasDevice, dallas.DallasPinComponent)
DS2438Component = dallas.dallas_ns.class_("DS2438Component", cg.PollingComponent, dallas.DallasDevice)
DS2438Threshold = dallas.dallas_ns.enum("Threshold")

DS2438Thresholds = {
    "None": DS2438Threshold.ZeroBits,
    "2Bits": DS2438Threshold.TwoBits,
    "4Bits": DS2438Threshold.FourBits,
    "8Bits": DS2438Threshold.EightBits,
}

CONF_DS2438 = "ds2438"

CONF_TEMP = "temp"
CONF_VCC = "vcc"
CONF_VAD = "vad"
CONF_CURRENT = "current"
CONF_ICA = "ica"
CONF_THRESHOLD = "threshold"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(DS2438Component),
            cv.Optional(CONF_TEMP): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_VCC): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_VAD): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ICA): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_SHUNT_RESISTANCE, default=0.001): cv.resistance,
            cv.Optional(CONF_THRESHOLD, default="None"): cv.enum(DS2438Thresholds),
        }
    )
    .extend(cv.polling_component_schema("never"))
   # .extend(cv.COMPONENT_SCHEMA)
    .extend(dallas.dallas_device_schema())
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)

    cg.add(var.set_current_resistor(config[CONF_SHUNT_RESISTANCE]))

    if sens_config := config.get(CONF_TEMP):
        sens = await sensor.new_sensor(sens_config)
        cg.add(var.set_temp_sensor(sens))
    if sens_config := config.get(CONF_VCC):
        sens = await sensor.new_sensor(sens_config)
        cg.add(var.set_vcc_sensor(sens))
    if sens_config := config.get(CONF_VAD):
        sens = await sensor.new_sensor(sens_config)
        cg.add(var.set_vad_sensor(sens))
    if sens_config := config.get(CONF_CURRENT):
        sens = await sensor.new_sensor(sens_config)
        cg.add(var.set_current_sensor(sens))
    if sens_config := config.get(CONF_ICA):
        sens = await sensor.new_sensor(sens_config)
        cg.add(var.set_ica_sensor(sens))

    await dallas.register_dallas_device(var, config)
