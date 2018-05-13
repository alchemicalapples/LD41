
#include "sdl.hpp"
#include "emberjs/config.hpp"

#include "utility.hpp"
#include "components.hpp"
#include "font.hpp"
#include "gui.hpp"
#include "resource_cache.hpp"
#include "sushi_renderer.hpp"
#include "systems.hpp"

#include <sushi/sushi.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <sol.hpp>
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>




////
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <iostream>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <cmath>
#include <functional>
#include <memory>
#include <random>

using namespace std::literals;

sushi::static_mesh sprite_mesh(const sushi::texture_2d& texture) {
    auto left = -texture.width / 2.f;
    auto right = texture.width / 2.f;
    auto bottom = -texture.height / 2.f;
    auto top = texture.height / 2.f;

    return sushi::load_static_mesh_data(
        {{left, bottom, 0.f},{left, top, 0.f},{right, top, 0.f},{right, bottom, 0.f}},
        {{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f},{0.f, 0.f, 1.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}});
}

template <typename... Ts>
auto vectorify(Ts&&... ts) {
    std::vector<std::common_type_t<Ts...>> rv;
    rv.reserve(sizeof...(ts));
    (rv.push_back(std::forward<Ts>(ts)), ...);
    return rv;
}

std::function<void()>* loop;
void main_loop() try {
    (*loop)();
} catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    std::terminate();
}

