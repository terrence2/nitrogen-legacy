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

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "utility.h"

using namespace std;


glit::UniformDesc::UniformDesc(const char* name, GLenum type,
                               uint8_t cols /* = 1 */, uint8_t rows /* = 1 */)
  : name_(name)
  , type_(type)
  , cols_(cols)
  , rows_(rows)
{}

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
            throw runtime_error("shader compilation failed with no output");
        }
        unique_ptr<GLchar> info(new GLchar[log_len]);
        glGetShaderInfoLog(id, log_len, nullptr, info.get());
        glDeleteShader(id);
        throw runtime_error(string("shader compilation failed:\n") +
                            string(info.release()));
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
  : Base(forward<BaseShader>(other)),
    vertexDesc(other.vertexDesc)
{
}

glit::Program::Program(VertexShader&& vs, FragmentShader&& fs,
                       vector<UniformDesc> inputVec /* = {} */)
  : vertexShader(forward<VertexShader>(vs)),
    fragmentShader(forward<FragmentShader>(fs)),
    id(glCreateProgram()),
    inputs(inputVec)
{
    if (!vertexShader.id)
        throw runtime_error("using moved or deleted vertex shader");
    if (!fragmentShader.id)
        throw runtime_error("using moved or deleted fragment shader");

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
            throw runtime_error("program link failure with no output");
        }
        unique_ptr<GLchar> info(new GLchar[log_len]);
        glDeleteProgram(id);
        throw runtime_error(string("shader program link failed:\n") +
                            string(info.release()));
    }
}

glit::Program::Program(Program&& other)
  : vertexShader(forward<VertexShader>(other.vertexShader)),
    fragmentShader(forward<FragmentShader>(other.fragmentShader)),
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

void
glit::Program::use() const
{
    if (!id)
        throw runtime_error("attempt to run a moved or deleted program");
    glUseProgram(id);
}

glit::Program::AutoEnableAttributes::AutoEnableAttributes(
        const Program& p, const VertexBuffer& vb)
  : program(p)
{
    if (vb.vertexDesc() != program.vertexShader.vertexDesc)
        throw runtime_error("mismatched vertex description");
    program.enableVertexAttribs();
}

glit::Program::AutoEnableAttributes::~AutoEnableAttributes()
{
    program.disableVertexAttribs();
}

void
glit::Program::enableVertexAttribs() const
{
    for (auto& attr : vertexShader.vertexDesc.attributes()) {
        GLint index = glGetAttribLocation(id, attr.name());
        if (index == -1) {
            throw runtime_error(string(
                        "failed to enable vertex attribute: " +
                        string(attr.name())));
        }
        attr.enable(index);
    }
}

void
glit::Program::disableVertexAttribs() const
{
    for (auto& attr : vertexShader.vertexDesc.attributes())
        attr.disable();
}
