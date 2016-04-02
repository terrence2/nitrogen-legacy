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

#include <memory>
#include <stdexcept>
#include <string>

#include <GLES2/gl2.h>

#include "shader.h"

template <GLenum Type>
glit::Shader<Type>::Shader(const char* source)
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
        throw std::runtime_error(std::string("shader compilation failed:\n") +
                                 std::string(info.release()));
    }
}

template <GLenum Type>
glit::Shader<Type>::~Shader()
{
    glDeleteShader(id);
}

template class glit::Shader<GL_FRAGMENT_SHADER>;
template class glit::Shader<GL_VERTEX_SHADER>;

glit::Program::Program(const VertexShader& vs, const FragmentShader& fs)
  : id(glCreateProgram())
{
    glAttachShader(id, vs.id);
    glAttachShader(id, fs.id);

    /*
glBindAttribLocation(g_context.prog_id, Context::Position_loc, "a_position");
glBindAttribLocation(g_context.prog_id, Context::Color_loc, "a_color");
glLinkProgram(g_context.prog_id);
GLint linked = 0;
glGetProgramiv(g_context.prog_id, GL_LINK_STATUS, &linked);
assert(linked);
g_context.u_time_loc = glGetUniformLocation(g_context.prog_id, "u_time");
*/
}
