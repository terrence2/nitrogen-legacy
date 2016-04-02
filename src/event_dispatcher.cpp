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
#include "event_dispatcher.h"

#include <iostream>

bool
glit::EventDispatcher::hasEventNamed(std::string event) const
{
    return bool(events.count(event));
}

void
glit::EventDispatcher::observe(std::string event, CallbackType func)
{
    auto iter = events.find(event);
    if (iter == events.end()) {
        auto entry = ValueType(event, CallbacksType());
        auto rv = events.insert(entry);
        if (!rv.second)
            throw std::runtime_error("failed to insert observer");
        iter = rv.first;
    }
    iter->second.push_back(func);
}

void
glit::EventDispatcher::notify(std::string event) const
{
    auto pair = events.find(event);
    if (pair == events.end()) {
        std::cout << "trying to dispatch to nonexistant event " <<
            event << std::endl;
        return;
    }

    std::cout << "Observed event " << event << std::endl;
    for (auto func : pair->second)
        func();
}
