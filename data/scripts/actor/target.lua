function update(eid, delta)
    local pos = entities:get_component(eid, component.position)
    local vel = entities:get_component(eid, component.velocity)

    if pos.y > 2 and vel.vy > 0 or pos.y < -2 and vel.vy < 0 then
        vel.vy = -vel.vy
    end
end

function on_collide(eid, other, manifold)
    local pos = component.position.new(entities:get_component(eid, component.position))

    local dirs = {
        { vx=5, vy=5 },
        { vx=5, vy=-5 },
        { vx=-5, vy=-5 },
        { vx=-5, vy=5 }
    }

    for _,dir in ipairs(dirs) do
        local vel = component.velocity.new()
        vel.vx = dir.vx
        vel.vy = dir.vy
        local ent = entities:create_entity()
        local animation = component.animation.new()
        animation.name = "target"
        animation.cycle = "walk"
        animation.frame = 0
        animation.t = 0
        entities:create_component(ent, pos)
        entities:create_component(ent, vel)
        entities:create_component(ent, animation)
    end

    entities:destroy_entity(eid)

    play_sfx("test")
    print("BOOM")
end
