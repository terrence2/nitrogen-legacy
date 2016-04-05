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
#include "icosphere.h"

#include <math.h>
#include <stdexcept>

glit::IcoSphere::IcoSphere(int iterations)
{
    float t = (1.f + sqrtf(5.f)) / 2.f;

    verts.push_back(Vertex{{-1,  t,  0}});
    verts.push_back(Vertex{{ 1,  t,  0}});
    verts.push_back(Vertex{{-1, -t,  0}});
    verts.push_back(Vertex{{ 1, -t,  0}});

    verts.push_back(Vertex{{ 0, -1,  t}});
    verts.push_back(Vertex{{ 0,  1,  t}});
    verts.push_back(Vertex{{ 0, -1, -t}});
    verts.push_back(Vertex{{ 0,  1, -t}});

    verts.push_back(Vertex{{ t,  0, -1}});
    verts.push_back(Vertex{{ t,  0,  1}});
    verts.push_back(Vertex{{-t,  0, -1}});
    verts.push_back(Vertex{{-t,  0,  1}});
}
