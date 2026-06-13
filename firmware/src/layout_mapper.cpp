#include "layout_mapper.h"

#include <unordered_map>

namespace {

constexpr uint16_t kUsageA = 0x04;
constexpr uint16_t kUsageZ = 0x1D;
constexpr uint16_t kUsage1 = 0x1E;
constexpr uint16_t kUsage0 = 0x27;
constexpr uint16_t kUsageEnter = 0x28;
constexpr uint16_t kUsageEsc = 0x29;
constexpr uint16_t kUsageBackspace = 0x2A;
constexpr uint16_t kUsageTab = 0x2B;
constexpr uint16_t kUsageSpace = 0x2C;
constexpr uint16_t kUsageMinus = 0x2D;
constexpr uint16_t kUsageEqual = 0x2E;
constexpr uint16_t kUsageLeftBracket = 0x2F;
constexpr uint16_t kUsageRightBracket = 0x30;
constexpr uint16_t kUsageBackslash = 0x31;
constexpr uint16_t kUsageSemicolon = 0x33;
constexpr uint16_t kUsageQuote = 0x34;
constexpr uint16_t kUsageComma = 0x36;
constexpr uint16_t kUsagePeriod = 0x37;
constexpr uint16_t kUsageSlash = 0x38;

const std::unordered_map<uint16_t, const char*> kUsUkMapping = {
    {kUsageEnter, "enter"},
    {kUsageEsc, "escape"},
    {kUsageBackspace, "backspace"},
    {kUsageTab, "tab"},
    {kUsageSpace, "space"},
    {kUsageMinus, "minus"},
    {kUsageEqual, "equals"},
    {kUsageLeftBracket, "left bracket"},
    {kUsageRightBracket, "right bracket"},
    {kUsageBackslash, "backslash"},
    {kUsageSemicolon, "semicolon"},
    {kUsageQuote, "quote"},
    {kUsageComma, "comma"},
    {kUsagePeriod, "period"},
    {kUsageSlash, "slash"},
};

}  // namespace

namespace echolocation {

LayoutMapper::LayoutMapper(LayoutType layout) : layout_(layout) {}

void LayoutMapper::set_layout(LayoutType layout) {
  layout_ = layout;
}

LayoutResult LayoutMapper::map_key(const KeyEvent& event) const {
  if (layout_ == LayoutType::kUS || layout_ == LayoutType::kUK) {
    return map_us_uk(event);
  }
  return map_fallback(event);
}

LayoutResult LayoutMapper::map_us_uk(const KeyEvent& event) const {
  if (event.usage >= kUsageA && event.usage <= kUsageZ) {
    const char letter = static_cast<char>('a' + (event.usage - kUsageA));
    return {std::string(1, letter), false};
  }

  if (event.usage >= kUsage1 && event.usage <= kUsage0) {
    const char digit = static_cast<char>('1' + (event.usage - kUsage1));
    const char mapped = (digit > '9') ? '0' : digit;
    return {std::string(1, mapped), false};
  }

  auto it = kUsUkMapping.find(event.usage);
  if (it != kUsUkMapping.end()) {
    return {it->second, false};
  }

  return map_fallback(event);
}

LayoutResult LayoutMapper::map_fallback(const KeyEvent& event) const {
  return {"key usage " + std::to_string(event.usage), true};
}

}  // namespace echolocation
