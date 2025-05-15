#pragma once
#include <memory>
#include <string>
#include <deque>
#include <iterator>
#include <cstring>
#include <variant>

namespace binary
{
    /**
     * @brief Exception class for binary editor errors.
     */
    class binary_exception : public std::exception
    {
    protected:
        std::string m_error_msg;

    public:
        /**
         * @brief Construct a binary_exception with an error message.
         * @param errorMsg The error message.
         */
        binary_exception(const std::string &errorMsg)
            : m_error_msg(errorMsg)
        {
        }
        /**
         * @brief Construct a binary_exception with an rvalue error message.
         * @param errorMsg The error message.
         */
        binary_exception(std::string &&errorMsg)
            : m_error_msg(std::move(errorMsg))
        {
        }
        /**
         * @brief Get the error message.
         * @return The error message as a C-string.
         */
        virtual const char *what() const noexcept override
        {
            return m_error_msg.c_str();
        }
    };

    /**
     * @brief Enum for chunk types.
     */
    enum class CHUNK_TYPE
    {
        MEMORY ///< Memory chunk
    };

    /**
     * @brief Interface for binary chunk.
     */
    class binary_chunk_interface
    {
    public:
        /**
         * @brief Create a sub-chunk from this chunk.
         * @param offset The offset to start from.
         * @param size The size of the sub-chunk.
         * @return Shared pointer to the sub-chunk.
         */
        virtual std::shared_ptr<binary_chunk_interface> create_sub_chunk(const size_t &offset, const size_t &size) const = 0;
        /**
         * @brief Get the size of the chunk.
         * @return The size in bytes.
         */
        virtual size_t size() const = 0;
        /**
         * @brief Get the data pointer of the chunk.
         * @return Pointer to the data.
         */
        virtual const uint8_t *get_data() const = 0;
        /**
         * @brief Get the type of the chunk.
         * @return The chunk type.
         */
        virtual CHUNK_TYPE get_type() const = 0;
        /**
         * @brief Clone the chunk.
         * @return Unique pointer to the cloned chunk.
         */
        virtual std::unique_ptr<binary_chunk_interface> clone() const = 0;
        /**
         * @brief Downscale the chunk size.
         * @param targeSize The new size.
         */
        virtual void downscale_size(const size_t &targeSize) = 0;
    };

    /**
     * @brief Implementation of a memory chunk.
     */
    class binary_chunk_memory : public binary_chunk_interface
    {
    private:
        std::shared_ptr<const std::unique_ptr<const uint8_t[]>> m_ppBlob = nullptr;
        size_t m_size = 0;
        size_t m_offset = 0;

    public:
        /**
         * @brief Construct a memory chunk.
         * @param pBlob The data pointer.
         * @param size The size of the data.
         * @param offset The offset in the data.
         * @throws binary_exception if offset > size or pBlob is nullptr.
         */
        binary_chunk_memory(std::unique_ptr<const uint8_t[]> &&pBlob, const size_t &size, const size_t &offset = 0)
            : m_size(size), m_offset(offset)
        {
            if (offset > size)
            {
                throw binary_exception("binary_chunk_memory::binary_chunk_memory err : offset must not be greater than size!");
            }
            if (pBlob == nullptr)
            {
                throw binary_exception("binary_chunk_memory::binary_chunk_memory err : pBlob must not be nullptr!");
            }
            m_ppBlob = std::make_shared<std::unique_ptr<const uint8_t[]>>(std::move(pBlob));
        }
        /**
         * @copydoc binary_chunk_interface::create_sub_chunk
         */
        virtual std::shared_ptr<binary_chunk_interface> create_sub_chunk(const size_t &offset, const size_t &size) const override final
        {
            if (offset + size > m_size)
            {
                throw binary_exception("binary_chunk_memory::create_sub_chunk err : (offset + size) must not be greater than m_Size!");
            }
            auto pRet = std::make_shared<binary_chunk_memory>(*this);
            pRet->m_offset = offset;
            pRet->m_size = size;
            return std::dynamic_pointer_cast<binary_chunk_interface>(pRet);
        }
        /**
         * @copydoc binary_chunk_interface::size
         */
        virtual size_t size() const override final
        {
            return m_size;
        }
        /**
         * @copydoc binary_chunk_interface::get_data
         */
        virtual const uint8_t *get_data() const override final
        {
            return m_ppBlob->get() + m_offset;
        }
        /**
         * @copydoc binary_chunk_interface::get_type
         */
        virtual CHUNK_TYPE get_type() const override final
        {
            return CHUNK_TYPE::MEMORY;
        }
        /**
         * @copydoc binary_chunk_interface::clone
         */
        virtual std::unique_ptr<binary_chunk_interface> clone() const override
        {
            return std::make_unique<binary_chunk_memory>(*this);
        }
        /**
         * @copydoc binary_chunk_interface::downscale_size
         */
        virtual void downscale_size(const size_t &targeSize) override final
        {
            m_size = targeSize;
        }
    };

