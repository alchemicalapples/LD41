function update(eid, delta)
    local pos = entities:get_component(eid, component.position)

    if input.left then
        pos.x = pos.x - 5.0 * delta
    end
    if input.right then
        pos.x = pos.x + 5.0 * delta
    end
    if input.down then
        pos.y = pos.y - 5.0 * delta
    end
    if input.up then
        pos.y = pos.y + 5.0 * delta
    end
end
