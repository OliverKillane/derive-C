#include <gtest/gtest.h>

#include <derive-c/utils/file_handle.h>

#include <unistd.h>

#include <cstdlib>
#include <filesystem>

struct FileHandleTests : testing::Test {
    std::filesystem::path mTempPath;

    void SetUp() override {
        std::string tmpl = (std::filesystem::temp_directory_path() / "dc_test_XXXXXX").string();
        int fd = mkstemp(tmpl.data());
        ASSERT_NE(fd, -1);
        close(fd);
        mTempPath = tmpl;
    }

    void TearDown() override { std::filesystem::remove(mTempPath); }
};

TEST_F(FileHandleTests, NewAndDelete) {
    DC_SCOPED(dc_file_handle) file = dc_file_handle_new(mTempPath.c_str(), "w");
    ASSERT_NE(dc_file_handle_get(&file), nullptr);
}

TEST_F(FileHandleTests, WriteAndRead) {
    {
        DC_SCOPED(dc_file_handle) file = dc_file_handle_new(mTempPath.c_str(), "w");
        fprintf(dc_file_handle_get(&file), "hello world");
    }

    {
        DC_SCOPED(dc_file_handle) file = dc_file_handle_new(mTempPath.c_str(), "r");
        char buffer[64] = {0};
        ASSERT_NE(fgets(buffer, sizeof(buffer), dc_file_handle_get(&file)), nullptr);
        EXPECT_STREQ(buffer, "hello world");
    }
}
