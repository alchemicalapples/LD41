function load_entity(data)
    local ent = entities:create_entity()
    for k,v in pairs(data) do
        local com = component[k].new()
        for k,v in pairs(v) do
            com[k] = v
        end
        entities:create_component(ent, com)
    end
end

function load_world(data)
    for k,v in ipairs(data) do
        load_entity(v)
    end
end
