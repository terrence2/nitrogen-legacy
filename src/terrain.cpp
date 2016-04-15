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
#include "terrain.h"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtx/polar_coordinates.hpp>

#include "icosphere.h"

using namespace glm;
using namespace std;

const /* static */ float
glit::Terrain::SubdivisionDistance[glit::Terrain::MaxSubdivisions] = {
    numeric_limits<float>::infinity(), // Always shown.
    5.f,  // 80 tris looks really blocky, so show more even pretty far out.
    2.f,
    1.f,
    1.f / float(1 << 1),
    1.f / float(1 << 2), // 5
    1.f / float(1 << 3),
    1.f / float(1 << 4),
    1.f / float(1 << 5),
    1.f / float(1 << 6),
    1.f / float(1 << 7), // 10
    1.f / float(1 << 8),
    1.f / float(1 << 9),
    1.f / float(1 << 10),
    1.f / float(1 << 11),
    1.f / float(1 << 12), // 15
    1.f / float(1 << 13),
    1.f / float(1 << 14),
    1.f / float(1 << 15),
    1.f / float(1 << 16),
    1.f / float(1 << 17), // 20
    1.f / float(1 << 18),
    1.f / float(1 << 19), // 22
};

/* static */ vec3
glit::Terrain::bisect(vec3 v0, vec3 v1)
{
    return v0 + ((v1 - v0) / 2.f);
}

void
glit::Terrain::subdivideFacet(vec3 p0, vec3 p1, vec3 p2,
                              vec3* c0, vec3* c1, vec3* c2) const
{
    *c0 = radius * normalize(bisect(p1, p2));
    *c1 = radius * normalize(bisect(p0, p2));
    *c2 = radius * normalize(bisect(p0, p1));
}

glit::Terrain::Terrain(float r)
  : programPoints(makePointsProgram())
  , wireframeMesh(Drawable(pointsProgram(), GL_LINES,
           make_shared<VertexBuffer>(VertexDescriptor::fromType<Facet::Vertex>()),
           make_shared<IndexBuffer>()))
  , radius(r)
{
    // Compute the subdivision distances from the table.
    for (size_t i = 0; i < util::ArrayLength(SubdivisionDistance); ++i) {
        float limit = SubdivisionDistance[i];
        float distance = radius * limit;
        SubdivisionRadiusSquared[i] = distance * distance;
    }

    // Use an IcoSphere to find the initial, static corners.
    IcoSphere sphere(0);
    size_t i = 0;
    for (auto& v : sphere.vertices()) {
        baseVerts.push_back(Facet::Vertex{radius * v.aPosition});
        ++i;
    }

    i = 0;
    for (auto& face : sphere.faceList()) {
        facets[i].verts[0] = &baseVerts[get<0>(face)];
        facets[i].verts[1] = &baseVerts[get<1>(face)];
        facets[i].verts[2] = &baseVerts[get<2>(face)];
        ++i;
    }
}

/* static */ shared_ptr<glit::Program>
glit::Terrain::makePointsProgram()
{
    auto VertexDesc = VertexDescriptor::fromType<Facet::Vertex>();
    VertexShader vs(
            R"SHADER(
            #include <noise2D.glsl>
            precision highp float;
            uniform mat4 uModelViewProj;
            attribute vec3 aPosition;
            varying vec4 vColor;
            void main()
            {
                float baseHeight = length(aPosition);
                vec3 dir = aPosition / baseHeight;
                vec2 polar = vec2(asin(dir.y), atan(dir.x, dir.z));
                float aslHeight = snoise(polar) * 100.f;
                vec4 adjusted = vec4(dir * (baseHeight + aslHeight), 1.0f);
                gl_Position = uModelViewProj * adjusted;
                vColor = vec4(255, 255, 255, 255);
            }
            )SHADER",
            VertexDesc);
    FragmentShader fs(
            R"SHADER(
            precision highp float;
            varying vec4 vColor;
            void main() {
                gl_FragColor = vColor;
            }
            )SHADER"
        );
    return make_shared<Program>(move(vs), move(fs), vector<UniformDesc>{
                Program::MakeInput<mat4>("uModelViewProj"),
            });
}

