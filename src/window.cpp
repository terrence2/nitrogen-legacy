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

#include "window.h"

#include <iostream>

glit::Window::Window()
  : state(State::PreInit)
  , bindings(nullptr)
{}

void
glit::Window::init()
{
    if (state != State::PreInit)
        throw std::runtime_error("Window already inited");

    if (!glfwInit())
        throw std::runtime_error("glfwInit failed");
    state = State::Inited;

    // Tie into error handling.
    glfwSetErrorCallback(errorCallback);

    // Ensure we get a webgl compatible context.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

    // Use "windowed fullscreen" by getting the current settings.
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    window = glfwCreateWindow(mode->width, mode->height, "AFS", monitor, NULL);
    if (!window)
        throw std::runtime_error("glfwCreateWindow failed");

    // Query the actual size we got.
    glfwGetWindowSize(window, &width_, &height_);

    // Listen for events.
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowCloseCallback(window, windowCloseCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    glfwMakeContextCurrent(window);

#define PRINT_GL_STRING(key) \
    std::cout << #key << ": " << glGetString(key) << std::endl;
FOR_EACH_GL_STRINGS(PRINT_GL_STRING)
#undef PRINT_GL_STRING
}

glit::Window::~Window()
{
    if (window)
        glfwDestroyWindow(window);
    if (state != State::PreInit)
        glfwTerminate();
}

/* static */ glit::Window*
glit::Window::fromGLFW(GLFWwindow* window)
{
    return static_cast<Window*>(glfwGetWindowUserPointer(window));
}

void
glit::Window::swap()
{
    glfwSwapBuffers(window);
    glfwPollEvents();
}

/* static */ void
glit::Window::errorCallback(int error, const char* description)
{
    // Fixme: this should probably throw to fit in with our other error
    // handling.
    fputs(description, stderr);
}

/* static */ void
glit::Window::keyCallback(GLFWwindow* window, int key, int scancode,
                          int action, int mods)
{
    Window* self = fromGLFW(window);
    printf("key is: %d %d %d %d\n", key, scancode, action, mods);
    self->bindings->dispatchKeyEvent(key, scancode, action, mods);
}

/* static */ void
glit::Window::windowCloseCallback(GLFWwindow* window)
{
    Window* self = fromGLFW(window);
    self->state = State::Done;
}

/* static */ void
glit::Window::windowSizeCallback(GLFWwindow* window, int width, int height)
{
    Window* self = fromGLFW(window);
    std::cout << "Window size changed to: " << width << " x " << height <<
                 std::endl;
    self->width_ = width;
    self->height_ = height;
    glViewport(0, 0, width, height);
}
