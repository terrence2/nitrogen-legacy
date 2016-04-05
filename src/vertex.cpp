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
#include "vertex.h"

void
glit::VertexAttrib::enable(GLint index) const
{
    if (index_ != -1)
        throw std::runtime_error("double-enable of attribute");

    index_ = index;
    glVertexAttribPointer(index_, size_, type_, normalized_, stride_, (void*)offset_);
    glEnableVertexAttribArray(index);
}

void
glit::VertexAttrib::disable() const
{
    if (index_ == -1)
        throw std::runtime_error("double disable of attribute");

    glDisableVertexAttribArray(GLuint(index_));
    index_ = -1;
}
