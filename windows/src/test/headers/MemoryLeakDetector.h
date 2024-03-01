#pragma once
#include <gtest/gtest.h>

namespace Loom {
class MemoryLeakDetector: public ::testing::Test
{
public:
	MemoryLeakDetector();
	~MemoryLeakDetector();

private:
	_CrtMemState _memState;
};
}
