#include "defs.hpp"

MoveList::MoveList()
{
    memset(list, 0, sizeof(list));
    count = 0;
}

void MoveList::add(const int move)
{
    list[count] = move;
    count++;
}

// Returns index from move list, if move is found
int MoveList::search(const int source, const int target, const int promoted) const
{
    for (int i = 0; i < count; i++) {
        // Parse move info
        int listMoveSource = getSource(list[i]);
        int listMoveTarget = getTarget(list[i]);
        int listMovePromoted = getPromoted(list[i]);
        // Check if source and target match
        if (listMoveSource == source && listMoveTarget == target && listMovePromoted == promoted)
            // Return index of move from movelist, if true
            return list[i];
    }
    return 0;
}
void MoveList::print() const
{
    printf("    Source   |   Target  |  Piece  |  Promoted  |  Capture  |  Two "
           "Square Push  |  Enpassant  |  Castling\n");
    printf("  "
           "---------------------------------------------------------------------"
           "--------------------------------------\n");
    for (int i = 0; i < count; i++) {
        printf("       %s    |    %s     |    %c    |     %c      |     %d     |   "
               "      %d         |      %d      |     %d\n",
               STR_COORDS[getSource(list[i])].c_str(), STR_COORDS[getTarget(list[i])].c_str(),
               PIECE_STR[getPiece(list[i])], PIECE_STR[getPromoted(list[i])], isCapture(list[i]),
               isTwoSquarePush(list[i]), isEnpassant(list[i]), isCastling(list[i]));
    }
    std::cout << "\n    Total number of moves: " << std::to_string(count) << "\n";
}

int encode(int source, int target, int piece, int promoted, bool isCapture, bool isTwoSquarePush,
           bool isEnpassant, bool isCastling)
{
    return source | (target << 6) | (piece << 12) | (promoted << 16) | (isCapture << 20) |
           (isTwoSquarePush << 21) | (isEnpassant << 22) | (isCastling << 23);
}

int getSource(const int move) { return move & 0x3F; }

int getTarget(const int move) { return (move & 0xFC0) >> 6; }

int getPiece(const int move) { return (move & 0xF000) >> 12; }

int getPromoted(const int move)
{
    int promoted = (move & 0xF0000) >> 16;
    return promoted ? promoted : EMPTY;
}

bool isCapture(const int move) { return move & 0x100000; }

bool isTwoSquarePush(const int move) { return move & 0x200000; }

bool isEnpassant(const int move) { return move & 0x400000; }

bool isCastling(const int move) { return move & 0x800000; }

std::string moveToStr(const int move)
{
    std::string moveStr = STR_COORDS[getSource(move)];
    moveStr += STR_COORDS[getTarget(move)];
    int piece;
    if ((piece = getPromoted(move)) != EMPTY)
        moveStr += std::tolower(PIECE_STR[piece]);
    return moveStr;
}

int parseMoveStr(const std::string& moveStr, const Board& board)
{
    int source = SQ((8 - (moveStr[1] - '0')), (moveStr[0] - 'a'));
    int target = SQ((8 - (moveStr[3] - '0')), (moveStr[2] - 'a'));
    int promoted = EMPTY;
    if (moveStr.length() == 5) {
        promoted = (int)PIECE_STR.find(moveStr[4]);
        if (ROW(target) == 0)
            promoted %= 6;
    }
    MoveList mL;
    genAllMoves(mL, board);
    int searchedMove = mL.search(source, target, promoted);
    return searchedMove;
}

static void generate(MoveList& moveList, const Board& board, MoveType moveType)
{
    moveList = MoveList();
    generatePawns(moveList, board, moveType);
    generateKnights(moveList, board, moveType);
    generateBishops(moveList, board, moveType);
    generateRooks(moveList, board, moveType);
    generateQueens(moveList, board, moveType);
    generateKings(moveList, board, moveType);
}

void genAllMoves(MoveList& moveList, const Board& board) { generate(moveList, board, AllMoves); }

void genCaptureMoves(MoveList& moveList, const Board& board) { generate(moveList, board, OnlyCaptures); }

