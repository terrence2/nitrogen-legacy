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

#include <stdio.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

#include "bindings.h"
#include "shader.h"
#include "window.h"


// Emscripten is getting its callbacks from the browsers refresh driver, so
// does not have enough context to pass us an argument to the callback.
static glit::Window gWindow;

void
do_loop()
{
    glClearColor(255, 0, 255, 255);
    glClear(GL_COLOR_BUFFER_BIT);
    gWindow.swap();
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


    glit::VertexShader vsTri(
        "attribute vec4 a_position;              \n"
        "attribute vec4 a_color;                 \n"
        "uniform float u_time;                   \n"
        "varying vec4 v_color;                   \n"
        "void main()                             \n"
        "{                                       \n"
        "    float sz = sin(u_time);             \n"
        "    float cz = cos(u_time);             \n"
        "    mat4 rot = mat4(                    \n"
        "     cz, -sz, 0,  0,                    \n"
        "     sz,  cz, 0,  0,                    \n"
        "     0,   0,  1,  0,                    \n"
        "     0,   0,  0,  1                     \n"
        "    );                                  \n"
        "    gl_Position = a_position * rot;     \n"
        "    v_color = a_color;                  \n"
        "}                                       \n"
        );
    glit::FragmentShader fsTri(
        "precision mediump float;                \n"
        "varying vec4 v_color;                   \n"
        "void main()                             \n"
        "{                                       \n"
        "    gl_FragColor = v_color;             \n"
        "}                                       \n"
        );

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

