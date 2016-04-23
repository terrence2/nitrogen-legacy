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

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>

#include "event_dispatcher.h"

namespace glit {

// Maps from a key or scancode to an event name. A game may create many
// DeviceBindings for different situations: menu usage vs walking vs driving,
// etc.
class InputBindings
{
  public:
    InputBindings(const EventDispatcher& eventDispatcher,
                  const std::string& name);
    void bindNamedKey(std::string event, int key, int mods = -1);
    void bindButton(std::string event, int axis);
    void bindMouseAxis(std::string event, int axis);

    enum class MouseScrollAxis {
        Up = 1,
        Down = -1,
        Left = -2,
        Right = 2
    };
    void bindMouseScroll(std::string event, MouseScrollAxis axis);

    // TODO: Have not had a need yet.
    //void bindScancode(std::string event, int scancode, int mods = -1);

  private:
    InputBindings(const InputBindings&) = delete;
    InputBindings(InputBindings&&) = delete;
    InputBindings& operator=(const InputBindings&) = delete;
    InputBindings& operator=(InputBindings&&) = delete;

    const EventDispatcher& dispatcher;
    const std::string name;

    friend class Window;
    void dispatchKeyEvent(int key, int scancode, int action, int mods) const;
    void dispatchMouseMotion(double x, double y, double dx, double dy) const;
    void dispatchMouseScroll(double x, double y) const;

    class KeyboardKeyEvent
    {
        std::string event_;
        int mods;

      public:
        KeyboardKeyEvent() : event_(""), mods(-1) {}
        KeyboardKeyEvent(std::string e, int m) : event_(e), mods(m) {}

        bool isBound() const { return event_ != ""; }
        bool hasModifier(int want) const {
            return mods == -1 || mods == want;
        }
        std::string event() const { return event_; }
    };
    KeyboardKeyEvent keyboard[GLFW_KEY_LAST];

    class MouseMotionEvent
    {
        std::string event_;
      public:
        MouseMotionEvent() : event_("") {}
        explicit MouseMotionEvent(std::string e) : event_(e) {}

        bool isBound() const { return event_ != ""; }
        std::string event() const { return event_; }
    };
    MouseMotionEvent mouseMotion[2];

    class MouseScrollEvent
    {
        std::string event_;
      public:
        MouseScrollEvent() : event_("") {}
        explicit MouseScrollEvent(std::string e) : event_(e) {}

        bool isBound() const { return event_ != ""; }
        std::string event() const { return event_; }
    };
    MouseScrollEvent mouseScroll_[4];
    const MouseScrollEvent& scrollEvent(MouseScrollAxis axis) const;
    MouseScrollEvent& scrollEvent(MouseScrollAxis axis);
};

} // namespace glit
