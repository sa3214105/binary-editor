#include <gtest/gtest.h>
#include "../src/binary_editor.hpp"

using namespace binary;

TEST(BinaryEditorTest, ConstructorAndSize) {
    std::unique_ptr<const uint8_t[]> data = std::make_unique<uint8_t[]>(10);
    binary_editor editor(std::move(data), 10);
    EXPECT_EQ(editor.size(), 10);
}

TEST(BinaryEditorTest, GetData) {
    std::unique_ptr<const uint8_t[]> data = std::make_unique<uint8_t[]>(10);
    for (size_t i = 0; i < 10; ++i) {
        const_cast<uint8_t*>(data.get())[i] = static_cast<uint8_t>(i);
    }
    binary_editor editor(std::move(data), 10);
    const uint8_t* retrieved_data = static_cast<const uint8_t*>(editor.get_data());
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(retrieved_data[i], static_cast<uint8_t>(i));
    }
}

TEST(BinaryEditorTest, PushBack) {
    std::unique_ptr<const uint8_t[]> data1 = std::make_unique<uint8_t[]>(5);
    std::unique_ptr<const uint8_t[]> data2 = std::make_unique<uint8_t[]>(5);
    binary_editor editor1(std::move(data1), 5);
    binary_editor editor2(std::move(data2), 5);
    editor1.push_back(editor2);
    EXPECT_EQ(editor1.size(), 10);
}

TEST(BinaryEditorTest, PushFront) {
    std::unique_ptr<const uint8_t[]> data1 = std::make_unique<uint8_t[]>(5);
    std::unique_ptr<const uint8_t[]> data2 = std::make_unique<uint8_t[]>(5);
    binary_editor editor1(std::move(data1), 5);
    binary_editor editor2(std::move(data2), 5);
    editor1.push_front(editor2);
    EXPECT_EQ(editor1.size(), 10);
}

TEST(BinaryEditorTest, Insert) {
    // 準備第一個 binary_editor
    std::unique_ptr<const uint8_t[]> data1 = std::make_unique<uint8_t[]>(10);
    for (size_t i = 0; i < 10; ++i) {
        const_cast<uint8_t*>(data1.get())[i] = static_cast<uint8_t>(i);
    }
    binary_editor editor1(std::move(data1), 10);

    // 準備第二個 binary_editor
    std::unique_ptr<const uint8_t[]> data2 = std::make_unique<uint8_t[]>(5);
    for (size_t i = 0; i < 5; ++i) {
        const_cast<uint8_t*>(data2.get())[i] = static_cast<uint8_t>(i + 100);
    }
    binary_editor editor2(std::move(data2), 5);

    // 在 offset 5 插入 editor2
    editor1.insert(5, editor2);

    // 驗證插入後的大小
    EXPECT_EQ(editor1.size(), 15);

    // 驗證插入後的數據
    const uint8_t* retrieved_data = static_cast<const uint8_t*>(editor1.get_data());
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(retrieved_data[i], static_cast<uint8_t>(i)); // 原始數據
    }
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(retrieved_data[i + 5], static_cast<uint8_t>(i + 100)); // 插入的數據
    }
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(retrieved_data[i + 10], static_cast<uint8_t>(i + 5)); // 原始數據的剩餘部分
    }
}

TEST(BinaryEditorTest, TidyChunks) {
    std::unique_ptr<const uint8_t[]> data1 = std::make_unique<uint8_t[]>(5);
    std::unique_ptr<const uint8_t[]> data2 = std::make_unique<uint8_t[]>(5);
    for (size_t i = 0; i < 5; ++i) {
        const_cast<uint8_t*>(data1.get())[i] = static_cast<uint8_t>(i);
        const_cast<uint8_t*>(data2.get())[i] = static_cast<uint8_t>(i + 5);
    }
    binary_editor editor1(std::move(data1), 5);
    binary_editor editor2(std::move(data2), 5);
    editor1.push_back(editor2);
    editor1.tidy_chunks();
    const uint8_t* retrieved_data = static_cast<const uint8_t*>(editor1.get_data());
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(retrieved_data[i], static_cast<uint8_t>(i));
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}