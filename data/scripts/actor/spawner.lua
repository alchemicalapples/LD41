function update(eid, delta)
    local spawner = entities:get_component(eid, component.spawner)

    spawner.next_spawn = spawner.next_spawn - delta
    if spawner.next_spawn <= 0 then
        local enemylist = {}
        for i,v in ipairs(spawner.spawnrates) do
            for j=1,v[2] do
                enemylist[#enemylist + 1] = i
            end
        end

        local enemyidx = enemylist[math.random(#enemylist)]
        local enemyname = spawner.spawnrates[enemyidx][1]

        spawner.spawnrates[enemyidx][2] = spawner.spawnrates[enemyidx][2] - 1
        if spawner.spawnrates[enemyidx][2] <= 0 then
            table.remove(spawner.spawnrates, enemyidx)
        end

        local enemyMove = entity_from_json(get_enemy(enemyname))
        local epos = entities:get_component(eid, component.position)
        local evel = component.velocity.new()
        evel.vx = 0
        evel.vy = -1
        entities:create_component(enemyMove, epos)
        entities:create_component(enemyMove, evel)

        spawner.next_spawn = 3
        if #spawner.spawnrates == 0 then
            entities:create_component(eid, component.death_timer.new())
        end
    end
end
