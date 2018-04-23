function update(eid, delta)
    for i=1,9 do
        if input["number_"..i.."_pressed"] then
            select_tower(i-1)
        end
    end
end

function on_death(eid)
    print("ded")
end
