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
  : programPoints(makePointsProgram()),
    radius(r)
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
        verts.push_back(Facet::Vertex{radius * v.aPosition});
        ++i;
    }

    i = 0;
    for (auto& face : sphere.faceList()) {
        facets[i].verts[0] = &verts[get<0>(face)];
        facets[i].verts[1] = &verts[get<1>(face)];
        facets[i].verts[2] = &verts[get<2>(face)];
        ++i;
    }

    /*
    for (size_t i = 0; i < 20; ++i) {
        facets[i].parent = nullptr;
        facets[i].ownVerts[0].aPosition = radius * sphere.faceVertexPosition<0>(i);
        facets[i].ownVerts[1].aPosition = radius * sphere.faceVertexPosition<1>(i);
        facets[i].ownVerts[2].aPosition = radius * sphere.faceVertexPosition<2>(i);
        facets[i].verts[0] = &facets[i].ownVerts[0];
        facets[i].verts[1] = &facets[i].ownVerts[1];
        facets[i].verts[2] = &facets[i].ownVerts[2];
        facets[i].childNumber = -i;
    }
    // Seed the facets from the icosphere corners.
    for (size_t i = 0; i < 20; ++i) {
        subdivideFacet(&facets[i],
                       radius * sphere.faceVertexPosition<0>(i),
                       radius * sphere.faceVertexPosition<1>(i),
                       radius * sphere.faceVertexPosition<2>(i));
    }
    */
}

