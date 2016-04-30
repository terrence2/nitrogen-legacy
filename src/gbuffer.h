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

#include <glm/glm.hpp>

#include "glwrapper.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"
#include "vertex.h"

namespace glit {

// Manager multiple render targets.
class GBuffer
{
    GLuint frameBuffer;
    std::shared_ptr<Texture> renderTargets[1];

    std::shared_ptr<Texture> colorBuffer() const { return renderTargets[0]; }
    //GLuint depthBuffer() const { return renderTargets[1]; }
    //GLuint stencilBuffer() const { return renderTargets[2]; }

    struct Vertex {
        glm::vec3 aPosition;
        glm::vec2 aTexCoord;

        static void describe(std::vector<VertexAttrib>& attribs) {
            attribs.push_back(MakeGLMVertexAttrib(Vertex, aPosition, false));
            attribs.push_back(MakeGLMVertexAttrib(Vertex, aTexCoord, false));
        }
    };

    static std::shared_ptr<glit::Program> makeDeferredRenderProgram();
    glit::Drawable screenRenderer;

    void bind() const;
    void unbind() const;

  public:
    GBuffer(int width, int height);
    ~GBuffer();

    void screenSizeChanged(int width, int height);

    struct AutoBindBuffer {
        AutoBindBuffer(const GBuffer& gbuf);
        ~AutoBindBuffer();
    };

    void deferredRender();

  private:
    GBuffer(GBuffer&&) = delete;
    GBuffer(const GBuffer&) = delete;
};

} // namespace glit
