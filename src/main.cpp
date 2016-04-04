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

#include <stdio.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

#include "polygon.h"
#include "bindings.h"
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
    glClearColor(255, 0, 255, 255);
    glClear(GL_COLOR_BUFFER_BIT);

    gProgram->run(*gTris, float(glfwGetTime()));

    gWindow.swap();
}

struct MyVertex {
    float a_position[3];
    uint8_t a_color[4];

    static void describe(std::vector<glit::VertexAttrib>& attribs) {
        attribs.push_back(MakeVertexAttrib(MyVertex, a_position, false));
        attribs.push_back(MakeVertexAttrib(MyVertex, a_color, true));
    }
};

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

    auto MyVertexDesc = glit::VertexDescriptor::fromType<MyVertex>();
    glit::VertexBuffer tris(MyVertexDesc);
    const MyVertex verts[] {
        { { 0.f,  .5f, 0.f},   {255,   0,   0, 255} },
        { {-.5f, -.5f, 0.f},   {  0, 255,   0, 255} },
        { { .5f, -.5f, 0.f},   {  0,   0, 255, 255} }
    };
    tris.buffer(sizeof(verts), (void*)verts);
    std::cout << "sizeof verts: " << sizeof(verts) << std::endl;

    glit::VertexShader vsTri(
            R"SHADER(
            precision highp float;
            uniform float u_time;
            attribute vec4 a_position;
            attribute vec4 a_color;
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
                gl_Position = a_position * rot;
                v_color = a_color;
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
    glit::Program::Input input("u_time", GL_FLOAT);
    glit::Program prog(vsTri, fsTri, {input});

    gProgram = &prog;
    gTris = &tris;

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