    /**
     * @brief Factory for creating binary chunks.
     */
    class binary_chunk_factory
    {
    public:
        /**
         * @brief Chunk creation strategy.
         */
        enum class CREATE_STRATEGY
        {
            AUTO,  ///< Automatically select strategy
            MEMORY ///< Always use memory chunk
        };

    private:
        CREATE_STRATEGY m_create_strategy = CREATE_STRATEGY::AUTO;

    public:
        /**
         * @brief Create a chunk using the current strategy.
         * @param pBlob The data pointer.
         * @param size The size of the data.
         * @param offset The offset in the data.
         * @return Shared pointer to the created chunk.
         * @throws binary_exception if strategy is unknown.
         */
        std::shared_ptr<binary_chunk_interface> create_chunk(std::unique_ptr<const uint8_t[]> &&pBlob, const size_t &size, const size_t &offset = 0) const
        {
            switch (m_create_strategy)
            {
            case CREATE_STRATEGY::AUTO:
            case CREATE_STRATEGY::MEMORY:
                return std::make_shared<binary_chunk_memory>(std::move(pBlob), size, offset);
            default:
                throw binary_exception("binary_chunk_factory::create_chunk err : unknown create strategy!");
            }
        }
    };

    /**
     * @brief Main class for binary editing.
     */
    class binary_editor
    {
    private:
        mutable std::deque<std::shared_ptr<binary_chunk_interface>> m_pChunks; ///< Chunks managed by the editor
        binary_chunk_factory m_binary_chunk_factory;                           ///< Factory for creating chunks
        bool m_auto_tidy = false;                                              ///< Whether to auto tidy chunks
        size_t m_auto_tidy_size = 0;                                           ///< Auto tidy threshold
    public:
        /**
         * @brief Default constructor.
         */
        binary_editor() = default;
        /**
         * @brief Construct editor from a blob.
         * @param pBlob The data pointer.
         * @param size The size of the data.
         */
        binary_editor(std::unique_ptr<const uint8_t[]> &&pBlob, const size_t &size)
        {
            m_pChunks.push_back(m_binary_chunk_factory.create_chunk(std::move(pBlob), size));
        }

