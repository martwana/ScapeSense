#pragma once

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>

inline void urlDecode(char *dst, size_t dstSize, const char *src,
                      size_t srcLen) {
  size_t out = 0;
  for (size_t i = 0; i < srcLen && out + 1 < dstSize; ++i) {
    if (src[i] == '+') {
      dst[out++] = ' ';
    } else if (src[i] == '%' && i + 2 < srcLen &&
               isxdigit(static_cast<unsigned char>(src[i + 1])) &&
               isxdigit(static_cast<unsigned char>(src[i + 2]))) {
      char hex[3] = {src[i + 1], src[i + 2], '\0'};
      dst[out++] = static_cast<char>(strtol(hex, nullptr, 16));
      i += 2;
    } else {
      dst[out++] = src[i];
    }
  }
  dst[out] = '\0';
}

inline void urlDecode(char *dst, size_t dstSize, const char *src) {
  urlDecode(dst, dstSize, src, strlen(src));
}

inline bool formField(char *dst, size_t dstSize, const char *body,
                      const char *name) {
  const size_t nameLen = strlen(name);
  const char *cursor = body;
  while (*cursor) {
    const char *end = strchr(cursor, '&');
    const size_t pairLen =
        end ? static_cast<size_t>(end - cursor) : strlen(cursor);
    if (pairLen > nameLen && cursor[nameLen] == '=' &&
        strncmp(cursor, name, nameLen) == 0) {
      urlDecode(dst, dstSize, cursor + nameLen + 1, pairLen - nameLen - 1);
      return true;
    }
    if (!end) {
      break;
    }
    cursor = end + 1;
  }
  return false;
}
