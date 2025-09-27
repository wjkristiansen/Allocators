#include <gtest/gtest.h>
#include "BuddySuballocator.h"
#include "RingSuballocator.h"

using std::cout;

namespace AllocatorsTest
{
	class BuddySuballocatorTestClass : public ::testing::Test
	{
	protected:
		void SetUp() override {}
		void TearDown() override {}
	};

	TEST_F(BuddySuballocatorTestClass, IndexListTest)
	{
		using IndexType = unsigned char;
		using IndexNodeType = IndexNode<IndexType>;

		std::vector<IndexNodeType> IndexTable(16);
		std::fill(IndexTable.begin(), IndexTable.end(), IndexNode<IndexType>()); // zero init
		using IndexTableType = decltype(IndexTable);
		using ListType = TIndexList<IndexType, IndexTableType>;
	
		ListType IndexList;

		EXPECT_TRUE(IndexList.Size() == 0);

		const IndexType TestIndices[]{ 14, 1, 0, 6, 3, 8, 5 };

		// Build list from the indices in TestIndices
		IndexType First = TestIndices[0];
		IndexType Last = TestIndices[0];
		IndexType i;
		IndexType NodeCount = IndexType(sizeof(TestIndices) / sizeof(TestIndices[0]));
		for (i = 0; i < NodeCount; ++i)
		{
			IndexType Index = TestIndices[i];

			auto It = IndexList.PushFront(Index, IndexTable);
			EXPECT_TRUE(IndexList.Size() == 1 + size_t(i));
			EXPECT_TRUE(It.Index() == Index);
			EXPECT_TRUE(IndexList.Begin() == It);
			It.MoveNext(IndexTable);
			if (i == 0)
			{
				EXPECT_TRUE(It == IndexList.End());
			}
			else
			{
				EXPECT_TRUE(It.Index() == First);
			}
			First = Index;
		}

		// Iterate through the list and make sure the values match the reverse order added
		for (auto It = IndexList.Begin(); It != IndexList.End(); It.MoveNext(IndexTable))
		{
			--i;
			EXPECT_TRUE(TestIndices[i] == It.Index());
		}

		// Remove a node from the middle...
		{
			auto It = IndexList.Remove(6, IndexTable);

			EXPECT_TRUE(0 == It.Index());
			It.MovePrev(IndexTable);
			EXPECT_TRUE(3 == It.Index());

			NodeCount--;
			EXPECT_TRUE(NodeCount == IndexList.Size());
		}

		// Remove the last node in the list...
		{
			auto It = IndexList.Remove(14, IndexTable);
			EXPECT_TRUE(It == IndexList.End());
			NodeCount--;
			EXPECT_TRUE(NodeCount == IndexList.Size());
		}

		// Remove the first node in the list...
		{
			auto It = IndexList.Remove(5, IndexTable);

			EXPECT_TRUE(8 == It.Index());
			It.MovePrev(IndexTable);
			EXPECT_TRUE(It == IndexList.Begin());

			NodeCount--;
			EXPECT_TRUE(NodeCount == IndexList.Size());
		}

		// Remove down to a single node...
		while (NodeCount > 1)
		{
			IndexList.Remove(IndexList.Begin().Index(), IndexTable);
			NodeCount--;
		}

		{
			auto It = IndexList.Begin();
			EXPECT_TRUE(1 == It.Index());
			It.MovePrev(IndexTable);
			EXPECT_TRUE(IndexList.End() == It);
			It = IndexList.Begin();
			It.MoveNext(IndexTable);
			EXPECT_TRUE(IndexList.End() == It);
		}

		// Remove the final node
		IndexList.Remove(IndexList.Begin().Index(), IndexTable);

		EXPECT_TRUE(0 == IndexList.Size());

		// Verify all removed nodes in the IndexTable are [index, index]
		for (auto i = 0; i < sizeof(TestIndices) / sizeof(TestIndices[0]); ++i)
		{
			auto index = TestIndices[i];
			EXPECT_EQ(IndexType(index), IndexTable[index].Next);
			EXPECT_EQ(IndexType(index), IndexTable[index].Prev);
		}
	}

