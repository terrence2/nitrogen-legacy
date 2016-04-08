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
#include "icosphere.h"

#include <math.h>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <glm/glm.hpp>

using glm::normalize;
using glm::vec3;

glit::IcoSphere::IcoSphere(int iterations)
{
    float t = (1.f + sqrtf(5.f)) / 2.f;

    verts.push_back(Vertex{normalize(vec3(-1.f,  t,  0.f))});
    verts.push_back(Vertex{normalize(vec3( 1.f,  t,  0.f))});
    verts.push_back(Vertex{normalize(vec3(-1.f, -t,  0.f))});
    verts.push_back(Vertex{normalize(vec3( 1.f, -t,  0.f))});

    verts.push_back(Vertex{normalize(vec3( 0.f, -1.f,  t))});
    verts.push_back(Vertex{normalize(vec3( 0.f,  1.f,  t))});
    verts.push_back(Vertex{normalize(vec3( 0.f, -1.f, -t))});
    verts.push_back(Vertex{normalize(vec3( 0.f,  1.f, -t))});

    verts.push_back(Vertex{normalize(vec3( t,  0.f, -1.f))});
    verts.push_back(Vertex{normalize(vec3( t,  0.f,  1.f))});
    verts.push_back(Vertex{normalize(vec3(-t,  0.f, -1.f))});
    verts.push_back(Vertex{normalize(vec3(-t,  0.f,  1.f))});

    // 5 faces around point 0
    faces.push_back(Face(0, 11, 5));
    faces.push_back(Face(0, 5, 1));
    faces.push_back(Face(0, 1, 7));
    faces.push_back(Face(0, 7, 10));
    faces.push_back(Face(0, 10, 11));

    // 5 adjacent faces
    faces.push_back(Face(1, 5, 9));
    faces.push_back(Face(5, 11, 4));
    faces.push_back(Face(11, 10, 2));
    faces.push_back(Face(10, 7, 6));
    faces.push_back(Face(7, 1, 8));

    // 5 faces around point 3
    faces.push_back(Face(3, 9, 4));
    faces.push_back(Face(3, 4, 2));
    faces.push_back(Face(3, 2, 6));
    faces.push_back(Face(3, 6, 8));
    faces.push_back(Face(3, 8, 9));

    // 5 adjacent faces
    faces.push_back(Face(4, 9, 5));
    faces.push_back(Face(2, 4, 11));
    faces.push_back(Face(6, 2, 10));
    faces.push_back(Face(8, 6, 7));
    faces.push_back(Face(9, 8, 1));

    for (int i = 0; i < iterations; ++i) {
        Faces nextFaces;
        for (Face& face : faces) {
            vec3 a = normalize(bisectEdge(verts[std::get<0>(face)].aPosition,
                                          verts[std::get<1>(face)].aPosition));
            vec3 b = normalize(bisectEdge(verts[std::get<1>(face)].aPosition,
                                          verts[std::get<2>(face)].aPosition));
            vec3 c = normalize(bisectEdge(verts[std::get<2>(face)].aPosition,
                                          verts[std::get<0>(face)].aPosition));
            uint16_t ia = verts.size();
            verts.push_back(Vertex{a});
            uint16_t ib = verts.size();
            verts.push_back(Vertex{b});
            uint16_t ic = verts.size();
            verts.push_back(Vertex{c});

            nextFaces.push_back(Face(std::get<0>(face), ia, ic));
            nextFaces.push_back(Face(std::get<1>(face), ib, ia));
            nextFaces.push_back(Face(std::get<2>(face), ic, ib));
            nextFaces.push_back(Face(ia, ib, ic));
        }
        faces = nextFaces;
    }
}

vec3
glit::IcoSphere::bisectEdge(vec3& v0, vec3& v1)
{
    return v0 + ((v1 - v0) / 2.f);
}

glit::PrimitiveData
glit::IcoSphere::uploadAsPoints() const
{
    //auto SphereVertexDesc = VertexDescriptor::fromType<IcoSphere::Vertex>();
    glit::VertexBuffer vertbuf(VertexDescriptor::fromType<IcoSphere::Vertex>());
    vertbuf.upload(verts);

    glit::IndexBuffer indbuf;
    std::vector<uint16_t> indices;
    for (uint16_t i = 0; i < verts.size(); ++i)
        indices.push_back(i);
    indbuf.upload(indices);

    return PrimitiveData(GL_POINTS, std::move(vertbuf), std::move(indbuf));
}

glit::PrimitiveData
glit::IcoSphere::uploadAsWireframe() const
{
    //auto SphereVertexDesc = VertexDescriptor::fromType<IcoSphere::Vertex>();
    glit::VertexBuffer vertbuf(VertexDescriptor::fromType<IcoSphere::Vertex>());
    vertbuf.upload(verts);

    glit::IndexBuffer indbuf;
    std::vector<uint16_t> indices;
    for (auto& face : faces) {
        indices.push_back(std::get<0>(face));
        indices.push_back(std::get<1>(face));
        indices.push_back(std::get<1>(face));
        indices.push_back(std::get<2>(face));
        indices.push_back(std::get<2>(face));
        indices.push_back(std::get<0>(face));
    }
    indbuf.upload(indices);

    return PrimitiveData(GL_LINES, std::move(vertbuf), std::move(indbuf));
}
