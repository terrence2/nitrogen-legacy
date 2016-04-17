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

using namespace std;

bool
glit::EventDispatcher::hasEventNamed(string event) const
{
    return bool(edgeHandlers.count(event)) ||
           bool(levelHandlers.count(event));
}

void
glit::EventDispatcher::onEdge(string event, EdgeCallbackType func)
{
    auto iter = edgeHandlers.find(event);
    if (iter == edgeHandlers.end()) {
        auto entry = EdgeValueType(event, EdgeCallbacksType());
        auto rv = edgeHandlers.insert(entry);
        if (!rv.second)
            throw runtime_error("failed to insert observer");
        iter = rv.first;
    }
    iter->second.push_back(func);
}

void
glit::EventDispatcher::onLevel(string event, LevelCallbackType func)
{
    auto iter = levelHandlers.find(event);
    if (iter == levelHandlers.end()) {
        auto entry = LevelValueType(event, LevelCallbacksType());
        auto rv = levelHandlers.insert(entry);
        if (!rv.second)
            throw runtime_error("failed to insert observer");
        iter = rv.first;
    }
    iter->second.push_back(func);
}

void
glit::EventDispatcher::notifyEdge(string event) const
{
    auto pair = edgeHandlers.find(event);
    if (pair == edgeHandlers.end()) {
        cout << "trying to dispatch to nonexistant event " << event << endl;
        return;
    }

    cout << "Observed event " << event << endl;
    for (auto func : pair->second)
        func();
}

void
glit::EventDispatcher::notifyLevel(string event, double level, double change) const
{
    cout << "level is: " << level << " d " << change << endl;

    auto pair = levelHandlers.find(event);
    if (pair == levelHandlers.end()) {
        cout << "trying to dispatch to nonexistant event " << event << endl;
        return;
    }

    cout << "Observed event " << event << endl;
    for (auto func : pair->second)
        func(level, change);
}
