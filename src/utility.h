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
#include <chrono>
#include <functional>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>

inline std::ostream&
operator<< (std::ostream &stream, const glm::vec3 &v)
{
    return stream << "{" << v.x << "," << v.y << "," << v.z << "}";
}

// Call the macro D on each string that can be passed to glGetString.
#define FOR_EACH_GL_STRINGS(D) \
    D(GL_VERSION) \
    D(GL_VENDOR) \
    D(GL_RENDERER) \
    D(GL_SHADING_LANGUAGE_VERSION) \
    D(GL_EXTENSIONS)

namespace glit {
// Turn a C++ type into its matching GLenum.
template <typename T> struct MapTypeToTraits{};
template <const char* Name> struct MapNameToTraits{};
#define MAKE_MAP(D) \
    D(float, GL_FLOAT, 1, 1) \
    D(uint8_t, GL_UNSIGNED_BYTE, 1, 1) \
    D(glm::vec2, GL_FLOAT, 2, 1) \
    D(glm::vec3, GL_FLOAT, 3, 1) \
    D(glm::mat4, GL_FLOAT, 4, 4)
#define EXPAND_MAP_ITEM(ty, en, rows_, cols_) \
    template <> struct MapTypeToTraits<ty> { \
        using type = ty; \
        static const GLenum gl_enum = (en); \
        static const uint8_t rows = (rows_); \
        static const uint8_t cols = (cols_); \
        static const uint8_t extent = (rows_) * (cols_); \
    };
MAKE_MAP(EXPAND_MAP_ITEM)
#undef EXPAND_MAP_ITEM
#undef MAKE_MAP

namespace util {

template <typename T>
inline const GLvoid*
BufferOffset(size_t offset)
{
    return reinterpret_cast<const GLvoid*>(offset * sizeof(T));
}

// Time the live range of timer and print out a tagged runtime
// on destruction.
class Timer
{
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::duration<double>;
    Clock::time_point tStart;
    std::string ident;

  public:
    Timer(std::string id) : tStart(Clock::now()), ident(id) {}
    ~Timer() {
        auto tEnd = Clock::now();
        Duration span = std::chrono::duration_cast<Duration>(tEnd - tStart);
        std::cout << ident << ": " << span.count() << "sec" << std::endl;
    }
};

// Runtime representation of a numerator / denominator pair.
class Ratio
{
    size_t numerator;
    size_t denominator;

  public:
    Ratio(size_t n, size_t d)
      : numerator(n), denominator(d)
    {}

    size_t operator*(size_t s) const {
        return s * numerator / denominator;
    }
};

// Statically derive the number of elements in a fixed-length array.
template<typename T, size_t N>
constexpr size_t
ArrayLength(T (&aArr)[N])
{
    return N;
}

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

inline std::string
join(std::vector<std::string> parts, std::string glue)
{
    size_t count = 0;
    for (auto& part : parts)
        count += part.size();
    count += (parts.size() - 1) * glue.size();

    std::string out;
    out.reserve(count);

    size_t i = 0;
    for (auto& part : parts) {
        if (i != 0)
            out += glue;
        out += part;
        ++i;
    }

    return out;
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
