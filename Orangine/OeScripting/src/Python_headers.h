#pragma

#ifdef _MSVC_STL_VERSION
#error Python_headers.h Must be included before the STL
#endif 

// Python must be included before any STL files.
// Additionally - if not linking against a python debug build,
//   need to undef the _DEBUG flag before including python headers.
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
