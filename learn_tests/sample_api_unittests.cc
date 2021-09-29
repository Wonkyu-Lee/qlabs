#include "qlabs/learn_tests/sample_api.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sample_api {
namespace {

TEST(SampleApi, ApiFunctionTest) {
  EXPECT_TRUE(CallApiFunction());
}

}  // namespace
}  // namespace sample_api
