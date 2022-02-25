#include <pybind11/embed.h>

#include "OeScripting_embedded_module.h"

#include <OeScripting/OeScripting_bindings.h>

namespace py = pybind11;

using namespace oe;

PYBIND11_EMBEDDED_MODULE(oe_bindings, m)
{
  OeScripting_bindings::create(m);
}

void oe::OeScripting_embedded_module::initStatics() {
  // This is just here to ensure that the embedded module is included in compilation.
  // Otherwise this whole compilation unit is skipped.
  OeScripting_bindings::setModuleName("oe_bindings");
}
