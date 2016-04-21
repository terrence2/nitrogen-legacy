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

#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>

#include "utility.h"

namespace glit {

// VertexAttrib contains all of the data required to map a piece of a program's
// vertex format onto the shader's input system, and verifies dynamically that
// the compiled program and the data format match.
//
// Both vertex data layout and how it maps to the underlying shader code
// are given to opengl via a single call. Most of the data is inherent to
// the Vertex class, but some of it can come from the program itself.
//
// void glVertexAttribPointer(
//      GLuint index,
//  	GLint size,
//  	GLenum type,
//  	GLboolean normalized,
//  	GLsizei stride,
//  	const GLvoid * pointer);
//
// Index may come from the shader, but may also be given to the shader;
// however, we then need to coordinate to ensure that no two attributes
// use the same index.
//
// Size is in [1,4] for number of components. Typically ArrayLength(name).
//
// The Type is an enum based on the type.
//
// Normalized tells GL whether to rerange when converting to float.
//
// Stride is typically sizeof(Vertex).
//
// Pointer is not a pointer, but the offset into the ABO where the first
// thing is stored. If the ABO is a single object, this is just
// offsetof(Vertex, name). Otherwise the position of the thing being drawn
// must also be included.
class VertexAttrib
{
    const char* name_;
    int size_;
    GLenum type_;
    bool normalized_;
    size_t stride_;
    size_t offset_;

    // Derived from the program.
    mutable GLint index_;

  public:
    VertexAttrib(const char* name, size_t size, GLenum type,
                 bool normalized, size_t stride, size_t offset);

    bool operator==(const VertexAttrib& other) const;
    bool operator !=(const VertexAttrib& other) const;

    const char* name() const { return name_; }
    GLenum type() const { return type_; }

    void enable(GLint index) const;
    void disable() const;
};

// Given a class with attribute named |attrname|, in |cls|'s scope,
// make a VertexAttrib describing that attribute. |normalized| is the
// only attribute that cannot be derived automatically.
#define MakeVertexAttrib(cls, attrname, normalized) \
    glit::VertexAttrib( \
        #attrname, \
        std::extent<decltype(attrname)>::value, \
        glit::MapTypeToTraits< \
                std::remove_extent< \
                    decltype(attrname) \
                >::type \
            >::gl_enum, \
        (normalized), \
        sizeof(cls), \
        offsetof(cls, attrname))

#define MakeGLMVertexAttrib(cls, attrname, normalized) \
    glit::VertexAttrib( \
        #attrname, \
        glit::MapTypeToTraits<decltype(attrname)>::extent, \
        glit::MapTypeToTraits<decltype(attrname)>::gl_enum, \
        (normalized), \
        sizeof(cls), \
        offsetof(cls, attrname))

// A static definition of a vertex's attributes.
class VertexDescriptor
{
    std::vector<VertexAttrib> attribs;

  public:
    VertexDescriptor() {}

    template <typename Vertex>
    static VertexDescriptor fromType() {
        VertexDescriptor self;
        Vertex::describe(self.attribs);
        return self;
    }

    const std::vector<VertexAttrib>& attributes() const { return attribs; }

    bool operator==(const VertexDescriptor& other) const;
    bool operator!=(const VertexDescriptor& other) const;
};

// Manage a GL buffer object.
class BufferBase
{
  protected:
    GLuint id;

    BufferBase();
    BufferBase(BufferBase&& other);
    ~BufferBase();

  private:
    BufferBase(const BufferBase&) = delete;
};

// A collection of verticies on the GPU.
class VertexBuffer : BufferBase
{
    const VertexDescriptor vertexDesc_;
    size_t numVerts_;

  public:
    explicit VertexBuffer(const VertexDescriptor& desc);
    VertexBuffer(VertexBuffer&& other);

    const VertexDescriptor& vertexDesc() const { return vertexDesc_; }
    size_t numVerts() const { return numVerts_; }
    bool hasData() const { return numVerts_ != size_t(-1); }

