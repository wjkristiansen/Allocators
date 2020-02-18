//================================================================================================
// BuddySuballocator 
//================================================================================================

#pragma once

//================================================================================================
// BuddySuballocator
//================================================================================================

#pragma once

//------------------------------------------------------------------------------------------------
// Returns the position of the most significant bit or 0xffffffff if no bits were set
constexpr unsigned long BitScanMSB(unsigned long mask)
{
    unsigned long count = ~0UL;

    if (mask > 0)
    {
        unsigned long temp = mask;
        if (temp & 0xffff0000)
        {
            count += 16;
            temp >>= 16;
        }

        if (temp & 0xff00)
        {
            count += 8;
            temp >>= 8;
        }

        if (temp & 0xf0)
        {
            count += 4;
            temp >>= 4;
        }

        switch (temp)
        {
        case 0:
            return count;
        case 1:
            return count + 1;
        case 2:
        case 3:
            return count + 2;
        case 4:
        case 5:
        case 6:
        case 7:
            return count + 3;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            return count + 4;
        }
    }
    return count;
}

//------------------------------------------------------------------------------------------------
// Returns the position of the most significant bit or 0xffffffff if no bits were set
constexpr unsigned long  BitScanMSB64(unsigned long long mask)
{
    return BitScanMSB((mask & 0xffffffff00000000) ? unsigned long(mask >> 32) : unsigned long(mask));
}

//------------------------------------------------------------------------------------------------
constexpr unsigned long Log2Ceil(unsigned long value)
{
    return value > 0 ? 1 + BitScanMSB(value - 1) : ~0UL;
}

//------------------------------------------------------------------------------------------------
constexpr unsigned long Log2Ceil(unsigned long long value)
{
    return value > 0 ? 1 + BitScanMSB64(value - 1) : ~0UL;
}

//------------------------------------------------------------------------------------------------
// Node data type.
template<class _IndexType>
struct IndexTableNode
{
    _IndexType Next = 0;
    _IndexType Prev = 0;
};

//------------------------------------------------------------------------------------------------
// Collection of indices linked bi-directionally.  List nodes are allocated from an index table
// of type _IndexTableType.  _IndexTableType must be indexable by type _IndexType and return 
// a reference to a IndexTableNode using operator[] such as an array of IndexTableNode
// elements or std::vector<IndexTableNode>.  
// 
// All values in the list must be uniqe, no value can exist in the list more than once.
//
// TIndexList assumes all unallocated nodes are initialized to [0,0].
//
// There is no reserved terminal value.  This is essentially a ring linked list, meaning
// that Next(Last()) == First().
//
// If the index table is shared with another IIndexList, care must be taken to avoid
// storing any given index in both lists.
//
// Example list:
// 1 <-> 5 <-> 3 <-> 2 <-> 7
//
//          -----------------------------------------------------------------
//    Index |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |
//          -----------------------------------------------------------------
//     Next |       |   5   |   7   |   2   |       |   3   |       |   1   |
//          -----------------------------------------------------------------
//     Prev |       |   7   |   3   |   5   |       |   1   |       |   2   |
//          -----------------------------------------------------------------
template<class _IndexType, class _IndexTableType>
class TIndexList
{
public:

    class Iterator
    {
        const TIndexList *m_pIndexList;
        _IndexType m_Index = 0;
        bool m_IsEnd = true;

    public:
        Iterator(const TIndexList *pIndexList) : 
            m_pIndexList(pIndexList) {};
        
        Iterator(const TIndexList *pIndexList, _IndexType Index) :
            m_pIndexList(pIndexList),
            m_Index(Index),
            m_IsEnd(false) {}

        Iterator(const Iterator& o) : 
            m_pIndexList(o.m_pIndexList), 
            m_Index(o.m_Index), 
            m_IsEnd(o.m_IsEnd) {}

        Iterator& operator=(const Iterator& o) 
        {
            m_pIndexList = o.m_pIndexList;
            m_Index = o.m_Index; 
            m_IsEnd = o.m_IsEnd; 

            return *this;
        }

        bool operator==(const Iterator& o) const
        {
            return 
                (m_pIndexList == o.m_pIndexList) && 
                ((m_IsEnd && o.m_IsEnd) || (!m_IsEnd && !o.m_IsEnd && (m_Index == o.m_Index)));
        }

        bool operator!=(const Iterator& o) const
        {
            return !operator==(o);
        }

