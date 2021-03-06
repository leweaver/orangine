#pragma once

#include <OeCore/Math_constants.h>
#include <OeCore/WindowsDefines.h>

#include <string>
#include <string_view>
#include <vectormath.hpp>

#include <g3log/g3log.hpp>

namespace oe {
std::exception&& log_exception_for_throw(
    std::exception&& e,
    const char* filename,
    int line,
    const char* function);
}

template <typename... TArgs> std::string string_format(const std::string& format, TArgs... args);

// For some reason, adding line breaks with \ here fails to compile.
// clang-format off

// Fast fail; preferred way of checking logic that is not based on user/file based input.
#define OE_CHECK(condition) if(!(condition)) { \
  const auto msg2 = oe_check_helper(#condition); \
  if (g3::internal::isLoggingInitialized()) LOG(FATAL) << msg2; else abort(); \
}
#define OE_CHECK_MSG(condition, msg) if(!(condition)) { \
  const auto msg2 = oe_check_helper(#condition, msg);   \
  if (g3::internal::isLoggingInitialized()) LOG(FATAL) << msg2; else abort(); \
}
#define OE_CHECK_FMT(condition, msg, ...) if(!(condition)) { \
  std::string msg_str = string_format((msg), __VA_ARGS__);   \
  OE_CHECK_MSG(condition, msg_str);\
}

// Throws the given exception, after logging it. Use in place of OE_CHECK if the condition is caused by user input.
#define OE_THROW(ex) throw oe::log_exception_for_throw(ex, __FILE__, __LINE__, static_cast<const char*>(__PRETTY_FUNCTION__))
// clang-format on

namespace oe {
std::string oe_check_helper(const char* condition, std::string_view msg = "");
void oe_set_enable_check_debugbreak(bool enabled);

template <typename T, size_t TN> constexpr size_t array_size(const T (&)[TN]) { return TN; }

// From boost
template <class T> void hash_combine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

constexpr float degrees_to_radians(float degrees) { return degrees * (oe::math::pi / 180.0f); }
constexpr float radians_to_degrees(float radians) { return radians * (180.0f / oe::math::pi); }

// Create a rotation matrix from input unit vector A to B.
bool create_rotation_between_unit_vectors(
    SSE::Matrix3& result,
    const SSE::Vector3& directionFrom,
    const SSE::Vector3& directionTo);

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring& wstring);

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string& str);

bool str_starts(const std::string& str, const std::string& prefix);
bool str_ends(const std::string& str, const std::string& suffix);

bool str_starts(const std::wstring& str, const std::wstring& prefix);
bool str_ends(const std::wstring& str, const std::wstring& suffix);

std::vector<std::string> str_split(const std::string& str, const std::string& delimeters = " ");

std::string str_replace_all(std::string str, const std::string& from, const std::string& to);

std::string str_trim(const std::string& str);

template <typename... TArgs> std::string string_format(const std::string& format, TArgs... args) {
  // From: https://stackoverflow.com/a/26221725
  const size_t size =
      std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
  if (size <= 0) {
    OE_THROW(std::invalid_argument("Invalid format string: " + format));
  }
  const std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

std::string hr_to_string(HRESULT hr);
std::wstring hr_to_wstring(HRESULT hr);

constexpr bool is_power_of_two(const uint32_t i) { return (i != 0u) && ((i - 1u) & i) == 0u; }

std::wstring expand_environment_strings(const std::wstring& source);
std::string getlasterror_to_str();
std::string errno_to_str();

// Helper class for COM exceptions
class com_exception : public std::exception {
  std::string _what;

 public:
  explicit com_exception(HRESULT hr);

  com_exception(HRESULT hr, std::string_view what);

  const char* what() const override { return _what.c_str(); }

 private:
  HRESULT _result;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr) {
  if (FAILED(hr)) {
    throw com_exception(hr);
  }
}

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr, const std::string_view what) {
  if (FAILED(hr)) {
    throw com_exception(hr, what);
  }
}

inline void decompose_matrix(
    const SSE::Matrix4& mat,
    SSE::Vector3& pos,
    SSE::Quat& rotation,
    SSE::Vector3& scale) {
  pos = mat.getTranslation();
  auto rotMat = mat.getUpper3x3();
  scale = SSE::Vector3(
      SSE::length(rotMat.getCol0()), SSE::length(rotMat.getCol1()), SSE::length(rotMat.getCol2()));
  rotMat.setCol0(rotMat.getCol0() / scale.getX());
  rotMat.setCol1(rotMat.getCol1() / scale.getY());
  rotMat.setCol2(rotMat.getCol2() / scale.getZ());
  rotation = SSE::Quat(rotMat);
}

template <typename TEnum, size_t TCount>
TEnum stringToEnum(const std::string& value, const std::string (&strings)[TCount]) {
  for (auto i = 0; i < TCount; i++) {
    {
      if (strings[i] == value)
        return static_cast<TEnum>(i);
    }
  }
  OE_THROW(std::runtime_error("Unknown enum value: " + value));
}
} // namespace oe

namespace DX {
inline void ThrowIfFailed(HRESULT hr) {
  if (FAILED(hr)) {
    throw oe::com_exception(hr);
  }
}
} // namespace DX
