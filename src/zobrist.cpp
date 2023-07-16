#include "defs.hpp"

uint64_t pieceKeys[12][64];
uint64_t enpassKeys[8];
uint64_t castlingKeys[16];
uint64_t sideKey;

uint64_t pieceLocks[12][64];
uint64_t enpassLocks[8];
uint64_t castlingLocks[16];
uint64_t sideLock;

void initZobrist()
{
    // Init piece keys and locks
    for (int piece = wP; piece <= bK; piece++) {
        for (int sq = 0; sq <= 63; sq++) {
            pieceKeys[piece][sq] = random64();
            pieceLocks[piece][sq] = random64();
        }
    }

    // Init enpassant files keys and locks
    for (int f = 0; f < 8; f++) {
        enpassKeys[f] = random64();
        enpassLocks[f] = random64();
    }

    // Init keys for the different castling rights variations
    for (int i = 0; i < 16; i++) {
        castlingKeys[i] = random64();
        castlingLocks[i] = random64();
    }

    sideKey = random64();
    sideLock = random64();
}

void updateZobristCastling(Board& board)
{
    board.key ^= castlingKeys[board.castling];
    board.lock ^= castlingLocks[board.castling];
}

void updateZobristEnpassant(Board& board)
{
    board.key ^= enpassKeys[COL(board.enpassant)];
    board.lock ^= enpassLocks[COL(board.enpassant)];
}

void updateZobristSide(Board& board)
{
    board.key ^= sideKey;
    board.lock ^= sideLock;
}

void updateZobristPiece(Board& board, int p, int sq)
{
    board.key ^= pieceKeys[p][sq];
    board.lock ^= pieceLocks[p][sq];
}

uint64_t genKey(const Board& board)
{
    // Reset lock before generating
    uint64_t output = 0ULL;
    // Hash pieces on squares
    int sq;
    uint64_t bitboardCopy;
    for (int piece = wP; piece <= bK; piece++) {
        bitboardCopy = board.pieces[piece];
        while (bitboardCopy) {
            sq = lsbIndex(bitboardCopy);
            output ^= pieceKeys[piece][sq];
            popBit(bitboardCopy, sq);
        }
    }
    // Hash enpassant square
    if (board.enpassant != NOSQ)
        output ^= enpassKeys[COL(board.enpassant)];
    // Hash castling rights
    output ^= castlingKeys[board.castling];
    // Hash side to move
    if (board.side == BLACK)
        output ^= sideKey;
    return output;
}

uint64_t genLock(const Board& board)
{
    // Reset lock before generating
    uint64_t output = 0ULL;
    // Hash pieces on squares
    int sq;
    uint64_t bitboardCopy;
    for (int piece = wP; piece <= bK; piece++) {
        bitboardCopy = board.pieces[piece];
        while (bitboardCopy) {
            sq = lsbIndex(bitboardCopy);
            output ^= pieceLocks[piece][sq];
            popBit(bitboardCopy, sq);
        }
    }
    // Hash enpassant square
    if (board.enpassant != NOSQ)
        output ^= enpassLocks[COL(board.enpassant)];
    // Hash castling rights
    output ^= castlingLocks[board.castling];
    // Hash side to move
    if (board.side == BLACK)
        output ^= sideLock;
    return output;
}