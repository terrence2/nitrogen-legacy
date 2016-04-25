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

#include <memory>

#include "camera.h"
#include "entity.h"
#include "terrain.h"

namespace glit {

class Player;
class Sun;

class Planet : public Entity
{
    // The terrain state. Updated by providing the camera in draw.
    Terrain terrain_;

    // The current rotational state.
    float rotation;

    // Reference to the sun.
    std::weak_ptr<Sun> sun;

    // Reference to the player for position info.
    std::weak_ptr<Player> player;

    explicit Planet(Planet&&) = delete;
    explicit Planet(const Planet&) = delete;

  public:
    explicit Planet(std::shared_ptr<Sun>& s);
    ~Planet() override;

    void setPlayer(std::shared_ptr<Player>& p);

    const Terrain& terrain() const { return terrain_; }

    void tick(double t, double dt) override;
    void draw(const Camera& camera) override;
};

} // namespace glit
