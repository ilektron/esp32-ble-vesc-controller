#pragma once

// Simple helper class for serializing and deserializing data
// TODO: instead of _p being the array iterator, perhaps we should have our own iterator type

#include "crc.h"
#include <array>
#include <assert.h>
#include <cstdint>
#include <math.h>
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

  float get() {
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

  uint16_t crc() { return crc16(_head, _len); }

  std::array<uint8_t, _size> data() { return _data; }

  void reset() {
    _len = 0u;
    _head = _data.begin();
  }

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

// Warning, this will put 520ish bytes onto the stack. Make sure any thread using this function has plenty of stack
// space
// TODO could use stream operators to make it simple to inject these values
class packet {
public:
  // Default constructor
  packet() = default;

  // Constructor that takes in a buffer, or anything that has begin and end and a len() function
  template <class T> packet(T buffer) { construct(buffer); }
  ~packet() = default;

  // Take in a buffer and insert it. Assumes that input has begin() and end(), should work with most stl containers
  // using a uint8_t type element
  template <class T> void construct(T buffer) {
    // Not SFINAE?

    // Set up the first byte to tell if this buffer is greater than 256
    auto mlen = std::distance(buffer.begin(), buffer.end());
    if (mlen > 256) {
      _buffer.append<uint8_t>(0x03);
      _buffer.append<uint16_t>(mlen);
    } else {
      _buffer.append<uint8_t>(0x02);
      _buffer.append<uint8_t>(mlen);
    }

    // Copy the buffer
    _buffer.copy(buffer);
    // Insert the CRC
    _buffer.append<uint16_t>(buffer.crc());
    // Add the final byte
    _buffer.append<uint8_t>(0x03);
  }

  // Used for reading out the data
  buffer<PACKET_MAX_LEN> data() { return _buffer; }

  const size_t len() { return _buffer.len(); }

  operator uint8_t *() { return _buffer; }

private:
  buffer<PACKET_MAX_LEN> _buffer;
};

}; // namespace vesc