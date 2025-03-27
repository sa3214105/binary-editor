#pragma once
#include <memory>
#include <string>
#include <deque>
#include <iterator>
namespace binary
{
    class binary_exception : public std::exception
    {
    protected:
        std::string m_error_msg;

    public:
        binary_exception(const std::string &errorMsg)
            : m_error_msg(errorMsg)
        {
        }
        binary_exception(std::string &&errorMsg)
            : m_error_msg(std::move(errorMsg))
        {
        }
        virtual const char *what() const noexcept override
        {
            return m_error_msg.c_str();
        }
    };

    enum class CHUNK_TYPE
    {
        MEMORY
    };

    class binary_chunk_interface
    {
    public:
        virtual std::shared_ptr<binary_chunk_interface> create_sub_chunk(const size_t &offset, const size_t &size) const = 0;
        virtual size_t size() const = 0;
        virtual const uint8_t *get_data() const = 0;
        virtual CHUNK_TYPE get_type() const = 0;
        virtual std::unique_ptr<binary_chunk_interface> clone() const = 0;
        virtual void downscale_size(const size_t &targeSize) = 0;
    };

    class binary_chunk_memory : public binary_chunk_interface
    {
    private:
        std::shared_ptr<const std::unique_ptr<const uint8_t[]>> m_ppBlob = nullptr;
        size_t m_size = 0;
        size_t m_offset = 0;
    public:
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
        virtual std::shared_ptr<binary_chunk_interface> create_sub_chunk(const size_t &offset, const size_t &size) const override final
        {
            if (offset + size > m_size)
            {
                throw binary_exception("binary_chunk_memory::create_sub_chunk err : (offset + size) must not be greater than m_Size!");
            }
            auto pRet = std::make_shared<binary_chunk_memory>(*this);
            pRet->m_offset = offset;
            pRet->m_size = size;
            return pRet;
        }
        virtual size_t size() const override final
        {
            return m_size;
        }
        virtual const uint8_t *get_data() const override final
        {
            return m_ppBlob->get() + m_offset;
        }
        virtual CHUNK_TYPE get_type() const override final
        {
            return CHUNK_TYPE::MEMORY;
        }
        virtual std::unique_ptr<binary_chunk_interface> clone() const override
        {
            return std::make_unique<binary_chunk_memory>(*this);
        }
        virtual void downscale_size(const size_t &targeSize) override final
        {
            m_size = targeSize;
        }
    };

    class binary_chunk_factory
    {
    public:
        enum class CREATE_STRATEGY
        {
            AUTO,
            MEMORY
        };
    private:
        CREATE_STRATEGY m_create_strategy = CREATE_STRATEGY::AUTO;
    public:
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

    class binary_editor
    {
    private:
        mutable std::deque<std::shared_ptr<binary_chunk_interface>> m_pChunks;
        binary_chunk_factory m_binary_chunk_factory;
        bool m_auto_tidy = false;
        size_t m_auto_tidy_size = 0;
    public:
        binary_editor() = default;
        binary_editor(std::unique_ptr<const uint8_t[]> &&pBlob, const size_t &size)
        {
            m_pChunks.push_back(m_binary_chunk_factory.create_chunk(std::move(pBlob), size));
        }
        
        size_t size() const
        {
            size_t ret = 0;
            for (auto &pChunk : m_pChunks)
            {
                ret += pChunk->size();
            }
            return ret;
        }
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
        const void *get_data() const
        {
            tidy_chunks();
            return m_pChunks.front()->get_data();
        }
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

        void push_back(const binary_editor &backEditor)
        {
            std::copy(backEditor.m_pChunks.begin(), backEditor.m_pChunks.end(), std::back_inserter(m_pChunks));
        }
        template<typename... Args>
        void emplace_back(Args &&... args)
        {
            m_pChunks.push_back(binary_editor(std::forward<Args>(args)...));
        }
        void push_front(const binary_editor &frontEditor)
        {
            std::copy(frontEditor.m_pChunks.rbegin(), frontEditor.m_pChunks.rend(), std::front_inserter(m_pChunks));
        }
        template<typename... Args>
        void emplace_front(Args &&... args)
        {
            m_pChunks.push_front(binary_editor(std::forward<Args>(args)...));
        }
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
                    // 在當前位置插入 editor 的 chunks
                    m_pChunks.insert(iter, editor.m_pChunks.begin(), editor.m_pChunks.end());
                }
                else
                {
                    // 將當前 chunk 分割為兩部分
                    auto pBeginChunk = (*iter)->create_sub_chunk(0, offset - currentOffset);
                    auto pEndChunk = (*iter)->create_sub_chunk(offset - currentOffset, (*iter)->size() - (offset - currentOffset));

                    // 替換當前 chunk，並插入 editor 的 chunks
                    iter = m_pChunks.erase(iter);
                    iter = ++m_pChunks.insert(iter, pBeginChunk);
                    iter = ++m_pChunks.insert(iter, editor.m_pChunks.begin(), editor.m_pChunks.end());
                    iter = ++m_pChunks.insert(iter, pEndChunk);
                }
                return; // 插入完成後退出
            }

            // 如果 offset 恰好在末尾，直接追加 editor 的 chunks
            m_pChunks.insert(m_pChunks.end(), editor.m_pChunks.begin(), editor.m_pChunks.end());
        }
        void clear()
        {
            m_pChunks.clear();
        }
    };
}
