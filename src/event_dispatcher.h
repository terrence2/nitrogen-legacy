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
    using CallbackType = std::function<void()>;
    using CallbacksType = std::vector<CallbackType>;
    using ValueType = std::pair<EventType, CallbacksType>;
    using RegistryType = std::unordered_map<EventType, CallbacksType>;
    RegistryType events;

  public:
    EventDispatcher() {}
    bool hasEventNamed(std::string event) const;
    void observe(std::string event, CallbackType func);
    void notify(std::string event) const;
};

} // namespace glit
