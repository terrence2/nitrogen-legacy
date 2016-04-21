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

#include <glm/gtc/quaternion.hpp>

#include "entity.h"
#include "mesh.h"

namespace glit {

// NOTE! In our simulation the sun goes around the earth.
class Sun : public Entity
{
    // FIXME: note that we've sped this up to see lighting in action.
    //constexpr static double AngularVelocity = (2.0 * M_PI) / (24 * 60 * 60);
    constexpr static double AngularVelocity = 5000.0 * (2.0 * M_PI) / (24 * 60 * 60);

    Mesh mesh;  // Mostly for visualization.
    double ang;

  public:
    explicit Sun(Mesh&& m);

    // Rays from the sun are coming from this direction. We assume all rays are
    // parallel, given the distance.
    glm::vec3 sunDirection() const {
        glm::quat q = angleAxis(float(ang), glm::vec3(0.f, 1.f, 0.f));
        glm::vec3 dir = q * glm::vec3(0.f, 0.f, 1.f);
        return -dir;
    }

    static std::shared_ptr<Sun> create();
    void tick(double t, double dt) override;
    void draw(const Camera& camera) override;
};

} // namespace glit
