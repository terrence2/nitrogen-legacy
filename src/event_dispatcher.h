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

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace glit {

class EventDispatcher
{
    using EventType = std::string;

    using EdgeCallbackType = std::function<void()>;
    using EdgeCallbacksType = std::vector<EdgeCallbackType>;
    using EdgeValueType = std::pair<EventType, EdgeCallbacksType>;
    using EdgeRegistryType = std::unordered_map<EventType, EdgeCallbacksType>;
    EdgeRegistryType edgeHandlers;

    using LevelCallbackType = std::function<void(double, double)>;
    using LevelCallbacksType = std::vector<LevelCallbackType>;
    using LevelValueType = std::pair<EventType, LevelCallbacksType>;
    using LevelRegistryType = std::unordered_map<EventType, LevelCallbacksType>;
    LevelRegistryType levelHandlers;

  public:
    EventDispatcher() {}
    bool hasEventNamed(std::string event) const;
    void onEdge(std::string event, EdgeCallbackType func);
    void onLevel(std::string event, LevelCallbackType func);
    void notifyEdge(std::string event) const;
    void notifyLevel(std::string event, double level, double change) const;
};

} // namespace glit
