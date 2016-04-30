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
#include "gbuffer.h"

#include <utility>

#include "utility.h"
#include "window.h"

#include <glm/glm.hpp>

using namespace std;
using namespace glm;

glit::GBuffer::GBuffer(int width, int height)
  : frameBuffer(0)
  , screenRenderer(makeDeferredRenderProgram(), GL_TRIANGLE_STRIP,
                   make_shared<VertexBuffer>(VertexDescriptor::fromType<Vertex>()),
                   make_shared<IndexBuffer>())
{
    // Create the frame buffer.
    glad_glGenFramebuffers(1, &frameBuffer);

    // Use the screen size change mechanism to initialize our FBO.
    screenSizeChanged(width, height);

    // Generate fragments for us to use for blitting.
    vector<Vertex> verts{
        {{-1.f, -1.f, 0.f}, {0.f, 0.f}},
        {{-1.f,  1.f, 0.f}, {0.f, 1.f}},
        {{ 1.f, -1.f, 0.f}, {1.f, 0.f}},
        {{ 1.f,  1.f, 0.f}, {1.f, 1.f}},
    };
    vector<uint8_t> indices{0, 1, 2, 3};
    screenRenderer.vertexBuffer()->upload(verts);
    screenRenderer.indexBuffer()->upload(indices);
}

glit::GBuffer::~GBuffer()
{
    glDeleteFramebuffers(1, &frameBuffer);
}

/* static */ shared_ptr<glit::Program>
glit::GBuffer::makeDeferredRenderProgram()
{
    // Generate the lighting program.
    auto VertexDesc = VertexDescriptor::fromType<Vertex>();
    VertexShader vs(
            R"SHADER(
            ///////////////////////////////////////////////////////////////////
            #version 100
            precision highp float;
            attribute vec3 aPosition;
            attribute vec2 aTexCoord;
            varying vec2 vTexCoord;

            void main()
            {
                gl_Position = vec4(aPosition, 1.0);
                vTexCoord = aTexCoord;
            }
            ///////////////////////////////////////////////////////////////////
            )SHADER",
            VertexDesc);
    FragmentShader fs(
            R"SHADER(
            ///////////////////////////////////////////////////////////////////
            #version 100
            precision highp float;
            uniform sampler2D uDiffuseColor;
            varying vec2 vTexCoord;

            void main() {
                gl_FragColor = texture2D(uDiffuseColor, vTexCoord);
            }
            ///////////////////////////////////////////////////////////////////
            )SHADER"
        );
    auto prog = make_shared<Program>(move(vs), move(fs), vector<UniformDesc>{
                Program::MakeInput<Texture>("uDiffuseColor"),
            });

    return prog;
}

void
glit::GBuffer::deferredRender()
{
    screenRenderer.draw(*colorBuffer());
}

void
glit::GBuffer::screenSizeChanged(int width, int height)
{
    // (Re)Create the target texture buffers.
    renderTargets[0] = Texture::makeForScreen(width, height);

    // Update the frame buffer to target the new textures.
    glad_glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glad_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, colorBuffer()->id(), 0);
    GLenum status = glad_glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw runtime_error("Failed to create frame buffer: " +
                            FrameBufferErrorToString(status));
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glit::GBuffer::AutoBindBuffer::AutoBindBuffer(const GBuffer& gbuf)
{
    if (!gbuf.frameBuffer)
        throw runtime_error("attempting to bind an unconfigured fbo");

    // FIXME: do we need to bind the color buffer first?
    // FIXME: we need the shader to output to gl_Frag...SOMETHING
    // FIXME: we need to draw the texture to screen afterwards.
    glBindFramebuffer(GL_FRAMEBUFFER, gbuf.frameBuffer);

    //static const GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT};
    static const GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(util::ArrayLength(DrawBuffers), DrawBuffers);
}

glit::GBuffer::AutoBindBuffer::~AutoBindBuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
