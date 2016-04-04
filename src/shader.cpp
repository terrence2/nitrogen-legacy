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
glit::BaseShader<Type>::~BaseShader()
{
    glDeleteShader(id);
}

template class glit::BaseShader<GL_FRAGMENT_SHADER>;
template class glit::BaseShader<GL_VERTEX_SHADER>;

glit::Program::Program(const VertexShader& vs, const FragmentShader& fs,
                       std::vector<Input> inputVec /* = {} */)
  : vertexShader(vs), fragmentShader(fs), id(glCreateProgram()),
    inputs(inputVec)
{
    glAttachShader(id, vs.id);
    glAttachShader(id, fs.id);
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

glit::Program::~Program()
{
    glDetachShader(id, fragmentShader.id);
    glDetachShader(id, vertexShader.id);
    glDeleteProgram(id);
}
