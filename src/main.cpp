
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

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

    lua["entities"] = std::ref(entities);

    auto global_table = sol::table(lua.globals());
    scripting::register_type<ember_database>(global_table);

    auto component_table = lua.create_named_table("component");
    scripting::register_type<component::position>(component_table);
    scripting::register_type<component::script>(component_table);

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


    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    auto error = [](const char *msg) {
        perror(msg);
        exit(0);
    };

    auto config = emberjs::get_config();

    const auto display_width = int(config["display"]["width"]);
    const auto display_height = int(config["display"]["height"]);
    const auto aspect_ratio = float(display_width) / float(display_height);

    auto g_window = SDL_CreateWindow("LD41", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display_width, display_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    auto glcontext = SDL_GL_CreateContext(g_window);

    glEnable(GL_DEPTH_TEST);

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
        {{-0.5f, 0.f, 0.5f},{-0.5f, 0.f, -0.5f},{0.5f, 0.f, -0.5f},{0.5f, 0.f, 0.5f}},
        {{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f},{0.f, 1.f, 0.f}},
        {{0.f, 1.f},{0.f, 0.f},{1.f, 0.f},{1.f, 1.f}},
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

    using clock = std::chrono::steady_clock;
    auto prev_time = clock::now();

    auto framerate_buffer = std::vector<std::chrono::nanoseconds>();
    framerate_buffer.reserve(10);

    loop = [&]{
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

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (handle_gui_input(event)) break;
            if (handle_game_input(event)) break;
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto proj = glm::ortho(-10.f, 10.f, -10.f, 10.f, 10.f, -10.f);
        auto view = glm::mat4(1.f);

        auto frustum = sushi::frustum(proj*view);

        entities.visit([&](const component::position& pos){
            auto modelmat = glm::mat4(1);
            modelmat = glm::translate(modelmat, {pos.x, pos.y, 0});

            sushi::set_uniform("normal_mat", glm::inverseTranspose(view*modelmat));
            sushi::set_uniform("MVP", (proj*view*modelmat));
            sushi::set_texture(0, *texture_cache.get("test"));
            sushi::draw_mesh(sprite_mesh);
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