        _IndexType Index() const { return m_Index; }
        void MoveNext();
        void MovePrev();
    };

private:
    _IndexType m_Size = _IndexType(0);
    _IndexType m_FirstIndex = 0;
    _IndexType m_LastIndex = 0;
    _IndexTableType& m_IndexTable;

public:

    // Constructs an empty list using the given IndexableCollection to store nodes.
    TIndexList(_IndexTableType& IndexTable) :
        m_IndexTable(IndexTable)
    {
    };

    const _IndexType &Size() const { return m_Size; }

    Iterator PushFront(_IndexType Index)
    {
        Iterator It(this, Index);

        if (0 == Size())
        {
            // New node in empty list
            m_IndexTable[Index].Prev = Index;
            m_IndexTable[Index].Next = Index;
            m_LastIndex = Index;
        }
        else
        {
            // Insert the node at the start of the list
            _IndexType Prev = m_LastIndex;
            _IndexType Next = m_FirstIndex;
            m_IndexTable[Index].Prev = Prev;
            m_IndexTable[Index].Next = Next;
            m_IndexTable[Next].Prev = Index;
            m_IndexTable[Prev].Next = Index;
        }
        m_FirstIndex = Index;
        ++m_Size;

        return It;
    }

    // Returns the Iterator for the first element in the list.
    Iterator Begin() const
    {
        return Iterator(this, m_FirstIndex);
    }

    // Returns the end iterator.
    Iterator End() const
    {
        return Iterator(this);
    }

    // Removes the element for the given iterator location.
    // Returns the Iterator for the next location in the list.
    // Returns End() if the list is empty or the given Index was not in the list.
    Iterator Remove(_IndexType Index)
    {
        // Initial Result is the End iterator
        Iterator It = End(); 

        if (--m_Size == 0)
        {
            // List is now empty
            m_FirstIndex = 0;
            m_LastIndex = 0;
        }
        else
        {
            _IndexType Prev = m_IndexTable[Index].Prev;
            _IndexType Next = m_IndexTable[Index].Next;
            m_IndexTable[Next].Prev = Prev;
            m_IndexTable[Prev].Next = Next;

            if (m_FirstIndex == Index)
            {
                m_FirstIndex = Next;
            }

            if (m_LastIndex == Index)
            {
                m_LastIndex = Prev;
            }
            else
            {
                It = Iterator(this, Next);
            }
        }

        m_IndexTable[Index].Prev = 0;
        m_IndexTable[Index].Next = 0;

        return It;
    }
};

template<class _IndexType, class _IndexTableType>
void TIndexList<_IndexType, _IndexTableType>::Iterator::MoveNext()
{
    m_Index = m_pIndexList->m_IndexTable[m_Index].Next;
    m_IsEnd = m_Index == m_pIndexList->m_FirstIndex;
}

template<class _IndexType, class _IndexTableType>
void TIndexList<_IndexType, _IndexTableType>::Iterator::MovePrev()
{
    m_Index = m_pIndexList->m_IndexTable[m_Index].Prev;
    m_IsEnd = m_Index == m_pIndexList->m_LastIndex;
}


//------------------------------------------------------------------------------------------------
// Describes an array of bits
template<class _SizeType, _SizeType _Size>
class TBitArray
{
    static _SizeType NumBytes = _Size / 8;
    char Bytes[NumBytes] = {};

public:
    bool Get(_SizeType Index) const
    {
        _SizeType ByteIndex = Index / 8;
        _SizeType Mask = 1 << (Index & 0xff);
        return (Bytes[ByteIndex] & Mask) == Mask;
    }

    void Set(_SizeType Index, bool Value)
    {
        _SizeType ByteIndex = Index / 8;
        _SizeType Mask = 1 << (Index & 0xff);
        if (Value)
        {
            Bytes[ByteIndex] &= ~Mask; // Clear bit
        }
        else
        {
            Bytes[ByteIndex] |= Mask; // Set bit
        }
    }

    bool operator[](_SizeType Index) const
    {
        return Get(Index);
    }
};

//------------------------------------------------------------------------------------------------
// Represents a logical sub-allocation.
// Offset is the start of the allocation range.
// Size is the actual size of the allocation.
template<typename _SizeType>
struct TBuddyBlock
{
    _SizeType Offset;
    _SizeType Size;
};