	TEST_F(BuddySuballocatorTestClass, BasicSuballocatorTest)
	{
		using IndexType = unsigned char;
		constexpr size_t MaxAllocations = 32;
		TBuddySuballocator<IndexType> TestSuballocator(MaxAllocations);

		// Some basic tests
		auto Block1 = TestSuballocator.Allocate(6);
		EXPECT_EQ(0, Block1.Start());
		EXPECT_EQ(8, Block1.Size());
		EXPECT_EQ(16, TestSuballocator.MaxAllocationSize());
		EXPECT_EQ(24, TestSuballocator.TotalFree());

		auto Block2 = TestSuballocator.Allocate(16);
		EXPECT_EQ(16, Block2.Start());
		EXPECT_EQ(16, Block2.Size());
		EXPECT_EQ(8, TestSuballocator.MaxAllocationSize());
		EXPECT_EQ(8, TestSuballocator.TotalFree());

		auto Block3 = TestSuballocator.Allocate(8);
		EXPECT_EQ(8, Block3.Start());
		EXPECT_EQ(8, Block3.Size());
		EXPECT_EQ(0, TestSuballocator.MaxAllocationSize());
		EXPECT_EQ(0, TestSuballocator.TotalFree());

		// Should now be fully allocated
		bool ExceptionHit = false;
		try
		{
			auto FailBlock = TestSuballocator.Allocate(1);
		}
		catch (BuddySuballocatorException &)
		{
			ExceptionHit = true;
		}
		EXPECT_TRUE(ExceptionHit);

		// Free up the two adjacent 8-byte blocks
		TestSuballocator.Free(Block1);
		EXPECT_EQ(8, TestSuballocator.MaxAllocationSize());
		EXPECT_EQ(8, TestSuballocator.TotalFree());
		TestSuballocator.Free(Block3);
		EXPECT_EQ(16, TestSuballocator.MaxAllocationSize());
		EXPECT_EQ(16, TestSuballocator.TotalFree());

		// Should be 16 bytes available
		auto Block4 = TestSuballocator.Allocate(16);
		EXPECT_EQ(0, Block4.Start());
		EXPECT_EQ(16, Block4.Size());
		EXPECT_EQ(0, TestSuballocator.MaxAllocationSize());
		EXPECT_EQ(0, TestSuballocator.TotalFree());

		// Free remaining allocations
		TestSuballocator.Free(Block4);
		TestSuballocator.Free(Block2);

		// Verify the full range can be allocated
		auto Block5 = TestSuballocator.Allocate(32);
		EXPECT_EQ(0, Block5.Start());
		EXPECT_EQ(32, Block5.Size());
		EXPECT_EQ(0, TestSuballocator.MaxAllocationSize());
		EXPECT_EQ(0, TestSuballocator.TotalFree());
	}

	TEST_F(BuddySuballocatorTestClass, BuddySuballocatorStress)
	{
		using IndexType = unsigned char;
		constexpr size_t MaxAllocations = 4;
		TBuddySuballocator<IndexType> TestSuballocator(MaxAllocations);
		std::list<TBuddyBlock<IndexType>> Blocks;

		// Allocate all possible smallest allocations
		for (int i = 0; i < MaxAllocations; ++i)
		{
			auto Block = TestSuballocator.Allocate(1);
			Blocks.push_back(Block);
			EXPECT_EQ(0, Block.Order());
		}

		// Verify no allocations remain
		{
			bool ExceptionHit = false;
			try
			{
				auto FailBlock = TestSuballocator.Allocate(1);
			}
			catch (BuddySuballocatorException&)
			{
				ExceptionHit = true;
			}
			EXPECT_TRUE(ExceptionHit);
		}

		// Free up even allocations
		for (auto it = Blocks.begin(); it != Blocks.end();)
		{
			auto Block = *it;
			if (0 == (Block.Start() % 2))
			{
				TestSuballocator.Free(*it);
				it = Blocks.erase(it);
			}
			else
			{
				++it;
			}
		}

		// Verify no size-2 allocations are available due to fragmentation
		{
			bool ExceptionHit = false;
			try
			{
				auto FailBlock = TestSuballocator.Allocate(2);
			}
			catch (BuddySuballocatorException&)
			{
				ExceptionHit = true;
			}
			EXPECT_TRUE(ExceptionHit);
		}

		// Verify reallocation of size-1 allocations
		for (int i = 0; i < MaxAllocations; i += 2)
		{
			auto Block = TestSuballocator.Allocate(1);
			Blocks.push_back(Block);
			EXPECT_EQ(0, Block.Order());
		}

		// Free first half of allocations
		for (auto it = Blocks.begin(); it != Blocks.end();)
		{
			auto Block = *it;
			if (Block.Start() < MaxAllocations / 2)
			{
				TestSuballocator.Free(Block);
				it = Blocks.erase(it);
			}
			else
			{
				++it;
			}
		}

		// Allocate all available size-2 blocks
		for (int i = 0; i < MaxAllocations / 4; ++i)
		{
			auto Block = TestSuballocator.Allocate(2);
			EXPECT_EQ(1, Block.Order());
			Blocks.push_back(Block);
		}

		// Verify no allocations remain
		{
			bool ExceptionHit = false;
			try
			{
				auto FailBlock = TestSuballocator.Allocate(1);
			}
			catch (BuddySuballocatorException& e)
			{
				if (e.T == BuddySuballocatorException::Type::Unavailable)
				{
					ExceptionHit = true;
				}
			}
			EXPECT_TRUE(ExceptionHit);
		}

		// Free all blocks
		for (auto it = Blocks.begin(); it != Blocks.end();)
		{
			TestSuballocator.Free(*it);
			it = Blocks.erase(it);
		}

		// Verify can now allocate full size
		{
			auto Block = TestSuballocator.Allocate(MaxAllocations);
			EXPECT_EQ(IndexType(0), Block.Start());
			EXPECT_EQ(size_t(MaxAllocations), Block.Size());
			TestSuballocator.Free(Block);
		}

		// Verify exceptions for freeing invalid blocks
		{
			// Free block that was never allocated
			TBuddyBlock<IndexType> Block(0, 0);
			bool ExceptionHit = false;
			try
			{
				TestSuballocator.Free(Block);
			}
			catch (BuddySuballocatorException& e)
			{
				if (e.T == BuddySuballocatorException::Type::NotAllocated)
				{
					ExceptionHit = true;
				}
			}
			EXPECT_TRUE(ExceptionHit);

			// Free block with the same Start but different size
			ExceptionHit = false;
			Block = TestSuballocator.Allocate(4);
			try
			{
				TBuddyBlock<IndexType> BadBlock(Block.Start(), Block.Order() - 1);
				TestSuballocator.Free(BadBlock);
			}
			catch (BuddySuballocatorException& e)
			{
				if (e.T == BuddySuballocatorException::Type::NotAllocated)
				{
					ExceptionHit = true;
				}
			}
			EXPECT_TRUE(ExceptionHit);
			TestSuballocator.Free(Block);
		}
	}

