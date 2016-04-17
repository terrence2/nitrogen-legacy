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
    Terrain(float r);
    //Mesh uploadAsPoints() const;
    Mesh* uploadAsWireframe(glm::vec3 pointOfInterest,
                            glm::vec3 viewDirection);

    float heightAt(glm::vec3 pos) const;
    float heightAt(glm::vec2 latlon) const;

  private:
    //std::shared_ptr<Program> pointsProgram() const { return programPoints; }
    std::shared_ptr<Program> programPoints;
    static std::shared_ptr<Program> makePointsProgram();
    Mesh wireframeMesh;


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
        //   \    /c2\    /
        //    \c1/    \c0/
        //     \/______\/
        //      \      /
        //       \    /
        //        \p2/
        //         \/
        //
        struct Vertex {
            // Store the verts in lat/lon for transfer to the GPU.
            glm::vec3 aPosition;

            static void describe(std::vector<VertexAttrib>& attribs) {
                attribs.push_back(MakeGLMVertexAttrib(Vertex, aPosition, false));
            }
        };
        Facet* children; // 4 wide

        Vertex* verts[3];

        // The children use a combination of our verts and pointers to verts
        // stored here.
        Vertex childVerts[3];

        Facet() {
            memset(this, 0, sizeof(Facet));
        }
    };
    std::vector<Facet::Vertex> baseVerts;



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
    const static float SubdivisionDistance[MaxSubdivisions];
    float SubdivisionRadiusSquared[MaxSubdivisions];
    //
    //
    // Units and Numeric Precision
    // ===========================
    //
    // Ideally we'd express everything in meters and be done. That would,
    // however, put our interesting computations (on and just above the surface
    // of the planet) at around 6,371,000m for an earth-like terrain. Since the
    // max float integer is 16,777,216, this is venturing dangerously close
    // into loss-of-precision territory, right where we care about it most.
    // Instead, we use kilometers for all of the calculations here. This puts
    // the surface calculations back in the thousands and leaves us plenty of
    // room for centimeter granularity in the near field.
    float radius;
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

    static glm::vec3 bisect(glm::vec3 v0, glm::vec3 v1);
    void subdivideFacet(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2,
                        glm::vec3* c0, glm::vec3* c1, glm::vec3* c2) const;
    void reshape(size_t level, Facet& self,
                 glm::vec3 viewPosition, glm::vec3 viewDirection);

    // Given a |facet| at |level|, fill with facet's verts and recurse
    // if more detail is needed.
    Facet* buildDisplayVerts(uint8_t level, Facet& parent, size_t parentBase,
                           Facet* self, uint8_t triPosition,
                           glm::vec3 poi, glm::vec3 viewDirection,
                           std::vector<Facet::Vertex>& verts,
                           std::vector<uint32_t>& indices) const;

    // Given that the tree has already been balanced for the active view,
    // walk current tree and emit verticies for all active children, inserting
    // joining tris as necessary between levels.
    void drawSubtree(Facet& facet,
                     std::vector<Facet::Vertex>& verts,
                     std::vector<uint32_t>& indices) const;

    // Given a list of indices, max draw sized index buffer.
    static std::shared_ptr<IndexBuffer> makeMaxIndexBuffer(const uint16_t* Indices,
                                                           size_t numIndices);

    static uint32_t pushVertex(Facet::Vertex* insert,
                               std::vector<Facet::Vertex>& verts);
    void deleteChildren(Facet& self);

    // The base icosohedron points and initial facets.
    Facet facets[20];
};

} // namespace glit
