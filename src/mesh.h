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

#include "shader.h"
#include "vertex.h"

namespace glit {

// Tells us how to make a single draw call: i.e. a vertex and index buffer
// with a mode and a draw range over the index buffer plus the program we
// expect to use to do the drawing.
class Drawable
{
    std::shared_ptr<Program> shader;
    std::shared_ptr<VertexBuffer> vb;
    std::shared_ptr<IndexBuffer> ib;
    GLenum mode;
    size_t start;
    size_t count;  // 0 for all.

    Drawable(const Drawable&) = delete;

  public:
    Drawable(std::shared_ptr<Program> program,
             GLenum mode,
             std::shared_ptr<VertexBuffer> verts,
             std::shared_ptr<IndexBuffer> indices,
             size_t start = 0, size_t count = 0);
    Drawable(Drawable&& other);

    std::shared_ptr<VertexBuffer> vertexBuffer() const {
        return vb;
    }

    std::shared_ptr<IndexBuffer> indexBuffer() const {
        return ib;
    }

    template <typename ...Args>
    void draw(Args&&... args) const {
        AutoBindVertexBuffer vbind(*vb);
        AutoBindIndexBuffer ibind(*ib);
        {
            shader->use();
            shader->bindUniforms<0>(args...);
            Program::AutoEnableAttributes aea(*shader, *vb);
            {
                size_t cnt = count == 0 ? ib->numIndices() : count;
                auto offset = util::BufferOffset<uint16_t>(start);
                glDrawElements(mode, cnt, ib->type(), offset);
            } // Disable attributes.
        } // Unbind buffers.
    }
};

// A collection of drawables that represent a single thing.
class Mesh
{
    std::vector<Drawable> drawables;

  public:
    Mesh();
    explicit Mesh(Drawable&& d);
    explicit Mesh(std::vector<Drawable>&& ds);

    Drawable& drawable(size_t offset) { return drawables[offset]; }

    template <typename ...Args>
    void draw(Args&&... args) const {
        for (auto& drawable : drawables) {
            // TODO: what if our programs need different args?
            drawable.draw(std::forward<Args>(args)...);
        }
    }
};

} // namespace glit
