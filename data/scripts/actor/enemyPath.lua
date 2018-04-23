once = false

function update(eid, delta)
    --- test code for path finding ---
    doOnce(eid)
    local epos2 = entities:get_component(enemyMove, component.position)
    
end

function doOnce(eid)
    if not once then
        enemyMove = entities:create_entity()
        local animation = component.animation.new()
        local epos = entities:get_component(eid, component.position)
        epos.x = 9
        epos.y = 0
        --local vx,vy = normalize_dimension((enemy_pos.x-pos.x),(enemy_pos.y-pos.y))
        local evel = component.velocity.new()
        evel.vx = 0
        evel.vy = -1
        animation.name = "enemy"
        animation.cycle = "walk"
        entities:create_component(enemyMove, epos)
        entities:create_component(enemyMove, evel)
        -- entities:create_component(enemyMove, aabb)
        entities:create_component(enemyMove, animation)
        --print("hey")
        print (level1_path_logic)
        for k,v in ipairs(level1_path_logic) do
            for k2,v2 in pairs(v) do
                print(k..v2)
            end
        end

        once = true
    end

end
