#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

class Preferences {
public:
  inline static std::map<std::string, std::vector<uint8_t>> values;
  inline static bool available = true;
  inline static bool failWrites = false;
  bool begin(const char *, bool = false) { return available; }
  bool clear() { if (failWrites) return false; values.clear(); return true; }
  bool remove(const char *key) { return values.erase(key) > 0; }
  size_t getBytesLength(const char *key) { return values[key].size(); }
  size_t getBytes(const char *key, void *out, size_t size) {
    const auto &bytes = values[key];
    const size_t count = std::min(size, bytes.size());
    if (count) std::memcpy(out, bytes.data(), count);
    return count;
  }
  size_t putBytes(const char *key, const void *data, size_t size) {
    if (failWrites) return 0;
    const auto *bytes = static_cast<const uint8_t *>(data);
    values[key] = std::vector<uint8_t>(bytes, bytes + size);
    return size;
  }
  size_t getString(const char *, char *out, size_t size) { if (size) out[0] = '\0'; return 0; }
  size_t putString(const char *, const char *) { return 1; }
  uint8_t getUChar(const char *, uint8_t fallback = 0) { return fallback; }
  uint16_t getUShort(const char *, uint16_t fallback = 0) { return fallback; }
  uint32_t getUInt(const char *, uint32_t fallback = 0) { return fallback; }
  int32_t getInt(const char *, int32_t fallback = 0) { return fallback; }
  float getFloat(const char *, float fallback = 0) { return fallback; }
  size_t putUChar(const char *, uint8_t) { return 1; }
  size_t putUShort(const char *, uint16_t) { return 2; }
  size_t putUInt(const char *, uint32_t) { return 4; }
  size_t putInt(const char *, int32_t) { return 4; }
  size_t putFloat(const char *, float) { return 4; }
};
