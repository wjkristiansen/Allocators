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

    //------------------------------------------------------------------------------------------------
    // Returns true if the index references a degenerate (unused) index
    bool IsDegenerate() const
    {
        return Next == 0 && Prev == 0;
    }

    //------------------------------------------------------------------------------------------------
    // Allocated nodes store the log2 of allocation size in both Prev and Next
    bool IsAllocated() const
    {
        return Next != 0 && Next != _IndexType(-1) && Next == Prev;
    }

    //------------------------------------------------------------------------------------------------
    // Returns the size of the allocation for a given node
    size_t AllocatedSize() const
    {
        size_t Size = 0;
        if (IsAllocated())
        {
            return 1 << (Next - 1);
        }
    }
};

//------------------------------------------------------------------------------------------------
// Collection of indices linked bi-directionally.  List nodes are allocated from an index table
// of type _IndexTableType.  _IndexTableType must be indexable by type _IndexType and return 
// a reference to a IndexNode using operator[] such as an array of IndexNode
// elements or std::vector<IndexNode>.  
// 
// All values in the list must be uniqe, no value can exist in the list more than once.
//
// TIndexList assumes all unallocated nodes are initialized to [Max_IndexType_Value,Max_IndexType_Value].
//
// The Max_IndexType_Value is reserved as a terminal value (i.e. _IndexType(-1)).  This means the full
// range of indexable values is 0 through Max_IndexType_Value - 1.
//
// If the index table is shared with another IIndexList, care must be taken to avoid
// storing any given index in both lists.
//
// Degenerate (unused) indices must have Prev and Next set to 0
//
// Example list:
// _IndexType -> 3-bit unsigned integer (max value is 7)
// 1 <-> 5 <-> 3 <-> 2 <-> 6
//
//          ------------------------------------------------------------------
//    Index |   0   |   1   |   2   |   3   |   4   |   5   |   6   ||   7   |
//          ------------------------------------------------------------------
//     Next |  (0)  |   5   |   6   |   2   |  (0)  |   3   |   7*  ||   -   |
//          ------------------------------------------------------------------
//     Prev |  (0)  |   7*  |   3   |   5   |  (0)  |   1   |   2   ||   -   |
//          ------------------------------------------------------------------
template<class _IndexType, class _IndexTableType>
class TIndexList
{
public:
    static_assert(_IndexType(-1) > _IndexType(0), "_IndexType must be unsigned");

    static const _IndexType _TermValue = _IndexType(-1);

    class Iterator
    {
        const TIndexList* m_pIndexList;
        _IndexType m_Index = 0;

    public:
        Iterator(const TIndexList* pIndexList) :
            m_pIndexList(pIndexList) {};

        Iterator(const TIndexList* pIndexList, _IndexType Index) :
            m_pIndexList(pIndexList),
            m_Index(Index) {}

        Iterator(const Iterator& o) :
            m_pIndexList(o.m_pIndexList),
            m_Index(o.m_Index) {}

        Iterator& operator=(const Iterator& o)
        {
            m_pIndexList = o.m_pIndexList;
            m_Index = o.m_Index;

            return *this;
        }

        bool operator==(const Iterator& o) const
        {
            return (m_pIndexList == o.m_pIndexList) && (m_Index == o.m_Index);
        }

        bool operator!=(const Iterator& o) const
        {
            return !operator==(o);
        }

        _IndexType Index() const { return m_Index; }
        void MoveNext(const _IndexTableType& IndexTable);
        void MovePrev(const _IndexTableType& IndexTable);
    };

private:
    size_t m_Size = 0;
    _IndexType m_FirstIndex = _TermValue;
    _IndexType m_LastIndex = _TermValue;

public:

    // Constructs an empty list using the given IndexableCollection to store nodes.
    TIndexList() = default;

    const size_t Size() const { return m_Size; }

    Iterator PushFront(_IndexType Index, _IndexTableType& IndexTable)
    {
        Iterator It(this, Index);

        if (0 == Size())
        {
            // New node in empty list
            IndexTable[Index].Prev = _TermValue;
            IndexTable[Index].Next = _TermValue;
            m_LastIndex = Index;
        }
        else
        {
            // Insert the node at the start of the list
            _IndexType Prev = _TermValue;
            _IndexType Next = m_FirstIndex;
            IndexTable[Index].Prev = Prev;
            IndexTable[Index].Next = Next;
            IndexTable[Next].Prev = Index;
        }
        m_FirstIndex = Index;
        ++m_Size;

        return It;
    }

    void PopFront(_IndexTableType& IndexTable)
    {
        if (m_Size > 0)
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
        return Iterator(this, _TermValue);
    }

