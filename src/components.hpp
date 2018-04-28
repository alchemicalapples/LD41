#ifndef LD41_COMPONENTS_HPP
#define LD41_COMPONENTS_HPP

#include "sushi/sushi.hpp"

#include "json.hpp"
#include "scripting.hpp"
#include "entities.hpp"

#include <Meta.h>

#include <functional>
#include <string>
#include <type_traits>

#include <memory>
#include <chrono>

#define REGISTER(NAME, ...)                                           \
        }                                                             \
        namespace meta {                                              \
        template <> constexpr auto registerName<component::NAME>() {  \
            return #NAME;                                             \
        }                                                             \
        template <> inline auto registerMembers<component::NAME>() {  \
            using comtype = component::NAME;                          \
            return members(__VA_ARGS__);                              \
        }                                                             \
        }                                                             \
        namespace component {
#define MEMBER(FIELD) member(#FIELD, &comtype::FIELD)

namespace component {

void register_components(sol::table& component_table);

struct net_id {
    ember_database::net_id id;
};

REGISTER(net_id,
         MEMBER(id))

struct position {
    float x = 0;
    float y = 0;
};

REGISTER(position,
         MEMBER(x),
         MEMBER(y))

struct velocity {
    float vx = 0;
    float vy = 0;
};

REGISTER(velocity,
         MEMBER(vx),
         MEMBER(vy))

struct aabb {
    float left = 0;
    float right = 0;
    float bottom = 0;
    float top = 0;
};

REGISTER(aabb,
         MEMBER(left),
         MEMBER(right),
         MEMBER(bottom),
         MEMBER(top))

struct script {
    std::string name;
};

REGISTER(script,
         MEMBER(name))

struct detector {
    float radius;
    std::vector<ember_database::ent_id> entity_list;
};

REGISTER(detector,
         MEMBER(radius),
         MEMBER(entity_list))

struct tower {
    ember_database::ent_id current_target;
    double time = 0;
    double delay = 1;
    double range = 1;
    double speed = 1;
    std::string bullet_type = "standard";
};

REGISTER(tower,
         MEMBER(current_target),
         MEMBER(time),
         MEMBER(delay),
         MEMBER(range),
         MEMBER(speed),
         MEMBER(bullet_type))

struct ball {
    std::string state = "none";
    float angle = 0;
    float power = 0;
    ember_database::ent_id marker;
    float land_x;
    float land_y;
    float reset_x;
    float reset_y;
};

REGISTER(ball,
         MEMBER(state),
         MEMBER(angle),
         MEMBER(power),
         MEMBER(marker),
         MEMBER(land_x),
         MEMBER(land_y),
         MEMBER(reset_x),
         MEMBER(reset_y))

struct animation {
    std::string name;
    std::string cycle;
    int frame = 0;
    float t = 0;
    float offset_x = 0;
    float offset_y = 0;
    float rot = 0;
};

REGISTER(animation,
         MEMBER(name),
         MEMBER(cycle),
         MEMBER(frame),
         MEMBER(t),
         MEMBER(offset_x),
         MEMBER(offset_y),
         MEMBER(rot))

struct death_timer {
    double time = 0;
};

REGISTER(death_timer,
         MEMBER(time))

struct bullet {
    ember_database::ent_id tower;
};

REGISTER(bullet,
         MEMBER(tower))

struct health {
    int max_health;
};

REGISTER(health,
         MEMBER(max_health))

struct speed {
    float speedness;
};

REGISTER(speed,
         MEMBER(speedness))

struct pathing {
    int next_tile = 0;
};

REGISTER(pathing,
         MEMBER(next_tile))

struct spawner {
    float next_spawn = 0;
    int max_number;
};

REGISTER(spawner,
         MEMBER(next_spawn),
         MEMBER(max_number))

using enemy_tag = ginseng::tag<struct emeny_tag_t>;
REGISTER(enemy_tag)

using bullet_tag = ginseng::tag<struct bullet_tag_t>;
REGISTER(bullet_tag)

} //namespace component

#undef MEMBER
#undef REGISTER

#endif //LD41_COMPONENTS_HPP
