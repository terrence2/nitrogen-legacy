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

#include "camera.h"
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
        verts.push_back(Vertex{v.aPosition * Camera::FarDistance});
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
            #extension GL_EXT_draw_buffers : require
            precision highp float;
            attribute vec3 aPosition;
            uniform mat4 uModelViewProj;
            varying vec3 vPosition;

            void main()
            {
                vPosition = aPosition;
                gl_Position = uModelViewProj * vec4(aPosition, 1.0);
            }
            ///////////////////////////////////////////////////////////////////
            )SHADER",
            VertexDesc);
    FragmentShader fs(
            R"SHADER(
            ///////////////////////////////////////////////////////////////////
            #version 100
            #extension GL_EXT_draw_buffers : require
            precision highp float;
            #include <noise3D.glsl>
            varying vec3 vPosition;

            float fbm(vec3 pos) {
               // sum(i=0..n, w**i * noise(s**i * xyz))
               float acc = 0.0;
               const float AmplitudeDelta = 0.5;
               const float ScaleDelta = 2.0;
               float a = AmplitudeDelta;
               float s = ScaleDelta;
               for (int i = 0; i < 4; ++i) {
                   acc += a * snoise(s * pos);
                   a *= AmplitudeDelta;
                   s *= ScaleDelta;
               }
               return acc;
            }

            void main() {
                float intensity = fbm(vPosition / 1000.0);
                gl_FragData[0] = vec4(intensity, intensity, intensity, 1.0);
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
