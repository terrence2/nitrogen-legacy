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
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vertex.h"

namespace glit {

// Information required to look up and bind a uniform.
class UniformDesc
{
    const char* name_;
    GLenum type_;
    uint8_t cols_;
    uint8_t rows_;

    UniformDesc() = delete;

  public:
    UniformDesc(const char* name, GLenum type,
                uint8_t cols = 1, uint8_t rows = 1);
    const char* name() const { return name_; }
    GLenum glEnum() const { return type_; }
    bool isScalar() const { return cols_ == 1 && rows_ == 1; }
    bool isVector() const { return cols_ != 1 && rows_ == 1; }
    bool isMatrix() const { return cols_ != 1 && rows_ != 1; }
};

// Manages a shader id and the compilation process.
template <GLenum Type>
class BaseShader
{
    friend class Program;
    GLuint id;

    BaseShader(const BaseShader&) = delete;

  protected:
    static std::string bundleImports(const std::string& source);
    static void loadIncludeFile(const std::string& line,
                                std::vector<std::string>& output);

  public:
    BaseShader(std::string source);
    BaseShader(BaseShader&& other);
    ~BaseShader();
};

// A shader which can accept attributes, as defined by a VertexDescriptor.
class VertexShader : public BaseShader<GL_VERTEX_SHADER>
{
    using Base = BaseShader<GL_VERTEX_SHADER>;

    friend class Program;
    const VertexDescriptor vertexDesc;

    VertexShader(const VertexShader&) = delete;

  public:
    VertexShader(const std::string& source, const VertexDescriptor& desc);
    VertexShader(VertexShader&& other);
};

// A shader that provides a screen buffer as output.
using FragmentShader = BaseShader<GL_FRAGMENT_SHADER>;

// A complete shader program.
class Program
{
  public:
    template <typename T>
    static UniformDesc MakeInput(const char* name) {
        return UniformDesc(name, MapTypeToTraits<T>::gl_enum,
                     MapTypeToTraits<T>::cols, MapTypeToTraits<T>::rows);
    }

    Program(VertexShader&& vs, FragmentShader&& fs,
            std::vector<UniformDesc> inputVec = {});
    Program(Program&& other);
    ~Program();

    void use() const;

    template <size_t N, typename Fst, typename ...Args>
    void bindUniforms(Fst&& fst, Args&&... args) const {
        if (inputs.size() - N != sizeof...(args) + 1)
            throw std::runtime_error("wrong number of inputs to shader");
        GLenum actual_enum = MapTypeToTraits<
            typename std::remove_cv<
                typename std::remove_reference<Fst>::type>::type>::gl_enum;
        if (inputs[N].glEnum() != actual_enum) {
            throw std::runtime_error(std::string("type mismatch at input ") +
                                     std::to_string(N));
        }
        GLint index = glGetUniformLocation(id, inputs[N].name());
        if (index == -1) {
            // The shader compiler might optimize out a perfectly reasonable
            // input. This gets particularly annoying when trying to debug a
            // shader. Instead of erroring out, we just ignore the missing
            // input and print a warning.
            std::cerr << "trying to bind to an unknown uniform " <<
                         inputs[N].name() << std::endl;
        } else {
            bindUniform(index, fst);
        }
        bindUniforms<N + 1>(args...);
    }
    template <size_t N>
    void bindUniforms() const {}
    void bindUniform(GLint index, float f) const { glUniform1f(index, f); }
    void bindUniform(GLint index, int i) const { glUniform1i(index, i); }
    void bindUniform(GLint index, const glm::vec3& v) const {
        glUniform3fv(index, 1, glm::value_ptr(v));
    }
    void bindUniform(GLint index, const glm::mat4& m) const {
        glUniformMatrix4fv(index, 1, GL_FALSE, glm::value_ptr(m));
    }

    class AutoEnableAttributes
    {
        AutoEnableAttributes(AutoEnableAttributes&&) = delete;
        AutoEnableAttributes(const AutoEnableAttributes&) = delete;

        const Program& program;

      public:
        AutoEnableAttributes(const Program& p, const VertexBuffer& vb);
        ~AutoEnableAttributes();
    };

  private:
    void enableVertexAttribs() const;
    void disableVertexAttribs() const;

    VertexShader vertexShader;
    FragmentShader fragmentShader;
    GLuint id;
    std::vector<UniformDesc> inputs;

    Program(const Program&) = delete;
};

} // namespace glit