void generatePawns(MoveList& moveList, const Board& board, MoveType moveType)
{
    uint64_t bitboardCopy, attackCopy;
    int promotionStart, direction, doublePushStart, piece;
    int source, target;
    // If side to move is white
    if (board.side == WHITE) {
        piece = wP;
        promotionStart = A7;
        direction = SOUTH;
        doublePushStart = A2;
    }
    // If side to move is black
    else {
        piece = bP;
        promotionStart = A2;
        direction = NORTH;
        doublePushStart = A7;
    }

    bitboardCopy = board.pieces[piece];

    while (bitboardCopy) {
        source = lsbIndex(bitboardCopy);
        target = source + direction;
        if (moveType != OnlyCaptures) {
			if ((board.side == WHITE ? target >= A8 : target <= H1) &&
				!getBit(board.units[BOTH], target)) {
				// Quiet moves
				// Promotion
				if ((source >= promotionStart) && (source <= promotionStart + 7)) {
					moveList.add(
						encode(source, target, piece, (board.side == WHITE ? wQ : bQ), 0, 0, 0, 0));
					moveList.add(
						encode(source, target, piece, (board.side == WHITE ? wR : bR), 0, 0, 0, 0));
					moveList.add(
						encode(source, target, piece, (board.side == WHITE ? wB : bB), 0, 0, 0, 0));
					moveList.add(
						encode(source, target, piece, (board.side == WHITE ? wN : bN), 0, 0, 0, 0));
				} else {
					moveList.add(encode(source, target, piece, EMPTY, 0, 0, 0, 0));
					if ((source >= doublePushStart && source <= doublePushStart + 7) &&
						!getBit(board.units[BOTH], target + direction))
						moveList.add(encode(source, target + direction, piece, EMPTY, 0, 1, 0, 0));
				}
			}
        }
        // Capture moves
        attackCopy = pawnAttacks[board.side][source] & board.units[board.side ^ 1];
        while (attackCopy) {
            target = lsbIndex(attackCopy);
            // Capture move
            if ((source >= promotionStart) && (source <= promotionStart + 7)) {
                moveList.add(
                    encode(source, target, piece, (board.side == WHITE ? wQ : bQ), 1, 0, 0, 0));
                moveList.add(
                    encode(source, target, piece, (board.side == WHITE ? wR : bR), 1, 0, 0, 0));
                moveList.add(
                    encode(source, target, piece, (board.side == WHITE ? wB : bB), 1, 0, 0, 0));
                moveList.add(
                    encode(source, target, piece, (board.side == WHITE ? wN : bN), 1, 0, 0, 0));
            } else
                moveList.add(encode(source, target, piece, EMPTY, 1, 0, 0, 0));
            // Remove 'source' bit
            popBit(attackCopy, target);
        }
        // Generate enpassant capture
        if (board.enpassant != NOSQ) {
            uint64_t enpassCapture = pawnAttacks[board.side][source] & (1ULL << board.enpassant);
            if (enpassCapture) {
                int enpassTarget = lsbIndex(enpassCapture);
                moveList.add(encode(source, enpassTarget, piece, EMPTY, 1, 0, 1, 0));
            }
        }
        // Remove bits
        popBit(bitboardCopy, source);
    }
}

void generateKnights(MoveList& moveList, const Board& board, MoveType moveType)
{
    int source, target, piece = board.side == WHITE ? wN : bN;
    uint64_t bitboardCopy = board.pieces[piece], attackCopy;
    while (bitboardCopy) {
        source = lsbIndex(bitboardCopy);

        attackCopy = knightAttacks[source] & (~board.units[board.side]);
        while (attackCopy) {
            target = lsbIndex(attackCopy);
            bool isCapture = getBit(board.units[board.side == WHITE ? BLACK : WHITE], target);
            if (!isCapture && moveType == OnlyCaptures) {
				popBit(attackCopy, target);
                continue;
            }
			moveList.add(encode(source, target, piece, EMPTY, isCapture, 0, 0, 0));
            popBit(attackCopy, target);
        }
        popBit(bitboardCopy, source);
    }
}

void generateBishops(MoveList& moveList, const Board& board, MoveType moveType)
{
    int source, target, piece = board.side == WHITE ? wB : bB;
    uint64_t bitboardCopy = board.pieces[piece], attackCopy;
    while (bitboardCopy) {
        source = lsbIndex(bitboardCopy);

        attackCopy = getBishopAttack(source, board.units[BOTH]) &
                     (board.side == WHITE ? ~board.units[WHITE] : ~board.units[BLACK]);
        while (attackCopy) {
            target = lsbIndex(attackCopy);
            bool isCapture = getBit(board.units[board.side == WHITE ? BLACK : WHITE], target);
            if (!isCapture && moveType == OnlyCaptures) {
				popBit(attackCopy, target);
                continue;
            }
			moveList.add(encode(source, target, piece, EMPTY, isCapture, 0, 0, 0));
            popBit(attackCopy, target);
        }
        popBit(bitboardCopy, source);
    }
}

