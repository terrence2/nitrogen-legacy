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
#pragma once

#include <unordered_map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "bindings.h"
#include "utility.h"

namespace glit {

class Window
{
    enum class State {
        PreInit = 0,
        Inited = 1,
        Done = 2
    } state;
    InputBindings* bindings;
    GLFWwindow* window;
    int width_;
    int height_;
    double lastMouse[2];

  public:
    Window();
    ~Window();
    void init();

    int width() const { return width_; }
    int height() const { return height_; }

    void quit() { state = State::Done; }
    bool isDone() const { return state == State::Done; }

    void setCurrentBindings(InputBindings& inputs) { bindings = &inputs; }
    void swap();

    static Window* fromGLFW(GLFWwindow* window);
    static void errorCallback(int error, const char* description);
    static void keyCallback(GLFWwindow* window, int key, int scancode,
                            int action, int mods);
    static void cursorPositionCallback(GLFWwindow* window,
                                       double xpos, double ypos);
    static void windowCloseCallback(GLFWwindow* window);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);
};

} // namespace glit
