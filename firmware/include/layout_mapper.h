#pragma once

#include <string>

#include "echolocation_core/key_event.h"
#include "echolocation_core/settings_model.h"

namespace echolocation {

struct LayoutResult {
  std::string spoken_token;
  bool used_fallback = false;
};

class LayoutMapper {
 public:
  explicit LayoutMapper(LayoutType layout = LayoutType::kUnknown);

  void set_layout(LayoutType layout);
  LayoutResult map_key(const KeyEvent& event) const;

 private:
  LayoutType layout_;
  LayoutResult map_us_uk(const KeyEvent& event) const;
  LayoutResult map_fallback(const KeyEvent& event) const;
};

}  // namespace echolocation
