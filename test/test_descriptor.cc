/** @file test_descriptor.cc
 * @brief Unit tests for the USB descriptor parsers in src/device.c.
 *
 * These parsers run on data supplied by the device before any of it has been
 * validated, so they are libuvc's main attack surface against a malicious or
 * merely broken camera. Each test feeds a hand-built descriptor and checks
 * that libuvc rejects it rather than reading out of bounds.
 *
 * Build with -DENABLE_SANITIZERS=ON to make the out-of-bounds cases fail
 * loudly; without a sanitizer several of them read stale heap memory and
 * "pass" while still being wrong.
 */
#include <gtest/gtest.h>

#include <cstdio>

#include "uvc_test_util.h"

namespace {

/** RAII wrapper around the synthetic config and the info block over it. */
class DescriptorTest : public ::testing::Test {
 protected:
  void SetUpConfig(int num_interfaces) {
    uvc_test_config_init(&tc_, num_interfaces);
    uvc_test_info_init(&info_, &tc_);
    initialised_ = true;
  }

  void TearDown() override {
    if (initialised_) {
      uvc_test_info_free(&info_);
      uvc_test_config_free(&tc_);
    }
  }

  uvc_test_config_t tc_{};
  uvc_device_info_t info_{};
  bool initialised_ = false;
};

/** Run @p body in a forked child and require it to exit cleanly.
 *
 * Several tests below drive the parser with descriptors that, on unfixed
 * code, read out of bounds. Under a sanitizer that aborts the process, which
 * would take every later test in the binary down with it. Running each such
 * case in its own child keeps one memory-safety failure from masking the
 * rest, and turns "the sanitizer fired" into an ordinary test failure.
 *
 * A hang is caught the same way: the child never exits, so the test times out
 * rather than wedging the run indefinitely.
 *
 * Anything the body wants to assert must be asserted inside the child, since
 * only its exit status crosses back. Use REQUIRE_IN_CHILD() for that: it
 * exits non-zero on failure, which surfaces as the death test failing.
 */
#define EXPECT_MEMORY_SAFE(body) \
  EXPECT_EXIT({ body; _exit(0); }, ::testing::ExitedWithCode(0), "")

/** Assert inside an EXPECT_MEMORY_SAFE body. A failure exits 1, which the
 * enclosing death test reports. */
#define REQUIRE_IN_CHILD(cond) \
  do { \
    if (!(cond)) { \
      fprintf(stderr, "REQUIRE_IN_CHILD failed: %s at %s:%d\n", \
              #cond, __FILE__, __LINE__); \
      _exit(1); \
    } \
  } while (0)

/** Parse a VC header against a config of @p num_interfaces, in a child.
 * Requires the parse to both stay in bounds and report failure. */
#define EXPECT_VC_HEADER_REJECTED(num_interfaces, block, block_len) \
  EXPECT_MEMORY_SAFE({ \
    uvc_test_config_t tc; \
    uvc_device_info_t info; \
    uvc_test_config_init(&tc, (num_interfaces)); \
    uvc_test_info_init(&info, &tc); \
    REQUIRE_IN_CHILD(uvc_parse_vc_header(nullptr, &info, (block), \
                                         (block_len)) != UVC_SUCCESS); \
    uvc_test_info_free(&info); \
    uvc_test_config_free(&tc); \
  })

/* A minimal, well-formed VideoControl header, UVC 1.00, no streaming
 * interfaces. Layout per UVC 1.5 spec Table 3-3:
 *   [0]     bLength
 *   [1]     bDescriptorType    = CS_INTERFACE (0x24)
 *   [2]     bDescriptorSubtype = VC_HEADER (0x01)
 *   [3..4]  bcdUVC
 *   [5..6]  wTotalLength
 *   [7..10] dwClockFrequency
 *   [11]    bInCollection
 *   [12..]  baInterfaceNr[]
 */

TEST_F(DescriptorTest, VcHeaderValidNoStreamingInterfaces) {
  static const unsigned char block[] = {
    0x0c, 0x24, 0x01, 0x00, 0x01, 0x0c, 0x00,
    0x80, 0x8d, 0x5b, 0x00,   /* dwClockFrequency = 6000000 */
    0x00                      /* bInCollection = 0 */
  };

  SetUpConfig(1);

  EXPECT_EQ(uvc_parse_vc_header(nullptr, &info_, block, sizeof(block)),
            UVC_SUCCESS);
  EXPECT_EQ(info_.ctrl_if.bcdUVC, 0x0100);
  EXPECT_EQ(info_.ctrl_if.dwClockFrequency, 6000000u);
  EXPECT_EQ(info_.stream_ifs, nullptr);
}

TEST_F(DescriptorTest, VcHeaderOneStreamingInterface) {
  static const unsigned char block[] = {
    0x0d, 0x24, 0x01, 0x00, 0x01, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01,   /* bInCollection = 1 */
    0x01    /* baInterfaceNr[0] = 1, which exists below */
  };

  SetUpConfig(2);

  EXPECT_EQ(uvc_parse_vc_header(nullptr, &info_, block, sizeof(block)),
            UVC_SUCCESS);
  ASSERT_NE(info_.stream_ifs, nullptr);
  EXPECT_EQ(info_.stream_ifs->bInterfaceNumber, 1);
}

TEST_F(DescriptorTest, VcHeaderAcceptsEveryKnownUvcVersion) {
  for (uint16_t bcd : {0x0100, 0x010a, 0x0110, 0x0150}) {
    unsigned char block[] = {
      0x0c, 0x24, 0x01, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    block[3] = static_cast<unsigned char>(bcd & 0xff);
    block[4] = static_cast<unsigned char>(bcd >> 8);

    SetUpConfig(1);
    EXPECT_EQ(uvc_parse_vc_header(nullptr, &info_, block, sizeof(block)),
              UVC_SUCCESS) << "bcdUVC = " << std::hex << bcd;
    EXPECT_EQ(info_.ctrl_if.bcdUVC, bcd);

    uvc_test_info_free(&info_);
    uvc_test_config_free(&tc_);
    initialised_ = false;
  }
}

TEST_F(DescriptorTest, VcHeaderRejectsUnsupportedVersion) {
  static const unsigned char block[] = {
    0x0c, 0x24, 0x01, 0xff, 0xff, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  SetUpConfig(1);

  EXPECT_EQ(uvc_parse_vc_header(nullptr, &info_, block, sizeof(block)),
            UVC_ERROR_NOT_SUPPORTED);
}

/* Regression test for libuvc/libuvc#300.
 *
 * baInterfaceNr[] holds raw interface indices straight off the wire.
 * uvc_parse_vc_header() passes each one to uvc_scan_streaming(), which does
 *
 *   if_desc = &(info->config->interface[interface_idx].altsetting[0]);
 *
 * with no bound against config->bNumInterfaces. A descriptor naming an
 * interface that does not exist reads a struct libusb_interface past the end
 * of the array and dereferences whatever its altsetting pointer happens to
 * be -- the NULL deref at src/device.c:1332 in the report, or an arbitrary
 * read in general.
 */
TEST_F(DescriptorTest, VcHeaderRejectsInterfaceIndexOutOfRange) {
  static const unsigned char block[] = {
    0x0d, 0x24, 0x01, 0x00, 0x01, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01,   /* bInCollection = 1 */
    0x7f    /* baInterfaceNr[0] = 127: no such interface */
  };

  /* One interface, so index 0 is the only valid one. The parse must stay in
     bounds *and* report the failure; both are checked in the child. */
  EXPECT_VC_HEADER_REJECTED(1, block, sizeof(block));
}

/* The same bug reached directly, which is where the sanitizer report points. */
TEST_F(DescriptorTest, ScanStreamingAcceptsInterfaceIndexInRange) {
  SetUpConfig(2);

  EXPECT_EQ(uvc_scan_streaming(nullptr, &info_, 0), UVC_SUCCESS);
  EXPECT_EQ(uvc_scan_streaming(nullptr, &info_, 1), UVC_SUCCESS);
}

/* Each out-of-range index gets its own child: on unfixed code every one of
 * them is an out-of-bounds read, and in a single process the first would
 * abort before the others ran. */
TEST_F(DescriptorTest, ScanStreamingRejectsInterfaceIndexOutOfRange) {
  /* Negative is included because the parameter is an int, so nothing in the
     type system rules it out even though today's callers pass a uint8_t. */
  for (int idx : {2, 255, -1}) {
    EXPECT_MEMORY_SAFE({
      uvc_test_config_t tc;
      uvc_device_info_t info;
      uvc_test_config_init(&tc, 2);
      uvc_test_info_init(&info, &tc);
      REQUIRE_IN_CHILD(uvc_scan_streaming(nullptr, &info, idx) != UVC_SUCCESS);
      uvc_test_info_free(&info);
      uvc_test_config_free(&tc);
    }) << "interface index " << idx;
  }
}

/* Every index in a multi-entry baInterfaceNr[] must be checked, not just the
 * first: a descriptor can name a valid interface and then a bogus one. */
TEST_F(DescriptorTest, VcHeaderRejectsLaterInterfaceIndexOutOfRange) {
  static const unsigned char block[] = {
    0x0e, 0x24, 0x01, 0x00, 0x01, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02,   /* bInCollection = 2 */
    0x01,   /* baInterfaceNr[0] = 1, valid */
    0x42    /* baInterfaceNr[1] = 66, not valid */
  };

  EXPECT_VC_HEADER_REJECTED(2, block, sizeof(block));
}

/* uvc_parse_vc() dispatches on bDescriptorType; anything that is not
 * CS_INTERFACE is skipped rather than misparsed. */
TEST_F(DescriptorTest, ParseVcIgnoresNonClassSpecificDescriptor) {
  static const unsigned char block[] = {
    0x09, 0x04, 0x01, 0x00, 0x00, 0x0e, 0x01, 0x00, 0x00
  };

  SetUpConfig(1);

  EXPECT_EQ(uvc_parse_vc(nullptr, &info_, block, sizeof(block)), UVC_SUCCESS);
}

/* uvc_scan_control() only guarantees three bytes are present before calling
 * uvc_parse_vc(), but every parser below it reads well past that:
 * uvc_parse_vc_header() alone reads block[3..4] for bcdUVC. The parser must
 * not read beyond the block_size it was given. */
TEST_F(DescriptorTest, ParseVcShortBlockStaysInBounds) {
  static const unsigned char block[] = { 0x03, 0x24, 0x01 };

  /* The return value is not the point; staying in bounds is. */
  EXPECT_MEMORY_SAFE({
    uvc_test_config_t tc;
    uvc_device_info_t info;
    uvc_test_config_init(&tc, 1);
    uvc_test_info_init(&info, &tc);
    (void)uvc_parse_vc(nullptr, &info, block, sizeof(block));
    uvc_test_info_free(&info);
    uvc_test_config_free(&tc);
  });
}

/* An extension unit's pin count and control size are both attacker
 * controlled and index into the block unbounded:
 *   num_in_pins      = block[21]
 *   size_of_controls = block[22 + num_in_pins]
 *   controls start at &block[23 + num_in_pins]
 */
TEST_F(DescriptorTest, ParseVcExtensionUnitOversizedControls) {
  unsigned char block[26] = {};

  block[0] = sizeof(block);   /* bLength */
  block[1] = 0x24;            /* CS_INTERFACE */
  block[2] = 0x06;            /* VC_EXTENSION_UNIT */
  block[3] = 0x05;            /* bUnitID */
  /* block[4..19] guidExtensionCode, block[20] bNumControls */
  block[21] = 0x00;           /* bNrInPins = 0 */
  block[22] = 0xff;           /* bControlSize, far beyond the block */

  EXPECT_MEMORY_SAFE({
    uvc_test_config_t tc;
    uvc_device_info_t info;
    uvc_test_config_init(&tc, 1);
    uvc_test_info_init(&info, &tc);
    (void)uvc_parse_vc(nullptr, &info, block, sizeof(block));
    uvc_test_info_free(&info);
    uvc_test_config_free(&tc);
  });
}

TEST_F(DescriptorTest, ParseVcExtensionUnitOversizedPinCount) {
  unsigned char block[26] = {};

  block[0] = sizeof(block);
  block[1] = 0x24;
  block[2] = 0x06;
  block[3] = 0x05;
  block[21] = 0xff;           /* bNrInPins: block[22 + 255] is out of bounds */

  EXPECT_MEMORY_SAFE({
    uvc_test_config_t tc;
    uvc_device_info_t info;
    uvc_test_config_init(&tc, 1);
    uvc_test_info_init(&info, &tc);
    (void)uvc_parse_vc(nullptr, &info, block, sizeof(block));
    uvc_test_info_free(&info);
    uvc_test_config_free(&tc);
  });
}

/* Input terminal: bControlSize at block[14] drives a descending loop from
 * 14 + block[14] down to 15, so a large value reads past the block. */
TEST_F(DescriptorTest, ParseVcInputTerminalOversizedControls) {
  unsigned char block[18] = {};

  block[0] = sizeof(block);
  block[1] = 0x24;
  block[2] = 0x02;            /* VC_INPUT_TERMINAL */
  block[3] = 0x01;            /* bTerminalID */
  block[4] = 0x01;            /* wTerminalType = ITT_CAMERA (0x0201) */
  block[5] = 0x02;
  block[14] = 0xff;           /* bControlSize */

  EXPECT_MEMORY_SAFE({
    uvc_test_config_t tc;
    uvc_device_info_t info;
    uvc_test_config_init(&tc, 1);
    uvc_test_info_init(&info, &tc);
    (void)uvc_parse_vc(nullptr, &info, block, sizeof(block));
    uvc_test_info_free(&info);
    uvc_test_config_free(&tc);
  });
}

/* Processing unit: same shape, bControlSize at block[7]. */
TEST_F(DescriptorTest, ParseVcProcessingUnitOversizedControls) {
  unsigned char block[13] = {};

  block[0] = sizeof(block);
  block[1] = 0x24;
  block[2] = 0x05;            /* VC_PROCESSING_UNIT */
  block[3] = 0x02;            /* bUnitID */
  block[7] = 0xff;            /* bControlSize */

  EXPECT_MEMORY_SAFE({
    uvc_test_config_t tc;
    uvc_device_info_t info;
    uvc_test_config_init(&tc, 1);
    uvc_test_info_init(&info, &tc);
    (void)uvc_parse_vc(nullptr, &info, block, sizeof(block));
    uvc_test_info_free(&info);
    uvc_test_config_free(&tc);
  });
}

/* uvc_scan_streaming() advances by block_size = buffer[0]. A zero bLength
 * never advances the cursor, so the loop spins forever. Wrapped in a death
 * test with a timeout so a regression fails the suite instead of hanging CI.
 */
TEST_F(DescriptorTest, ScanStreamingTerminatesOnZeroLengthBlock) {
  static const unsigned char extra[] = { 0x00, 0x24, 0x01, 0x00, 0x00 };

  /* If the zero-length guard is missing this never returns, and the test
     times out instead of wedging the whole run. */
  EXPECT_MEMORY_SAFE({
    uvc_test_config_t tc;
    uvc_device_info_t info;
    uvc_test_config_init(&tc, 1);
    uvc_test_config_set_extra(&tc, 0, extra, sizeof(extra));
    uvc_test_info_init(&info, &tc);
    (void)uvc_scan_streaming(nullptr, &info, 0);
    uvc_test_info_free(&info);
    uvc_test_config_free(&tc);
  });
}

/* A bLength larger than the bytes remaining underflows buffer_left, which is
 * a size_t, so the loop keeps going over memory past the buffer. */
TEST_F(DescriptorTest, ScanStreamingBlockSizeOverrunsBuffer) {
  static const unsigned char extra[] = { 0xff, 0x24, 0x01, 0x00, 0x00 };

  EXPECT_MEMORY_SAFE({
    uvc_test_config_t tc;
    uvc_device_info_t info;
    uvc_test_config_init(&tc, 1);
    uvc_test_config_set_extra(&tc, 0, extra, sizeof(extra));
    uvc_test_info_init(&info, &tc);
    (void)uvc_scan_streaming(nullptr, &info, 0);
    uvc_test_info_free(&info);
    uvc_test_config_free(&tc);
  });
}

/* Same underflow in uvc_scan_control()'s loop, reached through the VC path. */
TEST_F(DescriptorTest, ScanStreamingEmptyExtraIsSafe) {
  SetUpConfig(1);
  uvc_test_config_set_extra(&tc_, 0, nullptr, 0);

  EXPECT_EQ(uvc_scan_streaming(nullptr, &info_, 0), UVC_SUCCESS);
}

}  // namespace