	TEST_F(BuddySuballocatorTestClass, ScenarioSuballocatorTest)
	{
		using IndexType = unsigned char;
		constexpr size_t MaxAllocations = 32;
		TBuddySuballocator<IndexType> TestSuballocator(MaxAllocations);
		std::vector<char> TestData(MaxAllocations + 1, '-');
		TestData[MaxAllocations] = 0;

		class ScopedBuddyBlock
		{
			TBuddyBlock<IndexType> m_Block;
			TBuddySuballocator<IndexType>* m_pAllocator = nullptr;

		public:
			ScopedBuddyBlock() = default;
			ScopedBuddyBlock(const TBuddyBlock<IndexType>& Block, TBuddySuballocator<IndexType>* pAllocator) :
				m_Block(Block),
				m_pAllocator(pAllocator)
			{
			}
			ScopedBuddyBlock(const ScopedBuddyBlock& o) = delete;
			ScopedBuddyBlock(ScopedBuddyBlock&& o) noexcept :
				m_Block(o.m_Block),
				m_pAllocator(o.m_pAllocator)
			{
				o.m_pAllocator = nullptr;
				o.m_Block = TBuddyBlock<IndexType>();
			}

			~ScopedBuddyBlock()
			{
				if (m_pAllocator && m_Block != TBuddyBlock<IndexType>())
				{
					m_pAllocator->Free(m_Block);
				}
			}
			ScopedBuddyBlock& operator=(const ScopedBuddyBlock& o) = delete;
			ScopedBuddyBlock& operator=(ScopedBuddyBlock&& o) noexcept
			{
				m_Block = o.m_Block;
				m_pAllocator = o.m_pAllocator;
				o.m_Block = TBuddyBlock<IndexType>();
				o.m_pAllocator = nullptr;
				return *this;
			}

			const TBuddyBlock<IndexType>& Get() const { return m_Block; }
		};

		auto NewBlock = [&TestData, &TestSuballocator](size_t Size, char Fill)
		{
			ScopedBuddyBlock Block(TestSuballocator.Allocate(Size), &TestSuballocator);
			std::memset(&TestData[Block.Get().Start()], Fill, Size);
			return Block;
		};

		std::string s0 = TestData.data();

		auto Block1 = NewBlock(7, '1');
		std::string s1 = TestData.data();

		auto Block2 = NewBlock(2, '2');
		std::string s2 = TestData.data();

		auto Block3 = NewBlock(4, '3');
		std::string s3 = TestData.data();

		auto Block4 = NewBlock(4, '4');
		std::string s4 = TestData.data();

		auto Block5 = NewBlock(7, '5');
		std::string s5 = TestData.data();
	}

