import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID, CONF_PIN, CONF_DALLAS_ID, CONF_ADDRESS, CONF_INDEX


MULTI_CONF = True
AUTO_LOAD = ["sensor"]
CONF_ALERT_UPDATE_INTERVAL = "alert_update_interval"
CONF_ALERT_ACTIVITY = "activity_alert"

dallas_ns = cg.esphome_ns.namespace("dallas")
DallasComponent = dallas_ns.class_("DallasComponent", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DallasComponent),
        cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_ALERT_UPDATE_INTERVAL, default="never"): cv.update_interval,
    }
).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_ALERT_UPDATE_INTERVAL in config:
        cg.add(var.set_alert_update_interval(config[CONF_ALERT_UPDATE_INTERVAL]))

    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin))

DallasDevice = dallas_ns.class_("DallasDevice")

def dallas_device_schema():
    """Create a schema for a dallas device"""
    schema = {
      cv.GenerateID(CONF_DALLAS_ID): cv.use_id(DallasComponent),
      cv.Optional(CONF_ADDRESS): cv.hex_uint64_t,
      cv.Optional(CONF_INDEX): cv.positive_int,
	}
    return cv.Schema(schema).add_extra(cv.has_exactly_one_key(CONF_ADDRESS, CONF_INDEX))
    

async def register_dallas_device(var, config):
    """Register device with dallas hub"""
    if CONF_ADDRESS in config:
        cg.add(var.set_address(config[CONF_ADDRESS]))
    else:
        cg.add(var.set_index(config[CONF_INDEX]))

    hub = await cg.get_variable(config[CONF_DALLAS_ID])
    cg.add(var.set_parent(hub))
    cg.add(hub.register_sensor(var))

DallasPinComponent = dallas_ns.class_("DallasPinComponent")
DallasGPIOPin = dallas_ns.class_("DallasGPIOPin", cg.GPIOPin)