#if 0
glit::Mesh
glit::Terrain::uploadAsPoints() const
{
    util::Timer t("Terrain::upload");

    vector<Facet::Vertex> verts;
    for (size_t i = 0; i < 20; ++i) {
        verts.insert(verts.end(),
                     &facets[i].verts[0],
                     &facets[i].verts[6]);
    }
    auto vb = VertexBuffer::make(verts);

    vector<uint16_t> indices;
    for (uint16_t i = 0; i < verts.size(); ++i)
        indices.push_back(i);
    auto ib = IndexBuffer::make(indices);

    return Mesh(Drawable(pointsProgram(), GL_POINTS, move(vb), move(ib)));
}
#endif

float
glit::Terrain::heightAt(vec3 pos)
{
    return radius;
}

glit::Mesh*
glit::Terrain::uploadAsWireframe(vec3 viewPosition,
                                 vec3 viewDirection)
{
    wireframeMesh.drawable(0).vertexBuffer()->orphan<Facet::Vertex>();
    wireframeMesh.drawable(0).indexBuffer()->orphan();

    vector<Facet::Vertex> verts;
    vector<uint32_t> indices;

    {
        util::Timer t("Terrain::reshape");
        for (size_t i = 0; i < 20; ++i)
            reshape(0, facets[i], viewPosition, viewDirection);
    }

    {
        util::Timer t("Terrain::upload");
        for (size_t i = 0; i < 20; ++i)
            drawSubtree(facets[i], verts, indices);
    }

    wireframeMesh.drawable(0).vertexBuffer()->upload(verts);
    wireframeMesh.drawable(0).indexBuffer()->upload(indices);
    return &wireframeMesh;
}

void
glit::Terrain::deleteChildren(Facet& self)
{
    if (self.children) {
        for (size_t i = 0; i < 4; ++i)
            deleteChildren(self.children[i]);
        delete [] self.children;
    }
}

void
glit::Terrain::reshape(size_t level, Facet& self,
                       vec3 viewPosition, vec3 viewDirection)
{
    vec3 center = (self.verts[0]->aPosition +
                   self.verts[1]->aPosition +
                   self.verts[2]->aPosition) / 3.f;
    vec3 to = center - viewPosition;
    float dist2 = to.x * to.x + to.y * to.y + to.z * to.z;

    // FIXME: do backface removal at this level.
    if (level >= 8 ||
        dist2 > SubdivisionRadiusSquared[level])
    {
        if (self.children)
            deleteChildren(self);
        self.children = nullptr;
        return;
    }

    if (!self.children) {
        // Subdivide allocate and assign verts.
        subdivideFacet(self.verts[0]->aPosition,
                       self.verts[1]->aPosition,
                       self.verts[2]->aPosition,
                       &self.childVerts[0].aPosition,
                       &self.childVerts[1].aPosition,
                       &self.childVerts[2].aPosition);

        self.children = new Facet[4];

        self.children[0].verts[0] = self.verts[0];
        self.children[0].verts[1] = &self.childVerts[2];
        self.children[0].verts[2] = &self.childVerts[1];

        self.children[1].verts[0] = &self.childVerts[0];
        self.children[1].verts[1] = &self.childVerts[1];
        self.children[1].verts[2] = &self.childVerts[2];

        self.children[2].verts[0] = &self.childVerts[2];
        self.children[2].verts[1] = self.verts[1];
        self.children[2].verts[2] = &self.childVerts[0];

        self.children[3].verts[0] = &self.childVerts[1];
        self.children[3].verts[1] = &self.childVerts[0];
        self.children[3].verts[2] = self.verts[2];
    }

    reshape(level + 1, self.children[0], viewPosition, viewDirection);
    reshape(level + 1, self.children[1], viewPosition, viewDirection);
    reshape(level + 1, self.children[2], viewPosition, viewDirection);
    reshape(level + 1, self.children[3], viewPosition, viewDirection);
}

/* static */ uint32_t
glit::Terrain::pushVertex(Facet::Vertex* insert, vector<Facet::Vertex>& verts)
{
    uint32_t i0 = verts.size();
    verts.push_back(*insert);
    return i0;
}

void
glit::Terrain::drawSubtree(Facet& facet,
                           vector<Facet::Vertex>& verts,
                           vector<uint32_t>& indices) const
{
    if (facet.children) {
        drawSubtree(facet.children[0], verts, indices);
        drawSubtree(facet.children[1], verts, indices);
        drawSubtree(facet.children[2], verts, indices);
        drawSubtree(facet.children[3], verts, indices);
    } else {
        uint32_t i0 = pushVertex(facet.verts[0], verts);
        uint32_t i1 = pushVertex(facet.verts[1], verts);
        uint32_t i2 = pushVertex(facet.verts[2], verts);
        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i2);
        indices.push_back(i0);
    }
}
