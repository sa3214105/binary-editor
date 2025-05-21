#include "../src/binary_editor.hpp"
#include <gtest/gtest.h>

using namespace binary;
using namespace reader;
using namespace writer;

struct simplesample
{
    binary::binary_editor  editor;                 ///< The binary editor instance.
    binary_reader<uint8_t> value1{editor, 0};      ///< Reads the first byte from the editor.
    binary_reader<uint8_t> value2{editor, value1}; ///< Reads the next byte from the editor (offset = value1).
};

TEST(BinaryReaderTest, ReadValues)
{
    // Prepare a binary blob with known values
    std::vector<uint8_t> blob = {2, 99, 255};

    // Create sample
    simplesample sample{.editor = binary_editor(blob.data(), blob.size())};

    // Test value1 and value2
    EXPECT_EQ(sample.value1.get(), 2);
    EXPECT_EQ(sample.value2.get(), 255);
}

TEST(BinaryEditorTest, ConstructorAndSize)
{
    std::unique_ptr<const uint8_t[]> data = std::make_unique<uint8_t[]>(10);
    binary_editor                    editor(std::move(data), 10);
    EXPECT_EQ(editor.size(), 10);
}

TEST(BinaryEditorTest, GetData)
{
    std::unique_ptr<const uint8_t[]> data = std::make_unique<uint8_t[]>(10);
    for (size_t i = 0; i < 10; ++i)
    {
        const_cast<uint8_t*>(data.get())[i] = static_cast<uint8_t>(i);
    }
    binary_editor  editor(std::move(data), 10);
    const uint8_t* retrieved_data = static_cast<const uint8_t*>(editor.get_data());
    for (size_t i = 0; i < 10; ++i)
    {
        EXPECT_EQ(retrieved_data[i], static_cast<uint8_t>(i));
    }
}

TEST(BinaryEditorTest, PushBack)
{
    std::unique_ptr<const uint8_t[]> data1 = std::make_unique<uint8_t[]>(5);
    std::unique_ptr<const uint8_t[]> data2 = std::make_unique<uint8_t[]>(5);
    binary_editor                    editor1(std::move(data1), 5);
    binary_editor                    editor2(std::move(data2), 5);
    editor1.push_back(editor2);
    EXPECT_EQ(editor1.size(), 10);
}

TEST(BinaryEditorTest, PushFront)
{
    std::unique_ptr<const uint8_t[]> data1 = std::make_unique<uint8_t[]>(5);
    std::unique_ptr<const uint8_t[]> data2 = std::make_unique<uint8_t[]>(5);
    binary_editor                    editor1(std::move(data1), 5);
    binary_editor                    editor2(std::move(data2), 5);
    editor1.push_front(editor2);
    EXPECT_EQ(editor1.size(), 10);
}

TEST(BinaryEditorTest, Insert)
{
    // 準備第一個 binary_editor
    std::unique_ptr<const uint8_t[]> data1 = std::make_unique<uint8_t[]>(10);
    for (size_t i = 0; i < 10; ++i)
    {
        const_cast<uint8_t*>(data1.get())[i] = static_cast<uint8_t>(i);
    }
    binary_editor editor1(std::move(data1), 10);

    // 準備第二個 binary_editor
    std::unique_ptr<const uint8_t[]> data2 = std::make_unique<uint8_t[]>(5);
    for (size_t i = 0; i < 5; ++i)
    {
        const_cast<uint8_t*>(data2.get())[i] = static_cast<uint8_t>(i + 100);
    }
    binary_editor editor2(std::move(data2), 5);

    // 在 offset 5 插入 editor2
    editor1.insert(5, editor2);

    // 驗證插入後的大小
    EXPECT_EQ(editor1.size(), 15);

    // 驗證插入後的數據
    const uint8_t* retrieved_data = static_cast<const uint8_t*>(editor1.get_data());
    for (size_t i = 0; i < 5; ++i)
    {
        EXPECT_EQ(retrieved_data[i], static_cast<uint8_t>(i)); // 原始數據
    }
    for (size_t i = 0; i < 5; ++i)
    {
        EXPECT_EQ(retrieved_data[i + 5], static_cast<uint8_t>(i + 100)); // 插入的數據
    }
    for (size_t i = 0; i < 5; ++i)
    {
        EXPECT_EQ(retrieved_data[i + 10], static_cast<uint8_t>(i + 5)); // 原始數據的剩餘部分
    }
}

