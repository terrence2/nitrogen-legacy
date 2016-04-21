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
#include <tuple>
#include <vector>

#include <glm/vec3.hpp>

#include "mesh.h"
#include "shader.h"
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
    struct Face {
        int i0, i1, i2;
        glm::vec3 normal;

        Face(int v0, int v1, int v2, const std::vector<Vertex>& verts);
    };
    using Faces = std::vector<Face>;


    IcoSphere(int iterations);
    Mesh uploadAsPoints() const;
    Mesh uploadAsWireframe() const;

    const std::vector<Vertex>& vertices() const { return verts; }
    const Faces faceList() const { return faces; }

    template <size_t Offset>
    const glm::vec3& faceVertexPosition(size_t fc) const {
        return verts[std::get<Offset>(faces[fc])].aPosition;
    }

  private:
    static std::shared_ptr<Program> makePointsProgram();
    std::shared_ptr<Program> programPoints;

    glm::vec3 bisectEdge(glm::vec3& v0, glm::vec3& v1);

    std::vector<Vertex> verts;
    Faces faces;
};

} // namespace glit
