function update(eid, delta)
    local epos = entities:get_component(eid, component.position)
    local pathing = entities:get_component(eid, component.pathing)
    local newVel = entities:get_component(eid, component.velocity)
    local speed = entities:get_component(eid, component.speed)
    local dest = level1_path_logic[pathing.next_tile + 1]
    local dx = dest.x - epos.x
    local dy = dest.y - epos.y
    local dist = math.sqrt(dx*dx + dy*dy)
    if dist ~= 0 then
        newVel.vx = dx * speed.speedness / dist
        newVel.vy = dy * speed.speedness / dist
    end
    if dist < 0.0625 then
        pathing.next_tile = pathing.next_tile + 1
        if pathing.next_tile == #level1_path_logic then
            entities:create_component(eid, component.death_timer.new())
        end
    end
end

function on_collide(eid1, eid2, aabb)
    local is_bullet = entities:has_component(eid2, component.bullet_tag)
    local is_enemy = entities:has_component(eid2, component.enemy_tag)

    if is_bullet then
        local health = entities:get_component(eid1, component.health)
        local bullet = entities:get_component(eid2, component.bullet)
        local detector = entities:get_component(bullet.tower, component.detector)
        health.max_health = health.max_health - 1
        entities:destroy_entity(eid2)
        if health.max_health == 0 then
            entities:create_component(eid1, component.death_timer.new())
        end

        detector.entity_list:erase(detector.entity_list:find(eid2))
    elseif not is_enemy and entities:has_component(eid2, component.health) then
        local other_health = entities:get_component(eid2, component.health)
        other_health.max_health = other_health.max_health - 1
        if other_health.max_health <= 0 then
            entities:create_component(eid2, component.death_timer.new())
        end
        entities:create_component(eid1, component.death_timer.new())
    end
end