TEST(BinaryEditorTest, TidyChunks)
{
    std::unique_ptr<const uint8_t[]> data1 = std::make_unique<uint8_t[]>(5);
    std::unique_ptr<const uint8_t[]> data2 = std::make_unique<uint8_t[]>(5);
    for (size_t i = 0; i < 5; ++i)
    {
        const_cast<uint8_t*>(data1.get())[i] = static_cast<uint8_t>(i);
        const_cast<uint8_t*>(data2.get())[i] = static_cast<uint8_t>(i + 5);
    }
    binary_editor editor1(std::move(data1), 5);
    binary_editor editor2(std::move(data2), 5);
    editor1.push_back(editor2);
    editor1.tidy_chunks();
    const uint8_t* retrieved_data = static_cast<const uint8_t*>(editor1.get_data());
    for (size_t i = 0; i < 10; ++i)
    {
        EXPECT_EQ(retrieved_data[i], static_cast<uint8_t>(i));
    }
}

TEST(BinaryContainerReaderTest, BasicUsage)
{
    std::vector<uint8_t>             blob = {10, 20, 30, 40, 50, 60, 70, 80};
    binary_editor                    editor(blob.data(), blob.size());
    binary_container_reader<uint8_t> container(editor, 2, 4);

    EXPECT_EQ(container.size(), 4);
    EXPECT_EQ(container[0], 30);
    EXPECT_EQ(container[1], 40);
    EXPECT_EQ(container[2], 50);
    EXPECT_EQ(container[3], 60);

    // at() 越界
    EXPECT_THROW(container.at(4), reader_exception);

    // operator[] 越界
    EXPECT_THROW(container[4], reader_exception);

    // iterator 正確遍歷
    std::vector<uint8_t> values;
    for (auto it = container.begin(); it != container.end(); ++it)
    {
        values.push_back(*it);
    }
    EXPECT_EQ(values, (std::vector<uint8_t>{30, 40, 50, 60}));
}

TEST(BinaryContainerReaderTest, LargeData)
{
    // 建立 10000 筆遞增資料
    std::vector<uint32_t> blob(10000);
    for (size_t i = 0; i < blob.size(); ++i)
    {
        blob[i] = static_cast<uint32_t>(i * 2);
    }
    binary_editor                     editor(reinterpret_cast<uint8_t*>(blob.data()), blob.size() * sizeof(uint32_t));
    binary_container_reader<uint32_t> container(editor, 100 * sizeof(uint32_t), 5000);

    EXPECT_EQ(container.size(), 5000);
    EXPECT_EQ(container[0], 200);      // 第 100 筆
    EXPECT_EQ(container[4999], 10198); // 第 5099 筆

    // iterator 正確遍歷
    size_t idx = 0;
    for (auto it = container.begin(); it != container.end(); ++it, ++idx)
    {
        EXPECT_EQ(*it, static_cast<uint32_t>((idx + 100) * 2));
    }
}

TEST(WriterTest, WriteBackAndFront)
{
    binary_editor editor;

    // Write to back
    uint8_t v1 = 42;
    write_back(editor, v1);
    EXPECT_EQ(editor.size(), sizeof(uint8_t));
    EXPECT_EQ(*((uint8_t*)editor.get_data()), 42);

    // Write to front
    uint8_t v2 = 99;
    write_front(editor, v2);
    EXPECT_EQ(editor.size(), 2 * sizeof(uint8_t));
    // Front should be v2, back should be v1
    const uint8_t* data = static_cast<const uint8_t*>(editor.get_data());
    EXPECT_EQ(data[0], 99);
    EXPECT_EQ(data[1], 42);
}

TEST(WriterTest, WriteAt)
{
    binary_editor editor;
    // 先寫三個數字
    uint8_t a = 1, b = 2, c = 3;
    write_back(editor, a);
    write_back(editor, b);
    write_back(editor, c);

    // 在 offset 1 寫入 99
    uint8_t v = 99;
    write_at(editor, 1, v);

    // 預期結果: [1, 99, 2, 3]
    EXPECT_EQ(editor.size(), 4);
    const uint8_t* data = static_cast<const uint8_t*>(editor.get_data());
    EXPECT_EQ(data[0], 1);
    EXPECT_EQ(data[1], 99);
    EXPECT_EQ(data[2], 2);
    EXPECT_EQ(data[3], 3);
}

TEST(WriterTest, StructRead)
{
    binary_editor editor;
    write_back(editor, 1);
    write_back(editor, 2.0);
    write_back(editor, 'x');

    struct test_struct
    {
        binary_editor      binary;
        binary_reader<int> a{binary, 0};
        binary_reader<double> b{binary, sizeof(int)};
        binary_reader<char> c{binary, sizeof(int)+sizeof(double)};
    };

    test_struct ts{.binary = editor};
    EXPECT_EQ(ts.a.get(), 1);
    EXPECT_EQ(ts.b.get(), 2.0);
    EXPECT_EQ(ts.c.get(), 'x');
    EXPECT_EQ(ts.binary.size(), sizeof(int) + sizeof(double) + sizeof(char));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}