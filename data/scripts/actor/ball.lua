
ball_states = {}

function ball_states.none(eid, ball, delta)
    ball.state = "swing"
    ball.marker = entities:create_entity()

    local pos = component.position.new(entities:get_component(eid, component.position))

    ball.reset_x = pos.x
    ball.reset_y = pos.y

    pos.y = pos.y + 1

    local anim = component.animation.new()
    anim.name = "player"
    anim.cycle = "walk"

    entities:create_component(ball.marker, pos)
    entities:create_component(ball.marker, anim)
end

function ball_states.swing(eid, ball, delta)
    if input.left then
        ball.angle = ball.angle + math.pi/2 * delta * 3
        if ball.angle > math.pi/2 then
            ball.angle = math.pi/2
        end
    end
    if input.right then
        ball.angle = ball.angle - math.pi/2 * delta * 3
        if ball.angle < -math.pi/2 then
            ball.angle = -math.pi/2
        end
    end

    local pos = component.position.new(entities:get_component(eid, component.position))
    pos.x = pos.x + 3 * math.cos(ball.angle + math.pi/2)
    pos.y = pos.y + 3 * math.sin(ball.angle + math.pi/2)
    entities:create_component(ball.marker, pos)

    if input.shoot_pressed then
        ball.state = "shoot"
        entities:destroy_entity(ball.marker)
    end
end

function ball_states.shoot(eid, ball, delta)
    local power = get_powermeter()
    power = power + delta

    set_powermeter(power)

    if input.shoot_pressed or power > 1 then
        local pos = entities:get_component(eid, component.position)
        ball.land_x = pos.x + power * 16.7 * math.cos(ball.angle + math.pi/2)
        ball.land_y = pos.y + power * 16.7 * math.sin(ball.angle + math.pi/2)
        local vel = component.velocity.new()
        local dx = ball.land_x - pos.x
        local dy = ball.land_y - pos.y
        local dist = math.sqrt(dx^2 + dy^2)
        vel.vx = 10 * dx / dist
        vel.vy = 10 * dy / dist
        entities:create_component(eid, vel)
        ball.state = "flying"
        set_powermeter(0)
    end
end

function ball_states.flying(eid, ball, delta)
    local pos = entities:get_component(eid, component.position)
    local vel = entities:get_component(eid, component.velocity)
    if math.abs(ball.land_x - pos.x) < math.abs(vel.vx * delta) then
        local tower_eid = entity_from_json(ball.tower)
        local tpos = component.position.new()
        tpos.x = ball.land_x
        tpos.y = ball.land_y
        entities:create_component(tower_eid, tpos)
        ball.state = "none"
        pos.x = ball.reset_x
        pos.y = ball.reset_y
        vel.vx = 0
        vel.vy = 0
    end
end

function update(eid, delta)
    local ball = entities:get_component(eid, component.ball)
    ball_states[ball.state](eid, ball, delta)
end
