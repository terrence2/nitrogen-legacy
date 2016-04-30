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

#include "mesh.h"
#include "shader.h"
#include "utility.h"
#include "vertex.h"

#include <glm/glm.hpp>

using namespace std;
using namespace glm;


glit::GBuffer::GBuffer()
  : frameBuffer(0)
  , renderTargets{0, 0, 0}
{
}

glit::GBuffer::~GBuffer()
{
    cleanup();
}

void
glit::GBuffer::screenSizeChanged(int width, int height)
{
    cleanup();
    GLClearError();

    GLsizei w = GLsizei(width);
    GLsizei h = GLsizei(height);

    /*
    glGenRenderbuffers(util::ArrayLength(renderBuffers), renderBuffers);
    GLCheckError();

    glBindRenderbuffer(GL_RENDERBUFFER, colorBuffer());
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, w, h);
    GLCheckError();
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer());
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, w, h);
    GLCheckError();
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, stencilBuffer());
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, w, h);
    GLCheckError();
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    */

    /*
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, colorBuffer());
    GLCheckError();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthBuffer());
    GLCheckError();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, stencilBuffer());
    GLCheckError();
    */

    glGenTextures(3, renderTargets);
    glBindTexture(GL_TEXTURE_2D, colorBuffer());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, 0);
    GLCheckError();

    glad_glGenFramebuffers(1, &frameBuffer);
    glad_glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glad_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, colorBuffer(), 0);
    GLCheckError();

    GLenum status = glad_glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw runtime_error("Failed to create frame buffer: " +
                            FrameBufferErrorToString(status));
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void
glit::GBuffer::cleanup()
{
    if (frameBuffer) {
        glDeleteFramebuffers(1, &frameBuffer);
        frameBuffer = 0;
    }

    for (size_t i = 0; i < util::ArrayLength(renderTargets); ++i) {
        if (renderTargets[i]) {
            glDeleteTextures(1, &renderTargets[i]);
            renderTargets[i] = 0;
        }
    }
    GLCheckError();
}

void
glit::GBuffer::deferredRender()
{
    GLClearError();

    struct Vertex {
        vec3 aPosition;
        vec2 aTexCoord;

        static void describe(std::vector<VertexAttrib>& attribs) {
            attribs.push_back(MakeGLMVertexAttrib(Vertex, aPosition, false));
            attribs.push_back(MakeGLMVertexAttrib(Vertex, aTexCoord, false));
        }
    };
    vector<Vertex> verts{
        {{-1.f, -1.f, 0.f}, {0.f, 0.f}},
        {{-1.f,  1.f, 0.f}, {0.f, 1.f}},
        {{ 1.f, -1.f, 0.f}, {1.f, 0.f}},
        {{ 1.f,  1.f, 0.f}, {1.f, 1.f}},
    };
    vector<uint8_t> indices{0, 1, 2, 3};
    auto vb = VertexBuffer::make<Vertex>(verts);
    auto ib = IndexBuffer::make(indices);
    GLCheckError();

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
                Program::MakeInput<int>("uDiffuseColor"),
            });
    GLCheckError();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffer());
    GLCheckError();

    Drawable dr(prog, GL_TRIANGLE_STRIP, vb, ib);
    GLCheckError();
    //dr.draw(colorBuffer());
    dr.draw(0);
    GLCheckError();
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
    GLCheckError();
}

glit::GBuffer::AutoBindBuffer::~AutoBindBuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
