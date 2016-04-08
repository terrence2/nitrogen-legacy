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

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

// Call the macro D on each string that can be passed to glGetString.
#define FOR_EACH_GL_STRINGS(D) \
    D(GL_VERSION) \
    D(GL_VENDOR) \
    D(GL_RENDERER) \
    D(GL_SHADING_LANGUAGE_VERSION) \
    D(GL_EXTENSIONS)

namespace glit {
namespace util {

inline std::vector<std::string>
split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
        elems.push_back(item);
    return elems;
}

inline bool
matches_any(char needle, const char* haystack)
{
    const char* hay = haystack;
    while (*hay) {
        if (needle == *hay)
            return true;
        ++hay;
    }
    return false;
}

inline std::string&
ltrim(std::string& s, const char* chars = " \t\n\r\v")
{
    auto first = s.begin();
    auto last = s.end();
    while (first != last) {
        if (!matches_any(*first, chars))
            break;
        ++first;
    }
    s.erase(s.begin(), first);
    return s;
}

inline std::string&
rtrim(std::string& s, const char* chars = " \t\n\r\v")
{
    auto last = s.end();
    auto first = s.begin();
    while (last != first) {
        if (!matches_any(*(last - 1), chars))
            break;
        --last;
    }
    s.erase(last, s.end());
    return s;
}

// trim from both ends
inline std::string&
trim(std::string& s, const char* chars = " \t\n\r\v")
{
    return ltrim(rtrim(s, chars), chars);
}

inline bool
startswith(const std::string& haystack, const std::string& needle)
{
    return !haystack.compare(0, needle.size(), needle);
}

inline std::string&
replace(std::string& s, const char* chars, const char repl)
{
    auto first = s.begin();
    auto last = s.end();
    while (first != last) {
        if (matches_any(*first, chars))
            *first = repl;
        ++first;
    }
    return s;
}

constexpr int64_t
ipow(int64_t base, int exp, int64_t result = 1)
{
    return exp < 1
           ? result
           : ipow(base * base, exp / 2, (exp % 2)
                                        ? result * base
                                        : result);
}

} // namespace util
 } // namespace glit
