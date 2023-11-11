#include "defs.hpp"

void driver(Board& board, int depth, uint64_t& nodeCount, MoveType moveType)
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
        if (!makeMove(&board, moveList.list[i], moveType))
            continue;

        driver(board, depth - 1, nodeCount, moveType);

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

uint64_t perftTest(Board& board, const int depth, MoveType moveType)
{
    uint64_t nodeCount = 0ULL;
    MoveList moveList;
    if (moveType == AllMoves)
		genAllMoves(moveList, board);
    else
		genCaptureMoves(moveList, board);
    Board clone;
    for (int i = 0; i < moveList.count; i++) {
        // Clone the current state of the board
        clone = board;
        // Make move if it's not illegal (or check)
        if (!makeMove(&board, moveList.list[i], moveType))
            continue;

        uint64_t prevNodeCount = nodeCount;
        driver(board, depth - 1, nodeCount, moveType);

        // Restore board state
        board = clone;

        if (sInfo.debugMode) {
            std::cout << moveToStr(moveList.list[i]) << ": " << (nodeCount - prevNodeCount)
                      << "\n";
        }
    }
    std::string nodesStr =
        moveType == AllMoves ? "Total number of moves: " : "Total number of captures: ";
	std::cout << nodesStr << nodeCount << "\n";
    return nodeCount;
}
