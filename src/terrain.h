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

#include <limits>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "icosphere.h"
#include "mesh.h"
#include "shader.h"
#include "vertex.h"
#include "utility.h"

namespace glit {

// An ico-sphere that automatically scales its resolution dependent on
// the camera's view.
class Terrain
{
  public:
    Terrain(double r);
    ~Terrain();
    Mesh* uploadAsWireframe(const glm::dvec3& viewPosition,
                            const glm::dvec3& viewDirection);
    Mesh* uploadAsTriStrips(const glm::dvec3& viewPosition,
                            const glm::dvec3& viewDirection);

    double heightAt(glm::dvec3 pos) const;

  private:
    std::shared_ptr<Program> programPoints;
    static std::shared_ptr<Program> makePointsProgram();
    Mesh wireframeMesh;
    Mesh tristripMesh;

    double radius;

    // A facet is the subdividable piece of the terrain.
    //
    // Each facet is one side of the isocohedron (20 at the root), or one of
    // the subdivided sub-triangles within the isocohedron. Each facet holds
    // pointers to its four children. Children are usually null and are
    // constructed on the fly if we are close enough to the facet to make it
    // worth drawing and removed if we are too far away.
    //
    // Although there are only four triangles in each subdivision, the magical
    // power of exponential growth means that with only 23 levels, we can cover
    // the entire surface of the earth at sub-meter resolution. Obviously we
    // cannot draw 250K+ individual buffers in each frame, so we need to stream
    // the virtual tree out to the GPU on each frame. We'd probably need to do
    // this anyway since our camera is likely to be moving and any sort of
    // caching is just going to lead to unpredictable latency.
    struct Facet
    {
        // The vertex Numbers are as follows.
        //  ________________
        //  \p0    /\    p1/
        //   \ A  /c2\ C  /
        //    \c1/ B  \c0/
        //     \/______\/
        //      \      /
        //       \ D  /
        //        \p2/
        //         \/
        //

        // Given the world scale, we have to compute everything in double
        // precision to avoid juddering on small movements. This is not a huge
        // slowdown since we have to compute everything on the CPU anyway. For
        // upload, we translate everything to be camera origin before
        // truncating to floats so that near verticies are all small and have
        // comparatively high precision.
        struct CPUVertex {
            glm::dvec3 position;
            glm::vec3 normal;
        };
        struct GPUVertex {
            glm::vec3 aPosition;
            glm::vec3 aNormal;

            static GPUVertex fromCPU(const CPUVertex& v,
                                     const Facet& owner,
                                     const glm::dvec3& viewPosition);

            static void describe(std::vector<VertexAttrib>& attribs) {
                attribs.push_back(MakeGLMVertexAttrib(GPUVertex, aPosition, false));
                attribs.push_back(MakeGLMVertexAttrib(GPUVertex, aNormal, false));
            }
        };

        Facet* children; // 4 wide

        // Cached normal to speed up vertex normal computations.
        glm::vec3 normal;

        struct VertexAndIndex {
            CPUVertex vertex; // Refers to baseVerts or childVerts.
            uint32_t index; // Reset by reshape. Invalid is -1.
        };
        VertexAndIndex* verts[3];

        // The children use a combination of our verts and pointers to verts
        // stored here.
        VertexAndIndex childVerts[3];

        Facet() {
            //memset(this, 0, sizeof(Facet));
        }
        void init(VertexAndIndex* v0, VertexAndIndex* v1, VertexAndIndex* v2);
    };

    std::vector<Facet::VertexAndIndex> baseVerts;


    // LOD
    // ===
    //
    // The number of tris we want to show for any particular patch is going to
    // vary by its angle to us, it's relative smoothness, its height relative
    // to the surroundings, how interesting the content on it is, etc. Instead,
    // to simplify the computation (as we currently have to do this all on the
    // CPU), we use the distance alone. The following table is expressed in
    // terms of Radii of the planetary body. Moreover, we want to make the
    // distance calculations in the squared space to avoid the sqrt. This is
    // all multiplied out in the constructor into the real table.
    const static size_t MaxSubdivisions = 23;
    float EdgeLengths[MaxSubdivisions];
    //
    //
    // Allocation
    // ==========
    //
    // Fast allocation, and more importantly deallocation or reuse, is critical
    // for having even frame rates, since this scheme requires potentially 10's of
    // thousands of allocations and deallocations per frame, if the camera is
    // moving fast or low.
    //
    // Note: handwavy as this is still TODO.
    // All allocations are fixed size, so this is actually fairly easy to
    // achieve. Allocations check a free list first. If the free list is empty,
    // they bump allocate from the current end. Deallocation links the empty
    // facet into the head of the free list. This is, naturally, going to spike
    // to the maximum used size and never drop. We could compact, but in order
    // to maintain predictable performance, instead we simply cap the
    // allocation maximum and use a breadth first approach when visiting the
    // tree so that we only lose fine detail if we run out of room in the Facet
    // heap. This allows us to pre-allocate our entire terrain cache and avoid
    // thrashing with the rest of the system.

    // Each subdivision doubles angular resulution. Provide a LUT to get
    // this resolution for any given level.
    //float levelAngles[25];
    //static void buildLevelAngles(float angle0) const;

    static glm::dvec3 bisect(glm::dvec3 v0, glm::dvec3 v1);
    void subdivideFacet(glm::dvec3 p0, glm::dvec3 p1, glm::dvec3 p2,
                        glm::dvec3* c0, glm::dvec3* c1, glm::dvec3* c2) const;
    void reshape(const glm::dvec3& viewPosition,
                 const glm::dvec3& viewDirection);
    void reshapeN(size_t level, Facet& self,
                  const glm::dvec3& viewPosition,
                  const glm::dvec3& viewDirection);
    double reshapeHorizon;

    // Given that the tree has already been balanced for the active view,
    // walk current tree and emit verticies for all active children, inserting
    // joining tris as necessary between levels.
    void drawSubtreeTriStrip(const glm::dvec3& viewPosition,
                             std::vector<Facet::GPUVertex>& verts,
                             std::vector<uint32_t>& indices);
    void drawSubtreeTriStripN(Facet& facet, const glm::dvec3& viewPosition,
                              std::vector<Facet::GPUVertex>& verts,
                              std::vector<uint32_t>& indices) const;

    // Spit out complete triangles for all faces. We don't need joins because
    // they would just overlay lines that are already present.
    void drawSubtreeWireframe(Facet& facet, const glm::dvec3& viewPosition,
                              std::vector<Facet::GPUVertex>& verts,
                              std::vector<uint32_t>& indices) const;

    static uint32_t pushVertex(Facet::VertexAndIndex* insert,
                               const Facet& owner,
                               const glm::dvec3& viewPosition,
                               std::vector<Facet::GPUVertex>& verts);
    void deleteChildren(Facet& self);

    // The base icosohedron points and initial facets.
    Facet facets[20];
};

} // namespace glit
