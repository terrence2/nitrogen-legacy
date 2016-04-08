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
    glm::mat4 view;

  public:
    Camera() {
        // Default camera is at world origin pointing down -z.
        view = glm::mat4(1.f);
        projection = glm::perspective(glm::pi<float>() * 0.25f, 4.0f / 3.0f, 0.1f, 100.f);
    }

    // Combined projection and view, suitable for multiplying with a model
    // to get a final transform matrix.
    glm::mat4 transform() {
        return projection * view;
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
        //glm::vec3 position;
    };
    std::vector<DrawSet> scene;
};
static WorldState gWorld;

void
do_loop()
{
    glClearColor(0, 0, 0, 255);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto cameraMatrix = gWorld.camera.transform();

    for (auto& ds : gWorld.scene) {
        auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
        model = glm::rotate(model, float(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
        auto modelviewproj = cameraMatrix * model;
        ds.program.run(ds.primitive, modelviewproj);
    }

    gWindow.swap();
}

/*
struct MyVertex {
    float aPosition[3];
    uint8_t aColor[4];

    static void describe(std::vector<glit::VertexAttrib>& attribs) {
        attribs.push_back(MakeVertexAttrib(MyVertex, aPosition, false));
        attribs.push_back(MakeVertexAttrib(MyVertex, aColor, true));
    }
};
*/

void
setup_terrain()
{
    glit::Terrain terrain;
    auto prim = terrain.uploadAsWireframe();
    glit::VertexShader vsTerrain(
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
            prim.vertexBuffer().vertexDesc());
    glit::FragmentShader fsTerrain(
            R"SHADER(
            precision highp float;
            varying vec4 vColor;
            void main() {
                gl_FragColor = vColor;
            }
            )SHADER"
        );
    glit::Program progTerrain(std::move(vsTerrain), std::move(fsTerrain), {
                glit::Program::MakeInput<glm::mat4>("uModelViewProj"),
            });
    gWorld.scene.push_back(WorldState::DrawSet{std::move(progTerrain),
                                               std::move(prim)});
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

    setup_terrain();
    setup_perspective();

    /*
    glit::Terrain terrain;
    auto prim = terrain.uploadAsWireframe();
    glit::VertexShader vsTerrain(
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
            prim.vertexBuffer().vertexDesc());
    glit::FragmentShader fsTerrain(
            R"SHADER(
            precision highp float;
            varying vec4 vColor;
            void main() {
                gl_FragColor = vColor;
            }
            )SHADER"
        );
    glit::Program progTerrain(std::move(vsTerrain), std::move(fsTerrain), {
                glit::Program::MakeInput<glm::mat4>("uModelViewProj"),
            });
    gWorld.scene.push_back(WorldState::DrawSet{std::move(progTerrain),
                                               std::move(prim)});
    */

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

