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
#include "skybox.h"

#include "icosphere.h"

using namespace std;
using namespace glm;

glit::Skybox::Skybox()
  : drawable(makeSkyboxProgram(), GL_TRIANGLES,
             make_shared<VertexBuffer>(VertexDescriptor::fromType<Vertex>()),
             make_shared<IndexBuffer>())
{
    // Use an icosphere to minimize distortions from texel shape near corners.
    IcoSphere sphere(3);

    vector<Vertex> verts;
    vector<uint16_t> indices;
    for (auto& v : sphere.vertices())
        verts.push_back(Vertex{v.aPosition});
    for (auto& face : sphere.faceList()) {
        indices.push_back(face.i0);
        indices.push_back(face.i1);
        indices.push_back(face.i2);
    }
    drawable.vertexBuffer()->upload(verts);
    drawable.indexBuffer()->upload(indices);
}

/* static */ shared_ptr<glit::Program>
glit::Skybox::makeSkyboxProgram()
{
    // Generate the lighting program.
    auto VertexDesc = VertexDescriptor::fromType<Vertex>();
    VertexShader vs(
            R"SHADER(
            ///////////////////////////////////////////////////////////////////
            #version 100
            precision highp float;
            attribute vec3 aPosition;
            uniform mat4 uModelViewProj;

            void main()
            {
                gl_Position = uModelViewProj * vec4(aPosition, 1.0);
            }
            ///////////////////////////////////////////////////////////////////
            )SHADER",
            VertexDesc);
    FragmentShader fs(
            R"SHADER(
            ///////////////////////////////////////////////////////////////////
            #version 100
            //#include <noise3D.glsl>
            precision highp float;

            void main() {
                gl_FragData[0] = vec4(1.0, 0.0, 1.0, 1.0);
            }
            ///////////////////////////////////////////////////////////////////
            )SHADER"
        );
    auto prog = make_shared<Program>(move(vs), move(fs), vector<UniformDesc>{
                Program::MakeInput<mat4>("uModelViewProj"),
            });

    return prog;
}

void
glit::Skybox::tick(double t, double dt)
{
}

void
glit::Skybox::draw(const Camera& camera)
{
    // Transform back to the origin to draw the skybox.
    Camera cam(camera);
    cam.move(vec3(0.f, 0.f, 0.f));

    drawable.draw(cam.transform());
}
