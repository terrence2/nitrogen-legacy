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

glit::InputBindings::InputBindings(const EventDispatcher& eventDispatcher,
                                   const std::string& name_)
  : dispatcher(eventDispatcher), name(name_)
{
}

void
glit::InputBindings::dispatchKeyEvent(int key, int scancode, int action,
                                      int mods) const
{
    if (keyboard[key].isBound() && keyboard[key].hasModifier(mods)) {
        const char* state = action == 1 ? "+" : "-";
        dispatcher.notify(state + keyboard[key].event());
    }
}

void
glit::InputBindings::bindNamedKey(std::string event, int key,
                                  int mods /* = 0 */)
{
    if (!dispatcher.hasEventNamed("+" + event) &&
        !dispatcher.hasEventNamed("-" + event))
    {
        throw std::runtime_error("Cannot bind to unknown event " + event);
    }
    keyboard[key] = KeyboardKeyEvent(event, mods);
}
