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

#include <string>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "vertex.h"

namespace glit {

template <GLenum Type>
class BaseShader
{
    friend class Program;
    const GLuint id;

    BaseShader(const BaseShader&) = delete;
    BaseShader(BaseShader&& other) = delete;

  public:
    BaseShader(const char* source);
    ~BaseShader();
};

class VertexShader : public BaseShader<GL_VERTEX_SHADER>
{
    using Base = BaseShader<GL_VERTEX_SHADER>;

    friend class Program;
    const VertexDescriptor& vertexDesc;

  public:
    VertexShader(const char* source, const VertexDescriptor& desc)
      : Base(source), vertexDesc(desc)
    {}
};

using FragmentShader = BaseShader<GL_FRAGMENT_SHADER>;

// A shader program.
class Program
{
  public:
    using Input = std::pair<const char*, GLenum>;

    Program(const VertexShader& vs, const FragmentShader& fs,
            std::vector<Input> inputVec = {});
    ~Program();

    template <typename ...Args>
    void run(const VertexBuffer& vb, Args&&... args) {
        if (vb.vertexDesc() != vertexShader.vertexDesc)
            throw std::runtime_error("mismatched vertex description");
        if (inputs.size() != sizeof...(args))
            throw std::runtime_error("wrong number of inputs to shader");

        glUseProgram(id);
        vb.bind();
        bindUniforms<0>(args...);
        enableVertexAttribs();

        glDrawArrays(GL_TRIANGLES, 0, 3);

        disableVertexAttribs();
    }

    template <size_t N, typename Fst, typename ...Args>
    void bindUniforms(Fst&& fst, Args&&... args) {
        GLenum actual = MapTypeToGLEnum<
            typename std::remove_reference<Fst>::type>::value;
        if (inputs[N].second != actual) {
            throw std::runtime_error(std::string("type mismatch at input ") +
                                     std::to_string(N));
        }
        GLint index = glGetUniformLocation(id, inputs[N].first);
        if (index == -1) {
            throw std::runtime_error(std::string("unknown uniform ") +
                                     inputs[N].first);
        }
        bindUniform(index, fst);
        bindUniforms<N + 1>(args...);
    }
    template <size_t N>
    void bindUniforms() {}
    void bindUniform(GLint index, float f) { glUniform1f(index, f); }
    void bindUniform(GLint index, int i) { glUniform1i(index, i); }

    void enableVertexAttribs() {
        for (auto& attr : vertexShader.vertexDesc.attributes()) {
            GLint index = glGetAttribLocation(id, attr.name());
            attr.enable(index);
        }
    }

    void disableVertexAttribs() {
        for (auto& attr : vertexShader.vertexDesc.attributes())
            attr.disable();
    }

  private:
    const VertexShader& vertexShader;
    const FragmentShader& fragmentShader;
    const GLuint id;
    const std::vector<Input> inputs;

    Program(const Program&) = delete;
};

} // namespace glit
