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
			using ListType = TIndexList<IndexType>;
			ListType IndexList;

			// Initialize the table to size 16
			std::vector<ListType::NodeType> IndexTable(16);

			Assert::IsTrue(IndexList.Size() == 0);

			const IndexType TestIndices[]{ 15, 1, 0, 6, 3, 8, 5 };

			// Build list from the indices in TestIndices
			IndexType First = TestIndices[0];
			IndexType Last = TestIndices[0];
			IndexType i = 0;
			IndexType NodeCount = IndexType(_countof(TestIndices));
			for (; i < NodeCount; ++i)
			{
				IndexType Index = TestIndices[i];

				IndexList.PushFront(Index, IndexTable);
				Assert::IsTrue(IndexList.Size() == 1 + i);
				Assert::IsTrue(IndexList.Next(Index, IndexTable) == First);
				Assert::IsTrue(IndexList.Prev(Index, IndexTable) == Last);
				Assert::IsTrue(IndexList.First() == Index);
				Assert::IsTrue(IndexList.Last() == Last);
				First = Index;
			}

			// Iterate through the list and make sure the values match the reverse order added
			for (IndexType Index = IndexList.First();;)
			{
				--i;
				Assert::IsTrue(TestIndices[i] == Index);

				// Next element
				Index = IndexList.Next(Index, IndexTable);

				// The end of the list is reached when we loop back to the first node
				if (Index == IndexList.First())
				{
					break;
				}
			}

			// Iterate through the list in reverse order and make sure the values match the order added
			for (IndexType Index = IndexList.Last();;)
			{
				Assert::IsTrue(TestIndices[i] == Index);

				// Prev element
				Index = IndexList.Prev(Index, IndexTable);
				++i;

				// The start of the list is reached when we loop back to the last node
				if (Index == IndexList.Last())
				{
					break;
				}
			}

			// Remove a node from the middle...
			{
				IndexList.RemoveAt(6, IndexTable);
				Assert::IsTrue(3 == IndexList.Prev(0, IndexTable));
				Assert::IsTrue(0 == IndexList.Next(3, IndexTable));
				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove the last node...
			{
				IndexList.RemoveAt(15, IndexTable);
				Assert::IsTrue(1 == IndexList.Last());
				Assert::IsTrue(IndexList.First() == IndexList.Next(IndexList.Last(), IndexTable));
				Assert::IsTrue(IndexList.Last() == IndexList.Prev(IndexList.First(), IndexTable));
				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove the first node...
			{
				IndexList.RemoveAt(5, IndexTable);
				Assert::IsTrue(1 == IndexList.Last());
				Assert::IsTrue(IndexList.First() == IndexList.Next(IndexList.Last(), IndexTable));
				Assert::IsTrue(IndexList.Last() == IndexList.Prev(IndexList.First(), IndexTable));
				NodeCount--;
				Assert::IsTrue(NodeCount == IndexList.Size());
			}

			// Remove down to a single node...
			while (NodeCount > 1)
			{
				IndexList.RemoveAt(IndexList.Last(), IndexTable);
				NodeCount--;
			}

			Assert::IsTrue(1 == IndexList.Size());
			Assert::IsTrue(8 == IndexList.First());
			Assert::IsTrue(8 == IndexList.Last());
			Assert::IsTrue(8 == IndexList.Next(8, IndexTable));
			Assert::IsTrue(8 == IndexList.Prev(8, IndexTable));

			// Remove the final node
			IndexList.RemoveAt(8, IndexTable);
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
