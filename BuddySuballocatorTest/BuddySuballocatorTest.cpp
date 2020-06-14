#include "pch.h"
#include "CppUnitTest.h"
#include "BuddySuballocator.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using std::cout;

namespace BuddySuballocatorTest
{
	TEST_CLASS(BuddySuballocatorTestClass)
	{
	public:

		TEST_METHOD(IndexListTest)
		{
			using IndexType = unsigned __int8;
			using IndexNodeType = IndexNode<IndexType>;

			//			IndexNodeType IndexTable[16];
			std::vector<IndexNodeType> IndexTable(16);
			std::fill(IndexTable.begin(), IndexTable.end(), IndexNode<IndexType>()); // zero init
			using IndexTableType = decltype(IndexTable);
			using ListType = TIndexList<IndexType, IndexTableType>;
		
			ListType IndexList;

			Assert::IsTrue(IndexList.Size() == 0);

			const IndexType TestIndices[]{ 14, 1, 0, 6, 3, 8, 5 };

			// Build list from the indices in TestIndices
			IndexType First = TestIndices[0];
			IndexType Last = TestIndices[0];
			IndexType i;
			IndexType NodeCount = IndexType(_countof(TestIndices));
			for (i = 0; i < NodeCount; ++i)
			{
				IndexType Index = TestIndices[i];

				auto It = IndexList.PushFront(Index, IndexTable);
				Assert::IsTrue(IndexList.Size() == 1 + size_t(i));
				Assert::IsTrue(It.Index() == Index);
				Assert::IsTrue(IndexList.Begin() == It);
				It.MoveNext(IndexTable);
				if (i == 0)
				{
					Assert::IsTrue(It == IndexList.End());
				}
				else
				{
					Assert::IsTrue(It.Index() == First);
				}
				First = Index;
			}

			// Iterate through the list and make sure the values match the reverse order added
			for (auto It = IndexList.Begin(); It != IndexList.End(); It.MoveNext(IndexTable))
			{
				--i;
				Assert::IsTrue(TestIndices[i] == It.Index());
			}

			// Remove a node from the middle...
			{
				auto It = IndexList.Remove(6, IndexTable);

				Assert::IsTrue(0 == It.Index());
				It.MovePrev(IndexTable);
				Assert::IsTrue(3 == It.Index());

				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove the last node in the list...
			{
				auto It = IndexList.Remove(14, IndexTable);
				Assert::IsTrue(It == IndexList.End());
				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove the first node in the list...
			{
				auto It = IndexList.Remove(5, IndexTable);

				Assert::IsTrue(8 == It.Index());
				It.MovePrev(IndexTable);
				Assert::IsTrue(It == IndexList.Begin());

				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove down to a single node...
			while (NodeCount > 1)
			{
				IndexList.Remove(IndexList.Begin().Index(), IndexTable);
				NodeCount--;
			}

			{
				auto It = IndexList.Begin();
				Assert::IsTrue(1 == It.Index());
				It.MovePrev(IndexTable);
				Assert::IsTrue(IndexList.End() == It);
				It = IndexList.Begin();
				It.MoveNext(IndexTable);
				Assert::IsTrue(IndexList.End() == It);
			}

			// Remove the final node
			IndexList.Remove(IndexList.Begin().Index(), IndexTable);

			Assert::IsTrue(0 == IndexList.Size());

			// Verify all removed nodes in the IndexTable are [index, index]
			for (auto i = 0; i < _countof(TestIndices); ++i)
			{
				auto index = TestIndices[i];
				Assert::AreEqual<IndexType>(IndexType(index), IndexTable[index].Next);
				Assert::AreEqual<IndexType>(IndexType(index), IndexTable[index].Prev);
			}
		}

		TEST_METHOD(BasicSuballocatorTest)
		{
			using IndexType = unsigned char;
			constexpr size_t MaxAllocations = 32;
			TBuddySuballocator<IndexType> TestSuballocator(MaxAllocations);

			// Some basic tests
			auto Block1 = TestSuballocator.Allocate(6);
			Assert::AreEqual<IndexType>(0, Block1.Start());
			Assert::AreEqual<size_t>(8, Block1.Size());
			Assert::AreEqual<size_t>(16, TestSuballocator.MaxAllocationSize());
			Assert::AreEqual<size_t>(24, TestSuballocator.TotalFree());
			//Assert::IsTrue(TestSuballocator.IsAllocated(Block1));

			auto Block2 = TestSuballocator.Allocate(16);
			Assert::AreEqual<IndexType>(16, Block2.Start());
			Assert::AreEqual<size_t>(16, Block2.Size());
			Assert::AreEqual<size_t>(8, TestSuballocator.MaxAllocationSize());
			Assert::AreEqual<size_t>(8, TestSuballocator.TotalFree());

			auto Block3 = TestSuballocator.Allocate(8);
			Assert::AreEqual<IndexType>(8, Block3.Start());
			Assert::AreEqual<size_t>(8, Block3.Size());
			Assert::AreEqual<size_t>(0, TestSuballocator.MaxAllocationSize());
			Assert::AreEqual<size_t>(0, TestSuballocator.TotalFree());

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
			Assert::IsTrue(ExceptionHit);

			// Free up the two adjacent 8-byte blocks
			TestSuballocator.Free(Block1);
			Assert::AreEqual<size_t>(8, TestSuballocator.MaxAllocationSize());
			Assert::AreEqual<size_t>(8, TestSuballocator.TotalFree());
			TestSuballocator.Free(Block3);
			Assert::AreEqual<size_t>(16, TestSuballocator.MaxAllocationSize());
			Assert::AreEqual<size_t>(16, TestSuballocator.TotalFree());

			// Should be 16 bytes available
			auto Block4 = TestSuballocator.Allocate(16);
			Assert::AreEqual<IndexType>(0, Block4.Start());
			Assert::AreEqual<size_t>(16, Block4.Size());
			Assert::AreEqual<size_t>(0, TestSuballocator.MaxAllocationSize());
			Assert::AreEqual<size_t>(0, TestSuballocator.TotalFree());

			// Free remaining allocations
			TestSuballocator.Free(Block4);
			TestSuballocator.Free(Block2);

			// Verify the full range can be allocated
			auto Block5 = TestSuballocator.Allocate(32);
			Assert::AreEqual<IndexType>(0, Block5.Start());
			Assert::AreEqual<size_t>(32, Block5.Size());
			Assert::AreEqual<size_t>(0, TestSuballocator.MaxAllocationSize());
			Assert::AreEqual<size_t>(0, TestSuballocator.TotalFree());
		}

		TEST_METHOD(BuddySuballocatorStress)
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
				Assert::AreEqual<IndexType>(0, Block.Order());
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
				Assert::IsTrue(ExceptionHit);
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
				Assert::IsTrue(ExceptionHit);
			}

			// Verify reallocation of size-1 allocations
			for (int i = 0; i < MaxAllocations; i += 2)
			{
				auto Block = TestSuballocator.Allocate(1);
				Blocks.push_back(Block);
				Assert::AreEqual<IndexType>(0, Block.Order());
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
				Assert::AreEqual<IndexType>(1, Block.Order());
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
				Assert::IsTrue(ExceptionHit);
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
				Assert::AreEqual(Block.Start(), IndexType(0));
				Assert::AreEqual(Block.Size(), size_t(MaxAllocations));
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
				Assert::IsTrue(ExceptionHit);

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
				Assert::IsTrue(ExceptionHit);
				TestSuballocator.Free(Block);
			}
		}

		TEST_METHOD(ScenarioSuballocatorTest)
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


		TEST_METHOD(OperatingNearFullSuballocatorTest)
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

			Assert::AreEqual<size_t>(1, testSuballocator.TotalFree());

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

			Assert::IsTrue(exceptionHit);

			//Free the last block
			testSuballocator.Free(block7);

			//Verify there is only one allocation space left available
			Assert::AreEqual<size_t>(1, testSuballocator.TotalFree());

		}

		TEST_METHOD(BitArrayTest)
		{
			TBitArray<int> TestBitArray(16);

			// Verify init to false
			for (int i = 0; i < 16; ++i)
			{
				Assert::AreEqual(false, TestBitArray.Get(i));
			}

			// Verify setting and unsetting one bit at a time
			for (int i = 8; i < 16; ++i)
			{
				TestBitArray.Set(i, true);

				for (int j = 0; j < 16; ++j)
				{
					Assert::AreEqual(j == i, TestBitArray[j]);
				}

				TestBitArray.Set(i, false);
				Assert::AreEqual(TestBitArray.Get(i), false);
			}

			// Verify setting all bits
			for (int i = 0; i < 16; ++i)
			{
				Assert::AreEqual(TestBitArray[i], false);
				TestBitArray.Set(i, true);
				Assert::AreEqual(TestBitArray[i], true);
			}

			// Verify clearing and resetting one bit at a time
			for (int i = 0; i < 16; ++i)
			{
				Assert::AreEqual(TestBitArray[i], true);

				TestBitArray.Set(i, false);

				for (int j = 0; j < 16; ++j)
				{
					Assert::AreEqual(j != i, TestBitArray[j]);
				}

				TestBitArray.Set(i, true);
				Assert::AreEqual(TestBitArray.Get(i), true);
			}

			// Verify setting all bits back to false
			for (int i = 0; i < 16; ++i)
			{
				Assert::AreEqual(TestBitArray[i], true);
				TestBitArray.Set(i, false);
				Assert::AreEqual(TestBitArray[i], false);
			}
		}
	};
}