//------------------------------------------------------------------------------------------------
// TBuddySuballocator class
// 
// Manages allocation of logical ranges of integer values using an algorithm similar to that
// described at https://en.wikipedia.org/wiki/Buddy_memory_allocation, N being the size
// of the allocatable space.
//
// Allocated ranges are represented as allocation-blocks with an offset to the start of the allocation
// and the size of the allocated range.  Allocation-block sizes are the smallest power of two greater-than
// or equal-to the requested allocation size.
//
// Buddy allocation is often used in operating systems to manage system memory allocations of various sizes.  
// Free blocks of a given size are itrusively linked so that available blocks can be quickly allocated, split, 
// or merged.  Since the buddy suballocator is used only for logical rather than physical allocations, such intrusive
// links are not availale.  Therefore, memory must be set aside to contain these links.  The buddy suballocator 
// refers to this memory as an IndexTable.  The index table is an indexable collections (e.g. an array)
// of N/2 IndexTableNode elements.  This collection is a proxy for the intrusive links that buddy allocators
// typically use for system memory allocation.
// 
// _Size is the full size of the allocatable range.  _Size must be a power of two.
//
// _SizeType is the type of integers representing the allocation space.
// 
// For example: Given a space of size 16 - the following graph represents the set of possible allocations:
//
// Order | Block Offsets
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   0   |  0000 |  0001 |  0010 |  0011 |  0100 |  0101 |  0110 |  0111 |  1000 |  1001 |  0010 |  1011 |  1100 |  1101 |  1110 |  1111 |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   1   |      0000     |      0010     |      0100     |      0110     |      1000     |      1010     |      1100     |      1110     |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   2   |              0000             |              0100             |              1000             |              1100             |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   3   |                              0000                             |                              1000                             |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   4   |                                                              0000                                                             |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//
//
// Using this sample, the set of non-splittable allocation blocks can be identified using a unique id as follows:
//
// Order = Log2Ceil(Size) = 4
// BlockSize = 2 ^ Order (i.e. 1 << Order)
// BuddyOffset = Offset ^ BlockSize
//
// Level | StateIndex
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   4   |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   3   |       7       |       8       |       9       |       10      |       11      |       12      |       13      |       14      |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   2   |               3               |               4               |               5               |               6               |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   1   |                               1                               |                               2                               |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
//   0   |                                                               0                                                               |
//       |-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|
// 
// Level = MaxOrder - Order
// IndexInLevel = Offset >> Order
// StateIndex = (1 << Level) + IndexInLevel - 1
// ParentStateIndex = (StateIndex - 1) >> 1

template<class _SizeType, _SizeType _Size, class _IndexTableType>
class TBuddySuballocator
{
    // The first NumOrders nodes are terminal nodes
    static const _SizeType NumNodes = _Size / 2;
    static const _SizeType MaxOrder = Log2Ceil(_Size);
    static const _SizeType StateBitArraySize = _Size / 2;
    typename TIndexList<_SizeType, _IndexTableType>::IndexNode m_BlockNodes[NumNodes];
    typename TIndexList<_SizeType, _IndexTableType> m_FreeBlocks[MaxOrder];
    typename TBitArray<_SizeType, NumNodes> m_StateBitArray;

    // Returns the location of the buddy block
    static _SizeType BuddyOffset(_SizeType Offset, _SizeType Order)
    {
        _SizeType Size = 1 << Order;
        return Offset ^ Size;
    }

    // Converts a Offset/Order pair to a StateIndex
    static _SizeType OffsetToStateIndex(_SizeType Offset, _SizeType Order)
    {
        _SizeType Level = MaxOrder - Order;
        _SizeType IndexInLevel = Offset >> Order;
        return (1 << Level) + IndexInLevel - 1;
    }

    // Gets the node index of the parent to node at StateIndex
    static _SizeType ParentStateIndex(_SizeType StateIndex)
    {
        return (StateIndex - 1) >> 1;
    }

    bool IsParentSplit(_SizeType Offset, _SizeType Order)
    {
        _SizeType StateIndex = ParentStateIndex(OffsetToStateIndex(Offset, Order));
        return m_StateBitArray[StateIndex];
    }

    TBuddyBlock<_SizeType> AllocateImpl(_SizeType Size)
    {
        auto Order = Log2Ceil(Size);

        if (m_FreeBlocks[Order].GetSize())
        {
            _SizeType Offset = m_FreeBlocks[Order].Get;
        }
    }

public:
    TBuddySuballocator() = default;

    TBuddyBlock<_SizeType> Allocate(_SizeType Size)
    {
        if (Size > _Size /*|| Order == -1UL*/)
        {
            return TBuddyBlock<_SizeType> { 0, 0 }; // Failed allocation
        }

        return AllocateImpl(Size);
    }

    void FreeUnits(const TBuddyBlock<_SizeType> &Allocation)
    {

    }
};