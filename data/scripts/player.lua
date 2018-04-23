function update(eid, delta)
    for i=1,9 do
        if input["number_"..i.."_pressed"] then
            select_tower(i-1)
        end
    end
    local health = entities:get_component(eid, component.health)
    set_health_display(health.max_health)
end

function on_death(eid)
    set_health_display(0)
end
