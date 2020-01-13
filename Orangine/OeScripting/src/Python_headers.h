#pragma

#ifdef _MSVC_STL_VERSION
#error Python_headers.h Must be included before the STL
#endif 

// Python must be included before any STL files.
// Additionally - if not linking against a python debug build,
//   need to undef the _DEBUG flag before including python headers.
#ifdef _DEBUG
#define _OE_DEBUG
#endif

#ifdef OeScripting_PYTHON_DEBUG
#undef _DEBUG
#endif

#include <Python.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#ifdef _OE_DEBUG
#define _DEBUG
#endif