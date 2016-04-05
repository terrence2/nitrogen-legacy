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
#include "vertex.h"
#include "window.h"


// Emscripten is getting its callbacks from the browsers refresh driver, so
// does not have enough context to pass us an argument to the callback.
static glit::Window gWindow;

static glit::Program* gProgram;
static glit::VertexBuffer* gTris;

void
do_loop()
{
    glClearColor(0, 0, 0, 255);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto projection = glm::perspective(glm::pi<float>() * 0.25f, 4.0f / 3.0f,
                                       0.1f, 100.f);
    auto view = glm::mat4(1.0f);
    auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
    model = glm::rotate(model, float(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
    auto modelviewproj = projection * view * model;

    gProgram->run(*gTris, modelviewproj);

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

    glit::IcoSphere sphere(0);
    //glit::VertexBuffer sphereBuf = sphere.uploadPoints();
    auto SphereVertexDesc = glit::VertexDescriptor::fromType<
                                    glit::IcoSphere::Vertex>();
    glit::VertexBuffer sphereGpuVerts(SphereVertexDesc);
    sphereGpuVerts.upload(GL_LINE_STRIP, sphere.verts);
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
            SphereVertexDesc);
    glit::FragmentShader fsSphere(
            R"SHADER(
            precision highp float;
            varying vec4 vColor;
            void main() {
                gl_FragColor = vColor;
            }
            )SHADER"
        );
    glit::Program progSphere(vsSphere, fsSphere, {
                glit::Program::MakeInput<glm::mat4>("uModelViewProj"),
            });

    gProgram = &progSphere;
    gTris = &sphereGpuVerts;

#if 0
    auto MyVertexDesc = glit::VertexDescriptor::fromType<MyVertex>();
    glit::VertexBuffer tris(MyVertexDesc);
    const std::vector<MyVertex> triVerts{
        { { 0.f,  .5f, 0.f},   {255,   0,   0, 255} },
        { {-.5f, -.5f, 0.f},   {  0, 255,   0, 255} },
        { { .5f, -.5f, 0.f},   {  0,   0, 255, 255} }
    };
    tris.upload(GL_TRIANGLES, triVerts);

    glit::VertexShader vsTri(
            R"SHADER(
            precision highp float;
            uniform float u_time;
            attribute vec4 aPosition;
            attribute vec4 aColor;
            varying vec4 v_color;
            void main()
            {
                float sz = sin(u_time);
                float cz = cos(u_time);
                mat4 rot = mat4(
                    cz, -sz, 0,  0,
                    sz,  cz, 0,  0,
                    0,   0,  1,  0,
                    0,   0,  0,  1
                );
                gl_Position = aPosition * rot;
                v_color = aColor;
            }
            )SHADER",
            MyVertexDesc
            /*,
            VaryingVec("v_color")
            */
        );
    glit::FragmentShader fsTri(
            R"SHADER(
            precision highp float;
            varying vec4 v_color;
            void main() {
                gl_FragColor = v_color;
            }
            )SHADER"
            /*,
            VaryingVec("v_color")
            */
        );
    glit::Program prog(vsTri, fsTri, {glit::Program::MakeInput<float>("u_time")});

    gProgram = &prog;
    gTris = &tris;
#endif

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

