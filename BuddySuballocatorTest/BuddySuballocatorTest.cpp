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
			using IndexTableNodeType = IndexTableNode<IndexType>;

//			IndexTableNodeType IndexTable[16];
			std::vector<IndexTableNodeType> IndexTable(16);
			using IndexTableType = decltype(IndexTable);
			using ListType = TIndexList<IndexType, IndexTableType>;

			ListType IndexList(IndexTable);

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

				auto It = IndexList.PushFront(Index);
				Assert::IsTrue(IndexList.Size() == 1 + i);
				Assert::IsTrue(It.Index() == Index);
				Assert::IsTrue(IndexList.Begin() == It);
				It.MoveNext();
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
			for (auto It = IndexList.Begin(); It != IndexList.End(); It.MoveNext())
			{
				--i;
				Assert::IsTrue(TestIndices[i] == It.Index());
			}

			// Remove a node from the middle...
			{
				auto It = IndexList.Remove(6);

				Assert::IsTrue(0 == It.Index());
				It.MovePrev();
				Assert::IsTrue(3 == It.Index());

				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove the last node in the list...
			{
				auto It = IndexList.Remove(15);
				Assert::IsTrue(It == IndexList.End());
				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove the first node in the list...
			{
				auto It = IndexList.Remove(5);

				Assert::IsTrue(8 == It.Index());
				It.MovePrev();
				Assert::IsTrue(It == IndexList.End());

				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove down to a single node...
			while (NodeCount > 1)
			{
				IndexList.Remove(IndexList.Begin().Index());
				NodeCount--;
			}

			{
				auto It = IndexList.Begin();
				Assert::IsTrue(1 == It.Index());
				It.MovePrev();
				Assert::IsTrue(IndexList.End() == It);
				It = IndexList.Begin();
				It.MoveNext();
				Assert::IsTrue(IndexList.End() == It);
			}

			// Remove the final node
			IndexList.Remove(IndexList.Begin().Index());

			Assert::IsTrue(0 == IndexList.Size());

			// Verify all nodes in the IndexTable are [0, 0]
			for (auto i = 0; i < _countof(TestIndices); ++i)
			{
				Assert::IsTrue(IndexTable[i].Next == 0);
				Assert::IsTrue(IndexTable[i].Prev == 0);
			}
		}
	};
}
