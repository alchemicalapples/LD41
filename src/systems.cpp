#include "systems.hpp"

#include "components.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <sushi/frustum.hpp>
#include <sushi/shader.hpp>

namespace systems {

void movement(DB& entities, double delta) {
    entities.visit(
        [&](component::position& pos, const component::velocity& vel) {
            pos.x += vel.vx * delta;
            pos.y += vel.vy * delta;
        });
}

struct collision_manifold {
    ember_database::ent_id eid1;
    ember_database::ent_id eid2;
    component::aabb region;
};

void collision(DB& entities, double delta, cache<sol::environment>& environment_cache) {
    std::vector<collision_manifold> collisions;

    entities.visit_pairs(
        [&](DB::ent_id eid1, const component::position& pos1, const component::aabb& aabb1p) {
            auto aabb1 = aabb1p;
            aabb1.left += pos1.x;
            aabb1.right += pos1.x;
            aabb1.bottom += pos1.y;
            aabb1.top += pos1.y;
            return [&, eid1, aabb1](DB::ent_id eid2, const component::position& pos2, const component::aabb& aabb2p) {
                auto aabb2 = aabb2p;
                aabb2.left += pos2.x;
                aabb2.right += pos2.x;
                aabb2.bottom += pos2.y;
                aabb2.top += pos2.y;

                auto manifold = collision_manifold{};
                manifold.eid1 = eid1;
                manifold.eid2 = eid2;
                manifold.region.left = std::max(aabb1.left, aabb2.left);
                manifold.region.right = std::min(aabb1.right, aabb2.right);
                manifold.region.bottom = std::max(aabb1.bottom, aabb2.bottom);
                manifold.region.top = std::min(aabb1.top, aabb2.top);

                if (manifold.region.left < manifold.region.right && manifold.region.bottom < manifold.region.top) {
                    collisions.push_back(manifold);
                }
            };
        });

    for (auto& collision : collisions) {
        auto call_script = [&](DB::ent_id eid1, DB::ent_id eid2, const component::aabb& aabb) {
            if (entities.has_component<component::script>(eid1)) {
                auto& script = entities.get_component<component::script>(eid1);
                auto script_ptr = environment_cache.get(script.name);
                auto on_collide = (*script_ptr)["on_collide"];
                if (on_collide.valid()) {
                    on_collide(eid1, eid2, aabb);
                }
            }
        };
        call_script(collision.eid1, collision.eid2, collision.region);
        call_script(collision.eid2, collision.eid1, collision.region);
    }
}

void scripting(DB& entities, double delta, cache<sol::environment>& environment_cache) {
    entities.visit(
        [&](DB::ent_id eid, const component::script& script) {
            auto update = (*environment_cache.get(script.name))["update"];
            if (update.valid()) {
                update(eid, delta);
            }
        });
}

void detection(DB& entities, double delta, cache<sol::environment>& environment_cache) {
    //Billi
    entities.visit(
        [&](DB::ent_id tower_eid, component::detector& detector, const component::position& tower_pos){
            detector.entity_list.erase(
                std::remove_if(begin(detector.entity_list), end(detector.entity_list), [&](auto& eid) {
                        return !entities.exists(eid);
                    }),
                end(detector.entity_list));
            entities.visit([&](DB::ent_id enemy_eid, const component::position& enemy_pos, component::enemy_tag){
                    if(tower_eid == enemy_eid)
                        return;
                    auto iter = std::find(detector.entity_list.begin(),detector.entity_list.end(),enemy_eid);
                    bool found =  iter != detector.entity_list.end();
                    auto sqr = [](auto x) { return x*x; };
                    bool within_radius = detector.radius > sqrt(sqr(tower_pos.x-enemy_pos.x)+sqr(tower_pos.y - enemy_pos.y));

                    bool dying = entities.has_component<component::death_timer>(enemy_eid);


                    std::function<void(DB::ent_id eid, DB::ent_id other)>
                        on_enter = [](auto,auto){};
                    if (entities.has_component<component::script>(tower_eid)) {
                        auto& script = entities.get_component<component::script>(tower_eid);
                        auto script_ptr = environment_cache.get(script.name);
                        auto on_enter_proxy = (*script_ptr)["on_enter"];
                        if (on_enter_proxy.valid()) {
                            on_enter = on_enter_proxy;
                        }
                    }

                    // add entity_id
                    if(!found && within_radius && !dying){
                        detector.entity_list.push_back(enemy_eid);
                        on_enter(tower_eid, enemy_eid);
                    }
                    std::function<void(DB::ent_id eid, DB::ent_id other)>
                        on_leave = [](auto,auto){};
                    if (entities.has_component<component::script>(tower_eid)) {
                        auto& script = entities.get_component<component::script>(tower_eid);
                        auto script_ptr = environment_cache.get(script.name);
                        auto on_leave_proxy = (*script_ptr)["on_leave"];
                        if (on_leave_proxy.valid()) {
                            on_leave = on_leave_proxy;
                        }
                    }
                    //remove entity_id
                    if(found && (!within_radius || dying)){
                        detector.entity_list.erase(iter);
                        on_leave(tower_eid, enemy_eid);
                    }
                });
        });
}

void death_timer(DB& entities, double delta, cache<sol::environment>& environment_cache) {
    // Death timer system
    entities.visit(
        [&](DB::ent_id eid) {
            if (entities.has_component<component::death_timer>(eid)) {
                auto& timer = entities.get_component<component::death_timer>(eid);
                timer.time -= delta;
                if (timer.time <= 0) {
                    if (entities.has_component<component::script>(eid)) {
                        auto& script = entities.get_component<component::script>(eid);
                        auto env_ptr = environment_cache.get(script.name);
                        auto on_death = (*env_ptr)["on_death"];
                        if (on_death.valid()) {
                            on_death(eid);
                        }
                    }
                    entities.destroy_entity(eid);
                }
            }
        });
}

void render(DB& entities, double delta, glm::mat4 proj, glm::mat4 view, sushi::static_mesh& sprite_mesh, cache<sushi::texture_2d>& texture_cache, cache<nlohmann::json>& animation_cache) {
    auto frustum = sushi::frustum(proj*view);
    entities.visit(
        [&](const component::position& pos, component::animation& anim){
            if (frustum.contains({pos.x, pos.y, 0.f}, std::sqrt(0.5*0.5*2.f))) {
                auto modelmat = glm::mat4(1); // need
                modelmat = glm::translate(modelmat, {int(pos.x*16)/16.f, int(pos.y*16)/16.f, 0});

                modelmat = glm::scale(modelmat, {anim.scale, anim.scale, anim.scale});
                modelmat = glm::rotate(modelmat, anim.rot, {0, 0, 1});
                modelmat = glm::translate(modelmat, {anim.offset_x, anim.offset_y, 0});

                // animation code
                auto jsonAnim = *animation_cache.get(anim.name);
                auto tMilliSecond = float(jsonAnim[anim.cycle]["frame"][anim.frame]["t"]) / 1000.f;
                anim.t += delta / 10;
                if (anim.t > tMilliSecond) {
                    int nextFrame = jsonAnim[anim.cycle]["frame"][anim.frame]["nextFrame"];
                    anim.frame = nextFrame;
                    anim.t = 0;
                }
                auto pathToTexture = jsonAnim[anim.cycle]["frame"][anim.frame]["path"];

                sushi::set_texture(0, *texture_cache.get(pathToTexture));
                sushi::set_uniform("normal_mat", glm::inverseTranspose(view*modelmat));
                sushi::set_uniform("MVP", (proj*view*modelmat));
                sushi::draw_mesh(sprite_mesh);
            }
        });
}

void fire_damage(DB& entities, double delta) {
    entities.visit(
        [&](DB::ent_id eid, component::fire_damage& fire, component::health& health){
            fire.next -= delta;
            fire.duration -= delta;

            if (fire.next <= 0) {
                fire.next = fire.rate;
                health.max_health -= 1;
            }

            if (fire.duration <= 0) {
                entities.destroy_component<component::fire_damage>(eid);
            }

            if (health.max_health <= 0) {
                entities.create_component(eid, component::death_timer{});
            }
        });
}

} //namespace systems
