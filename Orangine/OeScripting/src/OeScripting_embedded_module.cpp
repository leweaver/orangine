#include <pybind11/embed.h>

#include <OeScripting/OeScripting_bindings.h>

namespace py = pybind11;

using namespace oe;

PYBIND11_EMBEDDED_MODULE(oe_bindings, m)
{
  OeScripting_bindings::create(m);
}