int main(int argc, char* argv[]) try {
    std::cout << "Init..." << std::endl;

    bool running = true;

    ember_database entities;

    std::cout << "Creating Lua state..." << std::endl;

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

    auto nlohmann_table = lua.create_named_table("component");
    nlohmann_table.new_usertype<nlohmann::json>("json");

    lua["entities"] = std::ref(entities);

    auto global_table = sol::table(lua.globals());
    scripting::register_type<ember_database>(global_table);

    auto component_table = lua.create_named_table("component");
    component::register_components(component_table);

    auto input_table = lua.create_named_table("input");

    std::cout << "Initializing soloud..." << std::endl;

    SoLoud::Soloud soloud;
    soloud.init();

    std::cout << "Creating caches..." << std::endl;

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
                return sol::make_object(lua, json.get<int>());
            case value_t::number_unsigned:
                return sol::make_object(lua, json.get<unsigned>());
            case value_t::number_float:
                return sol::make_object(lua, json.get<double>());
            default:
                return sol::make_object(lua, sol::nil);
        }
    };

    std::cout << "Loading config..." << std::endl;

    auto config = emberjs::get_config();

    const auto display_width = int(config["display"]["width"]);
    const auto display_height = int(config["display"]["height"]);
    const auto aspect_ratio = float(display_width) / float(display_height);

    auto json_to_lua_rec = [&](const nlohmann::json& json) {
        return json_to_lua(json, json_to_lua);
    };

    auto mesh_cache = resource_cache<sushi::static_mesh, std::string>([](const std::string& name){
        return sushi::load_static_mesh_file("data/models/" + name + ".obj");
    });

    auto texture_cache = resource_cache<sushi::texture_2d, std::string>([](const std::string& name){
        return sushi::load_texture_2d("data/textures/" + name + ".png", false, false, true, false);
    });

    auto animation_cache = resource_cache<nlohmann::json, std::string>([](const std::string& name) {
        std::ifstream file ("data/animations/" + name + ".json");
        nlohmann::json json;
        file >> json;
        return std::make_shared<nlohmann::json>(json);
    });

    auto path_logic_cache = resource_cache<sol::table, std::string>([&](const std::string& name) {
        std::ifstream file ("data/stages/pathlogic/" + name + ".json");
        nlohmann::json json;
        file >> json;
        return json_to_lua_rec(json);
    });

    auto font_cache = resource_cache<msdf_font, std::string>([](const std::string& fontname){
        return msdf_font("data/fonts/"+fontname+".ttf");
    });

    auto environment_cache = resource_cache<sol::environment, std::string>{[&](const std::string& name) {
            auto env = sol::environment(lua, sol::create, lua.globals());
            lua.safe_script_file("data/scripts/"+name+".lua", env);
            return env;
        }};

    auto sfx_cache = resource_cache<SoLoud::Wav, std::string>{[&](const std::string& name) {
        auto wav = std::make_shared<SoLoud::Wav>();
        wav->load(("data/sound/sfx/"+name+".wav").c_str());
        return wav;
    }};

    auto music_cache = resource_cache<SoLoud::Wav, std::string>{[&](const std::string& name) {
        auto wav = std::make_shared<SoLoud::Wav>();
        wav->load(("data/sound/sfx/"+name+".ogg").c_str());
        wav->setLooping(1);
        wav->setVolume(config["volume"]);
        return wav;
    }};

    auto tile_level_cache = resource_cache<nlohmann::json, std::string>{[&](const std::string& name) {
            std::ifstream file ("data/stages/"+name+".json");
            nlohmann::json jsonLevel;
            file >> jsonLevel;
            return std::make_shared<nlohmann::json>(jsonLevel);
        }};
    std::cout << "Setting helper functions..." << std::endl;

    std::function<void()> main_menu_loop;
    std::function<void()> gameplay_loop;
    std::function<void()> game_over_loop;
    std::function<void()> win_loop;

    auto set_game_state = [&](const std::string& name) {
        if (name == "main_menu") loop = &main_menu_loop;
        else if (name == "gameplay") loop = &gameplay_loop;
        else if (name == "game_over") loop = &game_over_loop;
        else if (name == "win") loop = &win_loop;
        else std::cerr << "Invalid game state." << std::endl;
    };

    lua["set_game_state"] = set_game_state;

    auto play_sfx = [&](const std::string& name) {
        auto wav_ptr = sfx_cache.get(name);
        soloud.stopAudioSource(*wav_ptr);
        soloud.play(*wav_ptr);
    };

    auto play_music = [&](const std::string& name) {
        auto wav_ptr = music_cache.get(name);
        soloud.stopAudioSource(*wav_ptr);
        soloud.play(*wav_ptr);
    };
    play_music("UnholyHoles");

    auto current_level = "level1"s;

    auto load_stage = [&](const std::string& name) {
        entities.visit([&](ember_database::ent_id eid) {
                entities.destroy_entity(eid);
            });
        current_level = name;
        std::ifstream file ("data/stages/" + name + ".json");
        nlohmann::json json;
        file >> json;
        auto loader_ptr = environment_cache.get("system/loader");
        (*loader_ptr)["load_world"](json_to_lua_rec(json["entities"]));
        lua["path_logic"] = *path_logic_cache.get(current_level + "pathlogic");
    };

    auto load_next_stage = [&]() {
        auto json = *tile_level_cache.get(current_level);
        if (!json["next_stage"].is_null()) {
            load_stage(json["next_stage"]);
        } else {
            set_game_state("win");
        }
    };

    auto entity_from_json = [&](const nlohmann::json& json) {
        auto loader_ptr = environment_cache.get("system/loader");
        auto eid = (*loader_ptr)["load_entity"](json_to_lua_rec(json)).get<ember_database::ent_id>();
        return eid;
    };

    auto get_tile_at = [&](int x, int y)->int {
          auto json = *tile_level_cache.get(current_level);
          for(auto& tile : json["tileset"]){
            if(tile["x"] == x && tile["y"] == y)
              return tile["tile"];
          }
          return -1;
    };

    lua["load_stage"] = load_stage;
    lua["load_next_stage"] = load_next_stage;
    lua["play_sfx"] = play_sfx;
    lua["play_music"] = play_music;
    lua["entity_from_json"] = entity_from_json;
    lua["get_tile_at"] = get_tile_at;
    lua["path_logic"] = *path_logic_cache.get(current_level + "pathlogic");

    std::cout << "Initializing SDL..." << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    auto error = [](const char *msg) {
        perror(msg);
        exit(0);
    };

    std::cout << "Opening window..." << std::endl;

    auto g_window = SDL_CreateWindow("LD41", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display_width, display_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    std::cout << "Setting window attributes..." << std::endl;
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

    std::cout << "Creating GL context..." << std::endl;
    auto glcontext = SDL_GL_CreateContext(g_window);

#ifndef __EMSCRIPTEN__
    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        throw std::runtime_error("Failed to load OpenGL extensions.");
    }
