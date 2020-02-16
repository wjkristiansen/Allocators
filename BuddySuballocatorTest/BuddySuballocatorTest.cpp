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
			int i = 0;
			for (; i < _countof(TestIndices); ++i)
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
		}
	};
}
