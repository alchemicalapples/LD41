function update(eid, delta)
local pos = entities:get_component(eid, component.position)
local tower = entities:get_component(eid, component.tower)
  local detector = entities:get_component(eid, component.detector)
  tower.time = tower.time + delta
  if #detector.entity_list > 0 and tower.time > 4  then
      tower.time = 0
      local enemy_pos = entities:get_component(detector.entity_list[1], component.position)
      local bpos = component.position.new()
      bpos.x = pos.x
      bpos.y = pos.y

      local vx,vy = normalize_dimension((enemy_pos.x-pos.x),(enemy_pos.y-pos.y))
      local bvel = component.velocity.new()
      bvel.vx = vx*2
      bvel.vy = vy*2
      local aabb = component.aabb.new()
      aabb.left = -0.5
      aabb.right = 0.5
      aabb.bottom = -0.5
      aabb.top = 0.5
      local bullet = entities:create_entity()
      local animation = component.animation.new()
      animation.name = "bullet"
      animation.cycle = "walk"
      entities:create_component(bullet, bpos)
      entities:create_component(bullet, bvel)
      entities:create_component(bullet, aabb)
      entities:create_component(bullet, animation)
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
