
ball_states = {}

function ball_states.none(eid, ball, delta)
    ball.state = "swing"
    ball.marker = entities:create_entity()

    local pos = component.position.new(entities:get_component(eid, component.position))
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
        local pos = component.position.new(entities:get_component(eid, component.position))
        entities:create_component(ball.marker, pos)
        local vel = component.velocity.new()
        vel.vx = 3 * math.cos(ball.angle + math.pi/2)
        vel.vy = 3 * math.sin(ball.angle + math.pi/2)
        entities:create_component(ball.marker, vel)
    end
end

function ball_states.shoot(eid, ball, delta)
    if input.shoot_pressed then
        local mpos = entities:get_component(ball.marker, component.position)
        ball.land_x = mpos.x
        ball.land_y = mpos.y
        local pos = entities:get_component(eid, component.position)
        local vel = component.velocity.new()
        local dx = mpos.x - pos.x
        local dy = mpos.y - pos.y
        local dist = math.sqrt(dx^2 + dy^2)
        vel.vx = 3 * dx / dist
        vel.vy = 3 * dy / dist
        entities:create_component(eid, vel)
        ball.state = "flying"
        entities:destroy_entity(ball.marker)
    end
end

function ball_states.flying(eid, ball, delta)
    local pos = entities:get_component(eid, component.position)
    local vel = entities:get_component(eid, component.velocity)
    if math.abs(ball.land_x - pos.x) < math.abs(vel.vx * delta) then
        local tower_eid = entities:create_entity()
        local tpos = component.position.new()
        tpos.x = ball.land_x
        tpos.y = ball.land_y
        local ttower = component.tower.new()
        ttower.time = 1
        local detector = component.detector.new()
        detector.radius = 3
        local tscript = component.script.new()
        tscript.name = "actor/tower"
        local tanim = component.animation.new()
        tanim.name = "enemy"
        tanim.cycle = "walk"
        entities:create_component(tower_eid, tpos)
        entities:create_component(tower_eid, ttower)
        entities:create_component(tower_eid, detector)
        entities:create_component(tower_eid, tscript)
        entities:create_component(tower_eid, tanim)
        entities:destroy_entity(eid)
    end
end

function update(eid, delta)
    local ball = entities:get_component(eid, component.ball)
    ball_states[ball.state](eid, ball, delta)
end
