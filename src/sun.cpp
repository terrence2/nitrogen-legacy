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
#include "sun.h"

#include <glm/gtc/matrix_transform.hpp>

#include "icosphere.h"

using namespace glm;
using namespace std;

glit::Sun::Sun(glit::Mesh&& m)
  : mesh(forward<Mesh>(m))
  , ang(0.0)
{}

/* static */ shared_ptr<glit::Sun>
glit::Sun::create()
{
    IcoSphere sphere(0);
    auto mesh = sphere.uploadAsWireframe();
    return make_shared<Sun>(move(mesh));
}

void
glit::Sun::tick(double t, double dt)
{
    /*
    ang += AngularVelocity * dt;
    while (ang > (2.0 * M_PI))
        ang -= (2.0 * M_PI);
    */
}

void
glit::Sun::draw(const Camera& camera)
{
    quat q = angleAxis(float(ang), vec3(0.f, 1.f, 0.f));
    vec3 dir = q * vec3(0.f, 0.f, 1.f);

    // Ideally we would multiply out to 150,000,000 kilometers and scale the
    // mesh to 696,300 km, but floating point precision would make this
    // comedically jumpy. Instead, we draw out at 10000 and scaled down
    // proportionally in size to maintain roughly the same occlusion ratio.
    vec3 pos = 10000.f * dir;
    float s = 46.42;

    auto model = translate(mat4(1.f), pos);
    model = scale(model, vec3(s, s, s));
    auto mvp = camera.transform() * model;
    mesh.draw(mvp);
}
