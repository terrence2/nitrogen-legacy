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
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>

namespace glit {

template<typename T, size_t N>
constexpr size_t
ArrayLength(T (&aArr)[N])
{
    return N;
}

// Turn a C++ type into its matching GLenum.
template <typename T> struct MapTypeToTraits{};
template <const char* Name> struct MapNameToTraits{};
#define MAKE_MAP(D) \
    D(float, GL_FLOAT, 1, 1) \
    D(uint8_t, GL_UNSIGNED_BYTE, 1, 1) \
    D(glm::vec3, GL_FLOAT, 3, 1) \
    D(glm::mat4, GL_FLOAT, 4, 4)
#define EXPAND_MAP_ITEM(ty, en, rows_, cols_) \
    template <> struct MapTypeToTraits<ty> { \
        using type = ty; \
        static const GLenum gl_enum = (en); \
        static const uint8_t rows = (rows_); \
        static const uint8_t cols = (cols_); \
        static const uint8_t extent = (rows_) * (cols_); \
    };
MAKE_MAP(EXPAND_MAP_ITEM)
#undef EXPAND_MAP_ITEM

#define COMPARE(ty, en, _2, _3) if (name == std::string(#ty)) { return en; }
inline GLenum
MapTypeNameToGLenum(std::string name) {
    MAKE_MAP(COMPARE)
    return -1;
}
#undef COMPARE

#undef MAKE_MAP

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
                 bool normalized, size_t stride, size_t offset)
      : name_(name), size_(size), type_(type), normalized_(normalized),
        stride_(stride), offset_(offset), index_(-1)
    {}

    bool operator==(const VertexAttrib& other) const {
        return name_ == other.name_ &&
               index_ == other.index_ &&
               size_ == other.size_ &&
               type_ == other.type_ &&
               normalized_ == other.normalized_ &&
               stride_ == other.stride_ &&
               offset_ == other.offset_;
    }
    bool operator !=(const VertexAttrib& other) const {
        return !operator==(other);
    }

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
    template <typename Vertex>
    static VertexDescriptor fromType() {
        VertexDescriptor self;
        Vertex::describe(self.attribs);
        return self;
    }

    const std::vector<VertexAttrib>& attributes() const { return attribs; }

    bool operator==(const VertexDescriptor& other) const {
        return attribs == other.attribs;
    }
    bool operator !=(const VertexDescriptor& other) const {
        return !operator==(other);
    }
};

// Manage a GL buffer object.
class BufferBase
{
    BufferBase(const BufferBase&) = delete;

  protected:
    BufferBase() : id(0) {
        glGenBuffers(1, &id);
    }
    BufferBase(BufferBase&& other) : id(other.id) {
        other.id = 0;
    }
    ~BufferBase() {
        if (id)
            glDeleteBuffers(1, &id);
    }

    GLuint id;
};

// A collection of verticies on the GPU.
class VertexBuffer : BufferBase
{
    const VertexDescriptor vertexDesc_;
    size_t numVerts_;

    VertexBuffer(const VertexBuffer&) = delete;

  public:
    VertexBuffer(const VertexDescriptor& desc)
      : BufferBase(), vertexDesc_(desc), numVerts_(-1)
    {}
    VertexBuffer(VertexBuffer&& other)
      : BufferBase(std::forward<BufferBase>(other)),
        vertexDesc_(other.vertexDesc_),
        numVerts_(other.numVerts_)
    {
        other.numVerts_ = -1;
    }

    const VertexDescriptor& vertexDesc() const { return vertexDesc_; }
    size_t numVerts() const { return numVerts_; }

    void bind() const {
        if (numVerts_ == size_t(-1))
            throw std::runtime_error("no vertex data uploaded");
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }

    static void unbind() {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    template <typename VertexType>
    void upload(const std::vector<VertexType>& verts) {
        if (vertexDesc_ != VertexDescriptor::fromType<VertexType>())
            throw std::runtime_error("attempting to upload into wrong buffer type");
        numVerts_ = verts.size();
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(VertexType),
                     &verts[0], GL_STATIC_DRAW);
        std::cout << "uploaded " << verts.size() << " verts" << std::endl;
    }
};

// A collection of indexes on the GPU.
class IndexBuffer : BufferBase
{
    size_t numIndices_;

  public:
    IndexBuffer() : BufferBase(), numIndices_(-1) {}
    IndexBuffer(IndexBuffer&& other)
      : BufferBase(std::forward<BufferBase>(other)),
        numIndices_(other.numIndices_)
    {
        other.numIndices_ = -1;
    }

    size_t numIndices() const { return numIndices_; }

    void bind() const {
        if (numIndices_ == size_t(-1))
            throw std::runtime_error("no index data uploaded");
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    }

    static void unbind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void upload(const std::vector<uint16_t>& indices) {
        numIndices_ = indices.size();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t),
                     &indices[0], GL_STATIC_DRAW);
        std::cout << "uploaded " << indices.size() << " indices" << std::endl;
    }
};

// A collection of buffers suitable for drawing a thing.
class PrimitiveData
{
    VertexBuffer vb;
    IndexBuffer ib;
    GLenum primitiveKind;

    PrimitiveData(const PrimitiveData&) = delete;

  public:
    PrimitiveData(GLenum kind, VertexBuffer&& v, IndexBuffer&& i)
      : vb(std::forward<VertexBuffer>(v)),
        ib(std::forward<IndexBuffer>(i)),
        primitiveKind(kind)
    {}
    PrimitiveData(PrimitiveData&& other)
      : vb(std::forward<VertexBuffer>(other.vb)),
        ib(std::forward<IndexBuffer>(other.ib)),
        primitiveKind(other.primitiveKind)
    {
        other.primitiveKind = -1;
    }

    const VertexBuffer& vertexBuffer() const { return vb; }

    void bind() const {
        vb.bind();
        ib.bind();
    }

    void unbind() const {
        ib.unbind();
        vb.unbind();
    }

    void draw() const {
        glDrawElements(primitiveKind, ib.numIndices(), GL_UNSIGNED_SHORT, 0);
    }
};

class AutoBindPrimitiveData
{
    AutoBindPrimitiveData(AutoBindPrimitiveData&&) = delete;
    AutoBindPrimitiveData(const AutoBindPrimitiveData&) = delete;

    const PrimitiveData& primitive;

  public:
    AutoBindPrimitiveData(const PrimitiveData& p) : primitive(p) { primitive.bind(); }
    ~AutoBindPrimitiveData() { primitive.unbind(); }
};

} // namespace glit
