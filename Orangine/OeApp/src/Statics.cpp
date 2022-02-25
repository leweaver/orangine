#include <OeApp/Statics.h>
#include <OeApp/OeApp_bindings.h>
#include "OeApp_embedded_module.h"

void oe::app::initStatics() {
  OeApp_embedded_module::initStatics();
}

void oe::app::destroyStatics() {
  OeApp_embedded_module::destroyStatics();
}