/* static */ shared_ptr<glit::Program>
glit::Terrain::makePointsProgram()
{
    auto VertexDesc = VertexDescriptor::fromType<Facet::Vertex>();
    VertexShader vs(
            R"SHADER(
            precision highp float;
            uniform mat4 uModelViewProj;
            attribute vec3 aPosition;
            varying vec4 vColor;
            void main()
            {
                gl_Position = uModelViewProj * vec4(aPosition, 1.0f);
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

static uint32_t IndexOffsetsForTri[4][3] = {
    { 0, 0, 0},
    { 0, 0, 0},
    { 0, 0, 0},
    { 0, 0, 0}
};

glit::Terrain::Facet*
glit::Terrain::buildDisplayVerts(uint8_t level, Facet& parent, size_t parentBase,
                                 Facet* self, uint8_t triPosition,
                                 vec3 viewPosition, vec3 viewDirection,
                                 vector<Facet::Vertex>& verts,
                                 vector<uint32_t>& indices) const
{
#if 0
    // Use parent base and triPosition to look up the indices of the vertices
    // that are shared with our parent.
    uint32_t i0, i4, i5;
    switch (triPosition) {
    case 0: i0 = 0; i4 = 2; i5 = 1; break;
    case 1: i0 = 3; i4 = 1; i5 = 2; break;
    case 2: i0 = 2; i4 = 4; i5 = 3; break;
    case 3: i0 = 1; i4 = 3; i5 = 5; break;
    }
    i0 += parentBase;
    i4 += parentBase;
    i5 += parentBase;

    // Look up the shared vertices and use those to determine if we want to
    // cull this subdivision or draw it.
    // FIXME: Check our distance from the viewpoint.
    if (level == 3) {
        // FIXME: it's possible we could move out of range of an entire tree in
        // a single step, so we need to recurse here.
        if (self)
            delete self;
        return nullptr;
    }

    // If self does not already exist, create it now and populate its vertices.
    if (self == nullptr) {
        // FIXME: find a way to use unique_ptr for this.
        self = new Facet();

        // Bisect to get intermediate verticies.
        subdivideFacet(self, verts[i0].aPosition,
                             verts[i4].aPosition,
                             verts[i5].aPosition);
    }

    // Load the new verts into the buffer and strip using a combination of
    // parent base and self-base.
    size_t base = verts.size();
    verts.push_back(self->verts[1]);
    verts.push_back(self->verts[2]);
    verts.push_back(self->verts[3]);
    uint32_t i1 = base;
    uint32_t i2 = base + 1;
    uint32_t i3 = base + 2;
    for (uint32_t i : vector<uint32_t>{i0, i1,  i1, i2,  i2, i0,
                                       i1, i3,  i3, i2,  i2, i1,
                                       i2, i3,  i3, i4,  i4, i2,
                                       i1, i5,  i5, i3,  i3, i1}) {
        indices.push_back(i);
    }

    // Recurse into each child.
    for (uint8_t j = 0; j < 4; ++j) {
        /*
        // The next level below this one does not have 6 verts to pull from,
        // only 3. Some of its verts need to come from the parent and some
        // from the grandparent.
        //
        auto child = buildDisplayVerts(
                              level + 1, *self, base, self->children[j], j,
                              viewPosition, viewDirection,
                              verts, indices);
        self->children[j] = child;
        */
        /*
        if (!child) {
            // If we did not subdivide, add indices for the larger tri.
            uint32_t i0, i1, i2;
            switch (j) {
            case 0: i0 = 0; i1 = 2; i2 = 1; break;
            case 1: i0 = 1; i1 = 3; i2 = 2; break;
            case 2: i0 = 2; i1 = 3; i2 = 4; break;
            case 3: i0 = 1; i1 = 5; i2 = 3; break;
            }
            indices.push_back(base + i0);
            indices.push_back(base + i1);
            indices.push_back(base + i1);
            indices.push_back(base + i2);
            indices.push_back(base + i2);
            indices.push_back(base + i0);
        }
        */
    }
    return self;
#endif
    return nullptr;

    // FIXME: recurse into children.
#if 0
    verts.insert(verts.end(),
                 &facet.verts[0],
                 &facet.verts[6]);

    return;

    // FIXME: Escape hatch until we verify that we're not gonna just dump
    // all the verts by accident.
    if (level > 10)
        return;

    for (size_t i = 0; i < 4; ++i) {
        size_t i0, i1, i2;
        switch (i) {
        case 0: { i0 = 0; i1 = 2; i2 = 1; break; }
        case 1: { i0 = 3; i1 = 1; i2 = 2; break; }
        case 2: { i0 = 2; i1 = 4; i2 = 3; break; }
        case 3: { i0 = 1; i1 = 3; i2 = 5; break; }
        }

        const vec3& v0 = facet.verts[i0].aPosition;
        const vec3& v1 = facet.verts[i1].aPosition;
        const vec3& v2 = facet.verts[i2].aPosition;

        vec3 v0N = cross(v1 - v0, v2 - v0);
        float ang = dot(v0 - pointOfInterest, v0N);
        /*
        if (ang > 0)
            continue;
        */

        // We can pick any vertex for each child: the distances should be
        // calibrated such that it doesn't matter that much. We want to
        // use the fewest number of vertices so we pick 0 and 3 as we only
        // need two for that configuration.
        vec3 direct = v0 - pointOfInterest;
        float d2 = direct.x * direct.x +
                   direct.y * direct.y +
                   direct.z * direct.z;
        bool inRange = d2 < SubdivisionRadiusSquared[level];

        /*
        cout << "At " << (int)level << " -> " << i << ": "
            << checkVert << " to " << pointOfInterest << " => "
            << sqrt(d2) << " <? "
            << sqrt(SubdivisionRadiusSquared[level])
            << ": " << inRange << endl;
        */

        if (!inRange && facet.children[i]) {
            delete facet.children[i];
            facet.children[i] = nullptr;
        } else if (inRange && !facet.children[i]) {
            facet.children[i] = new Facet();
            subdivideFacet(facet.children[i], v0, v1, v2);
        }

        if (inRange) {
            buildDisplayVerts(level + 1, &facet, *facet.children[i],
                              pointOfInterest, viewDirection,
                              verts, indices);
        }
    }
#endif
}

glit::Mesh
glit::Terrain::uploadAsWireframe(vec3 viewPosition,
                                 vec3 viewDirection)
{

    vector<Facet::Vertex> verts;
    vector<uint32_t> indices;

    {
        util::Timer t("Terrain::reshape");
        for (size_t i = 0; i < 20; ++i)
            reshape(0, facets[i], viewPosition, viewDirection);
    }

    {
        util::Timer t("Terrain::upload");
        unordered_map<Facet::Vertex*, uint32_t> vmap;
        for (size_t i = 0; i < 20; ++i)
            drawSubtree(facets[i], verts, indices, vmap);
    }

    auto vb = VertexBuffer::make(verts);
    auto ib = IndexBuffer::make(indices);
    return Mesh(Drawable(pointsProgram(), GL_LINES, move(vb), move(ib)));
}

void
glit::Terrain::reshape(size_t level, Facet& self,
                       vec3 viewPosition, vec3 viewDirection) const
{
    // FIXME: do backface removal at this level.
    if (level >= 5)
        return;

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

        reshape(level + 1, self.children[0], viewPosition, viewDirection);
        reshape(level + 1, self.children[1], viewPosition, viewDirection);
        reshape(level + 1, self.children[2], viewPosition, viewDirection);
        reshape(level + 1, self.children[3], viewPosition, viewDirection);
    }
}

/* static */ uint32_t
glit::Terrain::memoizeVertex(Facet::Vertex* insert,
                             vector<Facet::Vertex>& verts,
                             vector<uint32_t>& indices,
                             unordered_map<Facet::Vertex*, uint32_t>& vmap)
{
    /*
    auto rv = vmap.insert(pair<Facet::Vertex*, uint32_t>(insert, -1));
    if (rv.second) {
        // The vert did not exist; push it onto verts and put the position into
        // the returned iterator.
        uint32_t i0 = verts.size();
        verts.push_back(*insert);
        rv.first->second = i0;
        return i0;
    }
    // The vert already exists, so get it from the returned iterator.
    return rv.first->second;
    */

    uint32_t i0 = verts.size();
    verts.push_back(*insert);
    return i0;
}

void
glit::Terrain::drawSubtree(Facet& facet,
                           vector<Facet::Vertex>& verts,
                           vector<uint32_t>& indices,
                           unordered_map<Facet::Vertex*, uint32_t> vmap) const
{
    if (facet.children) {
        drawSubtree(facet.children[0], verts, indices, vmap);
        drawSubtree(facet.children[1], verts, indices, vmap);
        drawSubtree(facet.children[2], verts, indices, vmap);
        drawSubtree(facet.children[3], verts, indices, vmap);
    } else {
        uint32_t i0 = memoizeVertex(facet.verts[0], verts, indices, vmap);
        uint32_t i1 = memoizeVertex(facet.verts[1], verts, indices, vmap);
        uint32_t i2 = memoizeVertex(facet.verts[2], verts, indices, vmap);
        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i2);
        indices.push_back(i0);
    }
}
