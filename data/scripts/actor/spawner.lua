function update(eid, delta)
    local spawner = entities:get_component(eid, component.spawner)
    spawner.next_spawn = spawner.next_spawn - delta
    if spawner.next_spawn <= 0 then
        local enemyMove = entity_from_json(get_random_enemy())
        local epos = entities:get_component(eid, component.position)
        local evel = component.velocity.new()
        evel.vx = 0
        evel.vy = -1
        entities:create_component(enemyMove, epos)
        entities:create_component(enemyMove, evel)
        spawner.next_spawn = 3
    end
end
