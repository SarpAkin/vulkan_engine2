#include <cstdlib>

#include <vke/vke.hpp>

#include "render/render_server.hpp"

int main(){
    vke::RenderServer engine;
    engine.init();

    engine.run();
}