function on_collide(eid, other, aabb)
    if entities:has_component(other, component.enemy_tag) then
        if entities:has_component(other, component.fire_damage) then
            local fire = entities:get_component(other, component.fire_damage)
            fire.duration = 2
        else
            local fire = component.fire_damage.new()
            fire.duration = 2
            fire.rate = 0.4
            entities:create_component(other, fire)
        end
    end
end
