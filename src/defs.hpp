#pragma once

#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#define VERSION "1.1"

#define _MY_ASSERT(condition, message) custom_assert(condition, message, __LINE__, __FILE__)
inline void custom_assert(bool cond, std::string msg, int line, std::string filename)
{
    if (!cond) {
        std::cout << "[ERROR]: Failed assert() at '" << filename << "' on line " << line << "\n";
        std::cout << "         Message: \"" << msg << "\"\n";
        std::terminate();
    }
}

// clang-format off
enum Sq {
    NOSQ = -1,
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1,
};
// clang-format on

#define ROW(sq) (((int)sq) >> 3)
#define COL(sq) (((int)sq) & 7)
#define SQ(r, f) (((int)r) * 8 + ((int)f))
#define FLIP(sq) (((int)sq) ^ 56)
#define COLORLESS(piece) (((int)piece) % 6)
#define SQCLR(r, f) (((r) + (f) + 1) & 1)

#define setBit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define getBit(bitboard, square) (((bitboard) & (1ULL << (square))) ? 1 : 0)
#define popBit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

/* Pieces */
enum Piece { wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK, EMPTY };
enum PieceTypes { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum Color { WHITE, BLACK, BOTH };

enum CastlingRights { c_wk, c_wq, c_bk, c_bq };

/* Direction offsets */
enum Direction {
    NORTH = 8,
    SOUTH = -8,
    WEST = -1,
    EAST = 1,
    NE = 9,     // NORTH + EAST
    NW = 7,     // NORTH + WEST
    SE = -7,    // SOUTH + EAST
    SW = -9,    // SOUTH + WEST
    NE_N = 17,  // 2(NORTH) + EAST -> 'KNIGHT ONLY'
    NE_E = 10,  // NORTH + 2(EAST) -> 'KNIGHT ONLY'
    NW_N = 15,  // 2(NORTH) + WEST -> 'KNIGHT ONLY'
    NW_W = 6,   // NORTH + 2(WEST) -> 'KNIGHT ONLY'
    SE_S = -15, // 2(SOUTH) + EAST -> 'KNIGHT ONLY'
    SE_E = -6,  // SOUTH + 2(EAST) -> 'KNIGHT ONLY'
    SW_S = -17, // 2(SOUTH) + WEST -> 'KNIGHT ONLY'
    SW_W = -10, // SOUTH + 2(WEST) -> 'KNIGHT ONLY'
};

extern const std::string PIECE_STR;
extern const std::string STR_COORDS[65];

// attack.cpp
extern uint64_t pawnAttacks[2][64];      // [color][square]
extern uint64_t knightAttacks[64];       // [square]
extern uint64_t kingAttacks[64];         // [square]
extern uint64_t bishopOccMasks[64];      // [square]
extern uint64_t bishopAttacks[64][512];  // [square][occupancy variations]
extern uint64_t rookOccMasks[64];        // [square]
extern uint64_t rookAttacks[64][4096];   // [square][occupancy variations]
extern const int bishopRelevantBits[64]; // [square]
extern const int rookRelevantBits[64];   // [square]

void initAttacks();
void initLeapers();
void initSliding(const PieceTypes piece);
void genPawnAttacks(const Color side, const int sq);
void genKnightAttacks(const int sq);
void genKingAttacks(const int sq);
uint64_t genBishopOccupancy(const int sq);
uint64_t genBishopAttack(const int sq, const uint64_t blockerBoard);
uint64_t genRookOccupancy(const int sq);
uint64_t genRookAttack(const int sq, const uint64_t blockerBoard);
uint64_t setOccupancy(const int index, const int relevantBits, uint64_t attackMask);

// bitboard.cpp
void printBits(const uint64_t bitboard);
inline int countBits(uint64_t bitboard)
{
    int count = 0;
    for (count = 0; bitboard; count++, bitboard &= bitboard - 1)
        ;
    return count;
}
inline int lsbIndex(const uint64_t bitboard)
{
    return bitboard > 0 ? countBits(bitboard ^ (bitboard - 1)) - 1 : 0;
}

// board.cpp
extern const std::string FEN_POSITIONS[8];
extern const uint8_t CASTLING_RIGHTS[64];

struct Board
{
  public:
    Board();
    void parseFen(const std::string& fen_str);

    Piece findPiece(const Sq sq) const;
    void updateUnits();
    void changeSide();
    void display() const;
    bool sqAttacked(Sq sq, Color color) const;
    bool inCheck() const;

  public:
    // Piece positions
    uint64_t pieces[12];
    uint64_t units[3];

    // State of the board
    Color side;
    Sq enpassant;
    uint8_t castling;
    uint32_t halfMoves;
    uint32_t fullMoves;

    // Position key
    uint64_t key;
    uint64_t lock;

    uint64_t repetitionTable[1000];
    int32_t repetitionIndex;