        /**
         * @brief Construct editor from a blob.
         * @param pBlob The data pointer.
         * @param size The size of the data.
         */
        binary_editor(const uint8_t *pBlob, const size_t &size)
        {
            auto buffer = std::make_unique<uint8_t[]>(size);
            memcpy(buffer.get(), pBlob, size);
            *this = binary_editor(std::move(buffer), size);
        }
        /**
         * @brief Get the total size of all chunks.
         * @return Total size in bytes.
         */
        size_t size() const
        {
            size_t ret = 0;
            for (auto &pChunk : m_pChunks)
            {
                ret += pChunk->size();
            }
            return ret;
        }
        /**
         * @brief Merge all chunks into one.
         */
        void tidy_chunks() const
        {
            size_t totalSize = size();
            std::unique_ptr<uint8_t[]> pBlob = std::make_unique<uint8_t[]>(totalSize);
            auto pCurrent = pBlob.get();
            for (auto &pChunk : m_pChunks)
            {
                memcpy(pCurrent, pChunk->get_data(), pChunk->size());
                pCurrent += pChunk->size();
            }
            *const_cast<binary_editor *>(this) = binary_editor(std::move(pBlob), totalSize);
        }
        /**
         * @brief Get the pointer to the merged data.
         * @return Pointer to the data.
         */
        const void *get_data() const
        {
            tidy_chunks();
            return m_pChunks.front()->get_data();
        }
        /**
         * @brief Create a sub-editor from a range.
         * @param offset The offset to start from.
         * @param size The size of the sub-editor.
         * @return The sub-editor.
         * @throws binary_exception if range is invalid.
         */
        binary_editor create_sub_editor(const size_t &offset, const size_t size) const
        {
            // check size
            size_t currentSize = this->size();
            if (offset + size > currentSize)
            {
                throw binary_exception("binary_editor::create_sub_editor err : (offset + size) must not be greater than m_Size!");
            }

            // clone data
            size_t currentOffset = 0;
            size_t retSize = 0;
            binary_editor ret;
            for (const auto &pChunk : m_pChunks)
            {
                // check whether push in chunk is needed
                if (currentOffset + pChunk->size() <= offset)
                {
                    currentOffset += pChunk->size();
                    continue;
                }

                // clone chunk and resize
                auto pNewChunk = pChunk->clone();
                if (retSize + pChunk->size() > size)
                {
                    pNewChunk->downscale_size(size - retSize);
                    retSize = size;
                }
                else
                {
                    retSize += pChunk->size();
                }

                // push in chunk and check the size
                ret.m_pChunks.push_back(std::move(pNewChunk));
                if (retSize == size)
                {
                    break;
                }
            }

            return ret;
        }
        /**
         * @brief Append another editor's chunks to the back.
         * @param backEditor The editor to append.
         */
        void push_back(const binary_editor &backEditor)
        {
            std::copy(backEditor.m_pChunks.begin(), backEditor.m_pChunks.end(), std::back_inserter(m_pChunks));
        }
        /**
         * @brief Emplace a new chunk at the back.
         * @tparam Args Constructor arguments for the chunk.
         * @param args Arguments to forward.
         */
        template <typename... Args>
        void emplace_back(Args &&...args)
        {
            m_pChunks.push_back(m_binary_chunk_factory.create_chunk(std::forward<Args>(args)...));
        }
        /**
         * @brief Append another editor's chunks to the front.
         * @param frontEditor The editor to prepend.
         */
        void push_front(const binary_editor &frontEditor)
        {
            std::copy(frontEditor.m_pChunks.rbegin(), frontEditor.m_pChunks.rend(), std::front_inserter(m_pChunks));
        }
        /**
         * @brief Emplace a new chunk at the front.
         * @tparam Args Constructor arguments for the chunk.
         * @param args Arguments to forward.
         */
        template <typename... Args>
        void emplace_front(Args &&...args)
        {
            m_pChunks.push_front(m_binary_chunk_factory.create_chunk(std::forward<Args>(args)...));
        }
        /**
         * @brief Insert another editor's chunks at a specific offset.
         * @param offset The offset to insert at.
         * @param editor The editor whose chunks to insert.
         * @throws binary_exception if offset is invalid.
         */
        void insert(const size_t &offset, const binary_editor &editor)
        {
            if (offset > size())
            {
                throw binary_exception("binary_editor::insert err : offset must not be greater than m_Size!");
            }

            size_t currentOffset = 0;
            for (auto iter = m_pChunks.begin(); iter != m_pChunks.end(); ++iter)
            {
                if (currentOffset + (*iter)->size() <= offset)
                {
                    currentOffset += (*iter)->size();
                    continue;
                }

                if (currentOffset == offset)
                {
                    // Insert editor's chunks at current position
                    m_pChunks.insert(iter, editor.m_pChunks.begin(), editor.m_pChunks.end());
                }
                else
                {
                    // Split current chunk into two parts
                    auto pBeginChunk = (*iter)->create_sub_chunk(0, offset - currentOffset);
                    auto pEndChunk = (*iter)->create_sub_chunk(offset - currentOffset, (*iter)->size() - (offset - currentOffset));

                    // Replace current chunk and insert editor's chunks
                    iter = m_pChunks.erase(iter);
                    iter = ++m_pChunks.insert(iter, pBeginChunk);
                    iter = ++m_pChunks.insert(iter, editor.m_pChunks.begin(), editor.m_pChunks.end());
                    iter = ++m_pChunks.insert(iter, pEndChunk);
                }
                return; // Exit after insertion
            }

            // If offset is at the end, just append editor's chunks
            m_pChunks.insert(m_pChunks.end(), editor.m_pChunks.begin(), editor.m_pChunks.end());
        }
        /**
         * @brief Clear all chunks.
         */
        void clear()
        {
            m_pChunks.clear();
        }
    };
}

