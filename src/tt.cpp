#include "defs.hpp"

TT::TT() : key(0ULL), lock(0ULL), depth(0), flag(F_EXACT), score(0) {}

static const int ONE_MB = 0x100000;


#if 1
HashTable tt;

HashTable::HashTable() : table(nullptr), entryCount(0), currentAge(0) {}

void HashTable::init(int MB)
{
    _MY_ASSERT(MB >= 1, "Minimum size of transposition table is 1 MB");
    _MY_ASSERT(MB <= 1024, "Maximum size of transposition table is 1024 MB");

    const int HASH_SIZE = ONE_MB * MB;
    entryCount = HASH_SIZE / sizeof(TT);

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

void HashTable::deinit() { delete[] table; }

void HashTable::clear()
{
    for (int i = 0; i < entryCount; i++) {
        table[i] = TT();
    }
}

// read hash entry data
int HashTable::read(Board& board, int alpha, int beta, int depth)
{
    TT entry = table[board.key % entryCount];

    // make sure we're dealing with the exact position we need
    if (entry.key == board.key && entry.lock == board.lock) {
        // make sure that we match the exact depth our search is now at
        if (entry.depth >= depth) {
            // extract stored score from TT entry
            int score = entry.score;

            // retrieve score independent from the actual path
            // from root node (position) to current node (position)
            if (score < -MATE_SCORE)
                score += ply;
            if (score > MATE_SCORE)
                score -= ply;

            // match the exact (PV node) score
            if (entry.flag == F_EXACT)
                // return exact (PV node) score
                return score;

            // match alpha (fail-low node) score
            if ((entry.flag == F_ALPHA) && (score <= alpha))
                // return alpha (fail-low node) score
                return alpha;

            // match beta (fail-high node) score
            if ((entry.flag == F_BETA) && (score >= beta))
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

    // store score independent from the actual path
    // from root node (position) to current node (position)
    if (score < -MATE_SCORE)
        score -= ply;
    if (score > MATE_SCORE)
        score += ply;

    // write hash entry data
    entry->key = board.key;
    entry->lock = board.lock;
    entry->score = score;
    entry->flag = flag;
    entry->depth = depth;
}

#else
static int HashEntries;
TT* ttable = nullptr;
void initTTable(int MB)
{
    _MY_ASSERT(MB >= 1, "Minimum size of transposition table is 1 MB");
    _MY_ASSERT(MB <= 128, "Maximum size of transposition table is 128 MB");

    const int HASH_SIZE = ONE_MB * MB;
    HashEntries = HASH_SIZE / sizeof(TT);

    if (ttable != nullptr) {
        std::cout << "Clearing memory!\n";
        deinitTTable();
    }

    ttable = new TT[HashEntries];
    if (ttable == nullptr) {
        int reducedSize = HASH_SIZE / 2;
        std::cout << "[ERROR]: Couldn't allocate " << HASH_SIZE << "MB\n";
        std::cout << "[ INFO]: Trying to allocate " << reducedSize << "MB\n";
        initTTable(reducedSize);
    }

    clearTTable();
    std::cout << "Transposition table initialized with size of " << MB << " MB(" << HashEntries
              << " entries)\n";
}

void deinitTTable() { delete[] ttable; }

void clearTTable()
{
    for (int i = 0; i < HashEntries; i++) {
        ttable[i] = TT();
    }
}

// read hash entry data
int readTTEntry(Board& board, int alpha, int beta, int depth)
{
    TT entry = ttable[board.key % HashEntries];

    // make sure we're dealing with the exact position we need
    if (entry.key == board.key && entry.lock == board.lock) {
        // make sure that we match the exact depth our search is now at
        if (entry.depth >= depth) {
            // extract stored score from TT entry
            int score = entry.score;

            // retrieve score independent from the actual path
            // from root node (position) to current node (position)
            if (score < -MATE_SCORE)
                score += ply;
            if (score > MATE_SCORE)
                score -= ply;

            // match the exact (PV node) score
            if (entry.flag == F_EXACT)
                // return exact (PV node) score
                return score;

            // match alpha (fail-low node) score
            if ((entry.flag == F_ALPHA) && (score <= alpha))
                // return alpha (fail-low node) score
                return alpha;

            // match beta (fail-high node) score
            if ((entry.flag == F_BETA) && (score >= beta))
                // return beta (fail-high node) score
                return beta;
        }
    }

    // if hash entry doesn't exist
    return NO_TT_ENTRY;
}

// write hash entry data
void writeTTEntry(Board& board, int score, int depth, TTFlag flag)
{
    TT* entry = &ttable[board.key % HashEntries];

    // store score independent from the actual path
    // from root node (position) to current node (position)
    if (score < -MATE_SCORE)
        score -= ply;
    if (score > MATE_SCORE)
        score += ply;

    // write hash entry data
    entry->key = board.key;
    entry->lock = board.lock;
    entry->score = score;
    entry->flag = flag;
    entry->depth = depth;
}
#endif