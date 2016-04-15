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
#include "mesh.h"

using namespace std;
using namespace glm;

glit::Drawable::Drawable(shared_ptr<Program> program,
                         GLenum mode,
                         shared_ptr<VertexBuffer> verts,
                         shared_ptr<IndexBuffer> indices,
                         size_t start /* = 0 */, size_t count /* = 0 */)
  : shader(program)
  , vb(verts)
  , ib(indices)
  , mode(mode)
  , start(start)
  , count(count)
{}

glit::Drawable::Drawable(Drawable&& other)
  : shader(other.shader)
  , vb(other.vb)
  , ib(other.ib)
  , mode(other.mode)
  , start(other.start)
  , count(other.count)
{}

glit::Mesh::Mesh()
{}

glit::Mesh::Mesh(Drawable&& d)
{
    drawables.push_back(forward<Drawable>(d));
}

glit::Mesh::Mesh(vector<Drawable>&& ds)
  : drawables(forward<vector<Drawable>>(ds))
{
}
