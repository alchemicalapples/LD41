function load_entity(data)
    local ent = entities:create_entity()
    for k,v in pairs(data) do
        print("  "..k..":")
        local com = component[k].new()
        for k,v in pairs(v) do
            print("    "..k..": "..v)
            com[k] = v
        end
        entities:create_component(ent, com)
    end
end

function load_world(data)
    print("Loading stage")
    for k,v in ipairs(data) do
        print(k..":")
        load_entity(v)
    end
end
