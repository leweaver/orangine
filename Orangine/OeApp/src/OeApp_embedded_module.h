#pragma once

namespace oe::app {
class OeApp_embedded_module {
 public:
  static void initStatics();
  static void destroyStatics() {}
};
}