void generateRooks(MoveList& moveList, const Board& board, MoveType moveType)
{
    int source, target, piece = board.side == WHITE ? wR : bR;
    uint64_t bitboardCopy = board.pieces[piece], attackCopy;
    while (bitboardCopy) {
        source = lsbIndex(bitboardCopy);

        attackCopy = getRookAttack(source, board.units[BOTH]) &
                     (board.side == WHITE ? ~board.units[WHITE] : ~board.units[BLACK]);
        while (attackCopy) {
            target = lsbIndex(attackCopy);
            bool isCapture = getBit(board.units[board.side == WHITE ? BLACK : WHITE], target);
            if (!isCapture && moveType == OnlyCaptures) {
				popBit(attackCopy, target);
                continue;
            }
			moveList.add(encode(source, target, piece, EMPTY, isCapture, 0, 0, 0));
            popBit(attackCopy, target);
        }
        popBit(bitboardCopy, source);
    }
}

void generateQueens(MoveList& moveList, const Board& board, MoveType moveType)
{
    int source, target, piece = board.side == WHITE ? wQ : bQ;
    uint64_t bitboardCopy = board.pieces[piece], attackCopy;
    while (bitboardCopy) {
        source = lsbIndex(bitboardCopy);

        attackCopy = getQueenAttack(source, board.units[BOTH]) &
                     (board.side == WHITE ? ~board.units[WHITE] : ~board.units[BLACK]);
        while (attackCopy) {
            target = lsbIndex(attackCopy);
            bool isCapture = getBit(board.units[board.side == WHITE ? BLACK : WHITE], target);
            if (!isCapture && moveType == OnlyCaptures) {
				popBit(attackCopy, target);
                continue;
            }
			moveList.add(encode(source, target, piece, EMPTY, isCapture, 0, 0, 0));
            popBit(attackCopy, target);
        }
        popBit(bitboardCopy, source);
    }
}

void generateKings(MoveList& moveList, const Board& board, MoveType moveType)
{
    /* NOTE: Checks aren't handled by the move generator,
             it's handled by the make move function.
    */
    int source, target, piece = board.side == WHITE ? wK : bK;
    uint64_t bitboard = board.pieces[piece], attackCopy;
    while (bitboard != 0) {
        source = lsbIndex(bitboard);

        attackCopy =
            kingAttacks[source] & (board.side == WHITE ? ~board.units[WHITE] : ~board.units[BLACK]);
        while (attackCopy != 0) {
            target = lsbIndex(attackCopy);
            bool isCapture = getBit(board.units[board.side == WHITE ? BLACK : WHITE], target);
            if (!isCapture && moveType == OnlyCaptures) {
				popBit(attackCopy, target);
                continue;
            }
			moveList.add(encode(source, target, piece, EMPTY, isCapture, 0, 0, 0));
            // Remove target bit to move onto the next bit
            popBit(attackCopy, target);
        }
        // Remove source bit to move onto the next bit
        popBit(bitboard, source);
    }
    // Generate castling moves
    if (moveType != OnlyCaptures) {
		if (board.side == WHITE)
			genWhiteCastling(moveList, board);
		else
			genBlackCastling(moveList, board);
    }
}

void genWhiteCastling(MoveList& moveList, const Board& board)
{
	// Kingside castling
	if (board.castling & (1 << c_wk)) {
		// Check if path is obstructed
		if (!getBit(board.units[BOTH], F1) && !getBit(board.units[BOTH], G1)) {
			// Is e1 or f1 attacked by a black piece?
			if (!board.sqAttacked(E1, BLACK) && !board.sqAttacked(F1, BLACK))
				moveList.add(encode(E1, G1, wK, EMPTY, 0, 0, 0, 1));
		}
	}
	// Queenside castling
	if (getBit(board.castling, c_wq)) {
		// Check if path is obstructed
		if (!getBit(board.units[BOTH], B1) && !getBit(board.units[BOTH], C1) &&
			!getBit(board.units[BOTH], D1)) {
			// Is d1 or e1 attacked by a black piece?
			if (!board.sqAttacked(D1, BLACK) && !board.sqAttacked(E1, BLACK))
				moveList.add(encode(E1, C1, wK, EMPTY, 0, 0, 0, 1));
		}
	}
}
void genBlackCastling(MoveList& moveList, const Board& board)
{
    // Kingside castling
    if (getBit(board.castling, c_bk)) {
        // Check if path is obstructed
        if (!getBit(board.units[BOTH], F8) && !getBit(board.units[BOTH], G8)) {
            // Is e8 or f8 attacked by a white piece?
            if (!board.sqAttacked(E8, WHITE) && !board.sqAttacked(F8, WHITE))
                moveList.add(encode(E8, G8, bK, EMPTY, 0, 0, 0, 1));
        }
    }
    // Queenside castling
    if (getBit(board.castling, c_bq)) {
        // Check if path is obstructed
        if (!getBit(board.units[BOTH], B8) && !getBit(board.units[BOTH], C8) &&
            !getBit(board.units[BOTH], D8)) {
            // Is d8 or e8 attacked by a white piece?
            if (!board.sqAttacked(D8, WHITE) && !board.sqAttacked(E8, WHITE))
                moveList.add(encode(E8, C8, bK, EMPTY, 0, 0, 0, 1));
        }
    }
}

