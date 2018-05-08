slow_factor = 0.85

function on_enter(eid, other)
    if entities:has_component(other, component.speed) then
        local speed = entities:get_component(other, component.speed)
        speed.speedness = speed.speedness * slow_factor
    end
end

function on_leave(eid, other)
    if entities:has_component(other, component.speed) then
        local speed = entities:get_component(other, component.speed)
        speed.speedness = speed.speedness / slow_factor
    end
end

function on_death(eid)
    local detect = entities:get_component(eid, component.detector)
    for _,other in ipairs(detect.entity_list) do
        if entities:has_component(other, component.speed) then
            local speed = entities:get_component(other, component.speed)
            speed.speedness = speed.speedness / slow_factor
        end
    end
end
