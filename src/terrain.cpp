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
    numeric_limits<float>::infinity(),  // 80 tris looks really blocky, so show more even pretty far out.
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

glit::Terrain::Terrain()
  : programPoints(makePointsProgram())
  , wireframeMesh(Drawable(programPoints, GL_LINES,
           make_shared<VertexBuffer>(VertexDescriptor::fromType<Facet::Vertex>()),
           make_shared<IndexBuffer>()))
  , tristripMesh(Drawable(programPoints, GL_TRIANGLE_STRIP,
           make_shared<VertexBuffer>(VertexDescriptor::fromType<Facet::Vertex>()),
           make_shared<IndexBuffer>()))
  , radius(100000.f)
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
        baseVerts.push_back(Facet::VertexAndIndex{{radius * v.aPosition},
                                                  uint32_t(-1)});
        ++i;
    }

    i = 0;
    for (auto& face : sphere.faceList()) {
        facets[i].verts[0] = &baseVerts[face.i0];
        facets[i].verts[1] = &baseVerts[face.i1];
        facets[i].verts[2] = &baseVerts[face.i2];
        ++i;
    }

    float ang0 = acosf(dot(facets[0].verts[0]->vertex.aPosition / radius,
                           facets[0].verts[1]->vertex.aPosition / radius));
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
    auto VertexDesc = VertexDescriptor::fromType<Facet::Vertex>();
    VertexShader vs(
            R"SHADER(
            ///////////////////////////////////////////////////////////////////
            #version 100
            //#version 300 es
            precision highp float;
            uniform mat4 uModelViewProj;
            uniform vec3 uCameraPosition;
            attribute vec3 aPosition;
            varying vec3 vColor;
            varying vec2 vLatLon;
            varying vec3 vNormal;

            const float PI = 3.1415925;
            const float NormalSampleScale = PI / 16.0 / 1.0;
            // everest is 8.8km; radius is 6371km;
            const float HeightScale = 8.8 * 10.0 / 6371.0;

            #include <noise2D.glsl>

            /*
            float fbm(vec2 pos, int octaves, float lacunarity, float gain) {
                float sum = 0.0;
                float freq = 1.0;
                float amp = gain;
                for (int i = 0; i < octaves; ++i) {
                    float n = snoise(pos * freq);
                    sum += n * amp;
                    freq *= lacunarity;
                    amp *= gain;
                }
                return sum;
            }
            */

            vec2 polar(vec3 dir) {
                return vec2(asin(dir.y), atan(dir.x, dir.z));
            }

            vec3 heightAt(vec2 latlon, vec3 dir, float radius) {
                float adjustment = snoise(latlon) * HeightScale;
                //float adjustment = fbm(latlon, 1, 2.0, HeightScale);
                return dir * (radius + adjustment);
            }

            // create a quaternion from an angle and axis.
            vec4 angleAxis(float angle, vec3 axis) {
                float s = sin(angle * 0.5);
                return vec4(axis.x * s,
                            axis.y * s,
                            axis.z * s,
                            cos(angle * 0.5));
            }

            vec3 quatRotate(vec4 quat, vec3 v) {
                vec3 QuatVector = quat.xyz;
                vec3 uv = cross(QuatVector, v);
                vec3 uuv = cross(QuatVector, uv);
                return v + ((uv * quat.w) + uuv) * 2.0;
            }

            vec3 heightAtOffset(vec3 dir, vec3 perp, float angle, float radius) {
                vec3 base = normalize(dir + (tan(angle) * perp));
                return heightAt(polar(base), base, radius);
            }

            void main()
            {
                float radius = length(aPosition);
                vec3 posDir = aPosition / radius;
                vec2 posLatLon = polar(posDir);
                vec3 pos = heightAt(posLatLon, posDir, radius);

                // We need to sample around the point to derive a normal.
                //
                // We want to sample such that transitions are smooth per-pixel,
                // but we don't know about pixels. Answer: use the distance from
                // the world-space position to the camera to derive approximately
                // how much screen real-estate is taken by the vertex's fragments.
                //
                // FIXME: we could probably take this from the depth buffer.
                //  The value we get here is transformed: (z/w+1)/2
                //    float depth = texture2D( depthBuf, texCoords ).r;
                float dist = length(pos - uCameraPosition);

                // We'll need to experiment with the ratio.
                // ang should get smaller as dist gets smaller.
                // at dist of 6371, we have like 40 tris... so like... PI/8?... ish?
                // so something like: 6371.0 : PI/8 :: dist : ???
                float ang = dist * NormalSampleScale;

                // Find a vector perpendicular to dir at position.
                // Note that we don't really care about the direction.
                vec3 perp;
                if (abs(dot(posDir, vec3(0.0, 1.0, 0.0))) > 0.1) {
                    perp = normalize(cross(posDir, vec3(1.0, 0.0, 0.0)));
                } else {
                    perp = normalize(cross(posDir, vec3(0.0, 1.0, 0.0)));
                }

                // Get 3 positions around pos at 120 degree angles.
                vec4 rot = angleAxis(radians(120.0), posDir);
                vec3 v0 = heightAtOffset(posDir, perp, ang, radius);
                perp = quatRotate(rot, perp);
                vec3 v1 = heightAtOffset(posDir, perp, ang, radius);
                perp = quatRotate(rot, perp);
                vec3 v2 = heightAtOffset(posDir, perp, ang, radius);

                vec3 vFace0N = normalize(cross(v0 - pos, v1 - pos));
                vec3 vFace1N = normalize(cross(v1 - pos, v2 - pos));
                vec3 vFace2N = normalize(cross(v2 - pos, v0 - pos));

                gl_Position = uModelViewProj * vec4(pos, 1.0);
                vColor = vec3(1.0);
                vNormal = normalize((vFace0N + vFace1N + vFace2N) / 3.0);
                vLatLon = posLatLon;
            }
            ///////////////////////////////////////////////////////////////////
            )SHADER",
            VertexDesc);
    FragmentShader fs(
            R"SHADER(
            ///////////////////////////////////////////////////////////////////
            #version 100
            //#version 300 es
            precision highp float;
            const float PI = 3.1415925;
            uniform vec3 uSunDirection;
            varying vec2 vLatLon;
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
glit::Terrain::heightAt(vec3 pos) const
{
    return radius;
}

float
glit::Terrain::heightAt(glm::vec2 latlon) const
{
    return radius;
}

glit::Mesh*
glit::Terrain::uploadAsWireframe(vec3 viewPosition,
                                 vec3 viewDirection)
{
    wireframeMesh.drawable(0).vertexBuffer()->orphan<Facet::Vertex>();
    wireframeMesh.drawable(0).indexBuffer()->orphan();

    reshape(viewPosition, viewDirection);

    vector<Facet::Vertex> verts;
    vector<uint32_t> indices;
    {
        //util::Timer t("Terrain::upload");
        for (size_t i = 0; i < 20; ++i)
            drawSubtreeWireframe(facets[i], verts, indices);
    }

    wireframeMesh.drawable(0).vertexBuffer()->upload(verts);
    wireframeMesh.drawable(0).indexBuffer()->upload(indices);
    return &wireframeMesh;
}

glit::Mesh*
glit::Terrain::uploadAsTriStrips(vec3 viewPosition,
                                 vec3 viewDirection)
{
    tristripMesh.drawable(0).vertexBuffer()->orphan<Facet::Vertex>();
    tristripMesh.drawable(0).indexBuffer()->orphan();

    reshape(viewPosition, viewDirection);

    vector<Facet::Vertex> verts;
    vector<uint32_t> indices;
    drawSubtreeTriStrip(verts, indices);

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
    }
}

