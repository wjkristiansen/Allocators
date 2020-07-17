#include "pch.h"
#include "CppUnitTest.h"
#include "RingSuballocator.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace RingSuballocatorTest
{
	TEST_CLASS(RingSuballocatorTest)
	{
	public:
		
		TEST_METHOD(SimpleRingSuballocatorTest)
		{
			TRingSuballocator<uint8_t> Allocator(256);

			uint8_t Loc = Allocator.Allocate(256);
			Assert::IsTrue(Loc == 0);
			Assert::IsTrue(0 == Allocator.FreeSize());
			Assert::IsTrue(256 == Allocator.AllocatedSize());
			Allocator.Free(100);
			Assert::IsTrue(156 == Allocator.AllocatedSize());
			Assert::IsTrue(100 == Allocator.FreeSize());
			Loc = Allocator.Allocate(99);
			Assert::IsTrue(Loc == 0);
			Assert::IsTrue(1 == Allocator.FreeSize());
			Allocator.Free(155);
			Assert::IsTrue(156 == Allocator.FreeSize());
			Loc = Allocator.Allocate(100);
			Assert::IsTrue(Loc == 99);
			Assert::IsTrue(Allocator.FreeSize() == 56);
			Loc = Allocator.Allocate(50);
			Assert::IsTrue(Loc == 199);
		}
	};
}
