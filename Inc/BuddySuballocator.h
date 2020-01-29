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
// Sub-allocates ranges of out of a set of all integers between 0 and N - 1 using a buddy-allocation
// algorithm similar to that described at https://en.wikipedia.org/wiki/Buddy_memory_allocation, N being the size
// of the allocatable space.
//
// Allocated ranges are represented as allocation-blocks with an offset to the start of the allocation
// and the size of the allocated range.  Allocation-block sizes are the smallest power of two greater-than
// or equal-to the requested allocation size.
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

template<class _SizeType, _SizeType _Size>
class TBuddySuballocator
{
    // Maximum fragmentation can pathologically occur if the full set
    // of order 0 (size 1) ranges are individually allocated followed by freeing every other allocation.
    // The resulting linked list of free order 0 entries is _Size / 2.
    // Note that for very large ranges, the node pool can still occupy a large
    // amount of space.
    //
    // The first NumOrders nodes are terminal nodes
    static const _SizeType NumNodes = _Size / 2;
    static const _SizeType MaxOrder = Log2Ceil(_Size);
    static const _SizeType StateBitArraySize = _Size / 2;
    typename TSharedIndexNodesList<_SizeType>::IndexNode m_BlockNodes[NumNodes];
    typename TSharedIndexNodesList<_SizeType> m_FreeBlocks[MaxOrder];
    typename TBitArray<_SizeType, NumNodes> m_StateBitArray;

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

    TAllocationBlock<_SizeType> AllocateImpl(_SizeType Size)
    {
        auto Order = Log2Ceil(Size);

        if (m_FreeBlocks[Order].GetSize())
        {
            _SizeType Offset = m_FreeBlocks[Order].Get;
        }
    }

public:
    TBuddySuballocator() = default;

    TAllocationBlock<_SizeType> Allocate(_SizeType Size)
    {
        if (Size > _Size || Order == -1UL)
        {
            return TAllocationBlock<_SizeType> { 0, 0 }; // Failed allocation
        }

        return AllocateImpl(Size);
    }

    void FreeUnits(const TAllocationBlock<_SizeType> &Allocation)
    {

    }
};