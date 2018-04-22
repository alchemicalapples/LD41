function update(eid, delta)

end


function on_collide(eid1, eid2, aabb)
  local is_bullet = entities:has_component(eid2, component.bullet_tag)

  if is_bullet then
  local health = entities:get_component(eid1, component.health)
  local bullet = entities:get_component(eid2, component.bullet)
  local detector = entities:get_component(bullet.tower, component.detector)
  health.max_health = health.max_health - 1
  entities:destroy_entity(eid2)
  if health.max_health == 0 then
    entities:destroy_entity(eid1)
    end

    detector.entity_list:erase(detector.entity_list:find(eid2))
  end
end
