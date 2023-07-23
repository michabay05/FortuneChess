#include "defs.hpp"

// clang-format off
/*
SMP Data - bit structure

                              BITS                                    NAME         SIZE     START INDEX
0000000000000000000000000000000000000000000000011111111111111111   (score + INF)  17 bits        0
0000000000000000000000000000000000000000011111100000000000000000      depth        6 bits        17
0000000000000000000000000000000000000001100000000000000000000000      flag         2 bits        23

*/
// clang-format on

const int SMP_INF = INF + 1000;
#define EXTRACT_SCORE(x) ((x & 0x1FFFF) - SMP_INF)
#define EXTRACT_DEPTH(x) ((x >> 17) & 0x3F)
#define EXTRACT_FLAG(x) ((x >> 23) & 0x3)

#define FOLD_DATA(score, de, flag) ((score + SMP_INF) | (de << 17) | (flag << 23))

TT::TT()
    //: key(0ULL), lock(0ULL), depth(0), flag(F_EXACT), score(0), age(0), smpKey(0ULL),
    //: smpData(0ULL)
    : age(0), smpKey(0ULL), smpData(0ULL)
{
}

static const int ONE_MB = 0x100000;

const int NO_TT_ENTRY = 100'000;

HashTable hashTable[1];

HashTable::HashTable() : table(nullptr), entryCount(0), currentAge(0), newWrite(0), overWrite(0) {}

void HashTable::init(int MB)
{
    _MY_ASSERT(MB >= 1, "Minimum size of transposition table is 1 MB");
    _MY_ASSERT(MB <= 1024, "Maximum size of transposition table is 1024 MB");

    const int HASH_SIZE = ONE_MB * MB;
    // entryCount = HASH_SIZE / sizeof(TT);
    entryCount = 1'000'000;

    if (table != nullptr) {
        std::cout << "Clearing memory!\n";
        deinit();
    }

    table = new TT[entryCount];
    if (table == nullptr) {
        int reducedSize = HASH_SIZE / 2;
        std::cout << "[ERROR]: Couldn't allocate " << HASH_SIZE << "MB\n";
        std::cout << "[ INFO]: Trying to allocate " << reducedSize << "MB\n";
        init(reducedSize);
    }

    clear();
    std::cout << "Transposition table initialized with size of " << MB << " MB(" << entryCount
              << " entries)\n";
}

void HashTable::deinit()
{
    if (uci.debugMode)
        std::cout << "Deinitialized the transposition table!\n";
    delete[] table;
}

void HashTable::clear()
{
    for (int i = 0; i < entryCount; i++) {
        table[i] = TT();
    }
    currentAge = 0;
    newWrite = 0;
    overWrite = 0;
    entriesFilled = 0;

    std::cout << "Cleared transposition table!\n";
}

#if 0
void verifyEntrySMP(TT entry)
{
    uint64_t smpData = FOLD_DATA(entry.score, entry.depth, entry.flag);
    uint64_t smpKey = smpData ^ entry.key;

    _MY_ASSERT(smpData == entry.smpData, "[ERROR]: smpData != entry.smpData");
    _MY_ASSERT(smpKey == entry.smpKey, "[ERROR]: smpKey != entry.smpKey");

    int depth = EXTRACT_DEPTH(smpData);
    //printBits((uint64_t)smpData);
    int flag = EXTRACT_FLAG(smpData);
    int score = EXTRACT_SCORE(smpData);

    _MY_ASSERT(depth == entry.depth, "[ERROR]: depth != entry.depth");
    _MY_ASSERT(flag == entry.flag, "[ERROR]: flag != entry.flag");
    _MY_ASSERT(score == entry.score, "[ERROR]: score != entry.score");
}
#endif

