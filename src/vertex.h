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

#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

namespace glit {

template<typename T, size_t N>
constexpr size_t
ArrayLength(T (&aArr)[N])
{
    return N;
}

// Turn a C++ type into its matching GLenum.
template <typename T> struct MapTypeToGLEnum {};
#define MAKE_MAP(D) \
    D(float, GL_FLOAT) \
    D(uint8_t, GL_UNSIGNED_BYTE)
#define EXPAND_MAP_ITEM(ty, en) \
    template <> struct MapTypeToGLEnum<ty> { \
        static const GLenum value = en; \
    };
MAKE_MAP(EXPAND_MAP_ITEM)
#undef EXPAND_MAP_ITEM
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
        glit::MapTypeToGLEnum< \
                std::remove_extent< \
                    decltype(attrname) \
                >::type \
            >::value, \
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

// A collection of verticies on the GPU.
class VertexBuffer
{
    GLuint id;
    const VertexDescriptor& vertexDesc_;

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&&) = delete;

  public:
    VertexBuffer(const VertexDescriptor& desc);
    ~VertexBuffer();

    const VertexDescriptor& vertexDesc() const { return vertexDesc_; }

    void bind() const;
    void buffer(size_t size, void* data);
};

} // namespace glit
