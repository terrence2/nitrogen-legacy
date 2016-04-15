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
#include "vertex.h"

using namespace std;

glit::VertexAttrib::VertexAttrib(const char* name, size_t size, GLenum type,
                                 bool normalized, size_t stride, size_t offset)
  : name_(name)
  , size_(size)
  , type_(type)
  , normalized_(normalized)
  , stride_(stride)
  , offset_(offset)
  , index_(-1)
{}

bool
glit::VertexAttrib::operator==(const VertexAttrib& other) const
{
    return name_ == other.name_ &&
           index_ == other.index_ &&
           size_ == other.size_ &&
           type_ == other.type_ &&
           normalized_ == other.normalized_ &&
           stride_ == other.stride_ &&
           offset_ == other.offset_;
}

bool
glit::VertexAttrib::operator !=(const VertexAttrib& other) const
{
    return !operator==(other);
}

void
glit::VertexAttrib::enable(GLint index) const
{
    if (index_ != -1)
        throw runtime_error("double-enable of attribute");

    index_ = index;
    glVertexAttribPointer(index_, size_, type_, normalized_, stride_, (void*)offset_);
    glEnableVertexAttribArray(index);
}

void
glit::VertexAttrib::disable() const
{
    if (index_ == -1)
        throw runtime_error("double disable of attribute");

    glDisableVertexAttribArray(GLuint(index_));
    index_ = -1;
}

bool
glit::VertexDescriptor::operator==(const VertexDescriptor& other) const
{
    return attribs == other.attribs;
}

bool
glit::VertexDescriptor::operator!=(const VertexDescriptor& other) const
{
    return !operator==(other);
}

glit::BufferBase::BufferBase()
  : id(0)
{
    glGenBuffers(1, &id);
}

glit::BufferBase::BufferBase(BufferBase&& other)
  : id(other.id)
{
    other.id = 0;
}

glit::BufferBase::~BufferBase()
{
    if (id)
        glDeleteBuffers(1, &id);
}

glit::VertexBuffer::VertexBuffer(const VertexDescriptor& desc)
  : BufferBase(), vertexDesc_(desc), numVerts_(-1)
{}

glit::VertexBuffer::VertexBuffer(VertexBuffer&& other)
  : BufferBase(forward<BufferBase>(other))
  , vertexDesc_(other.vertexDesc_)
  , numVerts_(other.numVerts_)
{
}

void
glit::VertexBuffer::bind() const
{
    if (!hasData())
        throw runtime_error("no vertex data uploaded");
    glBindBuffer(GL_ARRAY_BUFFER, id);
}

/* static */ void
glit::VertexBuffer::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

glit::IndexBuffer::IndexBuffer()
  : BufferBase(), numIndices_(-1)
{}

glit::IndexBuffer::IndexBuffer(IndexBuffer&& other)
  : BufferBase(forward<BufferBase>(other))
  , numIndices_(other.numIndices_)
{
    other.numIndices_ = -1;
}

void
glit::IndexBuffer::bind() const
{
    if (!hasData())
        throw std::runtime_error("no index data uploaded");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

/* static */ void
glit::IndexBuffer::unbind()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void
glit::IndexBuffer::orphan()
{
    size_t size = 1;
    if (type_ == GL_UNSIGNED_SHORT)
        size = sizeof(uint16_t);
    else if (type_ == GL_UNSIGNED_INT)
        size = sizeof(uint32_t);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices_ * size,
                 nullptr, GL_STATIC_DRAW);
    numIndices_ = -1;
}
