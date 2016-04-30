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

#include "bindings.h"
#include "glwrapper.h"
#include "utility.h"

namespace glit {

// This class wraps GLFW into a more C++ friendly interface. Currently
// it assumes that only a single window is used.
class Window
{
    // The underlying window that we are wrapping.
    GLFWwindow* window;

    // Main application state.
    enum class State {
        PreInit = 0,
        Inited = 1,
        Done = 2
    } state;

    // The bindings dispatch input events to handlers.
    InputBindings* bindings;

    // Some sorts of hardware events do not map well as "inputs". In these
    // cases we provide a simple callback system.
    using SizeChangedCallback = std::function<void(int, int)>;
    std::vector<SizeChangedCallback> windowSizeChangedCallbacks;

    // Cached hardware state.
    int width_;
    int height_;
    double lastMouse_[2];

  public:
    static const int DefaultWidth = 1280;
    static const int DefaultHeight = 720;

    Window();
    ~Window();
    void init(InputBindings& inputs);

    int width() const { return width_; }
    int height() const { return height_; }

    void quit() { state = State::Done; }
    bool isDone() const { return state == State::Done; }

    void setCurrentBindings(InputBindings& inputs) { bindings = &inputs; }
    void notifySizeChanged(SizeChangedCallback cb);

    void swap();

  private:
    static Window* fromGLFW(GLFWwindow* window);
    static void errorCallback(int error, const char* description);
    static void keyCallback(GLFWwindow* window, int key, int scancode,
                            int action, int mods);
    static void cursorPositionCallback(GLFWwindow* window,
                                       double xpos, double ypos);
    static void mouseScrollCallback(GLFWwindow* window,
                                    double xpos, double ypos);
    static void windowCloseCallback(GLFWwindow* window);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);
};

} // namespace glit