    // Removes the element for the given iterator location.
    // Returns the Iterator for the next location in the list.
    // Returns End() if the list is empty or the given Index was not in the list.
    Iterator Remove(_IndexType Index, _IndexTableType& IndexTable)
    {
        // Initial Result is the End iterator
        Iterator It = End();

        if (--m_Size == 0)
        {
            // List is now empty
            m_FirstIndex = _TermValue;
            m_LastIndex = _TermValue;
        }
        else
        {
            _IndexType Prev = IndexTable[Index].Prev;
            _IndexType Next = IndexTable[Index].Next;
            if (Prev == _TermValue)
            {
                m_FirstIndex = Next;
            }
            else
            {
                IndexTable[Prev].Next = Next;
            }

            if (Next == _TermValue)
            {
                m_LastIndex = Prev;
            }
            else
            {
                IndexTable[Next].Prev = Prev;
            }

            It = Iterator(this, Next);
        }

        // Degenerate the node by indexing self
        IndexTable[Index].Prev = 0;
        IndexTable[Index].Next = 0;

        return It;
    }
};

template<class _IndexType, class _IndexTableType>
void TIndexList<_IndexType, _IndexTableType>::Iterator::MoveNext(const _IndexTableType& IndexTable)
{
    m_Index = IndexTable[m_Index].Next;
}

