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
#include "bindings.h"

#include <iostream>

using namespace std;

glit::InputBindings::InputBindings(const EventDispatcher& eventDispatcher,
                                   const string& name_)
  : dispatcher(eventDispatcher), name(name_)
{
}

const glit::InputBindings::MouseScrollEvent&
glit::InputBindings::scrollEvent(MouseScrollAxis axis) const
{
    return const_cast<InputBindings*>(this)->scrollEvent(axis);
}

glit::InputBindings::MouseScrollEvent&
glit::InputBindings::scrollEvent(MouseScrollAxis axis)
{
    switch (axis) {
    case MouseScrollAxis::Left:  return mouseScroll_[0];
    case MouseScrollAxis::Down:  return mouseScroll_[1];
    case MouseScrollAxis::Up:    return mouseScroll_[2];
    case MouseScrollAxis::Right: return mouseScroll_[3];
    default:
        throw runtime_error("invalid axis passed to scrollEvent");
    }
}

void
glit::InputBindings::dispatchKeyEvent(int key, int scancode, int action,
                                      int mods) const
{
    if (keyboard[key].isBound() && keyboard[key].hasModifier(mods)) {
        const char* edge = action == 1 || action == 2 ? "+" : "-";
        dispatcher.notifyEdge(edge + keyboard[key].event());
    }
}

void
glit::InputBindings::dispatchMouseMotion(double x, double y,
                                         double dx, double dy) const
{
    if (mouseMotion[0].isBound())
        dispatcher.notifyLevel(mouseMotion[0].event(), x, dx);
    if (mouseMotion[1].isBound())
        dispatcher.notifyLevel(mouseMotion[1].event(), y, dy);
}

void
glit::InputBindings::dispatchMouseScroll(double x, double y) const
{
    if (y > 0.0) {
        auto up = scrollEvent(MouseScrollAxis::Up);
        if (up.isBound())
            dispatcher.notifyEdge(up.event());
    } else if (y < 0.0) {
        auto down = scrollEvent(MouseScrollAxis::Down);
        if (down.isBound())
            dispatcher.notifyEdge(down.event());
    }
    if (x > 0.0) {
        auto right = scrollEvent(MouseScrollAxis::Right);
        if (right.isBound())
            dispatcher.notifyEdge(right.event());
    } else if (x < 0.0) {
        auto left = scrollEvent(MouseScrollAxis::Left);
        if (left.isBound())
            dispatcher.notifyEdge(left.event());
    }
}

void
glit::InputBindings::bindNamedKey(string event, int key,
                                  int mods /* = 0 */)
{
    if (!dispatcher.hasEventNamed("+" + event) &&
        !dispatcher.hasEventNamed("-" + event))
    {
        throw runtime_error("Cannot bind to unknown event " + event);
    }
    keyboard[key] = KeyboardKeyEvent(event, mods);
}

void
glit::InputBindings::bindMouseAxis(string event, int axis)
{
    if (axis != 0 && axis != 1)
        throw runtime_error("only mouse position axis 0 and 1 are supported");

    mouseMotion[axis] = MouseMotionEvent(event);
}

void
glit::InputBindings::bindMouseScroll(std::string event, MouseScrollAxis axis)
{
    scrollEvent(axis) = MouseScrollEvent(event);
}
