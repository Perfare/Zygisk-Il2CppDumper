#ifndef WHALE_BASE_STRINGPRINTF_H_
#define WHALE_BASE_STRINGPRINTF_H_

#include <stdarg.h>
#include <string>

namespace whale {

// Returns a string corresponding to printf-like formatting of the arguments.
std::string StringPrintf(const char *fmt, ...)
__attribute__((__format__(__printf__, 1, 2)));

// Appends a printf-like formatting of the arguments to 'dst'.
void StringAppendF(std::string *dst, const char *fmt, ...)
__attribute__((__format__(__printf__, 2, 3)));

// Appends a printf-like formatting of the arguments to 'dst'.
void StringAppendV(std::string *dst, const char *format, va_list ap);

}  // namespace art

#endif // WHALE_BASE_STRINGPRINTF_H_
