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

#include <vector>

#include <glm/vec3.hpp>

#include "vertex.h"

namespace glit {

class IcoSphere
{
  public:
    struct Vertex {
        glm::vec3 aPosition;
        static void describe(std::vector<VertexAttrib>& attribs) {
            attribs.push_back(MakeGLMVertexAttrib(Vertex, aPosition, false));
        }
    };

    IcoSphere(int iterations);
    PrimitiveData uploadAsPoints() const;
    PrimitiveData uploadAsWireframe() const;

  private:
    using Face = std::tuple<uint16_t, uint16_t, uint16_t>;
    using Faces = std::vector<Face>;

    glm::vec3 bisectEdge(glm::vec3& v0, glm::vec3& v1);

    std::vector<Vertex> verts;
    Faces faces;
};

} // namespace glit
