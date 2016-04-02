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

#include <GLES2/gl2.h>

namespace glit {

template <GLenum Type>
class Shader
{
    friend class Program;
    const GLuint id;

    Shader(const Shader&) = delete;

  public:
    Shader(const char* source);
    Shader(Shader&& other) : id(other.id) { const_cast<GLuint&>(other.id) = 0; }
    ~Shader();
};

using VertexShader = Shader<GL_VERTEX_SHADER>;
using FragmentShader = Shader<GL_FRAGMENT_SHADER>;

class Program
{
    const GLuint id;

    Program(const Program&) = delete;

  public:
    Program(const VertexShader& vs, const FragmentShader& fs);
};

} // namespace glit

/*
oglplus::Context gl;

oglplus::VertexShader vs;

oglplus::FragmentShader fs;

oglplus::Program prog;

oglplus::VertexArray triangle;

oglplus::Buffer verts;
*/



#if 0
		// Set the vertex shader source
		vs.Source(" \
			#version 120\n \
			attribute vec3 Position; \
			void main(void) \
			{ \
				gl_Position = vec4(Position, 1.0); \
			} \
		");
		// compile it
		vs.Compile();

		// set the fragment shader source
		fs.Source(" \
			#version 120\n \
			void main(void) \
			{ \
				gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \
			} \
		");
		// compile it
		fs.Compile();

		// attach the shaders to the program
		prog.AttachShader(vs);
		prog.AttachShader(fs);
		// link and use it
		prog.Link();
		prog.Use();

		// bind the VAO for the triangle
		triangle.Bind();

		GLfloat triangle_verts[9] = {
			0.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f
		};
		// bind the VBO for the triangle vertices
		verts.Bind(Buffer::Target::Array);
		// upload the data
		Buffer::Data(
			Buffer::Target::Array,
			9,
			triangle_verts
		);
		// setup the vertex attribs array for the vertices
		VertexArrayAttrib vert_attr(prog, "Position");
		vert_attr.Setup<GLfloat>(3);
		vert_attr.Enable();

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
    /*
    (c) 2014 +++ Filip Stoklas, aka FipS, http://www.4FipS.com +++
    THIS CODE IS FREE - LICENSED UNDER THE MIT LICENSE
    ARTICLE URL: http://forums.4fips.com/viewtopic.php?f=3&t=1201
    */

    #define GL_GLEXT_PROTOTYPES

    #include <cstdio>
    #include <cassert>
    #include <GL/glut.h>

    struct Context
    {
       int width, height;
       GLuint vert_id, frag_id;
       GLuint prog_id, geom_id;
       GLint u_time_loc;
       
       enum { Position_loc, Color_loc };

       Context():
       width(400), height(300),
       vert_id(0), frag_id(0),
       prog_id(0), geom_id(0),
       u_time_loc(-1)
       {}

    } g_context;

    void init()
    {
       printf("init()\n");

       glClearColor(.3f, .3f, .3f, 1.f);

       auto load_shader = [](GLenum type, const char *src) -> GLuint
       {
          const GLuint id = glCreateShader(type);
          assert(id);
          glShaderSource(id, 1, &src, nullptr);
          glCompileShader(id);
          GLint compiled = 0;
          glGetShaderiv(id, GL_COMPILE_STATUS, &compiled);
          assert(compiled);
          return id;
       };

       g_context.vert_id = load_shader(
        GL_VERTEX_SHADER,
        "attribute vec4 a_position;              \n"
        "attribute vec4 a_color;                 \n"
        "uniform float u_time;                   \n"
        "varying vec4 v_color;                   \n"
        "void main()                             \n"
        "{                                       \n"
        "    float sz = sin(u_time);             \n"
        "    float cz = cos(u_time);             \n"
        "    mat4 rot = mat4(                    \n"
        "     cz, -sz, 0,  0,                    \n"
        "     sz,  cz, 0,  0,                    \n"
        "     0,   0,  1,  0,                    \n"
        "     0,   0,  0,  1                     \n"
        "    );                                  \n"
        "    gl_Position = a_position * rot;     \n"
        "    v_color = a_color;                  \n"
        "}                                       \n"
       );
       printf("- vertex shader loaded\n");

       g_context.frag_id = load_shader(
        GL_FRAGMENT_SHADER,
        "precision mediump float;                \n"
        "varying vec4 v_color;                   \n"
        "void main()                             \n"
        "{                                       \n"
        "    gl_FragColor = v_color;             \n"
        "}                                       \n"
       );
       printf("- fragment shader loaded\n");

       g_context.prog_id = glCreateProgram();
       assert(g_context.prog_id);
       glAttachShader(g_context.prog_id, g_context.vert_id);
       glAttachShader(g_context.prog_id, g_context.frag_id);
       glBindAttribLocation(g_context.prog_id, Context::Position_loc, "a_position");
       glBindAttribLocation(g_context.prog_id, Context::Color_loc, "a_color");
       glLinkProgram(g_context.prog_id);
       GLint linked = 0;
       glGetProgramiv(g_context.prog_id, GL_LINK_STATUS, &linked);
       assert(linked);
       g_context.u_time_loc = glGetUniformLocation(g_context.prog_id, "u_time");
       assert(g_context.u_time_loc >= 0);
       glUseProgram(g_context.prog_id);
       printf("- shader program linked & bound\n");

       struct Vertex { float x, y, z; unsigned char r, g, b, a; };
       const Vertex vtcs[] {
        {  0.f,  .5f, 0.f,   255, 0, 0, 255 },
        { -.5f, -.5f, 0.f,   0, 255, 0, 255 },
        {  .5f, -.5f, 0.f,   0, 0, 255, 255 }
       };
       glGenBuffers(1, &g_context.geom_id);
       assert(g_context.geom_id);
       glBindBuffer(GL_ARRAY_BUFFER, g_context.geom_id);
       glBufferData(GL_ARRAY_BUFFER, sizeof(vtcs), vtcs, GL_STATIC_DRAW);
       auto offset = [](size_t value) -> const GLvoid * { return reinterpret_cast<const GLvoid *>(value); };
       glVertexAttribPointer(Context::Position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offset(0));
       glEnableVertexAttribArray(Context::Position_loc);
       glVertexAttribPointer(Context::Color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), offset(3 * sizeof(float)));
       glEnableVertexAttribArray(Context::Color_loc);
       printf("- geometry created & bound\n");
    }

    void resize(int width, int height)
    {
       printf("resize(%d, %d)\n", width, height);
       
       g_context.width = width;
       g_context.height = height;
    }

    void draw()
    {
       glViewport(0, 0, g_context.width, g_context.height);
       glClear(GL_COLOR_BUFFER_BIT);

       glUniform1f(g_context.u_time_loc, glutGet(GLUT_ELAPSED_TIME) / 1000.f);
       glDrawArrays(GL_TRIANGLES, 0, 3);
       
       glutSwapBuffers();
    }

    void update()
    {
       glutPostRedisplay();   
    }

    int main(int argc, char *argv[])
    {
       printf("main()\n");

       glutInit(&argc, argv);
       glutInitWindowSize(g_context.width, g_context.height);
       glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

       glutCreateWindow("Hello Triangle! | 4FipS.com");

       glutReshapeFunc(resize);
       glutDisplayFunc(draw);
       glutIdleFunc(update);

       init();

       glutMainLoop();

       return 0;
    }
#endif
