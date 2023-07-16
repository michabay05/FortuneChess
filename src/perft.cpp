#include "defs.hpp"

void driver(Board& board, int depth, uint64_t& nodeCount)
{
    if (depth == 0) {
        nodeCount++;
        return;
    }
    MoveList moveList;
    genAllMoves(moveList, board);
    Board clone;
    for (int i = 0; i < moveList.count; i++) {
        // Clone the current state of the board
        clone = board;
        // Make move if it's not illegal (or check)
        if (!makeMove(&board, moveList.list[i], MoveType::AllMoves))
            continue;

        driver(board, depth - 1, nodeCount);

        // Restore board state
        board = clone;
        /* ============= FOR DEBUG PURPOSES ONLY ===============
        uint64_t generatedKey = genKey(board);
        uint64_t generatedLock = genLock(board);
        if (generatedKey != board.key) {
            std::cout << "\nBoard.MakeMove(" << moveToStr(moveList.list[i]) << ")\n";
            board.display();
            std::cout << "Key should've been 0x" << std::hex << board.key << "ULL instead of 0x"
                      << generatedKey << std::dec << "ULL\n";
            _ASSERT(false);
        }
        if (generatedLock != board.lock) {
            std::cout << "\nBoard.MakeMove(" << moveToStr(moveList.list[i]) << ")\n";
            board.display();
            std::cout << "Lock should've been 0x" << std::hex << board.lock << "ULL instead of 0x"
                      << generatedLock << std::dec << "ULL\n";
            _ASSERT(false);
        }
        ============= FOR DEBUG PURPOSES ONLY =============== */
    }
}

uint64_t perftTest(Board& board, const int depth, bool showDebugInfo)
{
    uint64_t nodeCount = 0ULL;
    MoveList moveList;
    genAllMoves(moveList, board);
    Board clone;
    for (int i = 0; i < moveList.count; i++) {
        // Clone the current state of the board
        clone = board;
        // Make move if it's not illegal (or check)
        if (!makeMove(&board, moveList.list[i], MoveType::AllMoves))
            continue;

        uint64_t prevNodeCount = nodeCount;
        driver(board, depth - 1, nodeCount);

        // Restore board state
        board = clone;

        if (showDebugInfo) {
            std::cout << moveToStr(moveList.list[i]) << ": " << (nodeCount - prevNodeCount)
                      << "\n";
        }
    }
	std::cout << "Nodes: " << nodeCount << "\n";
    return nodeCount;
}