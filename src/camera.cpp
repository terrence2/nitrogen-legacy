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
#include "camera.h"

#include <iostream>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utility.h"

using namespace glm;
using namespace std;

glit::Camera::Camera()
{
    // Default camera is at world origin pointing down -z.
    position = vec3(0.f, 0.f, 0.f);
    direction = vec3(0.f, 0.f, -1.f);
    up = vec3(0.f, 1.f, 0.f);
    projection = perspective(pi<float>() * 0.25f, 1.f,
                             NearDistance, FarDistance);
}

glit::Camera::Camera(const Camera& other)
  : projection(other.projection)
  , position(other.position)
  , direction(other.direction)
  , up(other.up)
{}

void
glit::Camera::screenSizeChanged(float width, float height)
{
    projection = perspective(pi<float>() * 0.25f, width / height,
                             NearDistance, FarDistance);
}

void
glit::Camera::move(const vec3& pos)
{
    position = pos;
}

void
glit::Camera::warp(const vec3& pos, const vec3& dir, const vec3& u)
{
    position = pos;
    direction = dir;
    up = u;
}

// Combined projection and view, suitable for multiplying with a model
// to get a final transform matrix.
mat4
glit::Camera::transform() const
{
    auto view = lookAt(position, position + direction, up);
    return projection * view;
}
