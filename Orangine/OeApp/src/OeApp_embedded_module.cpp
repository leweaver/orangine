#include <pybind11/embed.h>

#include "OeApp_embedded_module.h"

#include <OeApp/OeApp_bindings.h>

namespace py = pybind11;

using namespace oe;

PYBIND11_EMBEDDED_MODULE(oe_app_bindings, m)
{
  OeApp_bindings::create(m);
}

void oe::app::OeApp_embedded_module::initStatics() {
  // This is just here to ensure that the embedded module is included in compilation.
  // Otherwise this whole compilation unit is skipped.
  OeApp_bindings::setModuleName("oe_app_bindings");
}
