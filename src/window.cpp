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

#ifdef __EMSCRIPTEN__
# include <emscripten.h>
# include <html5.h>
#endif
#include <iostream>

using namespace std;

// Uncomment to monitor GL.
//#define LOG_GL_CALLS

#ifdef GLAD_DEBUG
static void gl_debug_nop(const char *name, void *funcptr, int len_args, ...) {}

static void
pre_gl_call(const char *name, void *funcptr, int len_args, ...)
{
#ifdef LOG_GL_CALLS
    printf("Calling: %s (%d arguments)\n", name, len_args);
#endif
}
static void
post_gl_call(const char *name, void *funcptr, int len_args, ...)
{
    glad_set_pre_callback(gl_debug_nop);
    glad_set_post_callback(gl_debug_nop);
    glit::GLCheckError();
    glad_set_pre_callback(pre_gl_call);
    glad_set_post_callback(post_gl_call);
}
#endif

glit::Window::Window()
  : window(nullptr)
  , state(State::PreInit)
  , bindings(nullptr)
  , width_(DefaultWidth)
  , height_(DefaultHeight)
{}

void
glit::Window::init(InputBindings& inputs)
{
    // Ensure that bindings are set before we can start delivering events.
    bindings = &inputs;

    if (state != State::PreInit)
        throw runtime_error("Window already inited");

    if (!glfwInit())
        throw runtime_error("glfwInit failed");
    state = State::Inited;

    // Tie into error handling.
    glfwSetErrorCallback(errorCallback);

    // Ensure we get a webgl compatible context.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

    // Use "windowed fullscreen" by getting the current settings.
    GLFWmonitor* monitor = nullptr;
#ifndef __EMSCRIPTEN__
    monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    width_ = mode->width;
    height_ = mode->height;
#endif
    window = glfwCreateWindow(width_, height_, "fsim", monitor, NULL);
    if (!window)
        throw runtime_error("glfwCreateWindow failed");

    // Query the actual size we got.
    glfwGetWindowSize(window, &width_, &height_);

    // Listen for events.
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowCloseCallback(window, windowCloseCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    // Query the initial cursor position so that dx/dy are sane
    // on our first update.
    glfwGetCursorPos(window, &lastMouse_[0], &lastMouse_[1]);

    // Do late binding of GL primitives.
    glfwMakeContextCurrent(window);
#ifndef __EMSCRIPTEN__
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

# ifdef GLAD_DEBUG
    // Print all GL calls so that we can pinpoint any errors.
    glad_set_pre_callback(pre_gl_call);
    glad_set_post_callback(post_gl_call);
# endif // GLAD_DEBUG
#else
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        throw runtime_error("GLEW init failed: " +
                            string((const char*)glewGetErrorString(err)));
    }
#endif

#define PRINT_GL_STRING(key) \
    cout << #key << ": " << glGetString(key) << endl;
FOR_EACH_GL_STRINGS(PRINT_GL_STRING)
#undef PRINT_GL_STRING
    // we need GL_OES_element_index_uint
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
    self->bindings->dispatchKeyEvent(key, scancode, action, mods);
}

/* static */ void
glit::Window::cursorPositionCallback(GLFWwindow* window,
                                     double xpos, double ypos)
{
    Window* self = fromGLFW(window);
    self->bindings->dispatchMouseMotion(xpos,
                                        ypos,
                                        self->lastMouse_[0] - xpos,
                                        self->lastMouse_[1] - ypos);
    self->lastMouse_[0] = xpos;
    self->lastMouse_[1] = ypos;
}

/* static */ void
glit::Window::mouseScrollCallback(GLFWwindow* window,
                                  double xpos, double ypos)
{
    Window* self = fromGLFW(window);
    self->bindings->dispatchMouseScroll(xpos, ypos);
}

/* static */ void
glit::Window::windowCloseCallback(GLFWwindow* window)
{
    Window* self = fromGLFW(window);
    self->state = State::Done;
}

void
glit::Window::notifySizeChanged(SizeChangedCallback cb)
{
    windowSizeChangedCallbacks.push_back(cb);
    cb(width(), height());
}

/* static */ void
glit::Window::windowSizeCallback(GLFWwindow* window, int width, int height)
{
    Window* self = fromGLFW(window);
    cout << "Window size changed to: " << width << " x " << height << endl;
    self->width_ = width;
    self->height_ = height;
    glViewport(0, 0, width, height);

    for (auto& cb : self->windowSizeChangedCallbacks)
        cb(width, height);
}
