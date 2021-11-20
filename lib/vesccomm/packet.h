#pragma once

#include "buffer.h"
#include <functional>
#include <map>

namespace vesc {

// This ended up really ugly. Not how I usually like to parse packets from a stream.

// Warning, this will put 520ish bytes onto the stack. Make sure any thread using this function has plenty of stack
// space
// TODO could use stream operators to make it simple to inject these values
class packet {
public:
  enum class VALIDATE_RESULT { VALID, INCOMPLETE, BAD_START, INVALID_CRC, BAD_END };

  // Default constructor
  packet() = default;

  // Constructor that takes in a buffer, or anything that has begin and end and a len() function
  template <class T> packet(T buffer) { construct(buffer); }
  packet(uint8_t *mdata, size_t mlen) { _buffer.append(mdata, mlen); }
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
  buffer<PACKET_MAX_LEN> &data() { return _buffer; }

  const size_t len() { return _buffer.len(); }

  operator uint8_t *() { return _buffer; }

  // If validate returns true, it is ready to parse
  // If validate returns false, it is missing data or not valid
  // After validating the packet, we'll trim out everything except the payload
  VALIDATE_RESULT validate() {
    auto start = _buffer.get<uint8_t>();
    auto mlen = 0u;
    if (start == 2u) {
      mlen = _buffer.get<uint8_t>();
    } else if (start == 3u) {
      mlen = _buffer.get<uint16_t>();
    } else {
      return VALIDATE_RESULT::BAD_START;
    }

    constexpr auto crc_end_bytes = 3u;
    if (_buffer.len() >= mlen + crc_end_bytes) {
      auto calc_crc = crc16(_buffer + start, mlen);
      _buffer.advance(mlen);
      auto crc = _buffer.get<uint16_t>();
      if (crc == calc_crc) {
        if (_buffer.get<uint8_t>() == 3u) {
          _buffer.reload();
          _buffer.advance(start);
          _buffer.truncate(crc_end_bytes);
          return VALIDATE_RESULT::VALID;
        } else {
          return VALIDATE_RESULT::BAD_END;
        }
      } else {
        return VALIDATE_RESULT::INVALID_CRC;
      }
    }

    _buffer.reload();
    // Not good! Something didn't add up
    return VALIDATE_RESULT::INCOMPLETE;
  }

private:
  buffer<PACKET_MAX_LEN> _buffer;
};

}; // namespace vesc