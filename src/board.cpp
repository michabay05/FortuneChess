#include "defs.hpp"

const std::string PIECE_STR = "PNBRQKpnbrqk ";

const std::string STR_COORDS[65] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "a7", "b7", "c7", "d7", "e7",
    "f7", "g7", "h7", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "a5", "b5",
    "c5", "d5", "e5", "f5", "g5", "h5", "a4", "b4", "c4", "d4", "e4", "f4", "g4",
    "h4", "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a2", "b2", "c2", "d2",
    "e2", "f2", "g2", "h2", "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", " " };

const uint8_t CASTLING_RIGHTS[64] = {
    7,  15, 15, 15, 3,  15, 15, 11, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 13, 15, 15, 15, 12, 15, 15, 14 };

const std::string FEN_POSITIONS[8] = {
    "8/8/8/8/8/8/8/8 w - - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 b - - 0 1",
    "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
};

Board::Board()
{
    memset(pieces, 0ULL, sizeof(pieces));
    memset(units, 0ULL, sizeof(units));
    memset(repetitionTable, 0ULL, sizeof(repetitionTable));

    side = WHITE;
    castling = 0;
    enpassant = NOSQ;
    halfMoves = 0;
    fullMoves = 1;
    key = 0;
    lock = 0;
    repetitionIndex = 0;
}

Piece Board::findPiece(const Sq sq) const
{
    for (int i = 0; i < 12; i++) {
        if (getBit(pieces[i], sq))
            return (Piece)i;
    }
    return EMPTY;
}

void Board::updateUnits()
{
    memset(units, 0, sizeof(units));
    for (int piece = PAWN; piece <= KING; piece++) {
        units[WHITE] |= pieces[piece];
        units[BLACK] |= pieces[piece + 6];
    }
    units[BOTH] = units[WHITE] | units[BLACK];
}

void Board::changeSide() { side = (Color)((int)side ^ 1); }

void Board::display() const
{
    std::cout << "\n    +---+---+---+---+---+---+---+---+\n";
    for (int r = 0; r < 8; r++) {
        std::cout << "  " << 8 - r << " |";
        for (int f = 0; f < 8; f++)
            std::cout << " " << PIECE_STR[findPiece((Sq)SQ(r, f))] << " |";
        std::cout << "\n    +---+---+---+---+---+---+---+---+\n";
    }
    std::cout << "      a   b   c   d   e   f   g   h\n\n";
    std::cout << "      Side to move: " << (side == WHITE ? "white" : "black") << "\n";
    std::cout << "         Castling: " << formatCastling() << "\n";
    std::cout << "         Enpassant: "
        << (enpassant != NOSQ ? STR_COORDS[enpassant] : "none") << "\n";
}

const std::string Board::formatCastling() const
{
    std::string castling_str = "----";
    if (castling & (1 << c_wk))
        castling_str[0] = 'K';
    if (castling & (1 << c_wq))
        castling_str[1] = 'Q';
    if (castling & (1 << c_bk))
        castling_str[2] = 'k';
    if (castling & (1 << c_bq))
        castling_str[3] = 'q';

    return castling_str;
}

bool Board::sqAttacked(Sq sq, Color color) const
{
    // Attacked by white pawns
    if ((color == WHITE) &&
        (pawnAttacks[BLACK][sq] & pieces[wP]))
        return true;
    // Attacked by black pawns
    if ((color == BLACK) &&
        (pawnAttacks[WHITE][sq] & pieces[bP]))
        return true;
    // Attacked by knights
    if ((knightAttacks[sq] & pieces[color == WHITE ? wN : bN]) != 0)
        return true;
    // Attacked by bishops
    if ((getBishopAttack(sq, units[BOTH]) & pieces[color == WHITE ? wB : bB]) != 0)
        return true;
    // Attacked by rooks
    if ((getRookAttack(sq, units[BOTH]) & pieces[color == WHITE ? wR : bR]) != 0)
        return true;
    // Attacked by queens
    if ((getQueenAttack(sq, units[BOTH]) & pieces[color == WHITE ? wQ : bQ]) != 0)
        return true;
    // Attacked by kings
    if ((kingAttacks[sq] & pieces[color == WHITE ? wK : bK]) != 0)
        return true;

    // If all of the above cases fail, return false
    return false;
}

bool Board::inCheck() const
{
    uint8_t piece = side == WHITE ? bK : wK;
    return sqAttacked((Sq)lsbIndex(pieces[piece]), side);
}

void Board::parseFen(const std::string& fen)
{
    *this = Board();

    int currIndex = 0;

    // Parse the piece portion of the fen and place them on the board
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            if (fen[currIndex] == ' ')
                break;
            if (fen[currIndex] == '/')
                currIndex++;
            if (fen[currIndex] >= '0' && fen[currIndex] <= '8') {
                file += (fen[currIndex] - '0');
                currIndex++;
            }
            if ((fen[currIndex] >= 'A' && fen[currIndex] <= 'Z') ||
                (fen[currIndex] >= 'a' && fen[currIndex] <= 'z')) {
                size_t piece;
                if ((piece = PIECE_STR.find(fen[currIndex])) != EMPTY) {
                    setBit(pieces[piece], SQ(rank, file));
                }
                currIndex++;
            }
        }
    }
    currIndex++;
    // Parse side to move
    if (fen[currIndex] == 'w') {
        side = WHITE;
    } else if (fen[currIndex] == 'b') {
        side = BLACK;
    }
    currIndex += 2;

    // Parse castling rights
    // std::string ab = fen.substr(currIndex);
    // std::cout << "'" << ab << "'\n";
    while (fen[currIndex] != ' ') {
        switch (fen[currIndex]) {
        case 'K':
            castling |= (1 << c_wk);
            break;
        case 'Q':
            castling |= (1 << c_wq);
            break;
        case 'k':
            castling |= (1 << c_bk);
            break;
        case 'q':
            castling |= (1 << c_bq);
            break;
        }
        currIndex++;
    }
    currIndex++;

    // Parse enpassant square
    if (fen[currIndex] != '-') {
        int f = fen[currIndex] - 'a';
        currIndex++;
        int r = 8 - (fen[currIndex] - '0');
        enpassant = (Sq)SQ(r, f);
        currIndex++;
    }
    else {
        enpassant = NOSQ;
        currIndex++;
    }
    currIndex++;

    size_t count;
    // Parse the number of half moves
    std::string spaceInd = fen.substr(currIndex);
    count = spaceInd.find(" ");
    halfMoves = atoi(fen.substr(currIndex, count - 1).c_str());
    currIndex += (int)count;

    // Parse the number of full moves
    spaceInd = fen.substr(currIndex);
    count = spaceInd.find(" ");
    fullMoves = atoi(fen.substr(currIndex, count - 1).c_str());

    // Update occupancy bit
    updateUnits();

    key = genKey(*this);
    lock = genLock(*this);
}