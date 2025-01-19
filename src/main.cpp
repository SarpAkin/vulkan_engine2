#include <cstdlib>

#include <vke/vke.hpp>

#include "render/render_server.hpp"
#include "render/window/window_sdl.hpp"

int main(){
    vke::RenderServer engine;
    engine.init();

    engine.run();
}