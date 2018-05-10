function update(eid, delta)
    local pos = entities:get_component(eid, component.position)
    local tower = entities:get_component(eid, component.tower)
    local detector = entities:get_component(eid, component.detector)

    tower.time = tower.time + delta

    if detector.entity_list:size() > 0 then
        for _,target_eid in ipairs(detector.entity_list) do
            if entities:exists(target_eid) and entities:has_component(target_eid, component.enemy_tag) then
                local enemy_pos = entities:get_component(detector.entity_list[1], component.position)
                local bpos = component.position.new()
                bpos.x = pos.x
                bpos.y = pos.y

                local vx,vy = normalize_dimension((enemy_pos.x-pos.x),(enemy_pos.y-pos.y))
                local bvel = component.velocity.new()
                bvel.vx = vx*tower.speed
                bvel.vy = vy*tower.speed

                -- turret rotation
                local anim = entities:get_component(eid, component.animation)
                anim.rot = math.atan(bvel.vy, bvel.vx)

                if tower.time > tower.delay then
                    tower.time = 0
                    -- if tower.speed == 2 then
                    --   play_sfx("standardshoot")
                    -- end
                    -- if tower.speed == 5 then
                      play_sfx("fireturret")
                    -- end

                    local aabb = component.aabb.new()
                    aabb.left = -0.25
                    aabb.right = 0.25
                    aabb.bottom = -0.25
                    aabb.top = 0.25
                    local animation = component.animation.new()
                    animation.name = "bullet"
                    animation.cycle = tower.bullet_type
                    -- animation.rot = anim.rot
                    local timer = component.death_timer.new()
                    timer.time = tower.range / tower.speed
                    local script = component.script.new()
                    script.name = "actor/firebullet"
                    local bullet = entities:create_entity()
                    entities:create_component(bullet, bpos)
                    entities:create_component(bullet, bvel)
                    entities:create_component(bullet, aabb)
                    entities:create_component(bullet, animation)
                    entities:create_component(bullet, timer)
                    entities:create_component(bullet, script)
                end

                break
            end
        end
    end
end

function normalize_dimension(x,y)
    local angle = math.atan2(x,y)
    local lx = math.sin(angle)
    local ly = math.cos(angle)
    return lx,ly
end

function on_enter(eid, other)
end

function on_leave(eid, other)
end
