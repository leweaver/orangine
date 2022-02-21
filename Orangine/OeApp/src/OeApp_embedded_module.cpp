#include <pybind11/embed.h>

#include <OeApp/OeApp_bindings.h>

namespace py = pybind11;

using namespace oe;

PYBIND11_EMBEDDED_MODULE(oe_app_bindings, m)
{
  OeApp_bindings::create(m);
}

