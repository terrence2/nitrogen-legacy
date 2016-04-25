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

#include <simplexnoise.h>

#include "icosphere.h"

using namespace glm;
using namespace std;

/* static */ dvec3
glit::Terrain::bisect(dvec3 v0, dvec3 v1)
{
    return v0 + ((v1 - v0) / 2.0);
}

void
glit::Terrain::subdivideFacet(dvec3 p0, dvec3 p1, dvec3 p2,
                              dvec3* c0, dvec3* c1, dvec3* c2) const
{
    *c0 = normalize(bisect(p1, p2));
    *c1 = normalize(bisect(p0, p2));
    *c2 = normalize(bisect(p0, p1));

    *c0 = *c0 * heightAt(*c0);
    *c1 = *c1 * heightAt(*c1);
    *c2 = *c2 * heightAt(*c2);
}

glit::Terrain::Terrain(double r)
  : programPoints(makePointsProgram())
  , wireframeMesh(Drawable(programPoints, GL_LINES,
           make_shared<VertexBuffer>(VertexDescriptor::fromType<Facet::GPUVertex>()),
           make_shared<IndexBuffer>()))
  , tristripMesh(Drawable(programPoints, GL_TRIANGLE_STRIP,
           make_shared<VertexBuffer>(VertexDescriptor::fromType<Facet::GPUVertex>()),
           make_shared<IndexBuffer>()))
  , radius(r)
{
    // Use an IcoSphere to find the initial, static corners.
    IcoSphere sphere(0);
    size_t i = 0;
    for (auto& v : sphere.vertices()) {
        baseVerts.push_back(Facet::VertexAndIndex{
                {dvec3(v.aPosition) * heightAt(dvec3(v.aPosition))},
                uint32_t(-1)});
        ++i;
    }

    i = 0;
    for (auto& face : sphere.faceList()) {
        facets[i].init(&baseVerts[face.i0], &baseVerts[face.i1], &baseVerts[face.i2]);
        ++i;
    }

    // We want a falloff so that we get more subdivisions near the camera and
    // they fall away in the distance. Ideally, we'd like the falloff to be
    // lower at higher altitudes. Not sure how to wrangle this. For now it's
    // just linear.
    //
    // Maybe something like:
    //   y = 1 - ln(x + 1) / 2
    float ang0 = acosf(dot(facets[0].verts[0]->vertex.position / radius,
                           facets[0].verts[1]->vertex.position / radius));
    float d0 = (radius * sin(ang0 / 2.f)) * 2.f;
    cout << "angle: " << ang0 << "; dist: " << d0 << endl;
    EdgeLengths[0] = d0;
    for (size_t i = 1; i < util::ArrayLength(EdgeLengths); ++i) {
        ang0 = ang0 / 2.0f;
        d0 = (radius * sin(ang0 / 2.f)) * 2.f;
        cout << "angle: " << ang0 << "; dist: " << d0 << endl;
        EdgeLengths[i] = d0;
    }

}

glit::Terrain::~Terrain()
{
    for (auto& facet : facets)
        deleteChildren(facet);
}

/* static */ shared_ptr<glit::Program>
glit::Terrain::makePointsProgram()
{
    auto VertexDesc = VertexDescriptor::fromType<Facet::GPUVertex>();
    VertexShader vs(
            R"SHADER(
            ///////////////////////////////////////////////////////////////////
            #version 100
            precision highp float;
            uniform mat4 uModelViewProj;
            uniform vec3 uCameraPosition;
            attribute vec3 aPosition;
            attribute vec3 aNormal;
            varying vec3 vColor;
            varying vec3 vNormal;

            void main()
            {
                gl_Position = uModelViewProj * vec4(aPosition, 1.0);
                vColor = vec3(1.0);
                //vNormal = normalize((vFace0N + vFace1N + vFace2N) / 3.0);
                vNormal = aNormal;
                //vLatLon = posLatLon;
            }
            ///////////////////////////////////////////////////////////////////
            )SHADER",
            VertexDesc);
    FragmentShader fs(
            R"SHADER(
            ///////////////////////////////////////////////////////////////////
            #version 100
            precision highp float;
            const float PI = 3.1415925;
            uniform vec3 uSunDirection;
            varying vec3 vNormal;
            varying vec3 vColor;

            void main() {
                float diffuse = dot(vNormal, -uSunDirection);
                gl_FragData[0] = vec4(vColor * diffuse, 1.0);
            }
            ///////////////////////////////////////////////////////////////////
            )SHADER"
        );
    return make_shared<Program>(move(vs), move(fs), vector<UniformDesc>{
                Program::MakeInput<mat4>("uModelViewProj"),
                Program::MakeInput<mat4>("uCameraPosition"),
                Program::MakeInput<vec3>("uSunDirection"),
            });
}