    void bind() const;
    static void unbind();

    template <typename VertexType>
    void orphan() {
        if (vertexDesc_ != VertexDescriptor::fromType<VertexType>())
            throw std::runtime_error("orphaning with wrong vertex type");
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, numVerts_ * sizeof(VertexType),
                     nullptr, GL_STATIC_DRAW);
        numVerts_ = -1;
    }

    template <typename VertexType>
    static std::shared_ptr<VertexBuffer> make(const std::vector<VertexType>& verts)
    {
        auto buf = std::make_shared<VertexBuffer>(
                VertexDescriptor::fromType<VertexType>());
        buf->upload(verts);
        return buf;
    }

    template <typename VertexType>
    void upload(const std::vector<VertexType>& verts,
                size_t offset = 0, size_t count = 0)
    {
        if (vertexDesc_ != VertexDescriptor::fromType<VertexType>())
            throw std::runtime_error("attempting to upload into wrong buffer type");
        numVerts_ = count == 0
                    ? verts.size() - offset
                    : std::min(verts.size() - offset, count);
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, numVerts_ * sizeof(VertexType),
                     &verts[offset], GL_STATIC_DRAW);
        //std::cout << "uploaded " << numVerts_ << " verts (" <<
        //             (numVerts_ * sizeof(VertexType))<< " bytes)" << std::endl;
    }

  private:
    VertexBuffer(const VertexBuffer&) = delete;
};

// A collection of indexes on the GPU.
class IndexBuffer : BufferBase
{
    size_t numIndices_;
    GLenum type_;

  public:
    IndexBuffer();
    IndexBuffer(IndexBuffer&& other);

    size_t numIndices() const { return numIndices_; }
    bool hasData() const { return numIndices_ != size_t(-1); }
    GLenum type() const { return type_; }

    void bind() const;
    static void unbind();
    void orphan();

    template <typename IntType>
    static std::shared_ptr<IndexBuffer> make(const std::vector<IntType>& indices) {
        auto buf = std::make_shared<IndexBuffer>();
        buf->upload(indices);
        return buf;
    }

    void upload(const std::vector<uint16_t>& indices) {
        type_ = GL_UNSIGNED_SHORT;
        numIndices_ = indices.size();
        bind();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices_ * sizeof(uint16_t),
                     &indices[0], GL_STATIC_DRAW);
        //std::cout << "uploaded " << numIndices_ << " indices (" <<
        //             (2 * numIndices_)<< " bytes)" << std::endl;
    }

    void upload(const std::vector<uint32_t>& indices) {
        type_ = GL_UNSIGNED_INT;
        numIndices_ = indices.size();
        bind();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices_ * sizeof(uint32_t),
                     &indices[0], GL_STATIC_DRAW);
        //std::cout << "uploaded " << numIndices_ << " indices (" <<
        //             (4 * numIndices_)<< " bytes)" << std::endl;
    }
};

template <typename BufferType>
class AutoBindBuffer
{
    const BufferType& buffer;

  public:
    AutoBindBuffer(const BufferType& buf)
      : buffer(buf)
    {
        buffer.bind();
    }
    ~AutoBindBuffer() {
        buffer.unbind();
    }
};

using AutoBindVertexBuffer = AutoBindBuffer<VertexBuffer>;
using AutoBindIndexBuffer = AutoBindBuffer<IndexBuffer>;

template <typename MeshType>
class AutoBindPrimitiveData
{
    AutoBindPrimitiveData(AutoBindPrimitiveData&&) = delete;
    AutoBindPrimitiveData(const AutoBindPrimitiveData&) = delete;

    const MeshType& primitive;

  public:
    AutoBindPrimitiveData(const MeshType& p) : primitive(p) { primitive.bind(); }
    ~AutoBindPrimitiveData() { primitive.unbind(); }
};

} // namespace glit
