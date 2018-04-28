#include "components.hpp"

#include "component_scripting.hpp"

namespace component {

void register_components(sol::table& component_table) {
    scripting::register_type<component::net_id>(component_table);
    scripting::register_type<component::position>(component_table);
    scripting::register_type<component::velocity>(component_table);
    scripting::register_type<component::aabb>(component_table);
    scripting::register_type<component::script>(component_table);
    scripting::register_type<component::detector>(component_table);
    scripting::register_type<component::tower>(component_table);
    scripting::register_type<component::ball>(component_table);
    scripting::register_type<component::animation>(component_table);
    scripting::register_type<component::death_timer>(component_table);
    scripting::register_type<component::bullet>(component_table);
    scripting::register_type<component::health>(component_table);
    scripting::register_type<component::speed>(component_table);
    scripting::register_type<component::pathing>(component_table);
    scripting::register_type<component::spawner>(component_table);

    scripting::register_type<component::enemy_tag>(component_table);
    scripting::register_type<component::bullet_tag>(component_table);
}

} //namespace compoennt
