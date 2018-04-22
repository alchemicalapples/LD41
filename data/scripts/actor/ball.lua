
ball_states = {}

function ball_states.none(eid, ball, delta)
    ball.state = "swing"
    ball.marker = entities:create_entity()

    local pos = component.position.new(entities:get_component(eid, component.position))
    pos.y = pos.y + 1

    entities:create_component(ball.marker, pos)
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
end

function update(eid, delta)
    local ball = entities:get_component(eid, component.ball)
    ball_states[ball.state](eid, ball, delta)
end
