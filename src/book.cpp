#include "book_consts.hpp"
#include "defs.hpp"

#include <fstream>

PolyEntry* bookEntries = nullptr;
static int BookEntriesCount;

void initBook()
{
    if (!uciUseBook)
        return;

    FILE* bookFile;
    fopen_s(&bookFile, "performance.bin", "rb");
    if (bookFile == NULL) {
        std::cout << "[ERROR]: Failed to *open* the opening book in.\n";
        return;
    }
    fseek(bookFile, 0, SEEK_END);
    int32_t fileSize = ftell(bookFile);
    if (fileSize < sizeof(PolyEntry)) {
        std::cout << "[ERROR]: No entry found!\n";
        uciUseBook = false;
        return;
    }
    BookEntriesCount = fileSize / sizeof(PolyEntry);
    
    bookEntries = new PolyEntry[BookEntriesCount];
    if (bookEntries == nullptr) {
        std::cout << "[ERROR]: Failed to *read* the opening book in.\n";
        return;
    }

    fread(bookEntries, sizeof(PolyEntry), BookEntriesCount, bookFile);
    char mbStr[6];
    snprintf(mbStr, 5, "%1.2f", (fileSize / 1'000'000.f));
    std::cout << "Opening book initialized with size of " << mbStr << " MB("
              << BookEntriesCount << " entries)\n";

    fclose(bookFile);
}

void deinitBook() { delete[] bookEntries; }

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