
#include "sdl.hpp"
#include "emberjs/config.hpp"

#include "utility.hpp"
#include "components.hpp"
#include "component_scripting.hpp"
#include "font.hpp"
#include "gui.hpp"
#include "resource_cache.hpp"
#include "sushi_renderer.hpp"

#include <sushi/sushi.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <sol.hpp>

#include <emscripten.h>
#include <emscripten/html5.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <cmath>
#include <functional>
#include <memory>

using namespace std::literals;

std::function<void()> loop;
void main_loop() {
    loop();
}

int main() try {
    std::cout << "Init..." << std::endl;

    ember_database entities;

    std::cout << "Creating Lua state..." << std::endl;

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

    lua["entities"] = std::ref(entities);

    auto global_table = sol::table(lua.globals());
    scripting::register_type<ember_database>(global_table);

    auto component_table = lua.create_named_table("component");
    scripting::register_type<component::net_id>(component_table);
    scripting::register_type<component::position>(component_table);
    scripting::register_type<component::velocity>(component_table);
    scripting::register_type<component::aabb>(component_table);
    scripting::register_type<component::script>(component_table);

    auto input_table = lua.create_named_table("input");

    std::cout << "Creating caches..." << std::endl;

    auto mesh_cache = resource_cache<sushi::static_mesh, std::string>([](const std::string& name){
            return sushi::load_static_mesh_file("data/models/" + name + ".obj");
        });

    auto texture_cache = resource_cache<sushi::texture_2d, std::string>([](const std::string& name){
            return sushi::load_texture_2d("data/textures/" + name + ".png", true, false, true, false);
        });

    auto font_cache = resource_cache<msdf_font, std::string>([](const std::string& fontname){
            return msdf_font("data/fonts/"+fontname+".ttf");
        });

    auto environment_cache = resource_cache<sol::environment, std::string>{[&](const std::string& name) {
            auto env = sol::environment(lua, sol::create, lua.globals());
            lua.script_file("data/scripts/"+name+".lua", env);
            return env;
        }};

    std::cout << "Initializing SDL..." << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    auto error = [](const char *msg) {
        perror(msg);
        exit(0);
    };

    std::cout << "Loading config..." << std::endl;

    auto config = emberjs::get_config();

    const auto display_width = int(config["display"]["width"]);
    const auto display_height = int(config["display"]["height"]);
    const auto aspect_ratio = float(display_width) / float(display_height);

    std::cout << "Opening window..." << std::endl;

    auto g_window = SDL_CreateWindow("LD41", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display_width, display_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    auto glcontext = SDL_GL_CreateContext(g_window);

    glEnable(GL_DEPTH_TEST);

    std::cout << "Loading shaders..." << std::endl;

    auto program = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, "data/shaders/basic.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, "data/shaders/basic.frag"),
    });

    auto program_msdf = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, "data/shaders/msdf.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, "data/shaders/msdf.frag"),
    });

    sushi::set_program(program);
    sushi::set_uniform("s_texture", 0);
    glBindAttribLocation(program.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program.get(), sushi::attrib_location::NORMAL, "normal");

    sushi::set_program(program_msdf);
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::NORMAL, "normal");

    auto sprite_mesh = sushi::load_static_mesh_data(
        {{-0.5f, 0.5f, 0.f},{-0.5f, -0.5f, 0.f},{0.5f, -0.5f, 0.f},{0.5f, 0.5f, 0.f}},
        {{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}}
    );

    auto renderer = sushi_renderer({display_width, display_height}, program, program_msdf, font_cache, texture_cache);

    auto root_widget = gui::screen({display_width, display_height});

    root_widget.show();

    auto version_stamp = std::make_shared<gui::label>();
    version_stamp->set_position({-1,-1});
    version_stamp->set_font("LiberationSans-Regular");
    version_stamp->set_size(renderer, 12);
    version_stamp->set_text(renderer, "ALPHA 0.0.0");
    version_stamp->set_color({1,0,1,1});
    version_stamp->show();

    auto framerate_stamp = std::make_shared<gui::label>();
    framerate_stamp->set_position({-1,-13});
    framerate_stamp->set_font("LiberationSans-Regular");
    framerate_stamp->set_size(renderer, 12);
    framerate_stamp->set_text(renderer, "");
    framerate_stamp->set_color({1,0,1,1});
    framerate_stamp->show();

    root_widget.add_child(version_stamp);
    root_widget.add_child(framerate_stamp);

    std::cout << "Loading stage..." << std::endl;

    {
        std::ifstream file ("data/stages/test.json");
        nlohmann::json json;
        file >> json;

        auto json_to_lua = [&](const nlohmann::json& json, auto& json_to_lua) -> sol::object {
            using value_t = nlohmann::json::value_t;
            switch (json.type()) {
                case value_t::null:
                    return sol::make_object(lua, sol::nil);
                case value_t::object: {
                    auto obj = lua.create_table();
                    for (auto it = json.begin(); it != json.end(); ++it) {
                        obj[it.key()] = json_to_lua(it.value(), json_to_lua);
                    }
                    return obj;
                }
                case value_t::array: {
                    auto obj = lua.create_table();
                    for (auto i = 0; i < json.size(); ++i) {
                        obj[i+1] = json_to_lua(json[i], json_to_lua);
                    }
                    return obj;
                }
                case value_t::string:
                    return sol::make_object(lua, json.get<std::string>());
                case value_t::boolean:
                    return sol::make_object(lua, json.get<bool>());
                case value_t::number_integer:
                case value_t::number_unsigned:
                case value_t::number_float:
                    return sol::make_object(lua, json.get<double>());
                default:
                    return sol::make_object(lua, sol::nil);
            }
        };

        auto loader_ptr = environment_cache.get("system/loader");

        (*loader_ptr)["load_world"](json_to_lua(json, json_to_lua));
    }

    auto handle_game_input = [&](const SDL_Event& event){
        switch (event.type) {
            case SDL_QUIT:
                std::cout << "Goodbye!" << std::endl;
                return true;
        }

        return false;
    };

    auto handle_gui_input = [&](SDL_Event& e){
        switch (e.type) {
            case SDL_MOUSEBUTTONDOWN: {
                switch (e.button.button) {
                    case SDL_BUTTON_LEFT: {
                        auto abs_click_pos = glm::vec2{e.button.x, display_height - e.button.y + 1};
                        auto widget_stack = get_descendent_stack(root_widget, abs_click_pos - root_widget.get_position());
                        while (!widget_stack.empty()) {
                            auto cur_widget = widget_stack.back();
                            auto widget_pos = get_absolute_position(*cur_widget);
                            auto rel_click_pos = abs_click_pos - widget_pos;
                            if (cur_widget->on_click) {
                                if (cur_widget->on_click(*cur_widget, rel_click_pos)) {
                                    return true;
                                }
                            }
                            widget_stack.pop_back();
                        }
                        break;
                    }
                }
                break;
            }
        }
        return false;
    };

    struct collision_manifold {
        ember_database::ent_id eid1;
        ember_database::ent_id eid2;
        component::aabb region;
    };
    std::vector<collision_manifold> collisions;

    using clock = std::chrono::steady_clock;
    auto prev_time = clock::now();

    auto framerate_buffer = std::vector<std::chrono::nanoseconds>();
    framerate_buffer.reserve(10);

    loop = [&]{
        // System

        auto now = clock::now();
        auto delta_time = now - prev_time;
        prev_time = now;
        framerate_buffer.push_back(delta_time);

        if (framerate_buffer.size() >= 10) {
            auto avg_frame_dur = std::accumulate(begin(framerate_buffer), end(framerate_buffer), 0ns) / framerate_buffer.size();
            auto framerate = 1.0 / std::chrono::duration<double>(avg_frame_dur).count();

            framerate_stamp->set_text(renderer, std::to_string(std::lround(framerate)) + "fps");
            framerate_buffer.clear();
        }

        auto delta = std::chrono::duration<double>(delta_time).count();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (handle_gui_input(event)) break;
            if (handle_game_input(event)) break;
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        auto update_input = [&](const std::string& name, SDL_Scancode key) {
            if (input_table[name].valid()) {
                auto prev = bool(input_table[name]);
                auto curr = bool(keys[key]);
                input_table[name] = curr;
                input_table[name+"_pressed"] = curr && !prev;
                input_table[name+"_released"] = !curr && prev;
            } else {
                input_table[name] = bool(keys[key]);
                input_table[name+"_pressed"] = bool(keys[key]);
                input_table[name+"_released"] = false;
            }
        };

        update_input("left", SDL_SCANCODE_LEFT);
        update_input("right", SDL_SCANCODE_RIGHT);
        update_input("up", SDL_SCANCODE_UP);
        update_input("down", SDL_SCANCODE_DOWN);
        update_input("shoot", SDL_SCANCODE_SPACE);

        // Update

        entities.visit([&](component::position& pos, const component::velocity& vel) {
            pos.x += vel.vx * delta;
            pos.y += vel.vy * delta;
        });

        collisions.clear();

        entities.visit_pairs([&](ember_database::ent_id eid1, const component::position& pos1, const component::aabb& aabb1p) {
            auto aabb1 = aabb1p;
            aabb1.left += pos1.x;
            aabb1.right += pos1.x;
            aabb1.bottom += pos1.y;
            aabb1.top += pos1.y;
            return [&, eid1, aabb1](ember_database::ent_id eid2, const component::position& pos2, const component::aabb& aabb2p) {
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
            auto call_script = [&](ember_database::ent_id eid1, ember_database::ent_id eid2, const component::aabb& aabb) {
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

        entities.visit([&](ember_database::ent_id eid, const component::script& script) {
            (*environment_cache.get(script.name))["update"](eid, delta);
        });

        // Render

        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto proj = glm::ortho(-10.f * aspect_ratio, 10.f * aspect_ratio, -10.f, 10.f, 10.f, -10.f);
        auto view = glm::mat4(1.f);

        auto frustum = sushi::frustum(proj*view);

        entities.visit([&](const component::position& pos){
            if (frustum.contains({pos.x, pos.y, 0.f}, std::sqrt(0.5*0.5*2.f))) {
                auto modelmat = glm::mat4(1);
                modelmat = glm::translate(modelmat, {pos.x, pos.y, 0});

                sushi::set_program(program);
                sushi::set_uniform("normal_mat", glm::inverseTranspose(view*modelmat));
                sushi::set_uniform("MVP", (proj*view*modelmat));
                sushi::set_texture(0, *texture_cache.get("test"));
                sushi::draw_mesh(sprite_mesh);
            }
        });

        renderer.begin();
        root_widget.draw(renderer, {0,0});
        renderer.end();

        SDL_GL_SwapWindow(g_window);
    };

    std::cout << "Success." << std::endl;

    emscripten_set_main_loop(main_loop, 0, 1);

    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return EXIT_SUCCESS;
} catch (const std::exception& e) {
    std::cout << "Fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
