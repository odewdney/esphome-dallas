import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import switch
from esphome.const import CONF_ID
import esphome.components.dallas as dallas

DS2405Switch = dallas.dallas_ns.class_("DS2405Switch", switch.Switch)

CONFIG_SCHEMA = (
    switch.switch_schema(DS2405Switch)
    .extend(dallas.dallas_device_schema())
)

async def to_code(config):
    var = await switch.new_switch(config)
    #await cg.register_component(var, config)
    await dallas.register_dallas_device(var, config)
