#include <iostream>

#include "defs.hpp"
#include "tests.hpp"

#define TEST 0

void runTests()
{
    test::parseFen();
    test::moveGeneration();
}

int main()
{
    initAttacks();
	initEvalMasks();
    initTTable(64);
    initZobrist();
#if TEST == 1
    run_tests();
#else
    uciLoop();
#endif
    deinitTTable();
}
