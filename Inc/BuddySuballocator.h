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
constexpr unsigned long BitScanMSB64(unsigned long long mask)
{
    return BitScanMSB((mask & 0xffffffff00000000) ? unsigned long(mask >> 32) : unsigned long(mask));
}

//------------------------------------------------------------------------------------------------
constexpr unsigned long Log2Ceil(unsigned int value)
{
    return (unsigned long)(value > 0 ? 1 + BitScanMSB((long) value - 1) : ~0UL);
}

//------------------------------------------------------------------------------------------------
constexpr unsigned long Log2Ceil(unsigned __int64 value)
{
    return (unsigned long)(value > 0 ? 1 + BitScanMSB64((unsigned long long) value - 1) : ~0UL);
}

//------------------------------------------------------------------------------------------------
// Node data type.
template<class _IndexType>
struct IndexNode
{
    _IndexType Next = 0;
    _IndexType Prev = 0;
};

//------------------------------------------------------------------------------------------------
// Collection of indices linked bi-directionally.  List nodes are allocated from an index table
// of type _IndexTableType.  _IndexTableType must be indexable by type _IndexType and return 
// a reference to a IndexNode using operator[] such as an array of IndexNode
// elements or std::vector<IndexNode>.  
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
        void MoveNext(_IndexTableType &IndexTable);
        void MovePrev(_IndexTableType &IndexTable);
    };

private:
    size_t m_Size = 0;
    _IndexType m_FirstIndex = 0;
    _IndexType m_LastIndex = 0;

public:

    // Constructs an empty list using the given IndexableCollection to store nodes.
    TIndexList() = default;

    const size_t Size() const { return m_Size; }

    Iterator PushFront(_IndexType Index, _IndexTableType &IndexTable)
    {
        Iterator It(this, Index);

        if (0 == Size())
        {
            // New node in empty list
            IndexTable[Index].Prev = Index;
            IndexTable[Index].Next = Index;
            m_LastIndex = Index;
        }
        else
        {
            // Insert the node at the start of the list
            _IndexType Prev = m_LastIndex;
            _IndexType Next = m_FirstIndex;
            IndexTable[Index].Prev = Prev;
            IndexTable[Index].Next = Next;
            IndexTable[Next].Prev = Index;
            IndexTable[Prev].Next = Index;
        }
        m_FirstIndex = Index;
        ++m_Size;

        return It;
    }

    void PopFront(_IndexTableType &IndexTable)
    {
        if (Size() > 0)
        {
            Remove(m_FirstIndex, IndexTable);
        }
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
    Iterator Remove(_IndexType Index, _IndexTableType &IndexTable)
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
            _IndexType Prev = IndexTable[Index].Prev;
            _IndexType Next = IndexTable[Index].Next;
            IndexTable[Next].Prev = Prev;
            IndexTable[Prev].Next = Next;

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

        IndexTable[Index].Prev = 0;
        IndexTable[Index].Next = 0;

        return It;
    }
};

template<class _IndexType, class _IndexTableType>
void TIndexList<_IndexType, _IndexTableType>::Iterator::MoveNext(_IndexTableType &IndexTable)
{
    m_Index = IndexTable[m_Index].Next;
    m_IsEnd = m_Index == m_pIndexList->m_FirstIndex;
}

template<class _IndexType, class _IndexTableType>
void TIndexList<_IndexType, _IndexTableType>::Iterator::MovePrev(_IndexTableType &IndexTable)
{
    m_Index = IndexTable[m_Index].Prev;
    m_IsEnd = m_Index == m_pIndexList->m_LastIndex;
}


//------------------------------------------------------------------------------------------------
// Describes an array of bits
template<class _IndexType, size_t _Size>
class TBitArray
{
    static const size_t NumBytes = _Size < 8 ? 1 : _Size / 8;
    char Bytes[NumBytes] = {0};

public:
    bool Get(_IndexType Index) const
    {
        _IndexType ByteIndex = Index / 8;
        _IndexType Mask = 1 << (Index & 0xff);
        return (Bytes[ByteIndex] & Mask) == Mask;
    }

    void Set(_IndexType Index, bool Value)
    {
        _IndexType ByteIndex = Index / 8;
        _IndexType Mask = 1 << (Index & 0xff);
        if (Value)
        {
            Bytes[ByteIndex] |= Mask; // Set bit
        }
        else
        {
            Bytes[ByteIndex] &= ~Mask; // Clear bit
        }
    }

    bool operator[](_IndexType Index) const
    {
        return Get(Index);
    }
};

//------------------------------------------------------------------------------------------------
// Represents a logical sub-allocation.
// Location is the start of the allocation range.
// Size is the actual size of the allocation.
template<typename _IndexType>
struct TBuddyBlock
{
    TBuddyBlock() :
        Location(0),
        Order(_IndexType(-1)) {}

    TBuddyBlock(_IndexType location, _IndexType order) :
        Location(location),
        Order(order) {}

    _IndexType Location;
    _IndexType Order;

