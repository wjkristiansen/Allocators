#include "pch.h"
#include "CppUnitTest.h"
#include "BuddySuballocator.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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
			using IndexTableType = decltype(IndexTable);
			using ListType = TIndexList<IndexType, IndexTableType>;

			ListType IndexList;

			Assert::IsTrue(IndexList.Size() == 0);

			const IndexType TestIndices[]{ 15, 1, 0, 6, 3, 8, 5 };

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
				auto It = IndexList.Remove(15, IndexTable);
				Assert::IsTrue(It == IndexList.End());
				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove the first node in the list...
			{
				auto It = IndexList.Remove(5, IndexTable);

				Assert::IsTrue(8 == It.Index());
				It.MovePrev(IndexTable);
				Assert::IsTrue(It == IndexList.End());

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

			// Verify all nodes in the IndexTable are [0, 0]
			for (auto i = 0; i < _countof(TestIndices); ++i)
			{
				Assert::IsTrue(IndexTable[i].Next == 0);
				Assert::IsTrue(IndexTable[i].Prev == 0);
			}
		}

		TEST_METHOD(BasicSuballocatorTest)
		{
			constexpr size_t MaxAllocations = 32;
			TBuddySuballocator<unsigned char, MaxAllocations> TestSuballocator;
			std::vector<unsigned char> TestData(MaxAllocations, '-');

			// Some basic tests
			auto Block1 = TestSuballocator.Allocate(6);
			Assert::IsTrue(0 == Block1.Location);
			Assert::IsTrue(8 == Block1.Size());

			auto Block2 = TestSuballocator.Allocate(16);
			Assert::IsTrue(16 == Block2.Location);
			Assert::IsTrue(16 == Block2.Size());

			auto Block3 = TestSuballocator.Allocate(8);
			Assert::IsTrue(8 == Block3.Location);
			Assert::IsTrue(8 == Block3.Size());

			// Should now be fully allocated
			auto FailBlock = TestSuballocator.Allocate(1);
			Assert::IsTrue(0 == FailBlock.Size());

			// Free up the two adjacent 8-byte blocks
			TestSuballocator.Free(Block1);
			TestSuballocator.Free(Block3);

			// Should be 16 bytes available
			auto Block4 = TestSuballocator.Allocate(16);
			Assert::IsTrue(0 == Block4.Location);
			Assert::IsTrue(16 == Block4.Size());
		}
	};
}
