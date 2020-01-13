//
// pch.h
// Header for standard system include files.
//

#pragma once

// Python must be included before any STL files
#ifdef Py_DEBUG
// This is redefined defined in pyconfig.h
#undef Py_DEBUG
#endif
#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

#include "OeCore/WindowsDefines.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>

#include <cstdio>

#include <g3log/g3log.hpp>