bool makeMove(Board* main, const int move, MoveType moveFlag)
{
    if (moveFlag == AllMoves) {
        Board clone = *main;

        // Parse move information
        int source = getSource(move);
        int target = getTarget(move);
        int piece = getPiece(move);
        int promoted = getPromoted(move);
        bool capture = isCapture(move);
        bool twoSquarePush = isTwoSquarePush(move);
        bool enpassant = isEnpassant(move);
        bool castling = isCastling(move);

        // Remove piece from 'source' and place on 'target'
        popBit(main->pieces[piece], source);
        updateZobristPiece(*main, piece, source);

        setBit(main->pieces[piece], target);
        updateZobristPiece(*main, piece, target);

        // If capture, remove piece of opponent bitboard
        if (capture) {
            for (int bbPiece = (main->side == WHITE ? bP : wP);
                 bbPiece <= (main->side == WHITE ? bK : wK); bbPiece++) {
                if (getBit(main->pieces[bbPiece], target)) {
                    popBit(main->pieces[bbPiece], target);
                    updateZobristPiece(*main, bbPiece, target);
                    break;
                }
            }
        }

        // Promotion move
        if (promoted != EMPTY) {
            int pawnType = piece == 0 ? (int)wP : (int)bP;
            popBit(main->pieces[pawnType], target);
            updateZobristPiece(*main, pawnType, target);

            setBit(main->pieces[promoted], target);
            updateZobristPiece(*main, promoted, target);
        }

        // Enpassant capture
        if (enpassant) {
            // If white to move
            int dir, pawnType;
            if (main->side == WHITE) {
                dir = NORTH;
                pawnType = bP;
            } else {
                dir = SOUTH;
                pawnType = wP;
            }
            popBit(main->pieces[pawnType], target + dir);
            updateZobristPiece(*main, pawnType, target + dir);
        }

        if (main->enpassant != NOSQ)
            updateZobristEnpassant(*main);
        // Reset enpassant, regardless of an enpassant capture
        main->enpassant = NOSQ;

        // Two Square Push move
        if (twoSquarePush) {
            if (main->side == WHITE)
                main->enpassant = (Sq)(target + NORTH);
            else
                main->enpassant = (Sq)(target + SOUTH);
            updateZobristEnpassant(*main);
        }

        // Castling
        if (castling) {
            int rookType, castlingSource, castlingTarget;
            switch (target) {
            case G1:
                rookType = wR;
                castlingSource = H1;
                castlingTarget = F1;
                break;
            case C1:
                rookType = wR;
                castlingSource = A1;
                castlingTarget = D1;
                break;
            case G8:
                rookType = bR;
                castlingSource = H8;
                castlingTarget = F8;
                break;
            case C8:
                rookType = bR;
                castlingSource = A8;
                castlingTarget = D8;
                break;
            default:
                _MY_ASSERT(false, "Unreachable!");
                break;
            }
            popBit(main->pieces[rookType], castlingSource);
            updateZobristPiece(*main, rookType, castlingSource);

            setBit(main->pieces[rookType], castlingTarget);
            updateZobristPiece(*main, rookType, castlingTarget);
        }

        // Update castling rights
        updateZobristCastling(*main);
        main->castling &= CASTLING_RIGHTS[source];
        main->castling &= CASTLING_RIGHTS[target];
        updateZobristCastling(*main);

        // Update units (or occupancies)
        main->updateUnits();

        // Change side
        main->changeSide();
        updateZobristSide(*main);

        /* ============= FOR DEBUG PURPOSES ONLY ===============
        uint64_t generatedKey = genKey(*main);
        uint64_t generatedLock = genLock(*main);
        if (generatedKey != main->key) {
            std::cout << "\nBoard.MakeMove(" << moveToStr(move) << ")\n";
            main->display();
            std::cout << "Key should've been 0x" << std::hex << main->key << "ULL instead of 0x"
                      << generatedKey << std::dec << "ULL\n";
            std::terminate();
        }
        if (generatedLock != main->lock) {
            std::cout << "\nBoard.MakeMove(" << moveToStr(move) << ")\n";
            main->display();
            std::cout << "Lock should've been 0x" << std::hex << main->lock << "ULL instead of 0x"
                      << generatedLock << std::dec << "ULL\n";
            std::terminate();
        }
        ============= FOR DEBUG PURPOSES ONLY =============== */

        // Check if king is in check
        if (main->inCheck()) {
            // Restore board and return false
            *main = clone;
            return false;
        } else {
            // Move is legal and was made, return true
            return true;
        }
    } else {
        // If capture, recall makeMove() and make move
        if (isCapture(move))
            return makeMove(main, move, AllMoves);
        // If not capture, don't make move
        return false;
    }
}