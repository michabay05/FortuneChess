#include <iostream>

#include "defs.hpp"
#include "tests.hpp"

#define TEST 0

void runTests()
{
    test::parseFen();
    test::polyKeyGeneration();
}

int main()
{
    srand((unsigned int)time(NULL));

    initAttacks();
    initBook();
	initEvalMasks();
	hashTable.init(DEFAULT_TT_SIZE);
    initZobrist();
#if TEST == 1
    runTests();
#else
    uciLoop();
#endif
    if (sInfo.quit) {
		deinitBook();
		hashTable.deinit();
    }
}
