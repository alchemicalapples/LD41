function update(eid, delta)
    local pos = entities:get_component(eid, component.position)

    local speed = 5.0

    if input.left then
        pos.x = pos.x - speed * delta
    end
    if input.right then
        pos.x = pos.x + speed * delta
    end
    if input.down then
        pos.y = pos.y - speed * delta
    end
    if input.up then
        pos.y = pos.y + speed * delta
    end

    if input.shoot_pressed then
        local bpos = component.position.new()
        bpos.x = pos.x
        bpos.y = pos.y
        local bvel = component.velocity.new()
        bvel.vx = speed * 2
        bvel.vy = 0
        local aabb = component.aabb.new()
        aabb.left = -0.5
        aabb.right = 0.5
        aabb.bottom = -0.5
        aabb.top = 0.
        local animation = component.animation.new()
        animation.name = "bullet"
        animation.cycle = "walk"
        local bullet = entities:create_entity()
        entities:create_component(bullet, bpos)
        entities:create_component(bullet, bvel)
        entities:create_component(bullet, aabb)
        entities:create_component(bullet, animation)
    end
end
