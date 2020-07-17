//================================================================================================
// RingSuballocator
//================================================================================================

#pragma once

template<typename _IndexType>
class TRingSuballocator
{
    _IndexType m_Start;
    _IndexType m_End;
    size_t m_Size;
    size_t m_FreeSize;

public:
    TRingSuballocator(size_t Size) :
        m_Start(0),
        m_End(0),
        m_Size(Size),
        m_FreeSize(Size) {}

    size_t FreeSize() const
    {
        return m_FreeSize;
    }

    size_t AllocatedSize() const
    {
        return m_Size - m_FreeSize;
    }

    _IndexType Allocate(size_t Size)
    {
        _IndexType Loc = m_End;
        if(Size > m_FreeSize)
        {
            throw std::bad_alloc();
        }

        m_FreeSize -= Size;
        m_End = _IndexType((m_End + Size) % m_Size);
        return Loc;
    }

    void Free(size_t Size)
    {
        if(Size > AllocatedSize())
        {
            Size = AllocatedSize();
        }
        
        m_FreeSize += Size;
        m_Start = _IndexType((m_Start + Size) % m_Size);
    }
};