#endif

    std::cout << "Loading shaders..." << std::endl;

#ifdef __EMSCRIPTEN__
    auto program = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, "data/shaders/GLES2/basic.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, "data/shaders/GLES2/basic.frag"),
    });

    auto program_msdf = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, "data/shaders/GLES2/msdf.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, "data/shaders/GLES2/msdf.frag"),
    });
#else
    auto program = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, "data/shaders/GL4/basic.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, "data/shaders/GL4/basic.frag"),
    });

    auto program_msdf = sushi::link_program({
        sushi::compile_shader_file(sushi::shader_type::VERTEX, "data/shaders/GL4/msdf.vert"),
        sushi::compile_shader_file(sushi::shader_type::FRAGMENT, "data/shaders/GL4/msdf.frag"),
    });
#endif

    sushi::set_program(program);
    sushi::set_uniform("s_texture", 0);
    glBindAttribLocation(program.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program.get(), sushi::attrib_location::NORMAL, "normal");

    sushi::set_program(program_msdf);
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::POSITION, "position");
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::TEXCOORD, "texcoord");
    glBindAttribLocation(program_msdf.get(), sushi::attrib_location::NORMAL, "normal");

    auto framebuffer = sushi::create_framebuffer(vectorify(sushi::create_uninitialized_texture_2d(320, 240)));
    auto framebuffer_mesh = sprite_mesh(framebuffer.color_texs[0]);

    auto sprite_mesh = sushi::load_static_mesh_data(
        {{-0.5f, 0.5f, 0.f},{-0.5f, -0.5f, 0.f},{0.5f, -0.5f, 0.f},{0.5f, 0.5f, 0.f}},
        {{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f}},
        {{0.f, 0.f},{0.f, 1.f},{1.f, 1.f},{1.f, 0.f}},
        {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}}
    );

    sushi::static_mesh tile_meshes[12];

    for (int i = 0; i < 12; i++) {
        //if (i > 8)
        int row = i / 8;
        int column = i % 8;
        float vOffSetRight = (column + 1) * (1.f/8);
        float vOffSetLeft = ((column) * (1.f/8));// - 0.125f;
        float uOffSetUp = (row + 1) * (1.f/2);
        float uOffSetDown = (row) * (1.f/2); 
        tile_meshes[i] = sushi::load_static_mesh_data(
            {{-0.5f, 0.5f, 0.f},{-0.5f, -0.5f, 0.f},{0.5f, -0.5f, 0.f},{0.5f, 0.5f, 0.f}},
            {{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f}},
            {{vOffSetLeft, uOffSetDown},{vOffSetLeft, uOffSetUp},{vOffSetRight, uOffSetUp},{vOffSetRight, uOffSetDown}},
            {{{{0,0,0},{1,1,1},{2,2,2}}},{{{2,2,2},{3,3,3},{0,0,0}}}}
        );
    }

    auto renderer = sushi_renderer({320, 240}, program, program_msdf, font_cache, texture_cache);

    auto main_menu_root_widget = gui::screen({320, 240});

    auto main_menu_bg = std::make_shared<gui::panel>();
    main_menu_bg->set_position({0,0});
    main_menu_bg->set_size({320,240});
    main_menu_bg->set_texture("bg/main_menu");
    main_menu_bg->show();

    main_menu_root_widget.add_child(main_menu_bg);

    auto root_widget = gui::screen({320, 240});

    root_widget.show();

    auto framerate_stamp = std::make_shared<gui::label>();
    framerate_stamp->set_position({-1,-1});
    framerate_stamp->set_font("LiberationSans-Regular");
    framerate_stamp->set_size(renderer, 12);
    framerate_stamp->set_text(renderer, "");
    framerate_stamp->set_color({1,0,1,1});
    framerate_stamp->show();

    auto health_label = std::make_shared<gui::label>();
    health_label->set_position({176,0});
    health_label->set_font("LiberationSans-Regular");
    health_label->set_size(renderer, 16);
    health_label->set_text(renderer, "Health:");
    health_label->set_color({0,0,0,1});
    health_label->show();

    auto powermeter_border_panel = std::make_shared<gui::panel>();
    powermeter_border_panel->set_position({-1,0});
    powermeter_border_panel->set_size({16,80});
    powermeter_border_panel->set_texture("powermeter_border");
    powermeter_border_panel->show();

    auto powermeter_panel = std::make_shared<gui::panel>();
    powermeter_panel->set_position({0,0});
    powermeter_panel->set_size({16,0});
    powermeter_panel->set_texture("powermeter");
    powermeter_panel->show();

    auto lastpower_panel = std::make_shared<gui::panel>();
    lastpower_panel->set_position({0,0});
    lastpower_panel->set_size({16,1});
    lastpower_panel->set_texture("powermeter");
    lastpower_panel->show();

    powermeter_border_panel->add_child(powermeter_panel);
    powermeter_border_panel->add_child(lastpower_panel);

    struct tower_info {
        std::shared_ptr<gui::panel> panel;
        nlohmann::json json;
    };

    auto tower_panels = std::vector<tower_info>{};
    tower_panels.reserve(9);

    auto add_tower = [&](const std::string& image, const nlohmann::json& json) {
        auto panel = std::make_shared<gui::panel>();
        panel->set_position({tower_panels.size()*32, 0});
        panel->set_size({32,32});
        panel->set_texture("tower_panel");
        panel->show();

        auto tower_image = std::make_shared<gui::panel>();
        tower_image->set_position({0,0});
        tower_image->set_size({32,32});
        tower_image->set_texture(image);
        tower_image->show();

        auto number_label = std::make_shared<gui::label>();
        number_label->set_position({0,-1});
        number_label->set_font("LiberationSans-Regular");
        number_label->set_size(renderer, 8);
        number_label->set_text(renderer, std::to_string(tower_panels.size()+1));
        number_label->set_color({0,0,0,1});
        number_label->show();

        panel->add_child(tower_image);
        panel->add_child(number_label);

        tower_panels.push_back({panel, json});
    };

    {
        std::ifstream file ("data/towers.json");
        nlohmann::json json;
        file >> json;

        for (auto& tower : json) {
            add_tower("towers/"+tower["name"].get<std::string>(), tower["template"]);
        }
    }

    root_widget.add_child(framerate_stamp);
    root_widget.add_child(health_label);
    root_widget.add_child(powermeter_border_panel);

    for (const auto& info : tower_panels) {
        root_widget.add_child(info.panel);
    }

    int selected_tower = 0;

    auto select_tower = [&](int i) {
        if (i < 0 || i >= tower_panels.size()) return;
        tower_panels[selected_tower].panel->set_texture("tower_panel");
        selected_tower = i;
        tower_panels[i].panel->set_texture("tower_panel_selected");
    };

    auto get_selected_tower = [&]() {
        return tower_panels[selected_tower].json;
    };

    select_tower(0);

    lua["select_tower"] = select_tower;
    lua["get_selected_tower"] = get_selected_tower;

    auto set_powermeter = [&](float percent) {
        powermeter_panel->set_size({16, 80*percent});
    };

    auto get_powermeter = [&]() {
        return powermeter_panel->get_size().y / 80.f;
    };

    lua["set_powermeter"] = set_powermeter;
    lua["get_powermeter"] = get_powermeter;

    auto set_last_power = [&](float percent) {
        lastpower_panel->set_position({0, 80*percent});
    };

    lua["set_last_power"] = set_last_power;

    auto set_health_display = [&](int health) {
        health_label->set_text(renderer, "Health: " + std::to_string(health));
    };

    lua["set_health_display"] = set_health_display;

    struct enemy_info {
        std::string name;
        nlohmann::json json;
    };

    std::vector<enemy_info> enemies;
    enemies.reserve(3);

    {
        std::ifstream file ("data/enemies.json");
        nlohmann::json json;
        file >> json;

        for (auto& enemy : json) {
            enemies.push_back({enemy["name"], enemy["template"]});
        }
    }

    auto rng = std::mt19937(std::random_device{}());
    auto roll_enemy = std::uniform_int_distribution<>(0, enemies.size()-1);

    auto get_enemy = [&](const std::string& name) {
        for (auto& enemy : enemies) {
            if (enemy.name == name) {
                return enemy.json;
            }
        }
        std::cerr << "enemy name not found: " << name << std::endl;
        return nlohmann::json{};
    };

    auto get_random_enemy = [&]() {
        return enemies[roll_enemy(rng)].json;
    };

    lua["get_enemy"] = get_enemy;
    lua["get_random_enemy"] = get_random_enemy;

    auto handle_game_input = [&](const SDL_Event& event){
        switch (event.type) {
            case SDL_QUIT:
                std::cout << "Goodbye!" << std::endl;
                running = false;
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


    auto get_entities_at = [&](float x, float y, float r) {
        auto vec = std::vector<ember_database::ent_id>{};
        vec.reserve(8);
        entities.visit([&](ember_database::ent_id eid, const component::position& pos) {
                if (glm::distance(glm::vec2{pos.x,pos.y}, glm::vec2{x,y}) < r) {
                    vec.push_back(eid);
                }
            });
        return vec;
    };

    lua["get_entities_at"] = get_entities_at;

    using clock = std::chrono::steady_clock;
    auto prev_time = clock::now();

    auto framerate_buffer = std::vector<std::chrono::nanoseconds>();
    framerate_buffer.reserve(10);

    auto make_menu_state = [&](const std::string& name, const auto& on_next) {
        auto fade = 0.0;
        auto fade_dir = 1.0;
        return [&, name, on_next, fade, fade_dir]() mutable {
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

            SDL_Event event[2]; // Array is needed to work around stack issue in SDL_PollEvent.
            while (SDL_PollEvent(&event[0]))
            {
                if (handle_gui_input(event[0])) break;
                if (handle_game_input(event[0])) break;
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

            update_input("shoot", SDL_SCANCODE_SPACE);

            // Update

            fade += delta * fade_dir;

            if (fade > 1.f) {
                fade = 1.f;
            }

            if (fade == 1.f && input_table["shoot_pressed"]) {
                fade_dir = -1.f;
            }

            if (fade < 0.f) {
                fade = 0;
                fade_dir = 1;
                on_next();
                return;
            }

            // Render

            sushi::set_framebuffer(framebuffer);
            glClearColor(0,0,0,1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glViewport(0, 0, 320, 240);

            auto proj = glm::ortho(-7.5f * aspect_ratio, 7.5f * aspect_ratio, -7.5f, 7.5f, 7.5f, -7.5f);
            auto view = glm::mat4(1.f);

            auto frustum = sushi::frustum(proj*view);

            main_menu_bg->set_texture("bg/"+name);
            renderer.begin();
            main_menu_root_widget.draw(renderer, {0,0});
            renderer.end();

            {
                sushi::set_framebuffer(nullptr);
                glClearColor(0,0,0,1);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glViewport(0, 0, 640, 480);

                auto projmat = glm::ortho(-160.f, 160.f, -120.f, 120.f, -1.f, 1.f);
                auto modelmat = glm::mat4(1.f);
                sushi::set_program(program);
                sushi::set_uniform("MVP", projmat * modelmat);
                sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
                sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
                sushi::set_uniform("s_texture", 0);
                sushi::set_uniform("tint", glm::vec4{fade,fade,fade,1});
                sushi::set_texture(0, framebuffer.color_texs[0]);
                sushi::draw_mesh(framebuffer_mesh);
            }

            SDL_GL_SwapWindow(g_window);

            lua.collect_garbage();
        };
    };

    auto screen_fade = 0.0;
    auto screen_fade_dir = 1.0;

    main_menu_loop = make_menu_state(
        "main_menu", [&]{
            entities.visit([&](ember_database::ent_id eid) {
                    entities.destroy_entity(eid);
                });
            load_stage("level1");
            screen_fade = 0.0;
            screen_fade_dir = 1.0;
            set_game_state("gameplay");
        });

    game_over_loop = make_menu_state(
        "game_over", [&] {
            set_game_state("main_menu");
        });

    win_loop = make_menu_state(
        "win", [&] {
            set_game_state("main_menu");
        });

    gameplay_loop = [&]{
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

        SDL_Event event[2]; // Array is needed to work around stack issue in SDL_PollEvent.
        while (SDL_PollEvent(&event[0]))
        {
            if (handle_gui_input(event[0])) break;
            if (handle_game_input(event[0])) break;
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

        update_input("number_1", SDL_SCANCODE_1);
        update_input("number_2", SDL_SCANCODE_2);
        update_input("number_3", SDL_SCANCODE_3);
        update_input("number_4", SDL_SCANCODE_4);
        update_input("number_5", SDL_SCANCODE_5);
        update_input("number_6", SDL_SCANCODE_6);
        update_input("number_7", SDL_SCANCODE_7);
        update_input("number_8", SDL_SCANCODE_8);
        update_input("number_9", SDL_SCANCODE_9);
        update_input("number_0", SDL_SCANCODE_0);

        // Update

        systems::movement(entities, delta);
        systems::collision(entities, delta, environment_cache);
        systems::scripting(entities, delta, environment_cache);
        systems::detection(entities, delta, environment_cache);
        systems::fire_damage(entities, delta);

        bool won = true;

        entities.visit([&](component::enemy_tag enemy){
          won = false;
        });

        entities.visit([&](component::spawner& spawner){
          won = false;
        });

          if(won)
              screen_fade_dir = -1.0;

        screen_fade += delta * screen_fade_dir;

        if (screen_fade > 1.0) {
            screen_fade = 1.0;
        }

        if (screen_fade < 0.0) {
            screen_fade = 0.0;
            screen_fade_dir = 1.0;
            load_next_stage();
        }

        systems::death_timer(entities, delta, environment_cache);

        // Render

        sushi::set_framebuffer(framebuffer);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, 320, 240);

        auto proj = glm::ortho(-7.5f * aspect_ratio, 7.5f * aspect_ratio, -7.5f, 7.5f, 7.5f, -7.5f);
        auto view = glm::mat4(1.f);
        view = glm::translate(view, {-9.5, 7, 0});

        auto frustum = sushi::frustum(proj*view);

        // Render Tiles
        // tileset mesh array index values:
        // 0 -> grass
        // 1 -> up / down straight path
        // 2 -> up / right turn path
        // 3 -> left / right straight path
        // 4 -> left / up turn path
        // 5 -> left / down turn path
        // 6 -> down / right turn path
        const auto& jsonLevel = *tile_level_cache.get(current_level);

        sushi::set_program(program);
        sushi::set_texture(0, *texture_cache.get("tileset"));
        for (auto& tile : jsonLevel["tileset"]) {
            auto modelmat = glm::mat4(1);
            // int x = int(tile["x"]) - 9;
            // int y = int(tile["y"]) - 7;
            // modelmat = glm::translate(modelmat, {x, -y, 0});
            modelmat = glm::translate(modelmat, {tile["x"], -int(tile["y"]), 0});
            sushi::set_uniform("normal_mat", glm::inverseTranspose(view*modelmat));
            sushi::set_uniform("MVP", (proj*view*modelmat));
            sushi::draw_mesh(tile_meshes[int(tile["tile"])]);
        }

        // Render Entities

        systems::render(entities, delta, proj, view, sprite_mesh, texture_cache, animation_cache);

        {
            sushi::set_framebuffer(nullptr);
            glClearColor(0,0,0,1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glViewport(0, 0, 640, 480);

            auto projmat = glm::ortho(-160.f, 160.f, -120.f, 120.f, -1.f, 1.f);
            auto modelmat = glm::mat4(1.f);
            sushi::set_program(program);
            sushi::set_uniform("MVP", projmat * modelmat);
            sushi::set_uniform("normal_mat", glm::transpose(glm::inverse(modelmat)));
            sushi::set_uniform("cam_forward", glm::vec3{0,0,-1});
            sushi::set_uniform("s_texture", 0);
            sushi::set_uniform("tint", glm::vec4{screen_fade,screen_fade,screen_fade,1});
            sushi::set_texture(0, framebuffer.color_texs[0]);
            sushi::draw_mesh(framebuffer_mesh);

            renderer.begin();
            root_widget.draw(renderer, {0,0});
            renderer.end();
        }

        SDL_GL_SwapWindow(g_window);

        lua.collect_garbage();
    };

    std::cout << "Success." << std::endl;

    loop = &main_menu_loop;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 1);
#else
    while (running) main_loop();
#endif

    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    return EXIT_SUCCESS;
} catch (const std::exception& e) {
    std::cout << "Fatal exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
