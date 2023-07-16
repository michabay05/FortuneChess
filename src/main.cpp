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
    initAttacks();
    initBook();
	initEvalMasks();
    initTTable(64);
    initZobrist();
#if TEST == 1
    runTests();
#else
    uciLoop();
#endif
    deinitBook();
    deinitTTable();
}
