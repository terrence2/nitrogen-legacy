// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <stdexcept>
#include <iostream>
#include <memory>
#include <type_traits>

#include <math.h>
#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "bindings.h"
#include "camera.h"
#include "entity.h"
#include "icosphere.h"
#include "planet.h"
#include "player.h"
#include "shader.h"
#include "sun.h"
#include "terrain.h"
#include "vertex.h"
#include "window.h"

using namespace glm;
using namespace std;

// Emscripten is getting its callbacks from the browsers refresh driver, so
// does not have enough context to pass us an argument to the callback.
static glit::Window gWindow;

class POI : public glit::Entity
{
    glit::Mesh primitive;

    explicit POI(POI&&) = delete;
    explicit POI(const POI&) = delete;

  public:
    vec3 position;
    vec3 view_direction;

    POI(glit::Mesh&& prim)
      : primitive(forward<glit::Mesh>(prim))
      , position(0.f, 6371.f * 2.f, 0.f)
      , view_direction(0.f, 0.f, -1.f)
    {}
    ~POI() override {}

    static shared_ptr<POI> create() {
        glit::IcoSphere sphere(3);
        auto mesh = sphere.uploadAsWireframe();
        return make_shared<POI>(move(mesh));
    }

    void tick(double t, double dt) override {
        auto rot = rotate(mat4(1.f), 0.001f,
                normalize(vec3(1.f, 0.f, -1.f)));
        float alt = 2.f * 6371.f + (6371.f * cosf(t / 2.7f));
        position = rot * vec4(normalize(position) * alt, 1.f);
    }

    void draw(const glit::Camera& camera) override {
        auto model = translate(mat4(1.0f), position);
        const static float s = 100.f;
        model = scale(model, vec3(s,s,s));
        //model = rotate(model, rotation, vec3(0.0f, 1.0f, 0.0f));
        auto modelviewproj = camera.transform() * model;
        primitive.draw(modelviewproj);
    }
};


struct WorldState
{
    // The camera state.
    glit::Camera camera;

    // Things to draw.
    vector<shared_ptr<glit::Entity>> entities;
};
static WorldState gWorld;


static void do_loop();
static int do_main();

int main()
{
#ifdef __EMSCRIPTEN__
    try {
        do_main();
    } catch(runtime_error& sre) {
        // Empscripten does not handle exceptions at the top level, so be sure
        // to print them out manually.
        cerr << "Runtime error: " << sre.what() << endl;
    }
#else
    // Otherwise, let the platform catch it.
    do_main();
#endif // __EMSCRIPTEN__
}

