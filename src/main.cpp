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
    srand(time(NULL));

    initAttacks();
    initBook();
	initEvalMasks();
#if 1
	tt.init(DEFAULT_TT_SIZE);
#else
    initTTable(128);
#endif
    initZobrist();
#if TEST == 1
    runTests();
#else
    //uciLoop();
    parse("position fen r3kb1r/3n1pp1/p6p/2pPp2q/Pp2N3/3B2PP/1PQ2P2/R3K2R w KQkq - 0 1");
    parse("debug on");
    parse("go depth 10");
#endif
    deinitBook();
    tt.deinit();
}
