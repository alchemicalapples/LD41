function update(eid, delta)
    local spawner = entities:get_component(eid, component.spawner)
    spawner.next_spawn = spawner.next_spawn - delta
    if spawner.next_spawn <= 0 then
        local enemyMove = entities:create_entity()
        local aabb = component.aabb.new()
        aabb.left = -0.5
        aabb.right = 0.5
        aabb.bottom = -0.5
        aabb.top = 0.5
        local animation = component.animation.new()
        animation.name = "enemy"
        animation.cycle = "walk"
        local speed = component.speed.new()
        speed.speedness = 2
        local epos = entities:get_component(eid, component.position)
        local evel = component.velocity.new()
        evel.vx = 0
        evel.vy = -1
        local script = component.script.new()
        script.name = "actor/enemy"
        local pathing = component.pathing.new()
        local health = component.health.new()
        health.max_health = 100
        entities:create_component(enemyMove, epos)
        entities:create_component(enemyMove, evel)
        entities:create_component(enemyMove, aabb)
        entities:create_component(enemyMove, speed)
        entities:create_component(enemyMove, animation)
        entities:create_component(enemyMove, script)
        entities:create_component(enemyMove, pathing)
        entities:create_component(enemyMove, health)
        entities:create_component(enemyMove, component.enemy_tag.new())
        spawner.next_spawn = 3
    end
end