// read hash entry data
int HashTable::read(Board& board, int alpha, int beta, int depth)
{
    TT entry = table[board.key % entryCount];

    uint64_t testKey = board.key ^ entry.smpData;

    // make sure we're dealing with the exact position we need
    // if (entry.key == board.key && entry.lock == board.lock) {
    if (entry.smpKey == testKey) {

        int smpDepth = EXTRACT_DEPTH(entry.smpData);
        int smpFlag = EXTRACT_FLAG(entry.smpData);
        int smpScore = EXTRACT_SCORE(entry.smpData);

        // make sure that we match the exact depth our search is now at
        if (smpDepth >= depth) {
            // extract stored score from TT entry
            int score = smpScore;

            // retrieve score independent from the actual path
            // from root node (position) to current node (position)
            if (score < -MATE_SCORE)
                score += ply;
            if (score > MATE_SCORE)
                score -= ply;

            // match the exact (PV node) score
            if (smpFlag == F_EXACT)
                // return exact (PV node) score
                return score;

            // match alpha (fail-low node) score
            if ((smpFlag == F_ALPHA) && (score <= alpha))
                // return alpha (fail-low node) score
                return alpha;

            // match beta (fail-high node) score
            if ((smpFlag == F_BETA) && (score >= beta))
                // return beta (fail-high node) score
                return beta;
        }
    }

    // if hash entry doesn't exist
    return NO_TT_ENTRY;
}

// write hash entry data
void HashTable::store(Board& board, int score, int depth, TTFlag flag)
{
    TT* entry = &table[board.key % entryCount];

    bool shouldReplace = false;
    if (entry->smpKey == 0) {
        // If the current entry has nothing written on it, place the new entry here
        newWrite++;
        shouldReplace = true;
    } else {
        overWrite++;
        if (entry->age < currentAge || EXTRACT_DEPTH(entry->smpData) <= depth) {
            shouldReplace = true;
        }
    }

    if (!shouldReplace)
        return;

    if (entriesFilled < entryCount)
        entriesFilled++;

    // store score independent from the actual path
    // from root node (position) to current node (position)
    if (score < -MATE_SCORE)
        score -= ply;
    if (score > MATE_SCORE)
        score += ply;

    uint64_t smpData = FOLD_DATA(score, depth, flag);
    // write hash entry data
#if 0
    entry->key = board.key;
    entry->lock = board.lock;
    entry->score = score;
    entry->flag = flag;
    entry->depth = depth;
#endif
    entry->age = currentAge;
    entry->smpData = smpData;
    entry->smpKey = smpData ^ board.key;
}

void dataCheck(const int move)
{
    int depth = rand() % MAX_PLY;
    int flag = rand() % 3;
    int score = rand() % INF;

    uint64_t data = FOLD_DATA(score, depth, flag);

    int extDepth = EXTRACT_DEPTH(data);
    int extScore = EXTRACT_SCORE(data);
    int extFlag = EXTRACT_FLAG(data);

    _MY_ASSERT(depth == extDepth, "depth != SMP_DEPTH");
    _MY_ASSERT(score == extScore, "score != SMP_SCORE");
    _MY_ASSERT(flag == extFlag, "flag != SMP_FLAG");
#if 0
    std::cout << " Orig: move:" << moveToStr(move) << " score: " << score << " depth: " << depth
              << " flag: " << flag << "\n";
    std::cout << "Check: move:" << moveToStr(extDepth) << " score: " << extScore
              << " depth: " << extDepth << " flag: " << extFlag << "\n\n";
#endif
}

void tempHashTest(const std::string fen)
{
    std::cout << "=======================================================\n> " << fen << "\n\n";

    Board board;
    board.parseFen(fen);

    MoveList moveList;
    genAllMoves(moveList, board);
    Board clone;
    for (int i = 0; i < moveList.count; i++) {
        // Clone the current state of the board
        clone = board;
        // Make move if it's not illegal (or check)
        if (!makeMove(&board, moveList.list[i], AllMoves))
            continue;

        // Restore board state
        board = clone;

        dataCheck(moveList.list[i]);
    }
}
