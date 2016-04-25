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
#include "player.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include <glm/vec2.hpp>

#include "planet.h"

using namespace glm;
using namespace std;

glit::Player::Player(std::shared_ptr<Planet>& p)
  : planet(p)
  , motionReq(0.f, 0.f, 0.f)
  , rotateReq(0.f, 0.f, 0.f)
  , rotateAxis(0.f, 0.f)
  , speed(1.0)
{
    // FIXME: currently we're just setting ourself at 0,0 lat/lon.
    // I'm not sure what we actually want to do to initialize the
    // player character.
    auto& terrain = p->terrain();

    dvec3 initial(0.f, 0.f, 1.f);
    pos = initial * (terrain.heightAt(initial) + 1000.0);

    // pointing north at the equator.
    dir = angleAxis(float(M_PI) / 2.f, vec3(1.f, 0.f, 0.f));
}

void
glit::Player::tick(double t, double dt)
{
    // Apply rotation requested via keyboard buttons.
    dquat qX = angleAxis(rotateReq[0] * dt / 2.0, dvec3(1.f, 0.f, 0.f));
    dquat qY = angleAxis(rotateReq[1] * dt / 2.0, dvec3(0.f, 1.f, 0.f));
    dquat qZ = angleAxis(rotateReq[2] * dt / 2.0, dvec3(0.f, 0.f, 1.f));
    dir = dir * qX * qY * qZ;

    // Apply rotation requested via mouse movement.
    dquat qXm = angleAxis(rotateAxis[0] * 0.001, dvec3(1.f, 0.f, 0.f));
    dquat qYm = angleAxis(rotateAxis[1] * 0.001, dvec3(0.f, 1.f, 0.f));
    dir = dir * qXm * qYm;
    rotateAxis[0] = 0.f;
    rotateAxis[1] = 0.f;

    // Apply button motion requests.
    if (length(motionReq) != 0.f)
        pos += speed * (dir * normalize(dvec3(motionReq)));
}

void
glit::Player::draw(const glit::Camera& camera)
{
}
