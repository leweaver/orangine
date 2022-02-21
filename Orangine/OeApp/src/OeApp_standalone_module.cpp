#include <pybind11/pybind11.h>

#include <OeApp/OeApp_bindings.h>

namespace py = pybind11;

using namespace oe;

PYBIND11_MODULE(oe_app_bindings, m)
{
  OeApp_bindings::create(m);
}

