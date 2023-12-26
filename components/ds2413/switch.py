import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import switch
from esphome.const import ( CONF_ID, CONF_NUMBER )
import esphome.components.dallas as dallas
from . import ( CONF_DS2413, DS2413Component )

DS2413Switch = dallas.dallas_ns.class_("DS2413Switch", switch.Switch)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_DS2413): cv.use_id(DS2413Component),
            cv.Required(CONF_NUMBER): cv.int_range(min=0, max=1),
        }
    )
    .extend(switch.switch_schema(DS2413Switch))
#    .extend(dallas.dallas_device_schema())
)

async def to_code(config):
    var = await switch.new_switch(config)

    parent = await cg.get_variable(config[CONF_DS2413])
    cg.add(var.set_parent(parent))
    num = config[CONF_NUMBER]
    cg.add(var.set_pin(num))

#    await dallas.register_dallas_device(var, config)
