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
#include "utility.h"

namespace glit {

// An ico-sphere that automatically scales in resolution dependent on
// the perspective.
class Terrain
{
  public:
    struct Vertex {
        glm::vec3 aPosition;
        static void describe(std::vector<VertexAttrib>& attribs) {
            attribs.push_back(MakeGLMVertexAttrib(Vertex, aPosition, false));
        }
    };

    Terrain();
    //PrimitiveData uploadAsPoints() const;
    PrimitiveData uploadAsWireframe() const;

  private:

    // A facet is the subdividable piece of the terrain.
    //
    // Each facet is one side of the isocohedron, or one of the subdivided
    // sub-triangles within the isocohedron. Since each level would create
    // only 4 new triangles, we subdivide each level by Iterations. So each
    // new facet adds 4**Iterations new triangls.
    //
    // In order of the children is left to right, top to bottom to ease
    // stripping of triangles through the mesh.
    //  ________________
    //  \      /\      /
    //   \ 1  /  \ 3  /
    //    \  / 2  \  /
    //     \/______\/
    //      \      /
    //       \ 4  /
    //        \  /
    //         \/
    //
    // Stripping then leads to this ordering.
    //
    // 1_______3________ 5
    //  \      /\      /
    //   \    /  \    /
    //    \  /    \  /
    //     \/______\/
    // 2, 8 \      / 4, 6 (degenerate)
    //       \    /
    //        \  /
    //         \/
    //         7 (degenerate)
    struct Facet
    {
        static const uint8_t Iterations = 5;

        Facet* children[util::ipow(4, 5)];
    };

    // The base icosohedron points and initial facets.
    std::vector<Vertex> icoVerts;
    Facet facets[20];

    using Face = std::tuple<uint16_t, uint16_t, uint16_t>;
    using Faces = std::vector<Face>;

    /*
    glm::vec3 bisectEdge(glm::vec3& v0, glm::vec3& v1);

    std::vector<Vertex> verts;
    */
    Faces faces;
};

} // namespace glit
