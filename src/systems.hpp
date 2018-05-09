#ifndef LD41_SYSTEMS_HPP
#define LD41_SYSTEMS_HPP

#include "entities.hpp"
#include "resource_cache.hpp"
#include "json.hpp"

#include <glm/glm.hpp>
#include <sol.hpp>
#include <sushi/texture.hpp>
#include <sushi/mesh.hpp>

namespace systems {

using DB = ember_database;

template <typename T>
using cache = resource_cache<T, std::string>;

void movement(DB& entities, double delta);
void collision(DB& entities, double delta, cache<sol::environment>& environment_cache);
void scripting(DB& entities, double delta, cache<sol::environment>& environment_cache);
void detection(DB& entities, double delta, cache<sol::environment>& environment_cache);
void death_timer(DB& entities, double delta, cache<sol::environment>& environment_cache);
void render(DB& entities, double delta, glm::mat4 proj, glm::mat4 view, sushi::static_mesh& sprite_mesh, cache<sushi::texture_2d>& texture_cache, cache<nlohmann::json>& animation_cache);
void fire_damage(DB& entities, double delta);

} //namespace systems

#endif //LD41_SYSTEMS_HPP