	TEST_F(BuddySuballocatorTestClass, OperatingNearFullSuballocatorTest)
	{
		using IndexType = unsigned int;
		constexpr size_t maxAllocations = 64;
		TBuddySuballocator<IndexType> testSuballocator(maxAllocations);

		auto block1 = testSuballocator.Allocate(32);
		auto block2 = testSuballocator.Allocate(16);
		auto block3 = testSuballocator.Allocate(8);
		auto block4 = testSuballocator.Allocate(4);
		auto block5 = testSuballocator.Allocate(2);
		auto block6 = testSuballocator.Allocate(1);

		//Verify that there is only one allocation left available
		EXPECT_EQ(1, testSuballocator.TotalFree());

		//Allocate the last space
		auto block7 = testSuballocator.Allocate(1);

		//Verify that there are no allocations left
		bool exceptionHit = false;
		try
		{
			auto block = testSuballocator.Allocate(1);
		}
		catch (BuddySuballocatorException &)
		{
			exceptionHit = true;
		}

		EXPECT_TRUE(exceptionHit);

		//Free the last block
		testSuballocator.Free(block7);

		//Verify there is only one allocation space left available
		EXPECT_EQ(1, testSuballocator.TotalFree());
	}

	TEST_F(BuddySuballocatorTestClass, BitArrayTest)
	{
		TBitArray<int> TestBitArray(16);

		// Verify init to false
		for (int i = 0; i < 16; ++i)
		{
			EXPECT_EQ(false, TestBitArray.Get(i));
		}

		// Verify setting and unsetting one bit at a time
		for (int i = 8; i < 16; ++i)
		{
			TestBitArray.Set(i, true);

			for (int j = 0; j < 16; ++j)
			{
				EXPECT_EQ(j == i, TestBitArray[j]);
			}

			TestBitArray.Set(i, false);
			EXPECT_EQ(false, TestBitArray.Get(i));
		}

		// Verify setting all bits
		for (int i = 0; i < 16; ++i)
		{
			EXPECT_EQ(false, TestBitArray[i]);
			TestBitArray.Set(i, true);
			EXPECT_EQ(true, TestBitArray[i]);
		}

		// Verify clearing and resetting one bit at a time
		for (int i = 0; i < 16; ++i)
		{
			EXPECT_EQ(true, TestBitArray[i]);

			TestBitArray.Set(i, false);

			for (int j = 0; j < 16; ++j)
			{
				EXPECT_EQ(j != i, TestBitArray[j]);
			}

			TestBitArray.Set(i, true);
			EXPECT_EQ(true, TestBitArray.Get(i));
		}

		// Verify setting all bits back to false
		for (int i = 0; i < 16; ++i)
		{
			EXPECT_EQ(true, TestBitArray[i]);
			TestBitArray.Set(i, false);
			EXPECT_EQ(false, TestBitArray[i]);
		}
	}

	class RingSuballocatorTest : public ::testing::Test
	{
	protected:
		void SetUp() override {}
		void TearDown() override {}
	};

	TEST_F(RingSuballocatorTest, SimpleRingSuballocatorTest)
	{
		TRingSuballocator<uint8_t> Allocator(256);

		uint8_t Loc = Allocator.Allocate(256);
		EXPECT_TRUE(Loc == 0);
		EXPECT_TRUE(0 == Allocator.FreeSize());
		EXPECT_TRUE(256 == Allocator.AllocatedSize());
		Allocator.Free(100);
		EXPECT_TRUE(156 == Allocator.AllocatedSize());
		EXPECT_TRUE(100 == Allocator.FreeSize());
		Loc = Allocator.Allocate(99);
		EXPECT_TRUE(Loc == 0);
		EXPECT_TRUE(1 == Allocator.FreeSize());
		Allocator.Free(155);
		EXPECT_TRUE(156 == Allocator.FreeSize());
		Loc = Allocator.Allocate(100);
		EXPECT_TRUE(Loc == 99);
		EXPECT_TRUE(Allocator.FreeSize() == 56);
		Loc = Allocator.Allocate(50);
		EXPECT_TRUE(Loc == 199);
		EXPECT_TRUE(6 == Allocator.FreeSize());
		bool HitBadAlloc = false;
		try
		{
			Allocator.Allocate(7);
		}
		catch (const std::bad_alloc &)
		{
			HitBadAlloc = true;
		}

		EXPECT_TRUE(HitBadAlloc);

		Allocator.Reset(256);
		EXPECT_TRUE(256 == Allocator.FreeSize());
		Loc = Allocator.Allocate(256);
		EXPECT_TRUE(Loc == 0);
		EXPECT_TRUE(0 == Allocator.FreeSize());
		EXPECT_TRUE(256 == Allocator.AllocatedSize());
		Allocator.Reset(156);

		Allocator.Allocate(1);
		Allocator.Allocate(2);
		Allocator.Allocate(3);
		Allocator.Allocate(4);
		EXPECT_TRUE(10 == Allocator.AllocatedSize());
		Allocator.Free(10);
		EXPECT_TRUE(0 == Allocator.AllocatedSize());
		Loc = Allocator.Allocate(1);
		EXPECT_TRUE(10 == Loc);
		Allocator.Reset(64);
	}
}
