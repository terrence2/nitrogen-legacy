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

#ifndef __EMSCRIPTEN__
# include <glad/glad.h>
#else
// Glad probes for each function at runtime. Under emscripten, this generates
// an exception for each method that is not present in webgl, resulting in a
// massive hang at startup. On that platform we instead use glew + glfw to
// import the right GL bits and play games to make the names line up with glad.
# include <GL/glew.h>
# define glad_glGenFramebuffers glGenFramebuffers
# define glad_glBindFramebuffer glBindFramebuffer
# define glad_glFramebufferTexture2D glFramebufferTexture2D
# define glad_glCheckFramebufferStatus glCheckFramebufferStatus
#endif
#include <GLFW/glfw3.h>