namespace reader
{
    /**
     * @brief Reads a value of type T from a binary_editor at a given offset.
     *
     * This class does not allow copy or move operations.
     *
     * @tparam T The type to read.
     *
     * @code
     * // Example usage:
     * std::vector<uint8_t> blob = {2, 99, 255};
     * binary::binary_editor editor(blob.data(), blob.size());
     * // Read first and second byte using binary_reader
     * reader::binary_reader<uint8_t> value1(editor, 0);
     * reader::binary_reader<uint8_t> value2(editor, value1);
     * uint8_t v1 = value1.get(); // v1 == 2
     * uint8_t v2 = value2.get(); // v2 == 99
     *
     * // Or wrap as a struct:
     * struct simplesample {
     *     binary::binary_editor editor; ///< The binary editor instance.
     *     reader::binary_reader<uint8_t> value1{editor, 0}; ///< Reads the first byte from the editor.
     *     reader::binary_reader<uint8_t> value2{editor, value1}; ///< Reads the next byte from the editor (offset = value1).
     * };
     * @endcode
     */
    template <typename T>
    class binary_reader
    {
    private:
        /**
         * @brief Holds either a direct offset or a reference to another binary_reader for dynamic offset calculation.
         */
        std::variant<size_t, std::reference_wrapper<binary_reader<size_t>>> offset_impl = 0;
        /**
         * @brief Reference to the binary_editor instance.
         */
        binary::binary_editor &editor;

        /**
         * @brief Calculates the offset for reading the value.
         * @return The offset as size_t.
         */
        size_t GetOffset()
        {
            return std::visit(
                [](auto &value) -> size_t
                {
                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>, size_t>)
                    {
                        return value;
                    }
                    else
                    {
                        return value.get();
                    }
                },
                offset_impl);
        }

    public:
        /**
         * @brief Construct a binary_reader with a direct offset.
         * @param editor_ Reference to the binary_editor.
         * @param offset The offset to read from.
         */
        binary_reader(binary::binary_editor &editor_, size_t offset)
            : offset_impl(offset),
              editor(editor_)
        {
        }

        /**
         * @brief Construct a binary_reader with an offset based on another binary_reader.
         * @param editor_ Reference to the binary_editor.
         * @param offset Reference to another binary_reader<size_t> for dynamic offset.
         */
        binary_reader(binary::binary_editor &editor_, binary_reader<size_t> &offset)
            : offset_impl(std::reference_wrapper<binary_reader<size_t>>(offset)),
              editor(editor_)
        {
        }

        /**
         * @brief Get the value from the binary editor at the computed offset.
         * @return Reference to the value of type T.
         */
        const T &get()
        {
            const void *data = editor.get_data();
            return *((T *)((const char *)data + GetOffset()));
        }

        /**
         * @brief Implicit conversion to the value.
         * @return Reference to the value of type T.
         */
        operator const T &()
        {
            const void *data = editor.get_data();
            return *((T *)((const char *)data + GetOffset()));
        }

        /**
         * @brief Deleted copy constructor.
         */
        binary_reader(const binary_reader &) = delete;
        /**
         * @brief Deleted copy assignment operator.
         */
        binary_reader &operator=(const binary_reader &) = delete;
        /**
         * @brief Deleted move constructor.
         */
        binary_reader(binary_reader &&) = delete;
        /**
         * @brief Deleted move assignment operator.
         */
        binary_reader &operator=(binary_reader &&) = delete;
    };
}