template<class _IndexType, class _IndexTableType>
void TIndexList<_IndexType, _IndexTableType>::Iterator::MovePrev(const _IndexTableType& IndexTable)
{
    m_Index = IndexTable[m_Index].Prev;
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
// Start is the start of the allocation range.
// Size is the actual size of the allocation.
template<typename _IndexType>
class TBuddyBlock
{
    _IndexType m_Start;
    unsigned char m_Order;

public:
    TBuddyBlock() :
        m_Start(0),
        m_Order(unsigned char(-1)) {}

    TBuddyBlock(_IndexType start, unsigned char order) :
        m_Start(start),
        m_Order(order) {}

    _IndexType Start() const { return m_Start; }
    unsigned char Order() const { return m_Order; }
    size_t Size() const { return m_Order == unsigned char(-1) ? 0 : 1 << m_Order; }

    bool operator==(TBuddyBlock& o) const { return m_Start == o.m_Start && m_Order == o.m_Order; }
    bool operator!=(TBuddyBlock& o) const { return !operator==(o); }
};

//------------------------------------------------------------------------------------------------
struct BuddySuballocatorException
{
    enum class Type
    {
        Unavailable,
        NotAllocated,
    };

    Type T;

    BuddySuballocatorException(Type t) : T(t) {}
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
// BuddyPushFront = Start ^ BlockSize
// ParentStart = min(Start, BuddyStart) = Start & ~(ParentBlockSize - 1)
// 
// For example: Given a space of size 16 - the following graph represents the set of possible allocations:
//
// MaxOrder = 4
//
// Order | Block Start Values
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
//   4   |   15  |   16  |   17  |   18  |   19  |   20  |   21  |   22  |   23  |   24  |   25  |   26  |   27  |   28  |   29  |   30  |
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
// IndexInLevel = Start >> Order
// StateIndex = (1 << Level) + IndexInLevel - 1
// ParentStateIndex = (StateIndex - 1) >> 1

template<class _IndexType, size_t _MaxSize>
class TBuddySuballocator
{
    static_assert(_IndexType(-1) > _IndexType(0), "_IndexType must be an unsigned type");

    // The first NumOrders nodes are terminal nodes
    static const size_t NumSplitStateBits = _MaxSize / 2;
    static const unsigned char MaxOrder = (unsigned char) Log2Ceil(_MaxSize);
    static const size_t SplitStateBitArraySize = _MaxSize / 2;

    IndexNode<_IndexType> m_AllocationTable[_MaxSize]; // Table of all possible allocations
    typename TIndexList<_IndexType, decltype(m_AllocationTable)> m_FreeAllocations[MaxOrder + 1];
    typename TBitArray<_IndexType, NumSplitStateBits> m_SplitStateBitArray;

    // Returns the buddy block
    static TBuddyBlock<_IndexType> BuddyBlock(const TBuddyBlock<_IndexType> &Block)
    {
        return TBuddyBlock<_IndexType>(Block.Start() ^ _IndexType(Block.Size()), Block.Order());
    }

    // Returns the parent block
    static TBuddyBlock<_IndexType> ParentBlock(const TBuddyBlock<_IndexType> &Block)
    {
        TBuddyBlock<_IndexType> ParentBlock;
        auto ParentBlockOrder = Block.Order() + 1;
        _IndexType ParentBlockSize = _IndexType(1 << ParentBlockOrder);

        if (ParentBlockOrder <= MaxOrder)
        {
            _IndexType ParentStart = Block.Start() & ~(ParentBlockSize - 1);
            ParentBlock = TBuddyBlock<_IndexType>(ParentStart, ParentBlockOrder);
        }

        return ParentBlock;
    }

    // Returns the state index of a block
    static _IndexType StateIndex(const TBuddyBlock<_IndexType> &Block)
    {
        _IndexType Level = MaxOrder - Block.Order();
        _IndexType IndexInLevel = Block.Start() >> Block.Order();
        return (1 << Level) + IndexInLevel - 1;
    }

    // Returns true if the the given block is split
    bool IsSplit(const TBuddyBlock<_IndexType> &Block) const
    {
        return m_SplitStateBitArray[StateIndex(Block)];
    }

    // Returns true if the block is Allocated
    // Committed blocks are either split or allocated
    bool IsAllocated(const TBuddyBlock<_IndexType>& Block) const
    {
        auto &Node = m_AllocationTable[Block.Start()];
        return Node.IsAllocated() && Node.Prev == Block.Order() + 1;
    }

    void TrackNodeAsAllocated(const TBuddyBlock<_IndexType>& Block)
    {
        // Encode the node with 1 + allocation order
        auto& Node = m_AllocationTable[Block.Start()];
        Node.Next = Node.Prev = Block.Order() + 1;
    }

    TBuddyBlock<_IndexType> AllocateImpl(_IndexType Order)
    {
        if (Order <= MaxOrder)
        {
            if (m_FreeAllocations[Order].Size())
            {
                auto It = m_FreeAllocations[Order].Begin();
                auto Start = It.Index();
                auto Block = TBuddyBlock<_IndexType>(Start, Order);
                m_FreeAllocations[Order].PopFront(m_AllocationTable);
                if (Order < MaxOrder)
                {
                    auto ParentBlock = TBuddySuballocator::ParentBlock(Block);
                    auto StateIndex = TBuddySuballocator::StateIndex(ParentBlock);
                    m_SplitStateBitArray.Set(StateIndex, false); // Mark the parent as not split
                }

                TrackNodeAsAllocated(Block);

                return Block;
            }
            else
            {
                auto ParentBlock = AllocateImpl(Order + 1);
                auto StateIndex = TBuddySuballocator::StateIndex(ParentBlock);

                if (ParentBlock.Order() != _IndexType(-1))
                {
                    // Split the parent block
                    _IndexType BlockSize = 1 << Order;
                    auto Block = TBuddyBlock<_IndexType>(ParentBlock.Start(), Order);
                    m_FreeAllocations[Order].PushFront(ParentBlock.Start() + BlockSize, m_AllocationTable);
                    m_SplitStateBitArray.Set(StateIndex, true); // Mark the parent as split

                    TrackNodeAsAllocated(Block);

                    return Block;
                }
            }
        }

        throw(BuddySuballocatorException(BuddySuballocatorException::Type::Unavailable));
    }

    void FreeImpl(const TBuddyBlock<_IndexType> &Block)
    {
        if (Block.Order() == MaxOrder)
        {
            // Add the block to the free list
            m_FreeAllocations[Block.Order()].PushFront(Block.Start(), m_AllocationTable);
        }
        else
        {
            auto Parent = ParentBlock(Block);

            if (IsSplit(Parent))
            {
                // Mark parent as not split
                m_SplitStateBitArray.Set(StateIndex(Parent), false);

                // Remove the buddy location from the free list
                auto Buddy = BuddyBlock(Block);
                m_FreeAllocations[Block.Order()].Remove(Buddy.Start(), m_AllocationTable);

                // Free the parent
                FreeImpl(Parent);
            }
            else
            {
                // Add the block to the free list
                m_FreeAllocations[Block.Order()].PushFront(Block.Start(), m_AllocationTable);

                // Mark the parent as split
                m_SplitStateBitArray.Set(StateIndex(Parent), true);
            }
        }
    }

public:
    TBuddySuballocator() :
        m_AllocationTable{ 0 }
    {
        m_FreeAllocations[MaxOrder].PushFront(0, m_AllocationTable);
    }

    TBuddyBlock<_IndexType> Allocate(size_t Size)
    {
        _IndexType Order = (_IndexType) Log2Ceil(Size);
        auto Block = AllocateImpl(Order);
        return Block;
    }

    void Free(const TBuddyBlock<_IndexType> &Block)
    {
        if (!IsAllocated(Block))
        {
            throw(BuddySuballocatorException(BuddySuballocatorException::Type::NotAllocated));
        }


        FreeImpl(Block);
    }

    size_t TotalFree() const
    { 
        size_t TotalFree(0);

        for (auto Order = MaxOrder; Order != (unsigned char)(-1); Order--)
        {
            size_t Size = size_t(1) << Order;

            if (m_FreeAllocations[Order].Size() > 0)
            {
                for (auto It = m_FreeAllocations[Order].Begin(); It != m_FreeAllocations[Order].End(); It.MoveNext(m_AllocationTable))
                {
                    TotalFree += Size;
                }
            }
        }

        return TotalFree;
    }

    size_t MaxAllocationSize() const
    {
        size_t MaxSize = 0;
        for (auto Order = MaxOrder; Order != (unsigned char)(-1); Order--)
        {
            if (m_FreeAllocations[Order].Size() != 0)
            {
                MaxSize = size_t(1) << Order;
                break;
            }
        }

        return MaxSize;
    }

};