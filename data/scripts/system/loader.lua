function load_entity(data)
    local ent = entities:create_entity()
    for k,v in pairs(data) do
        print("  "..k..":")
        if component[k] ~= nil then
            local com = component[k].new()
            for k,v in pairs(v) do
                print("    "..k..": "..v)
                if com[k] ~= nil then
                    com[k] = v
                else
                    print("Unknown field: "..k)
                end
            end
            entities:create_component(ent, com)
        else
            print("Uknown component: "..k)
        end
    end
    return ent
end

function load_world(data)
    print("Loading stage")
    for k,v in ipairs(data) do
        print(k..":")
        load_entity(v)
    end
end
