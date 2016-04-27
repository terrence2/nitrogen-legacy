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
#include "planet.h"

#include <memory>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "player.h"
#include "sun.h"

using namespace glm;
using namespace std;

glit::Planet::Planet(shared_ptr<Sun>& s)
  : terrain_(6371000.0) // m
  , sun(s)
{}

glit::Planet::~Planet()
{
}

void
glit::Planet::setPlayer(std::shared_ptr<Player>& p)
{
    player = p;
}

void
glit::Planet::tick(double t, double dt)
{
    // Note: the sun goes around us so that we don't have to worry about
    // numerical stability or multiply everything into this frame of reference.
}

void
glit::Planet::draw(const glit::Camera& camera)
{
    auto sunp = sun.lock();
    if (!sunp)
        throw runtime_error("no sun pointer in terrain draw");

    auto playerp = player.lock();
    if (!playerp)
        throw runtime_error("no player pointer in terrain draw");

    terrain_.draw(camera, sunp->sunDirection());
    /*
    auto pos = playerp->viewPosition();
    auto dir = playerp->viewDirection();

    auto modelviewproj = camera.transform();

    const_cast<Camera&>(camera).warp(vec3(0.f, 0.f, 0.f),
                       playerp->viewDirection(),
                       playerp->viewUp());

    //auto mesh = terrain_.uploadAsWireframe(pos, camera.viewDirection());
    auto mesh = terrain_.uploadAsTriStrips(pos, camera.viewDirection());
    //auto mesh = terrain_.uploadAsTriStrips(vec3(0.f, 6300.f, 0.f), vec3(0.f, 0.f, 0.f));
    mesh->draw(modelviewproj,
               vec3(pos),
               sunp->sunDirection(),
               float(terrain().radius()));
    */
}
