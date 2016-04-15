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

//#define GLFW_INCLUDE_ES2
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "bindings.h"
#include "icosphere.h"
#include "shader.h"
#include "terrain.h"
#include "vertex.h"
#include "window.h"

using namespace glm;
using namespace std;

// Emscripten is getting its callbacks from the browsers refresh driver, so
// does not have enough context to pass us an argument to the callback.
static glit::Window gWindow;

class Camera
{
    // I think relatively constant. Can certainly be updated by settings
    // changing the FOV. Probably also will need to tweak this as we render
    // so that we can represent things that are very far away while still
    // getting good z clipping up close.
    mat4 projection;

    // Updated constantly by input events.
    vec3 position;
    vec3 direction;

    constexpr static float NearDistance = 0.001f;
    constexpr static float FarDistance = 200000.f;

    Camera(Camera&&) = delete;
    Camera(const Camera&) = delete;

  public:
    Camera() {
        // Default camera is at world origin pointing down -z.
        position = vec3(0.f, 0.f, 0.f);
        direction = vec3(0.f, 0.f, -1.f);
        projection = perspective(pi<float>() * 0.25f, 1.f,
                                      NearDistance, FarDistance);
    }

    void screenSizeChanged(float width, float height) {
        projection = perspective(pi<float>() * 0.25f, width / height,
                                      NearDistance, FarDistance);
    }

    void warp(const vec3& pos, const vec3& dir) {
        position = pos;
        direction = dir;
    }

    // Combined projection and view, suitable for multiplying with a model
    // to get a final transform matrix.
    mat4 transform() const {
        auto view = lookAt(position, position + direction, vec3(0.f, 1.f, 0.f));
        return projection * view;
    }
};


class Entity
{
  public:
    virtual ~Entity() {}
    virtual void tick(double t, double dt) = 0;
    virtual void draw(const Camera& camera) = 0;
};

class POI : public Entity
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

    void draw(const Camera& camera) override {
        auto model = translate(mat4(1.0f), position);
        const static float s = 100.f;
        model = scale(model, vec3(s,s,s));
        //model = rotate(model, rotation, vec3(0.0f, 1.0f, 0.0f));
        auto modelviewproj = camera.transform() * model;
        primitive.draw(modelviewproj);
    }
};

class Planet : public Entity
{
    // The terrain state. Updated by providing the camera in draw.
    glit::Terrain terrain;

    // The current rotational state.
    float rotation;

    // Pointer to the point of interest that we want to treat as the camera.
    weak_ptr<POI> poi;

    explicit Planet(Planet&&) = delete;
    explicit Planet(const Planet&) = delete;

  public:
    explicit Planet(shared_ptr<POI> poi)
      : terrain(6371.f), //km
        rotation(0.f),
        poi(poi)
    {}
    ~Planet() override {}

    void tick(double t, double dt) override {
        /*
        rotation += dt;
        while (rotation > 2.0 * M_PI)
            rotation -= 2.0 * M_PI;
        */
    }

    void draw(const Camera& camera) override {
        auto model = rotate(mat4(1.f), rotation, vec3(0.0f, 1.0f, 0.0f));
        auto modelviewproj = camera.transform() * model;

        auto poip = poi.lock();
        if (!poip)
            return;

        auto mesh = terrain.uploadAsWireframe(poip->position,
                                              poip->view_direction);
        mesh.draw(modelviewproj);
    }
};


struct WorldState
{
    // The camera state.
    Camera camera;

    // Things to draw.
    vector<shared_ptr<Entity>> entities;
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
    dispatcher.observe("-quit", [](){gWindow.quit();});

    glit::InputBindings menuBindings(dispatcher, "MenuBindings");
    menuBindings.bindNamedKey("quit", GLFW_KEY_ESCAPE);

    //glit::SceneGraph scene;

    gWindow.init();
    gWindow.setCurrentBindings(menuBindings);
    //gWindow.setSceneGraph(scene);
    gWorld.camera.screenSizeChanged(gWindow.width(), gWindow.height());
    gWorld.camera.warp(vec3(0.f, 0.f, 6371.002f * 5), vec3(0.f, 0.f, -1.f));

    auto poi = POI::create();
    gWorld.entities.push_back(make_shared<Planet>(poi));
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
    gWorld.camera.warp(pos, dir);

    glClearColor(0, 0, 0, 255);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto e : gWorld.entities)
        e->tick(now, now - lastFrameTime);

    for (auto e : gWorld.entities)
        e->draw(gWorld.camera);

    gWindow.swap();
    lastFrameTime = now;
}