int
do_main()
{
    glit::EventDispatcher dispatcher;
    dispatcher.onEdge("-quit", [](){gWindow.quit();});

    glit::InputBindings menuBindings(dispatcher, "MenuBindings");
    menuBindings.bindNamedKey("quit", GLFW_KEY_ESCAPE);

    glit::InputBindings debugBindings(dispatcher, "DebugBindings");
    debugBindings.bindNamedKey("quit", GLFW_KEY_ESCAPE);

    //glit::SceneGraph scene;

    gWindow.init();
    gWindow.setCurrentBindings(debugBindings);
    //gWindow.setSceneGraph(scene);
    gWorld.camera.screenSizeChanged(gWindow.width(), gWindow.height());
    //gWorld.camera.warp(vec3(0.f, 0.f, 6371.002f * 5), vec3(0.f, 0.f, -1.f));

    auto poi = POI::create();
    auto sun = glit::Sun::create();
    auto planet = make_shared<glit::Planet>(sun);

    auto player = make_shared<glit::Player>(planet);
    dispatcher.onEdge("+ufoLeft", [&](){player->ufoStartLeft();});
    dispatcher.onEdge("-ufoLeft", [&](){player->ufoStopLeft();});
    dispatcher.onEdge("+ufoRight", [&](){player->ufoStartRight();});
    dispatcher.onEdge("-ufoRight", [&](){player->ufoStopRight();});
    dispatcher.onEdge("+ufoForward", [&](){player->ufoStartForward();});
    dispatcher.onEdge("-ufoForward", [&](){player->ufoStopForward();});
    dispatcher.onEdge("+ufoBackward", [&](){player->ufoStartBackward();});
    dispatcher.onEdge("-ufoBackward", [&](){player->ufoStopBackward();});
    dispatcher.onEdge("+ufoUp", [&](){player->ufoStartUp();});
    dispatcher.onEdge("-ufoUp", [&](){player->ufoStopUp();});
    dispatcher.onEdge("+ufoDown", [&](){player->ufoStartDown();});
    dispatcher.onEdge("-ufoDown", [&](){player->ufoStopDown();});
    dispatcher.onEdge("+ufoRotateUp", [&](){player->ufoStartRotateUp();});
    dispatcher.onEdge("-ufoRotateUp", [&](){player->ufoStopRotateUp();});
    dispatcher.onEdge("+ufoRotateDown", [&](){player->ufoStartRotateDown();});
    dispatcher.onEdge("-ufoRotateDown", [&](){player->ufoStopRotateDown();});
    dispatcher.onEdge("+ufoRotateLeft", [&](){player->ufoStartRotateLeft();});
    dispatcher.onEdge("-ufoRotateLeft", [&](){player->ufoStopRotateLeft();});
    dispatcher.onEdge("+ufoRotateRight", [&](){player->ufoStartRotateRight();});
    dispatcher.onEdge("-ufoRotateRight", [&](){player->ufoStopRotateRight();});
    dispatcher.onEdge("+ufoRotateCCW", [&](){player->ufoStartRotateCCW();});
    dispatcher.onEdge("-ufoRotateCCW", [&](){player->ufoStopRotateCCW();});
    dispatcher.onEdge("+ufoRotateCW", [&](){player->ufoStartRotateCW();});
    dispatcher.onEdge("-ufoRotateCW", [&](){player->ufoStopRotateCW();});
    dispatcher.onEdge("+ufoAccelerate", [&](){player->ufoAccelerate();});
    dispatcher.onEdge("-ufoAccelerate", [&](){player->ufoDecelerate();});

    dispatcher.onLevel("ufoYaw", [&](double l, double dl){
            player->ufoYawDelta(dl);});
    dispatcher.onLevel("ufoPitch", [&](double l, double dl){
            player->ufoPitchDelta(dl);});

    debugBindings.bindNamedKey("ufoLeft", GLFW_KEY_A);
    debugBindings.bindNamedKey("ufoRight", GLFW_KEY_D);
    debugBindings.bindNamedKey("ufoForward", GLFW_KEY_W);
    debugBindings.bindNamedKey("ufoBackward", GLFW_KEY_S);
    debugBindings.bindNamedKey("ufoUp", GLFW_KEY_SPACE);
    debugBindings.bindNamedKey("ufoDown", GLFW_KEY_X);
    debugBindings.bindNamedKey("ufoRotateUp", GLFW_KEY_UP);
    debugBindings.bindNamedKey("ufoRotateDown", GLFW_KEY_DOWN);
    debugBindings.bindNamedKey("ufoRotateLeft", GLFW_KEY_LEFT);
    debugBindings.bindNamedKey("ufoRotateRight", GLFW_KEY_RIGHT);
    debugBindings.bindNamedKey("ufoRotateCCW", GLFW_KEY_Q);
    debugBindings.bindNamedKey("ufoRotateCW", GLFW_KEY_E);

    debugBindings.bindMouseAxis("ufoYaw", 0);
    debugBindings.bindMouseAxis("ufoPitch", 1);

    debugBindings.bindMouseScroll("ufoAccelerate", 1);

    // Note: order is important here.
    gWorld.entities.push_back(player);
    gWorld.entities.push_back(sun);
    gWorld.entities.push_back(planet);
    gWorld.entities.push_back(poi);

#ifndef __EMSCRIPTEN__
    while (!gWindow.isDone())
        do_loop();
#else
    emscripten_set_main_loop(do_loop, 60, 1);
#endif

    return 0;
}

void
do_loop()
{
    static double lastFrameTime = 0.0;
    double now = glfwGetTime();

    float off = 6371.f * 5.f;
    vec3 pos(off * sin(now / 2.f), 0.f, off * cos(now / 2.f));
    vec3 dir = normalize(-pos);
    gWorld.camera.warp(pos, dir, vec3(0.f, 1.f, 0.f));

    glClearColor(0, 0, 0, 255);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    for (auto e : gWorld.entities)
        e->tick(now, now - lastFrameTime);

    // Slave the camera to the player.
    glit::Player* player = dynamic_cast<glit::Player*>(gWorld.entities[0].get());
    gWorld.camera.warp(player->viewPosition(),
                       player->viewDirection(),
                       player->viewUp());
    /*
    gWorld.camera.warp(vec3(0.f, 20000.f, 0.f),
                       vec3(0.f, -1.f, 0.f),
                       vec3(0.f, 0.f, -1.f));
    */

    for (auto e : gWorld.entities)
        e->draw(gWorld.camera);

    gWindow.swap();
    lastFrameTime = now;
}

