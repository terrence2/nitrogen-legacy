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
#include <type_traits>

#include <math.h>
#include <stdio.h>

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


// Emscripten is getting its callbacks from the browsers refresh driver, so
// does not have enough context to pass us an argument to the callback.
static glit::Window gWindow;

class Camera
{
    // I think relatively constant. Can certainly be updated by settings
    // changing the FOV. Probably also will need to tweak this as we render
    // so that we can represent things that are very far away while still
    // getting good z clipping up close.
    glm::mat4 projection;

    // Updated constantly by input events.
    glm::mat4 view;

    Camera(Camera&&) = delete;
    Camera(const Camera&) = delete;

  public:
    Camera() {
        // Default camera is at world origin pointing down -z.
        view = glm::mat4(1.f);
        projection = glm::perspective(glm::pi<float>() * 0.25f, 4.0f / 3.0f, 0.1f, 100.f);
    }

    // Combined projection and view, suitable for multiplying with a model
    // to get a final transform matrix.
    glm::mat4 transform() const {
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


class Planet : public Entity
{
    // The terrain state. Updated by providing the camera in draw.
    glit::Terrain terrain;

    // For drawing the wireframe view.
    glit::Program progWireframe;

    // The current rotational state.
    float rotation;

    // The constructor
    explicit Planet(glit::Program&& progWF)
      : progWireframe(std::forward<glit::Program>(progWF))
    {}

    explicit Planet(Planet&&) = delete;
    explicit Planet(const Planet&) = delete;

  public:
    ~Planet() override {}
    static Planet* create() {
        auto VertexDesc = glit::VertexDescriptor::fromType<glit::Terrain::Vertex>();
        glit::VertexShader vs(
                R"SHADER(
                precision highp float;
                uniform mat4 uModelViewProj;
                attribute vec3 aPosition;
                varying vec4 vColor;
                void main()
                {
                    gl_Position = uModelViewProj * vec4(aPosition, 1.0);
                    vColor = vec4(255, 255, 255, 255);
                }
                )SHADER",
                VertexDesc);
        glit::FragmentShader fs(
                R"SHADER(
                precision highp float;
                varying vec4 vColor;
                void main() {
                    gl_FragColor = vColor;
                }
                )SHADER"
            );
        glit::Program prog(std::move(vs), std::move(fs), {
                    glit::Program::MakeInput<glm::mat4>("uModelViewProj"),
                });
        return new Planet(std::move(prog));
    }

    void tick(double t, double dt) override {
        rotation += dt;
        while (rotation > 2.0 * M_PI)
            rotation -= 2.0 * M_PI;
    }

    void draw(const Camera& camera) override {
        auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
        model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        auto modelviewproj = camera.transform() * model;

        auto primitive = terrain.uploadAsWireframe();
        progWireframe.run(primitive, modelviewproj);
    }
};

struct WorldState
{
    // The camera state.
    Camera camera;

    // Crappiest scene graph ever.
    struct DrawSet {
        glit::Program program;
        glit::PrimitiveData primitive;
    };
    std::vector<DrawSet> scene;

    std::vector<Entity*> entities;

    ~WorldState() {
        for (auto* e : entities)
            delete e;
        entities.clear();
    }
};
static WorldState gWorld;

void
do_loop()
{
    static double lastFrameTime = 0.0;
    double now = glfwGetTime();

    glClearColor(0, 0, 0, 255);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto* e : gWorld.entities)
        e->tick(now, now - lastFrameTime);

    auto cameraMatrix = gWorld.camera.transform();

    for (auto& ds : gWorld.scene) {
        auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
        model = glm::rotate(model, float(now), glm::vec3(0.0f, 1.0f, 0.0f));
        auto modelviewproj = cameraMatrix * model;
        ds.program.run(ds.primitive, modelviewproj);
    }

    for (auto* e : gWorld.entities)
        e->draw(gWorld.camera);

    gWindow.swap();
    lastFrameTime = now;
}

void setup_perspective()
{
    glit::IcoSphere sphere(3);
    auto spherePrim = sphere.uploadAsWireframe();
    glit::VertexShader vsSphere(
            R"SHADER(
            precision highp float;
            uniform mat4 uModelViewProj;
            attribute vec3 aPosition;
            varying vec4 vColor;
            void main()
            {
                gl_Position = uModelViewProj * vec4(aPosition, 1.0);
                vColor = vec4(255, 255, 255, 255);
            }
            )SHADER",
            spherePrim.vertexBuffer().vertexDesc());
    glit::FragmentShader fsSphere(
            R"SHADER(
            precision highp float;
            varying vec4 vColor;
            void main() {
                gl_FragColor = vColor;
            }
            )SHADER"
        );
    glit::Program progSphere(std::move(vsSphere), std::move(fsSphere), {
                glit::Program::MakeInput<glm::mat4>("uModelViewProj"),
            });
    gWorld.scene.push_back(WorldState::DrawSet{std::move(progSphere),
                                               std::move(spherePrim)});
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

    gWorld.entities.push_back(Planet::create());
    setup_perspective();


#ifndef __EMSCRIPTEN__
    while (!gWindow.isDone())
        do_loop();
#else
    emscripten_set_main_loop(do_loop, 60, 1);
#endif

    return 0;
}

int main()
{
#ifdef __EMSCRIPTEN__
    try {
        do_main();
    } catch(std::runtime_error& sre) {
        // Empscripten does not handle exceptions at the top level, so be sure
        // to print them out manually.
        std::cerr << "Runtime error: " << sre.what() << std::endl;
    }
#else
    // Otherwise, let the platform catch it.
    do_main();
#endif // __EMSCRIPTEN__
}

