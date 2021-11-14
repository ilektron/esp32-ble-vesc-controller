#pragma once

// Simple helper class for serializing and deserializing data

#include <array>
#include <assert.h>
#include <cstdint>
#include <math.h>
#include <type_traits>

namespace vesc {

constexpr const auto PACKET_MAX_PL_LEN = 512u;
constexpr const auto PACKET_EXTRA_BYTES = 8u;
constexpr const auto PACKET_MAX_LEN = PACKET_MAX_PL_LEN + PACKET_EXTRA_BYTES;

template <std::size_t _size> class payload {
public:
  // Default initializor will just start at the beginning.
  payload() { _p = _data.begin(); }

  payload(std::initializer_list<uint8_t> d) {
    assert(_size == d.size());
    std::copy(d.begin(), d.end(), _data.begin());
  }

  ~payload() = default;

  size_t len() { return _size; }

  // Just throw a type in here and it will append it to the payload
  // This is a specialization for int types, and simply shifts the int into the
  // stream
  template <class T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type> int append(T val) {
    // Start by shifting MSB to LSB into payload
    for (size_t i = sizeof(T) - 1; i >= 0; i--, _p++) {
      *_p = (val >> (i * 8)) & 0xFF;
    }
    return 0;
  };

  // The float32 version of this function is pulled from append_float32_auto
  // The reasoning behind this is to create a portable method for serializing floating point values
  int append(float val) {
    int e = 0;
    float sig = frexpf(val, &e);
    float sig_abs = fabsf(sig);
    uint32_t sig_i = 0;

    if (sig_abs >= 0.5) {
      sig_i = (uint32_t)((sig_abs - 0.5f) * 2.0f * 8388608.0f);
      e += 126;
    }

    uint32_t res = ((e & 0xFF) << 23) | (sig_i & 0x7FFFFF);
    if (sig < 0) { res |= 1U << 31; }

    append<uint32_t>(res);
    return 0;
  }

  int append(const uint8_t *data, size_t len) {
    std::copy(data, data + len, _p);
    _p += len;
    return len;
  }

  // Shift out the value from the payload a byte at a time
  template <class T> T get() {
    T val{};
    for (size_t i = 0; i < sizeof(T); i++, _p++) {
      val |= (static_cast<T>(*_p) << (i * 8));
    }
    return val;
  };

  float get() {
    uint32_t res = get<uint32_t>();

    int e = (res >> 23) & 0xFF;
    uint32_t sig_i = res & 0x7FFFFF;
    bool neg = res & (1U << 31);

    float sig = 0.0;
    if (e != 0 || sig_i != 0) {
      sig = (float)sig_i / (8388608.0 * 2.0) + 0.5;
      e -= 126;
    }

    if (neg) { sig = -sig; }

    return ldexpf(sig, e);
  }

  std::array<uint8_t, _size> data() { return _data; }

private:
  // Keep the data and the index in an actual std::array for safe keeping
  std::array<uint8_t, _size> _data;
  typename std::array<uint8_t, _size>::iterator _p;
};

class packet {
public:
  packet() = default;
  ~packet() = default;

private:
  payload<PACKET_MAX_PL_LEN> _p;
};

}; // namespace vesc