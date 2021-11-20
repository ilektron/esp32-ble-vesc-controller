#pragma once

// Simple helper class for serializing and deserializing data
// TODO: instead of _p being the array iterator, perhaps we should have our own iterator type

#include "crc.h"
#include <algorithm>
#include <array>
#include <assert.h>
#include <cstdint>
#include <math.h>
#include <stdio.h>
#include <string>
#include <type_traits>

namespace vesc {

constexpr const auto PACKET_MAX_PL_LEN = 512u;
constexpr const auto PACKET_EXTRA_BYTES = 8u;
constexpr const auto PACKET_MAX_LEN = PACKET_MAX_PL_LEN + PACKET_EXTRA_BYTES;

template <std::size_t _size> class buffer {
public:
  // Default initializor will just start at the beginning.
  buffer() : _len{} { _head = _data.begin(); }

  buffer(std::initializer_list<uint8_t> d) : _len{} {
    assert(_size >= d.size());
    _head = _data.begin();
    copy(d);
  }

  ~buffer() = default;

  // TODO: This should return the filled capacity not the total capacity, but might work for now
  const size_t len() { return _len; }

  // Just throw a type in here and it will append it to the buffer
  // This is a specialization for int types, and simply shifts the int into the
  // stream
  template <class T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type> int append(T val) {
    // Start by shifting MSB to LSB into buffer
    for (int i = sizeof(T) - 1; i >= 0; i--, _len++) {
      *(end()) = (val >> (i * 8)) & 0xFF;
    }
    return 0;
  };

  // Type T must have a begin() and end() function that match with std::copy
  template <class T> void copy(T t) {
    std::copy(t.begin(), t.end(), end());
    _len += std::distance(t.begin(), t.end());
  }

  // The float32 version of this function is pulled from append_float32_auto
  // The reasoning behind this is to create a portable method for serializing floating point values
  int append(float val) {
    int e = 0;
    float sig = frexpf(val, &e);
    float sig_abs = fabsf(sig);
    uint32_t sig_i = 0;

    if (sig_abs >= 0.5f) {
      sig_i = static_cast<uint32_t>((sig_abs - 0.5f) * 2.0f * 8388608.0f);
      e += 126;
    }

    uint32_t res = ((e & 0xFF) << 23) | (sig_i & 0x7FFFFF);
    if (sig < 0) { res |= 1U << 31; }

    append<uint32_t>(res);
    return 0;
  }

  int append(const uint8_t *data, size_t mlen) {
    std::copy(data, data + mlen, end());
    _len += mlen;
    return mlen;
  }

  // Shift out the value from the buffer a byte at a time
  template <class T> T get() {
    T val{};

    for (int i = sizeof(T) - 1; i >= 0; i--, _head++) {
      val |= (static_cast<T>(*_head) << (i * 8));
      _len--;
    }
    return val;
  };

  // Returns a 32bit floating point
  float getf() {
    uint32_t res = get<uint32_t>();

    int e = (res >> 23) & 0xFF;
    uint32_t sig_i = res & 0x7FFFFF;
    bool neg = res & (1U << 31);

    float sig = 0.0f;
    if (e != 0 || sig_i != 0) {
      sig = static_cast<float>(sig_i) / (8388608.0f * 2.0f) + 0.5f;
      e -= 126;
    }

    if (neg) { sig = -sig; }

    return ldexpf(sig, e);
  }

  // Get a fixed point value
  float getfp(size_t bytes, float scale) {
    int32_t val{};

    for (int i = bytes - 1; i >= 0; i--, _head++) {
      val |= (static_cast<uint32_t>(*_head) << (i * 8ul));
      _len--;
    }
    return static_cast<float>(val) / scale;
  }

  std::string get_string() {
    // Let's see if there is a null termination
    auto res = std::find(_head, _data.end(), '\0');
    std::array<char, 20> num;
    sprintf(num.data(), "%i", std::distance(_data.begin(), _head));
    std::string ret = "(Unknown)" + std::string(num.data());
    if (res != _data.end()) {
      ret = std::string(_head, res);
      // Advance past the null character
      res++;
      _len -= std::distance(_head, res);
      _head = res;
    }
    return ret;
  }

  uint16_t crc() { return crc16(_head, _len); }

  std::array<uint8_t, _size> data() { return _data; }

  void reset() {
    _len = 0u;
    _head = _data.begin();
  }

  // This doesn't work as intended
  void reload() {
    if (_head == _data.end()) {
      _len = _data.size();
    } else {
      _len += std::distance(_data.begin(), _head);
    }
    _head = _data.begin();
  }

  // Skip forward without returning data
  void advance(size_t count) {
    _head += count;
    _len -= count;
  }

  // Remove some data off the end of the buffer
  void truncate(size_t count) { _len = _len > count ? _len - count : 0u; }

  // Move the contents to the left by count
  void shift(size_t count) {}

  typename std::array<uint8_t, _size>::iterator begin() { return _head; }
  typename std::array<uint8_t, _size>::iterator end() { return _head + _len; }

  operator uint8_t *() { return _data.data(); }

private:
  // Keep the data and the index in an actual std::array for safe keeping
  std::array<uint8_t, _size> _data;
  typename std::array<uint8_t, _size>::iterator _head;
  // We do it this way to avoid having a reference to the arrd::end() that doesn't work with arithmetic
  size_t _len;
};

}; // namespace vesc