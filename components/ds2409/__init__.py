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

DS2409Component = dallas.dallas_ns.class_("DS2409Component", cg.PollingComponent, dallas.DallasDevice)
DS2409GPIOPin = dallas.dallas_ns.class_("DS2409GPIOPin", cg.GPIOPin)
DS2409Network = dallas.dallas_ns.class_("DS2409Network", dallas.DallasNetwork)

CONF_DS2409 = "ds2409"



CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(DS2409Component),
            cv.Optional(CONF_ALERT_ACTIVITY, default=False): cv.boolean,
            cv.GenerateID("main"): cv.declare_id(DS2409Network),
            cv.GenerateID("aux"): cv.declare_id(DS2409Network),
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


    main = cg.Pvariable(config["main"], var.get_network(True))
    aux = cg.Pvariable(config["aux"], var.get_network(False))

    await cg.register_component(var, config)
    await dallas.register_dallas_device(var, config)

def validate_mode(value):
    if not (value[CONF_INPUT] or value[CONF_OUTPUT]):
        raise cv.Invalid("Mode must be either input or output")
    if value[CONF_INPUT] and value[CONF_OUTPUT]:
        raise cv.Invalid("Mode must be either input or output")
    return value

DS2409_PIN_SCHEMA = cv.All(
    {
        cv.GenerateID(): cv.declare_id(DS2409GPIOPin),
        cv.Required(CONF_DS2409): cv.use_id(DS2409Component),
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

@pins.PIN_SCHEMA_REGISTRY.register(CONF_DS2409, DS2409_PIN_SCHEMA)

async def ds2409_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    parent = await cg.get_variable(config[CONF_DS2409])

    cg.add(var.set_parent(parent))

    cg.add(var.set_inverted(config[CONF_INVERTED]))
    cg.add(var.set_flags(pins.gpio_flags_expr(config[CONF_MODE])))
    return var
