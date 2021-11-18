#include "packet.h"
#include <sstream>
#include <string>
#include <unity.h>
#include <vector>

// Helper function

std::string toString(vesc::packet p) {
  std::stringstream ss;

  for (const auto v : p.data()) {
    ss << "0x" << std::hex << static_cast<unsigned int>(v) << " ";
  }

  return ss.str();
}

void test_payload_init() {
  vesc::payload<1> p;
  TEST_ASSERT_EQUAL(p.len(), 0);
}

void test_payload_append_uint8_t() {
  vesc::payload<100> p;

  p.append<uint8_t>(0x11);
  TEST_ASSERT_EQUAL(p.len(), 1);
  auto i = p.get<uint8_t>();
  TEST_ASSERT_EQUAL(0x11, i);
}

void test_payload_append_int8_t() {
  vesc::payload<100> p;

  p.append<int8_t>(-5);
  TEST_ASSERT_EQUAL(p.len(), 1);
  auto i = p.get<int8_t>();
  TEST_ASSERT_EQUAL(-5, i);
}

void test_payload_append_uint16_t() {
  vesc::payload<100> p;

  p.append<uint16_t>(0xF011);
  TEST_ASSERT_EQUAL(p.len(), 2);
  auto i = p.get<uint16_t>();
  TEST_ASSERT_EQUAL(0xF011, i);
}

void test_payload_append_int16_t() {
  vesc::payload<100> p;

  p.append<int16_t>(-1022);
  TEST_ASSERT_EQUAL(p.len(), 2);
  auto i = p.get<int16_t>();
  TEST_ASSERT_EQUAL(-1022, i);
}

void test_payload_append_uint32_t() {
  vesc::payload<100> p;

  p.append<uint32_t>(0xF01142AA);
  TEST_ASSERT_EQUAL(p.len(), 4);
  auto i = p.get<uint32_t>();
  TEST_ASSERT_EQUAL(0xF01142AA, i);
}

void test_payload_append_int32_t() {
  vesc::payload<100> p;

  p.append<int32_t>(-10223932);
  TEST_ASSERT_EQUAL(p.len(), 4);
  auto i = p.get<int32_t>();
  TEST_ASSERT_EQUAL(-10223932, i);
}

void test_payload_append_float() {

  TEST_ASSERT_EQUAL(4, sizeof(float));
  vesc::payload<100> p;

  auto f = -3.2435234;
  p.append(f);
  TEST_ASSERT_EQUAL(4, p.len());
  auto i = p.get();
  TEST_ASSERT_EQUAL_FLOAT(f, i);
}

void test_payload_append_multiple() {
  vesc::payload<100> p;

  std::vector<int32_t> values = {100, -20, 23290809, -20200392, 0, -1, 1, 0x7FFFFFFF, 0x700000};

  for (auto v : values) {
    p.append<int32_t>(v);
  }

  for (auto v : values) {
    TEST_ASSERT_EQUAL(v, p.get<int32_t>());
  }
}

void test_payload_append_multiple_types() {
  vesc::payload<100> p;

  p.append<int32_t>(23984);
  p.append<uint32_t>(1);
  p.append<int32_t>(0);
  p.append<int16_t>(-23984);
  p.append(3.1415926f);
  p.append<int8_t>('\0');
  p.append<int8_t>('a');
  p.append<uint8_t>('!');
  p.append(23984.0f);
  p.append<int32_t>(0x112233);
  TEST_ASSERT_EQUAL(23984, p.get<int32_t>());
  TEST_ASSERT_EQUAL(1, p.get<uint32_t>());
  TEST_ASSERT_EQUAL(0, p.get<int32_t>());
  TEST_ASSERT_EQUAL(-23984, p.get<int16_t>());
  TEST_ASSERT_EQUAL_FLOAT(3.1415926f, p.get());
  TEST_ASSERT_EQUAL('\0', p.get<int8_t>());
  TEST_ASSERT_EQUAL('a', p.get<int8_t>());
  TEST_ASSERT_EQUAL('!', p.get<uint8_t>());
  TEST_ASSERT_EQUAL_FLOAT(23984.0f, p.get());
  TEST_ASSERT_EQUAL(0x112233, p.get<int32_t>());
}

