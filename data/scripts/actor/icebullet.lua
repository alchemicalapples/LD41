function on_death(eid)
    local script = component.script.new()
    script.name = "actor/icefield"
    local timer = component.death_timer.new()
    timer.time = 5
    local anim = component.animation.new()
    anim.name = "icefield"
    anim.cycle = "idle"
    anim.scale = 5
    local position = component.position.new(entities:get_component(eid, component.position))
    local detect = component.detector.new()
    detect.radius = 3
    local fieldent = entities:create_entity()
    entities:create_component(fieldent, script)
    entities:create_component(fieldent, timer)
    entities:create_component(fieldent, anim)
    entities:create_component(fieldent, position)
    entities:create_component(fieldent, detect)
end
