import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
import esphome.components.dallas as dallas
from esphome.const import (
    CONF_ID,
    CONF_INPUT,
    CONF_NUMBER,
    CONF_MODE,
    CONF_INVERTED,
    CONF_OUTPUT,
)
from esphome.components.dallas import ( CONF_ALERT_ACTIVITY )

DEPENDENCIES = ["dallas"]
MULTI_CONF = True

DS2405Device = dallas.dallas_ns.class_("DS2405Device", dallas.DallasDevice, dallas.DallasPinComponent)
DS2405Component = dallas.dallas_ns.class_("DS2405Component", cg.PollingComponent, dallas.DallasDevice, dallas.DallasPinComponent)
DS2405GPIOPin = dallas.dallas_ns.class_("DS2405GPIOPin", dallas.cg.GPIOPin)
CONF_DS2405 = "ds2405"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(DS2405Component),
            cv.Optional(CONF_ALERT_ACTIVITY, default=False): cv.boolean,
        }
    )
    .extend(cv.polling_component_schema("never"))
   # .extend(cv.COMPONENT_SCHEMA)
    .extend(dallas.dallas_device_schema())
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    if CONF_ALERT_ACTIVITY in config:
        cg.add(var.set_alert_activity(config[CONF_ALERT_ACTIVITY]))

    await cg.register_component(var, config)
    await dallas.register_dallas_device(var, config)

def validate_mode(value):
    if not (value[CONF_INPUT] or value[CONF_OUTPUT]):
        raise cv.Invalid("Mode must be either input or output")
    if value[CONF_INPUT] and value[CONF_OUTPUT]:
        raise cv.Invalid("Mode must be either input or output")
    return value

DS2405_PIN_SCHEMA = cv.All(
    {
        cv.GenerateID(): cv.declare_id(DS2405GPIOPin),
        cv.Required(CONF_DS2405): cv.use_id(DS2405Component),
        cv.Optional(CONF_MODE, default={}): cv.All(
            {
                cv.Optional(CONF_INPUT, default=False): cv.boolean,
                cv.Optional(CONF_OUTPUT, default=False): cv.boolean,
            },
            validate_mode,
        ),
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
)

@pins.PIN_SCHEMA_REGISTRY.register(CONF_DS2405, DS2405_PIN_SCHEMA)

async def ds2405_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_DS2405])

    cg.add(var.set_parent(parent))

    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    return var