void test_payload_append_buffer() {
  vesc::payload<100> p;

  std::string str = "Hello!";

  p.copy(str);

  TEST_ASSERT_EQUAL(6, p.len());
  TEST_ASSERT_EQUAL('H', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('e', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('l', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('l', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('o', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('!', p.get<uint8_t>());
}

void test_payload_copy_payload() {
  vesc::payload<6> p1 = {'H', 'e', 'l', 'l', 'o', '!'};
  vesc::payload<100> p2;

  TEST_ASSERT_EQUAL_MESSAGE(6, p1.len(), "Initializer list failed");

  p2.copy(p1);

  TEST_ASSERT_EQUAL_MESSAGE(6, p2.len(), "Copy didn't increase length of p2");
  TEST_ASSERT_EQUAL(p1.len(), p2.len());
  TEST_ASSERT_EQUAL('H', p2.get<uint8_t>());
  TEST_ASSERT_EQUAL('e', p2.get<uint8_t>());
  TEST_ASSERT_EQUAL('l', p2.get<uint8_t>());
  TEST_ASSERT_EQUAL('l', p2.get<uint8_t>());
  TEST_ASSERT_EQUAL('o', p2.get<uint8_t>());
  TEST_ASSERT_EQUAL('!', p2.get<uint8_t>());
}

void test_payload_initializer_list() {
  vesc::payload<6> p = {'H', 'e', 'l', 'l', 'o', '!'};

  TEST_ASSERT_EQUAL(6, p.len());
  TEST_ASSERT_EQUAL('H', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('e', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('l', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('l', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('o', p.get<uint8_t>());
  TEST_ASSERT_EQUAL('!', p.get<uint8_t>());
}

void test_packet_from_payload() {
  vesc::payload<6> p1 = {'H', 'e', 'l', 'l', 'o', '!'};

  vesc::packet packet1(p1);

  // Should be start byte + len + payload + 16 bit crc + stop
  TEST_ASSERT_EQUAL_MESSAGE(11, packet1.len(), toString(p1).c_str());

  vesc::payload<1u> p2 = {0};

  vesc::packet packet2(p2);

  TEST_ASSERT_EQUAL(6, packet2.len());

  vesc::payload<vesc::PACKET_MAX_PL_LEN> p3;
  for (auto i = 0ul; i < vesc::PACKET_MAX_PL_LEN / 2; i++) {
    p3.append<uint16_t>(i);
  }

  vesc::packet packet3(p3);

  TEST_ASSERT_EQUAL(vesc::PACKET_MAX_PL_LEN + 6, packet3.len());
}

void test_byte_order() {
  auto i = 0x11223344;
  vesc::payload<4> p;
  p.append<uint32_t>(i);

  TEST_ASSERT_EQUAL(0x11, p.get<uint8_t>());
  TEST_ASSERT_EQUAL(0x22, p.get<uint8_t>());
  TEST_ASSERT_EQUAL(0x33, p.get<uint8_t>());
  TEST_ASSERT_EQUAL(0x44, p.get<uint8_t>());
}

void test_simple_packet() {
  vesc::payload<1> p1 = {0};
  std::array<uint8_t, 6> p2 = {0x02, 0x01, 0x00, 0x00, 0x00, 0x03};
  vesc::packet pack(p1);

  for (auto i = 0; i < p2.size(); i++) {
    TEST_ASSERT_EQUAL(p2[i], pack.data().data().at(i));
  }
}

int main(int argc, char *argv[]) {
  UNITY_BEGIN();

  RUN_TEST(test_payload_init);
  RUN_TEST(test_payload_append_uint8_t);
  RUN_TEST(test_payload_append_int8_t);
  RUN_TEST(test_payload_append_uint16_t);
  RUN_TEST(test_payload_append_int16_t);
  RUN_TEST(test_payload_append_uint32_t);
  RUN_TEST(test_payload_append_int32_t);
  RUN_TEST(test_payload_append_float);
  RUN_TEST(test_payload_append_multiple);
  RUN_TEST(test_payload_append_multiple_types);
  RUN_TEST(test_payload_append_buffer);
  RUN_TEST(test_payload_copy_payload);
  RUN_TEST(test_payload_initializer_list);
  RUN_TEST(test_byte_order);

  RUN_TEST(test_packet_from_payload);
  RUN_TEST(test_simple_packet);
  UNITY_END();
  return 0;
}