    size_t Size() const { return Order == _IndexType(-1) ? 0 : 1 << Order; }
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
// of N/2 IndexNode elements.  This collection is a proxy for the intrusive links that buddy allocators
// typically use for system memory allocation.
// 
// _MaxSize is the full size of the allocatable range.  _MaxSize must be a power of two.
//
// _IndexType is the type of integers representing the allocation space.
//
// Order = Log2(BlockSize)
// BlockSize = 2 ^ Order (i.e. 1 << Order)
// BuddyLocation = Location ^ BlockSize
// ParentLocation = min(Location, BuddyLocation) = Location & ~(ParentBlockSize - 1)
// 
// For example: Given a space of size 16 - the following graph represents the set of possible allocations:
//
// Order = 4
//
// Order | Block Locations
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
// Using this sample, the full set of splittable allocation blocks can be identified using a unique id (allocation index) as follows:
//
// Level | BlockId
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
// IndexInLevel = Location >> Order
// StateIndex = (1 << Level) + IndexInLevel - 1
// ParentStateIndex = (StateIndex - 1) >> 1

template<class _IndexType, size_t _MaxSize>
class TBuddySuballocator
{
    // The first NumOrders nodes are terminal nodes
    static const size_t NumNodes = _MaxSize / 2;
    static const _IndexType MaxOrder = (_IndexType) Log2Ceil(_MaxSize);
    static const size_t StateBitArraySize = _MaxSize / 2;

    IndexNode<_IndexType> m_AllocationTable[_MaxSize]; // Table of all possible allocations
    typename TIndexList<_IndexType, decltype(m_AllocationTable)> m_FreeAllocations[MaxOrder + 1];
    typename TBitArray<_IndexType, NumNodes> m_StateBitArray;

    // Returns the buddy block
    static TBuddyBlock<_IndexType> BuddyBlock(const TBuddyBlock<_IndexType> &Block)
    {
        return TBuddyBlock<_IndexType>(Block.Location ^ _IndexType(Block.Size()), Block.Order);
    }

    // Returns the parent block
    static TBuddyBlock<_IndexType> ParentBlock(const TBuddyBlock<_IndexType> &Block)
    {
        TBuddyBlock<_IndexType> ParentBlock;
        auto ParentBlockOrder = Block.Order + 1;
        _IndexType ParentBlockSize = _IndexType(1 << ParentBlockOrder);

        if (ParentBlockOrder <= MaxOrder)
        {
            _IndexType Location = Block.Location & ~(ParentBlockSize - 1);
            ParentBlock = TBuddyBlock<_IndexType>(Location, ParentBlockOrder);
        }

        return ParentBlock;
    }

    // Returns the state index of a block
    static _IndexType StateIndex(const TBuddyBlock<_IndexType> &Block)
    {
        _IndexType Level = MaxOrder - Block.Order;
        _IndexType IndexInLevel = Block.Location >> Block.Order;
        return (1 << Level) + IndexInLevel - 1;
    }

    // Returns true if the parent node of the given block is split
    bool IsSplit(TBuddyBlock<_IndexType> &Block) const
    {
        return m_StateBitArray[StateIndex(Block)];
    }

    TBuddyBlock<_IndexType> AllocateImpl(_IndexType Order)
    {
        TBuddyBlock<_IndexType> Block;

        if (Order <= MaxOrder)
        {
            if (m_FreeAllocations[Order].Size())
            {
                auto It = m_FreeAllocations[Order].Begin();
                auto Location = It.Index();
                Block = TBuddyBlock<_IndexType>(Location, Order);
                m_FreeAllocations[Order].PopFront(m_AllocationTable);
                if (Order < MaxOrder)
                {
                    auto ParentBlock = TBuddySuballocator::ParentBlock(Block);
                    auto StateIndex = TBuddySuballocator::StateIndex(ParentBlock);
                    m_StateBitArray.Set(StateIndex, false); // Mark the parent as not split
                }
            }
            else
            {
                auto ParentBlock = AllocateImpl(Order + 1);
                auto StateIndex = TBuddySuballocator::StateIndex(ParentBlock);

                if (ParentBlock.Order != _IndexType(-1))
                {
                    // Split the parent block
                    _IndexType BlockSize = 1 << Order;
                    Block = TBuddyBlock<_IndexType>(ParentBlock.Location, Order);
                    m_FreeAllocations[Order].PushFront(ParentBlock.Location + BlockSize, m_AllocationTable);
                    m_StateBitArray.Set(StateIndex, true); // Mark the parent as split
                }
            }
        }

        return Block;
    }

    void FreeImpl(const TBuddyBlock<_IndexType> &Block)
    {
        if (Block.Order == MaxOrder)
        {
            // Add the block to the free list
            m_FreeAllocations[Block.Order].PushFront(Block.Location, m_AllocationTable);
        }
        else
        {
            auto Parent = ParentBlock(Block);

            if (IsSplit(Parent))
            {
                // Mark parent as not split
                m_StateBitArray.Set(StateIndex(Parent), false);

                // Remove the buddy location from the free list
                auto Buddy = BuddyBlock(Block);
                m_FreeAllocations[Block.Order].Remove(Buddy.Location, m_AllocationTable);

                // Free the parent
                FreeImpl(Parent);
            }
            else
            {
                // Add the block to the free list
                m_FreeAllocations[Block.Order].PushFront(Block.Location, m_AllocationTable);

                // Mark the parent as split
                m_StateBitArray.Set(StateIndex(Parent), true);
            }
        }
    }

public:
    TBuddySuballocator()
    {
        m_FreeAllocations[MaxOrder].PushFront(0, m_AllocationTable);
    }

    TBuddyBlock<_IndexType> Allocate(size_t Size)
    {
        _IndexType Order = (_IndexType) Log2Ceil(Size);
        return AllocateImpl(Order);
    }

    void Free(const TBuddyBlock<_IndexType> &Block)
    {
        FreeImpl(Block);
    }
};