  private:
    const std::string formatCastling() const;
};

// book.cpp
struct PolyEntry
{
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;

    PolyEntry();
};

extern PolyEntry* bookEntries;

void initBook();
void deinitBook();
uint64_t genPolyKey(const Board& board);
int getBookMove(Board& board);

// eval.cpp
void initEvalMasks();
int evaluatePos(Board& board);

// move.cpp
enum MoveType { AllMoves, OnlyCaptures };

struct MoveList
{
    int list[256];
    short count = 0;
    MoveList();
    void add(const int move);
    int search(const int source, const int target, const int promoted = EMPTY) const;
    void print() const;
};

int encode(int source, int target, int piece, int promoted, bool isCapture, bool isTwoSquarePush,
           bool isEnpassant, bool isCastling);
int getSource(const int move);
int getTarget(const int move);
int getPiece(const int move);
int getPromoted(const int move);
bool isCapture(const int move);
bool isTwoSquarePush(const int move);
bool isEnpassant(const int move);
bool isCastling(const int move);
std::string moveToStr(const int move);
int parseMoveStr(const std::string& moveStr, const Board& board);
void genAllMoves(MoveList& moveList, const Board& board);
void genCaptureMoves(MoveList& moveList, const Board& board);
void generatePawns(MoveList& moveList, const Board& board, MoveType moveType);
void generateKnights(MoveList& moveList, const Board& board, MoveType moveType);
void generateBishops(MoveList& moveList, const Board& board, MoveType moveType);
void generateRooks(MoveList& moveList, const Board& board, MoveType moveType);
void generateQueens(MoveList& moveList, const Board& board, MoveType moveType);
void generateKings(MoveList& moveList, const Board& board, MoveType moveType);
void genWhiteCastling(MoveList& moveList, const Board& board);
void genBlackCastling(MoveList& moveList, const Board& board);
bool makeMove(Board* main, const int move, MoveType moveFlag);

// magics.cpp
extern const uint64_t BISHOP_MAGICS[64];
extern const uint64_t ROOK_MAGICS[64];

uint64_t random64();

uint64_t findMagicNumber(const int sq, const int relevantBits, const PieceTypes piece);
void initMagics();
uint64_t getBishopAttack(const int sq, uint64_t blockerBoard);
uint64_t getRookAttack(const int sq, uint64_t blockerBoard);
uint64_t getQueenAttack(const int sq, uint64_t blockerBoard);

// perft.cpp
uint64_t perftTest(Board& board, const int depth, MoveType moveType);

// search.cpp
const int INF = 50'000;
const int MATE_VALUE = 49'000;
const int MATE_SCORE = 48'000;

const int MAX_PLY = 64;
const int FULL_DEPTH_MOVES = 4;
const int REDUCTION_LIMIT = 3;

extern int ply;
extern int64_t nodes;

void searchPos(Board& board, const int depth);

// tt.cpp
enum TTFlag { F_EXACT, F_ALPHA, F_BETA };
const int NO_TT_ENTRY = 100'000;

// The entry structure for the transposition table
struct TT
{
    uint64_t key;  // "almost" unique chess position identifier
    uint64_t lock;
    int depth;     // current search depth
    TTFlag flag;   // flag the type of node (fail-low/fail-high/PV)
    int score;     // score (alpha/beta/PV)

    TT();
};

#define DEFAULT_TT_SIZE 128

#if 1
struct HashTable
{
    TT* table;
    int entryCount;
    int currentAge;

    HashTable();
    void init(int MB);
    void deinit();
    int read(Board& board, int alpha, int beta, int depth);
    void store(Board& board, int score, int depth, TTFlag flag);
    void clear();
    void resize(int newMB);
};

extern HashTable tt;

#else
extern TT* ttable;
void initTTable(int MB);
void deinitTTable();
void clearTTable();
int readTTEntry(Board& board, int alpha, int beta, int depth);
void writeTTEntry(Board& board, int score, int depth, TTFlag flag);
#endif

// uci.cpp
extern bool uciQuit;
extern bool uciStop;
extern bool uciUseBook;
extern bool uciDebugMode;

void uciLoop();
long long getCurrTime();
void parse(const std::string& command);
void parsePos(const std::string& command);
void parseGo(const std::string& command);
void parseParamI(const std::string& cmdArgs, const std::string& argName, int& output);
void parseParam(const std::string& cmdArgs, const std::string& argName, std::string& output);
void parseSetOption(const std::string& command);
void checkUp();
void printEngineID();
void printEngineOptions();
void printHelpInfo();

// zobrist.cpp
void initZobrist();
uint64_t genKey(const Board& board);
uint64_t genLock(const Board& board);
void updateZobristCastling(Board& board);
void updateZobristEnpassant(Board& board);
void updateZobristSide(Board& board);
void updateZobristPiece(Board& board, int p, int sq);