void
glit::Terrain::reshape(vec3 viewPosition, vec3 viewDirection)
{
    for (auto& vert : baseVerts)
        vert.index = uint32_t(-1);

    for (size_t i = 0; i < 20; ++i)
        reshapeN(0, facets[i], viewPosition, viewDirection);
}

void
glit::Terrain::reshapeN(size_t level, Facet& self,
                       vec3 viewPosition, vec3 viewDirection)
{
    self.childVerts[0].index = uint32_t(-1);
    self.childVerts[1].index = uint32_t(-1);
    self.childVerts[2].index = uint32_t(-1);

    vec3 center = (self.verts[0]->vertex.aPosition +
                   self.verts[1]->vertex.aPosition +
                   self.verts[2]->vertex.aPosition) / 3.f;
    vec3 to = center - viewPosition;
    float dist2 = to.x * to.x + to.y * to.y + to.z * to.z;

    // FIXME: do backface removal at this level.
    if (level >= 20 ||
        (EdgeLengths[level] * EdgeLengths[level] * 100.f) < dist2)
        //dist2 > SubdivisionRadiusSquared[level])
    {
        if (self.children)
            deleteChildren(self);
        self.children = nullptr;
        return;
    }

    if (!self.children) {
        // Subdivide allocate and assign verts.
        subdivideFacet(self.verts[0]->vertex.aPosition,
                       self.verts[1]->vertex.aPosition,
                       self.verts[2]->vertex.aPosition,
                       &self.childVerts[0].vertex.aPosition,
                       &self.childVerts[1].vertex.aPosition,
                       &self.childVerts[2].vertex.aPosition);

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

    reshapeN(level + 1, self.children[0], viewPosition, viewDirection);
    reshapeN(level + 1, self.children[1], viewPosition, viewDirection);
    reshapeN(level + 1, self.children[2], viewPosition, viewDirection);
    reshapeN(level + 1, self.children[3], viewPosition, viewDirection);
}

/* static */ uint32_t
glit::Terrain::pushVertex(Facet::VertexAndIndex* insert, vector<Facet::Vertex>& verts)
{
    if (insert->index != uint32_t(-1))
        return insert->index;
    insert->index = verts.size();
    verts.push_back(insert->vertex);
    return insert->index;
}

void
glit::Terrain::drawSubtreeTriStrip(vector<Facet::Vertex>& verts,
                                   vector<uint32_t>& indices)
{
    //util::Timer t("Terrain::upload");
    for (size_t i = 0; i < 20; ++i)
        drawSubtreeTriStripN(facets[i], verts, indices);
}

void
glit::Terrain::drawSubtreeTriStripN(Facet& facet,
                                    vector<Facet::Vertex>& verts,
                                    vector<uint32_t>& indices) const
{
    // Draw leaf triangles.
    if (!facet.children) {
        uint32_t i0 = pushVertex(facet.verts[0], verts);
        uint32_t i1 = pushVertex(facet.verts[1], verts);
        uint32_t i2 = pushVertex(facet.verts[2], verts);
        indices.push_back(i0);
        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);
        indices.push_back(i2);
        indices.push_back(i2);
        return;
    }

    // Recurse into our children.
    drawSubtreeTriStripN(facet.children[0], verts, indices);
    drawSubtreeTriStripN(facet.children[1], verts, indices);
    drawSubtreeTriStripN(facet.children[2], verts, indices);
    drawSubtreeTriStripN(facet.children[3], verts, indices);

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
        uint32_t i0, i1, i2;
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
glit::Terrain::drawSubtreeWireframe(Facet& facet,
                                    vector<Facet::Vertex>& verts,
                                    vector<uint32_t>& indices) const
{
    if (facet.children) {
        drawSubtreeWireframe(facet.children[0], verts, indices);
        drawSubtreeWireframe(facet.children[1], verts, indices);
        drawSubtreeWireframe(facet.children[2], verts, indices);
        drawSubtreeWireframe(facet.children[3], verts, indices);
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