#if 0
glit::Mesh
glit::Terrain::uploadAsPoints() const
{
    util::Timer t("Terrain::upload");

    vector<Facet::GPUVertex> verts;
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

double
glit::Terrain::heightAt(dvec3 dpos) const
{
    const static float freq = 2000.0f;
    double h = radius + 10000.0 * double(raw_noise_3d(dpos.x,
                                                    dpos.y,
                                                    dpos.z));
    cout << "h: " << (h - radius) << endl;
    return h;
}

glit::Mesh*
glit::Terrain::uploadAsWireframe(const dvec3& viewPosition,
                                 const dvec3& viewDirection)
{
    wireframeMesh.drawable(0).vertexBuffer()->orphan<Facet::GPUVertex>();
    wireframeMesh.drawable(0).indexBuffer()->orphan();

    reshape(viewPosition, viewDirection);

    vector<Facet::GPUVertex> verts;
    vector<uint32_t> indices;
    {
        //util::Timer t("Terrain::upload");
        for (size_t i = 0; i < 20; ++i)
            drawSubtreeWireframe(facets[i], viewPosition, verts, indices);
    }

    wireframeMesh.drawable(0).vertexBuffer()->upload(verts);
    wireframeMesh.drawable(0).indexBuffer()->upload(indices);
    return &wireframeMesh;
}

glit::Mesh*
glit::Terrain::uploadAsTriStrips(const dvec3& viewPosition,
                                 const dvec3& viewDirection)
{
    tristripMesh.drawable(0).vertexBuffer()->orphan<Facet::GPUVertex>();
    tristripMesh.drawable(0).indexBuffer()->orphan();

    reshape(viewPosition, viewDirection);

    vector<Facet::GPUVertex> verts;
    vector<uint32_t> indices;
    drawSubtreeTriStrip(viewPosition, verts, indices);

    tristripMesh.drawable(0).vertexBuffer()->upload(verts);
    tristripMesh.drawable(0).indexBuffer()->upload(indices);
    return &tristripMesh;
}

void
glit::Terrain::deleteChildren(Facet& self)
{
    if (self.children) {
        for (size_t i = 0; i < 4; ++i)
            deleteChildren(self.children[i]);
        delete [] self.children;
        self.children = nullptr;
    }
}

void
glit::Terrain::Facet::init(VertexAndIndex* v0, VertexAndIndex* v1, VertexAndIndex* v2)
{
    children = nullptr;

    verts[0] = v0;
    verts[1] = v1;
    verts[2] = v2;

    normal = normalize(cross(v1->vertex.position - v0->vertex.position,
                             v2->vertex.position - v0->vertex.position));

    // Uninitialized:
    //   childVerts
}

void
glit::Terrain::reshape(const dvec3& viewPosition, const dvec3& viewDirection)
{
    // Clear base verts cache'd upload index.
    for (auto& vert : baseVerts)
        vert.index = uint32_t(-1);

    reshapeHorizon = acos(radius / (radius + 10000.0)) +
                     acos(radius / glm::length(viewPosition));
    for (size_t i = 0; i < 20; ++i)
        reshapeN(0, facets[i], viewPosition, viewDirection);
}

void
glit::Terrain::reshapeN(size_t level, Facet& self,
                       const dvec3& viewPosition, const dvec3& viewDirection)
{
    // Max subdivision is ~1M resolution.
    if (level >= MaxSubdivisions)
        return deleteChildren(self);

    // Cull distant faces.
    dvec3 center = (self.verts[0]->vertex.position +
                    self.verts[1]->vertex.position +
                    self.verts[2]->vertex.position) / 3.0;
    dvec3 to = center - viewPosition;
    double dist2 = to.x * to.x + to.y * to.y + to.z * to.z;
    if ((EdgeLengths[level] * EdgeLengths[level] * (10.f * 10.f)) < dist2) {
        return deleteChildren(self);
    }

    // Cull back facing facets.
    double cosOfAng = dot(vec3(normalize(viewPosition)), self.normal);
    if (cosOfAng < 0)
        return deleteChildren(self);

    // Clear cached upload indices.
    self.childVerts[0].index = uint32_t(-1);
    self.childVerts[1].index = uint32_t(-1);
    self.childVerts[2].index = uint32_t(-1);

    if (!self.children) {
        // Subdivide allocate and assign verts.
        subdivideFacet(self.verts[0]->vertex.position,
                       self.verts[1]->vertex.position,
                       self.verts[2]->vertex.position,
                       &self.childVerts[0].vertex.position,
                       &self.childVerts[1].vertex.position,
                       &self.childVerts[2].vertex.position);

        self.children = new Facet[4];
        self.children[0].init(self.verts[0],
                              &self.childVerts[2],
                              &self.childVerts[1]);
        self.children[1].init(&self.childVerts[0],
                              &self.childVerts[1],
                              &self.childVerts[2]);
        self.children[2].init(&self.childVerts[2],
                              self.verts[1],
                              &self.childVerts[0]);
        self.children[3].init(&self.childVerts[1],
                              &self.childVerts[0],
                              self.verts[2]);
    }

    reshapeN(level + 1, self.children[0], viewPosition, viewDirection);
    reshapeN(level + 1, self.children[1], viewPosition, viewDirection);
    reshapeN(level + 1, self.children[2], viewPosition, viewDirection);
    reshapeN(level + 1, self.children[3], viewPosition, viewDirection);
}

/* static */ glit::Terrain::Facet::GPUVertex
glit::Terrain::Facet::GPUVertex::fromCPU(const CPUVertex& v,
                                         const Facet& owner,
                                         const dvec3& viewPosition)
{
    dvec3 actual = (v.position - viewPosition) / 100.0;
    //cout << "pushing: " << actual << endl;
    return GPUVertex{vec3(actual), owner.normal};
}

/* static */ uint32_t
glit::Terrain::pushVertex(Facet::VertexAndIndex* insert,
                          const Facet& owner,
                          const glm::dvec3& viewPosition,
                          vector<Facet::GPUVertex>& verts)
{
    if (insert->index != uint32_t(-1))
        return insert->index;
    insert->index = verts.size();
    verts.push_back(Facet::GPUVertex::fromCPU(insert->vertex, owner, viewPosition));
    return insert->index;
}

void
glit::Terrain::drawSubtreeTriStrip(const dvec3& viewPosition,
                                   vector<Facet::GPUVertex>& verts,
                                   vector<uint32_t>& indices)
{
    //util::Timer t("Terrain::upload");
    for (size_t i = 0; i < 20; ++i)
        drawSubtreeTriStripN(facets[i], viewPosition, verts, indices);
}

void
glit::Terrain::drawSubtreeTriStripN(Facet& facet, const dvec3& viewPosition,
                                    vector<Facet::GPUVertex>& verts,
                                    vector<uint32_t>& indices) const
{
    // Draw leaf triangles.
    if (!facet.children) {
        uint32_t i0 = pushVertex(facet.verts[0], facet, viewPosition, verts);
        uint32_t i1 = pushVertex(facet.verts[1], facet, viewPosition, verts);
        uint32_t i2 = pushVertex(facet.verts[2], facet, viewPosition, verts);
        indices.push_back(i0);
        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i2);
        indices.push_back(i2);
        return;
    }

    // Recurse into our children.
    drawSubtreeTriStripN(facet.children[0], viewPosition, verts, indices);
    drawSubtreeTriStripN(facet.children[1], viewPosition, verts, indices);
    drawSubtreeTriStripN(facet.children[2], viewPosition, verts, indices);
    drawSubtreeTriStripN(facet.children[3], viewPosition, verts, indices);

    // Draw joining tris between our children and grandchildren.
    //
    // Semi-Assumption: A maximum of one level of subdivision difference is
    // possible between siblings. e.g. At whatever range we subdivide, the next
    // subdivision range is always going to be more than one triangle centroid
    // away. This means:
    //   1- we only need to check locally between siblings, modulo the
    //      top-level facet checks above; this can be done by checking if one
    //      has children and the other does not.
    //   2- we only ever need to insert a single triangle to join facets of
    //      different levels.
    // Note that we make these checks in the parent because the parent knows
    // what is adjacent to which other child and where.
    if (facet.children[0].children != facet.children[1].children) {
        //uint32_t i0, i1, i2;
        if (facet.children[1].children) {
            /*
            i0 = facet.children[1].verts[2]->index;
            i1 = facet.children[1].verts[1]->index;
            i2 = facet.children[1].childVerts[0].index;
            if (i0 == uint32_t(-1)) throw runtime_error("a0: what?");
            if (i1 == uint32_t(-1)) throw runtime_error("a1: what?");
            if (i2 == uint32_t(-1)) throw runtime_error("a2: what?");
            indices.push_back(i0);
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i2);
            indices.push_back(i2);
            */
        } else {
            /*
            i0 = facet.children[0].verts[2]->index;
            i1 = facet.children[0].childVerts[0].index;
            i2 = facet.children[0].verts[1]->index;
            if (i0 == uint32_t(-1)) throw runtime_error("b0: what?");
            if (i1 == uint32_t(-1)) throw runtime_error("b1: what?");
            if (i2 == uint32_t(-1)) throw runtime_error("b2: what?");
            indices.push_back(i0);
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i2);
            indices.push_back(i2);
            */
        }
        /*
        indices.push_back(i0);
        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i2);
        indices.push_back(i2);
        */
    }
    if (facet.children[1].children != facet.children[2].children) {
        if (!facet.children[1].children) {
        } else {
        }
    }
}

void
glit::Terrain::drawSubtreeWireframe(Facet& facet, const dvec3& viewPosition,
                                    vector<Facet::GPUVertex>& verts,
                                    vector<uint32_t>& indices) const
{
    if (facet.children) {
        drawSubtreeWireframe(facet.children[0], viewPosition, verts, indices);
        drawSubtreeWireframe(facet.children[1], viewPosition, verts, indices);
        drawSubtreeWireframe(facet.children[2], viewPosition, verts, indices);
        drawSubtreeWireframe(facet.children[3], viewPosition, verts, indices);
    } else {
        uint32_t i0 = pushVertex(facet.verts[0], facet, viewPosition, verts);
        uint32_t i1 = pushVertex(facet.verts[1], facet, viewPosition, verts);
        uint32_t i2 = pushVertex(facet.verts[2], facet, viewPosition, verts);
        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i2);
        indices.push_back(i0);
    }
}
