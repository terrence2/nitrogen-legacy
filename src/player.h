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

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "camera.h"
#include "entity.h"

namespace glit {

class Planet;

class Player : public Entity
{
    // Gives us gravity and something to push off of.
    std::weak_ptr<Planet> planet;

    // State.
    glm::dvec3 pos; // head location
    glm::dquat dir; // view direction

    // Motion request from keyboard.
    glm::vec3 motionReq;
    glm::vec3 rotateReq;
    glm::vec2 rotateAxis;
    double speed;

  public:
    Player(std::shared_ptr<Planet>& p);

    const glm::dvec3& viewPosition() const { return pos; }
    glm::vec3 viewDirection() const { return glm::vec3(dir * glm::dvec3(0.f, 0.f, -1.f)); }
    glm::vec3 viewUp() const { return glm::vec3(dir * glm::dvec3(0.f, 1.f, 0.f)); }

    void nullIfExpected(float& ref, float expect) {
        if (ref == expect)
            ref = 0.f;
    }
    void ufoStartLeft() { motionReq[0] = -1.f; }
    void ufoStopLeft() { nullIfExpected(motionReq[0], -1.f); }
    void ufoStartRight() { motionReq[0] = 1.f; }
    void ufoStopRight() { nullIfExpected(motionReq[0], 1.f); }
    void ufoStartForward() { motionReq[2] = -1.f; }
    void ufoStopForward() { nullIfExpected(motionReq[2], -1.f); }
    void ufoStartBackward() { motionReq[2] = 1.f; }
    void ufoStopBackward() { nullIfExpected(motionReq[2], 1.f); }
    void ufoStartUp() { motionReq[1] = 1.f; }
    void ufoStopUp() { nullIfExpected(motionReq[1], 1.f); }
    void ufoStartDown() { motionReq[1] = -1.f; }
    void ufoStopDown() { nullIfExpected(motionReq[1], -1.f); }

    void ufoStartRotateDown() { rotateReq[0] = -1.f; }
    void ufoStopRotateDown() { nullIfExpected(rotateReq[0], -1.f); }
    void ufoStartRotateUp() { rotateReq[0] = 1.f; }
    void ufoStopRotateUp() { nullIfExpected(rotateReq[0], 1.f); }
    void ufoStartRotateLeft() { rotateReq[1] = 1.f; }
    void ufoStopRotateLeft() { nullIfExpected(rotateReq[1], 1.f); }
    void ufoStartRotateRight() { rotateReq[1] = -1.f; }
    void ufoStopRotateRight() { nullIfExpected(rotateReq[1], -1.f); }
    void ufoStartRotateCCW() { rotateReq[2] = 1.f; }
    void ufoStopRotateCCW() { nullIfExpected(rotateReq[2], 1.f); }
    void ufoStartRotateCW() { rotateReq[2] = -1.f; }
    void ufoStopRotateCW() { nullIfExpected(rotateReq[2], -1.f); }

    constexpr static double MaxSpeed = 10000.0;
    constexpr static double MinSpeed = 0.25;
    void ufoAccelerate() { if (speed < MaxSpeed) speed *= 2.f; }
    void ufoDecelerate() { if (speed > MinSpeed) speed /= 2.f; }

    // The request is in angleAxis form, so pitch and yaw are on converse axes.
    void ufoYawDelta(double dyaw) { rotateAxis[1] += dyaw; }
    void ufoPitchDelta(double dpitch) { rotateAxis[0] += dpitch; }

    void tick(double t, double dt) override;
    void draw(const Camera& camera) override;
};

} // namespace glit
