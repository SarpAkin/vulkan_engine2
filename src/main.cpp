#include <cstdlib>

#include <vke/vke.hpp>

#include "engine.hpp"
#include "window/window_sdl.hpp"

int main(){
    vke::RenderServer engine;
    engine.init();

    engine.run();
}