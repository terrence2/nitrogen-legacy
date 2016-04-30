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
#include "backtrace.h"

#ifndef __EMSCRIPTEN__

#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>

#include "utility.h"

using namespace std;
using namespace glit::util;

string
CaptureBacktrace(int skip = 0)
{
	void* callstack[128];
	char printBuffer[1024];
	ostringstream traceBuffer;
	int numFrames = backtrace(callstack, ArrayLength(callstack));
	char** symbols = backtrace_symbols(callstack, numFrames);

	for (int i = skip; i < numFrames; i++) {
		Dl_info info;
		if (dladdr(callstack[i], &info) && info.dli_sname) {
			char* demangled = nullptr;
			int status = -1;
			if (info.dli_sname[0] == '_')
				demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
			snprintf(printBuffer, sizeof(printBuffer),
                     "%-3d %*p %s + %zd\n",
					 i, int(2 + sizeof(void*) * 2), callstack[i],
					 status == 0
                         ? demangled
                         : info.dli_sname == 0
                            ? symbols[i]
                            : info.dli_sname,
					 intptr_t(callstack[i]) - intptr_t(info.dli_saddr));
			free(demangled);
		} else {
			snprintf(printBuffer, sizeof(printBuffer),
                     "%-3d %*p %s\n",
					 i, int(2 + sizeof(void*) * 2), callstack[i], symbols[i]);
		}
		traceBuffer << printBuffer;
	}
	free(symbols);
	if (numFrames == ArrayLength(callstack))
		traceBuffer << "[truncated]\n";
	return traceBuffer.str();
}

void
showBacktrace(int sig)
{
  cout << CaptureBacktrace() << endl;
  exit(1);
}
#endif // __EMSCRIPTEN__
