#include <OeApp/Yaml_config_reader.h>
#include <OeCore/EngineUtils.h>

#include <gtest/gtest.h>

#include "tests_main.h"

using oe::testing_helpers::CaptureLogger;
using oe::testing_helpers::mockFatalWasCalled;
using oe::testing_helpers::clearMockFatal;

class ReaderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto testData = std::stringstream(kTestData);
    _reader = std::make_unique<oe::Yaml_config_reader>(testData);
    _logger = std::make_unique<CaptureLogger>();

    // Disable OE_CHECK from triggering breakpoints
    oe::oe_set_enable_check_debugbreak(false);
  }

  void TearDown() override {
    _reader.reset();
    _logger.reset();
  }

  oe::Yaml_config_reader& reader() { return *_reader; }
  CaptureLogger& logger() { return *_logger; }

 private:
  const std::string kTestData = "hello:\n  world: nice\nstrList:\n- thing\nintList:\n- 1\n- 2\ndblList:\n- 0.1";
  std::unique_ptr<oe::Yaml_config_reader> _reader;
  std::unique_ptr<CaptureLogger> _logger;
};

TEST_F(ReaderTest, invalid_map_key)
{
  ASSERT_EQ(reader().readString("hello.nonworld"), "");
  auto logs = logger().flushAndReset();
  ASSERT_FALSE(logs.containsLog(WARNING));
  ASSERT_TRUE(logs.wasFatalCalled());
}

TEST_F(ReaderTest, valid_input)
{
  ASSERT_EQ(reader().readString("hello.world"), "nice");
  auto logs = logger().flushAndReset();
  ASSERT_FALSE(logger().flushAndReset().containsLog(WARNING));
  ASSERT_FALSE(logs.wasFatalCalled());

  ASSERT_EQ(reader().readString("strList.0"), "thing");
  logs = logger().flushAndReset();
  ASSERT_FALSE(logger().flushAndReset().containsLog(WARNING));
  ASSERT_FALSE(logs.wasFatalCalled());
}

TEST_F(ReaderTest, invalid_sequence_key)
{
  // Out of range index
  ASSERT_EQ(reader().readString("strList.1"), "");
  auto logs = logger().flushAndReset();
  ASSERT_FALSE(logs.containsLog(WARNING));
  ASSERT_TRUE(logs.wasFatalCalled());

  // Can't convert index to int
  ASSERT_EQ(reader().readString("strList.zero"), "");
  logs = logger().flushAndReset();
  ASSERT_TRUE(logs.containsLog("Failed to get config node; sequence index 'zero' cannot be converted to int.", WARNING));
  ASSERT_TRUE(logs.wasFatalCalled());
}

TEST_F(ReaderTest, default_values)
{
  reader().setDefault("testList.1", "one");
  ASSERT_EQ(reader().readString("testList.1"), "one");
  auto logs = logger().flushAndReset();
  ASSERT_FALSE(logs.containsLog(WARNING));
  ASSERT_FALSE(logs.wasFatalCalled());

  reader().setDefault("testList.2", (int64_t)2);
  ASSERT_EQ(reader().readInt("testList.2"), 2);
  logs = logger().flushAndReset();
  ASSERT_FALSE(logs.containsLog(WARNING));
  ASSERT_FALSE(logs.wasFatalCalled());

  reader().setDefault("testList.3", (double)0.3);
  ASSERT_EQ(reader().readDouble("testList.3"), 0.3);
  logs = logger().flushAndReset();
  ASSERT_FALSE(logs.containsLog(WARNING));
  ASSERT_FALSE(logs.wasFatalCalled());

  std::vector<std::string> defaultStrings = {"four", "fours"};
  reader().setDefault("testList.4", defaultStrings);
  ASSERT_EQ(reader().readString("testList.4.0"), "four");
  ASSERT_EQ(reader().readString("testList.4.1"), "fours");
  logs = logger().flushAndReset();
  ASSERT_FALSE(logs.containsLog(WARNING));
  ASSERT_FALSE(logs.wasFatalCalled());
}

TEST_F(ReaderTest, valid_sequences)
{
  auto strings = reader().readStringList("strList");
  ASSERT_FALSE(logger().flushAndReset().containsLog(WARNING));
  ASSERT_EQ(strings.size(), 1);
  ASSERT_EQ(strings[0], "thing");

  auto doubles = reader().readDoubleList("dblList");
  ASSERT_FALSE(logger().flushAndReset().containsLog(WARNING));
  ASSERT_EQ(doubles.size(), 1);
  ASSERT_EQ(doubles[0], 0.1);

  auto ints = reader().readIntList("intList");
  ASSERT_FALSE(logger().flushAndReset().containsLog(WARNING));
  ASSERT_EQ(ints.size(), 2);
  ASSERT_EQ(ints[0], 1);
  ASSERT_EQ(ints[1], 2);
}
