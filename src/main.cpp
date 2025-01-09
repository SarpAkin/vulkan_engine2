#include <cstdlib>

#include <vke/vke.hpp>

#include "engine.hpp"
#include "window/window_sdl.hpp"

int main(){
    vke::Engine engine;
    engine.init();

    engine.run();
}