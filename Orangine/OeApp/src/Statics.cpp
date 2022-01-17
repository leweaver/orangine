#include "pch.h"

#include <OeApp/Statics.h>
#include <OeApp/OeApp_bindings.h>

void oe::app::initStatics() {
  OeApp_bindings::initStatics();
}

void oe::app::destroyStatics() {
  OeApp_bindings::destroyStatics();
}
