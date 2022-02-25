#include <OeScripting/OeScripting_bindings.h>

// This file
PYBIND11_MODULE(oe_bindings, m)
{
  oe::OeScripting_bindings::create(m);
}
