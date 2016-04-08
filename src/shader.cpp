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
#include "shader.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "utility.h"


template <GLenum Type>
glit::BaseShader<Type>::BaseShader(const char* source)
  : id(glCreateShader(Type))
{
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    GLint compiled = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint log_len = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_len);
        if (!log_len) {
            glDeleteShader(id);
            throw std::runtime_error("shader compilation failed with no output");
        }
        std::unique_ptr<GLchar> info(new GLchar[log_len]);
        glGetShaderInfoLog(id, log_len, nullptr, info.get());
        glDeleteShader(id);
        throw std::runtime_error(std::string("shader compilation failed:\n") +
                                 std::string(info.release()));
    }
}

template <GLenum Type>
glit::BaseShader<Type>::BaseShader(BaseShader&& other)
  : id(other.id)
{
    other.id = 0;
}

template <GLenum Type>
glit::BaseShader<Type>::~BaseShader()
{
    if (id)
        glDeleteShader(id);
    id = 0;
}

template class glit::BaseShader<GL_FRAGMENT_SHADER>;
template class glit::BaseShader<GL_VERTEX_SHADER>;

glit::VertexShader::VertexShader(const char* source, const VertexDescriptor& desc)
  : Base(source),
    vertexDesc(desc)
{
}

glit::VertexShader::VertexShader(VertexShader&& other)
  : Base(std::forward<BaseShader>(other)),
    vertexDesc(other.vertexDesc)
{
}

glit::Program::Program(VertexShader&& vs, FragmentShader&& fs,
                       std::vector<UniformDesc> inputVec /* = {} */)
  : vertexShader(std::forward<VertexShader>(vs)),
    fragmentShader(std::forward<FragmentShader>(fs)),
    id(glCreateProgram()),
    inputs(inputVec)
{
    if (!vertexShader.id)
        throw std::runtime_error("using moved or deleted vertex shader");
    if (!fragmentShader.id)
        throw std::runtime_error("using moved or deleted fragment shader");

    glAttachShader(id, vertexShader.id);
    glAttachShader(id, fragmentShader.id);
    glLinkProgram(id);
    GLint linked = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint log_len = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_len);
        if (!log_len) {
            glDeleteProgram(id);
            throw std::runtime_error("program link failure with no output");
        }
        std::unique_ptr<GLchar> info(new GLchar[log_len]);
        glDeleteProgram(id);
        throw std::runtime_error(std::string("shader program link failed:\n") +
                                 std::string(info.release()));
    }
}

glit::Program::Program(Program&& other)
  : vertexShader(std::forward<VertexShader>(other.vertexShader)),
    fragmentShader(std::forward<FragmentShader>(other.fragmentShader)),
    id(other.id),
    inputs(other.inputs)
{
    other.id = 0;
}

glit::Program::~Program()
{
    if (id) {
        glDetachShader(id, fragmentShader.id);
        glDetachShader(id, vertexShader.id);
        glDeleteProgram(id);
    }
    id = 0;
}
