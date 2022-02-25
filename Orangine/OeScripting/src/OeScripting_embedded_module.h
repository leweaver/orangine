#pragma once

namespace oe {
class OeScripting_embedded_module {
 public:
  static void initStatics();
  static void destroyStatics() {}
};
}