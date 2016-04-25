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

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace glit {

class Camera
{
    // I think relatively constant. Can certainly be updated by settings
    // changing the FOV. Probably also will need to tweak this as we render
    // so that we can represent things that are very far away while still
    // getting good z clipping up close.
    glm::mat4 projection;

    // Updated constantly by input events.
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;

    constexpr static float NearDistance = 1.f;
    constexpr static float FarDistance = 1000000.f;

    Camera(Camera&&) = delete;
    Camera(const Camera&) = delete;

  public:
    Camera();

    const glm::vec3& viewPosition() const { return position; }
    const glm::vec3& viewDirection() const { return direction; }
    const glm::vec3& viewUp() const { return up; }

    void screenSizeChanged(float width, float height);
    void warp(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& u);

    // Combined projection and view, suitable for multiplying with a model
    // to get a final transform matrix.
    glm::mat4 transform() const;
};

} // namespace glit
