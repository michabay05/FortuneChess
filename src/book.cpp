#include "book_consts.hpp"
#include "defs.hpp"

#include <fstream>

PolyEntry* bookEntries = nullptr;
static int BookEntriesCount;

bool outOfBookMoves;

void initBook()
{
    if (!sInfo.useBook)
        return;

    FILE* bookFile;
    fopen_s(&bookFile, "performance.bin", "rb");
    if (bookFile == NULL) {
        std::cout << "[ERROR]: Failed to *open* the opening book in.\n";
        sInfo.useBook = false;
        return;
    }
    fseek(bookFile, 0, SEEK_END);
    int32_t fileSize = ftell(bookFile);
    if (fileSize < sizeof(PolyEntry)) {
        std::cout << "[ERROR]: No entry found!\n";
        sInfo.useBook = false;
        return;
    }
    BookEntriesCount = fileSize / sizeof(PolyEntry);

    bookEntries = new PolyEntry[BookEntriesCount];
    if (bookEntries == nullptr) {
        std::cout << "[ERROR]: Failed to *read* the opening book in.\n";
        sInfo.useBook = false;
        return;
    }

    fseek(bookFile, 0, SEEK_SET);
    int readEntries = (int)fread(bookEntries, sizeof(PolyEntry), BookEntriesCount, bookFile);
    if (readEntries == BookEntriesCount) {
        char mbStr[6];
        snprintf(mbStr, 5, "%1.2f", (fileSize / 1'000'000.f));
        if (sInfo.debugMode) {
			std::cout << "Opening book initialized with size of " << mbStr << " MB(" << BookEntriesCount
					  << " entries)\n";
        }
    }

    fclose(bookFile);
}

void deinitBook()
{
    if (sInfo.debugMode)
        std::cout << "Deinitialized the opening book\n";
    delete[] bookEntries;
}

PolyEntry::PolyEntry() : key(0ULL), move(0), weight(0), learn(0) {}

static const int POLY_PIECE[] = {1, 3, 5, 7, 9, 11, 0, 2, 4, 6, 8, 10};

static bool hasPawnForCapture(const Board& board)
{
    Piece pawnType;
    int rankOffset;
    if (board.side == WHITE) {
        pawnType = wP;
        rankOffset = 1;
    } else {
        pawnType = bP;
        rankOffset = -1;
    }
    int enpassRow = ROW(board.enpassant) + rankOffset;
    int leftSide = COL(board.enpassant) - 1;
    int rightSide = COL(board.enpassant) + 1;

    if ((leftSide > 0 && getBit(board.pieces[pawnType], SQ(enpassRow, leftSide))) ||
        (rightSide < 8 && getBit(board.pieces[pawnType], SQ(enpassRow, rightSide)))) {
        return true;
    }
    return false;
}

uint64_t genPolyKey(const Board& board)
{
    uint64_t finalPolyKey = 0;
    uint64_t bbCopy = 0;
    int sq, polyIndex;
    // Piece hashing
    for (int p = wP; p <= bK; p++) {
        bbCopy = board.pieces[p];
        while (bbCopy) {
            sq = lsbIndex(bbCopy);
            popBit(bbCopy, sq);

            sq = FLIP(sq);
            polyIndex = (64 * POLY_PIECE[p]) + (8 * ROW(sq) + COL(sq));
            finalPolyKey ^= POLY_RANDOM64[polyIndex];
        }
    }

    // Castling right hashing
    polyIndex = 768;
    for (uint8_t i = 0; i < 4; i++) {
        if (board.castling & (1 << i)) {
            finalPolyKey ^= POLY_RANDOM64[polyIndex + i];
        }
    }

    // Enpassant square hashing
    polyIndex = 772;
    if (board.enpassant != NOSQ && hasPawnForCapture(board)) {
        finalPolyKey ^= POLY_RANDOM64[polyIndex + COL(board.enpassant)];
    }

    // Side hashing
    if (board.side == WHITE) {
        finalPolyKey ^= POLY_RANDOM64[780];
    }

    return finalPolyKey;
}

static uint16_t endianSwap(uint16_t x)
{
    x = (x >> 8) | (x << 8);
    return x;
}

static uint64_t endianSwap(uint64_t x)
{
    x = (x >> 56) | ((x << 40) & 0x00FF000000000000) | ((x << 24) & 0x0000FF0000000000) |
        ((x << 8) & 0x000000FF00000000) | ((x >> 8) & 0x00000000FF000000) |
        ((x >> 24) & 0x0000000000FF0000) | ((x >> 40) & 0x000000000000FF00) | (x << 56);
    return x;
}

static int toInternalMove(const uint16_t polyMove, Board& board)
{
    int sourceFile = (polyMove >> 6) & 7;
    int sourceRow = (polyMove >> 9) & 7;
    int targetFile = (polyMove >> 0) & 7;
    int targetRow = (polyMove >> 3) & 7;
    int promotedPiece = (polyMove >> 12) & 7;

    int promoted;
    switch (promotedPiece) {
    case 1:
        promoted = KNIGHT;
        break;
    case 2:
        promoted = BISHOP;
        break;
    case 3:
        promoted = ROOK;
        break;
    case 4:
        promoted = QUEEN;
        break;
    default:
        promoted = EMPTY;
        break;
    }

    MoveList ml;
    genAllMoves(ml, board);

    int flippedSource = FLIP(SQ(sourceRow, sourceFile));
    int flippedTarget = FLIP(SQ(targetRow, targetFile));
    return ml.search(flippedSource, flippedTarget, promoted);
}

int getBookMove(Board& board)
{
    uint64_t polyKey = genPolyKey(board);
    // There should be a maximum of 32 book moves for any given position
    const int MAX_BOOK_MOVES = 32;
    int bookMoves[MAX_BOOK_MOVES];
    int bookMoveCount = 0;

    uint16_t entryMove;
    int internalMove;
    for (int i = 0; i < BookEntriesCount; i++) {
        if (polyKey != endianSwap(bookEntries[i].key))
            continue;
        entryMove = endianSwap(bookEntries[i].move);
        if (entryMove == 0)
            continue;
        internalMove = toInternalMove(entryMove, board);
        if (internalMove == 0)
            continue;
        bookMoves[bookMoveCount] = internalMove;
        bookMoveCount++;
        if (bookMoveCount >= MAX_BOOK_MOVES)
            break;
    }

    if (sInfo.debugMode) {
		for (int i = 0; i < bookMoveCount; i++)
			std::cout << moveToStr(bookMoves[i]) << "\n";
    }

    if (bookMoveCount) {
        int randIndex = rand() % bookMoveCount;
        return bookMoves[randIndex];
    } else {
        outOfBookMoves = true;
        return 0;
    }
}
