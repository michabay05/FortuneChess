#include "defs.hpp"
#include "eval_consts.hpp"

// file masks [square]
uint64_t fileMasks[64];

// rank masks [square]
uint64_t rankMasks[64];

// isolated pawn masks [square]
uint64_t isolatedMasks[64];

// white and black passed pawn masks [square]
uint64_t passedMasks[2][64];

// set file or rank mask
uint64_t setFileRankMask(int file, int rank)
{
    uint64_t mask = 0ULL;

    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            int square = r * 8 + f;

            if (file != -1) {
                if (f == file)
                    mask |= setBit(mask, square);
            } else if (rank != -1) {
                if (r == rank)
                    mask |= setBit(mask, square);
            }
        }
    }
    return mask;
}

// init evaluation masks
void initEvalMasks()
{
    int r = 0, f = 0;

    for (r = 0; r < 8; r++) {
        for (f = 0; f < 8; f++) {
            int square = r * 8 + f;
            /******** Init file masks ********/
            fileMasks[square] |= setFileRankMask(f, -1);
            /******** Init rank masks ********/
            rankMasks[square] |= setFileRankMask(-1, r);
        }
    }

    for (r = 0; r < 8; r++) {
        for (f = 0; f < 8; f++) {
            int square = r * 8 + f;

            /******** Init isolated masks ********/
            isolatedMasks[square] |= setFileRankMask(f - 1, -1);
            isolatedMasks[square] |= setFileRankMask(f + 1, -1);

            /******** White passed masks ********/
            passedMasks[0][square] |= setFileRankMask(f - 1, -1);
            passedMasks[0][square] |= setFileRankMask(f, -1);
            passedMasks[0][square] |= setFileRankMask(f + 1, -1);

            // loop over redudant ranks
            for (int i = 0; i < (8 - r); i++)
                // reset redudant bits
                passedMasks[0][square] &= ~rankMasks[(7 - i) * 8 + f];

            /******** Black passed masks ********/
            passedMasks[1][square] |= setFileRankMask(f - 1, -1);
            passedMasks[1][square] |= setFileRankMask(f, -1);
            passedMasks[1][square] |= setFileRankMask(f + 1, -1);

            // loop over redudant ranks
            for (int i = 0; i < r + 1; i++)
                // reset redudant bits
                passedMasks[1][square] &= ~rankMasks[i * 8 + f];
        }
    }
}

// get game phase score
static inline int calcPhaseScore(Board& board)
{
    /*
        The game phase score of the game is derived from the pieces
        (not counting pawns and kings) that are still on the board.
        The full material starting position game phase score is:

        4 * knight material score in the Opening +
        4 * bishop material score in the Opening +
        4 * rook material score in the Opening +
        2 * queen material score in the Opening
    */

    // white & black game phase scores
    int whitePieceScores = 0, blackPieceScores = 0;

    // loop over white pieces
    for (int piece = wN; piece <= wQ; piece++)
        whitePieceScores += countBits(board.pieces[piece]) * MATERIAL_SCORE[Opening][piece];

    // loop over white pieces
    for (int piece = bN; piece <= bQ; piece++)
        blackPieceScores += countBits(board.pieces[piece]) * -MATERIAL_SCORE[Opening][piece];

    // return game phase score
    return whitePieceScores + blackPieceScores;
}

static bool areSameColoredBishops(Board& board, Color side)
{
    uint64_t bbCopy = board.pieces[side * 6 + 2];
    int sq = 0;
    int bishopColor = -1;
    while (bbCopy) {
        sq = lsbIndex(bbCopy);
        int prevColor = bishopColor;
        bishopColor = SQCLR(ROW(sq), COL(sq));
        if (prevColor != -1 && bishopColor != prevColor) {
            std::cout << "Same color!\n";
            return false;
        }
        popBit(bbCopy, sq);
    }
    return true;
}

#define COUNT(piece) countBits(board.pieces[(piece)])
#define EXISTS(piece) (COUNT((piece)) > 0)

static bool isDrawByInsufficientMat(Board& board)
{
    /* Positions are drawn due to insufficient material if
        - KNN vs k
        - K   vs knn

        - KB  vs k
        - K   vs kb
        - KBB vs k (where all of white's bishops are on the light/dark squares)

        - KN vs kb
        - KB vs kn

        - K  vs k
    */
    // If the position has a queen or rook or pawn, the position isn't a draw
    if (EXISTS(wP) || EXISTS(bP) || EXISTS(wR) || EXISTS(bR) || EXISTS(wQ) || EXISTS(bQ))
        return false;

    if ((!EXISTS(wN) && !EXISTS(bN) && !EXISTS(wB) && !EXISTS(bB)) || // K  vs k
        (COUNT(wN) == 1 && COUNT(bN) == 1) ||                         // KN  vs kn
        (COUNT(wN) <= 2 && COUNT(bN) == 0) ||                         // KNN vs k
        (COUNT(wN) == 0 && COUNT(bN) <= 2) ||                         // K   vs knn
        (COUNT(wB) == 1 && COUNT(bB) == 0) ||                         // KB  vs kb
        (COUNT(wN) == 1 && COUNT(bB) == 1) ||                         // KN  vs kb
        (COUNT(wB) == 1 && COUNT(bN) == 1) ||                         // KB  vs kn
        (COUNT(wB) >= 2 && areSameColoredBishops(board, WHITE)) ||
        (COUNT(bB) >= 2 && areSameColoredBishops(board, BLACK))) {
        return true;
    }
    return false;
}

// position evaluation
int evaluatePos(Board& board)
{
    //if (isDrawByInsufficientMat(board))
    //    return 0;

    // get game phase score
    int phaseScore = calcPhaseScore(board);

    // game phase (Opening, middle game, Endgame)
    int game_phase = -1;

    // pick up game phase based on game phase score
    if (phaseScore > OPENING_PHASE_SCORE)
        game_phase = Opening;
    else if (phaseScore < ENDGAME_PHASE_SCORE)
        game_phase = Endgame;
    else
        game_phase = Middlegame;

    // static evaluation score
    int score = 0, openingScore = 0, endgameScore = 0;

    // current pieces bitboard copy
    uint64_t bbCopy;

    // init piece & square
    int sq;

    // penalties
    int doubledPawns = 0;

    // loop over piece bitboards
    for (int piece = wP; piece <= bK; piece++) {
        // init piece bitboard copy
        bbCopy = board.pieces[piece];

        // loop over pieces within a bitboard
        while (bbCopy) {
            // init square
            sq = lsbIndex(bbCopy);

            // get Opening/Endgame material score
            openingScore += MATERIAL_SCORE[Opening][piece];
            endgameScore += MATERIAL_SCORE[Endgame][piece];

            // score positional piece scores
            switch (piece) {
            // evaluate white pawns
            case wP:
                // get Opening/Endgame positional score
                openingScore += POSITIONAL_SCORE[Opening][PAWN][sq];
                endgameScore += POSITIONAL_SCORE[Endgame][PAWN][sq];

                // double pawn penalty
                doubledPawns = countBits(board.pieces[wP] & fileMasks[sq]);

                // on double pawns (tripple, etc)
                if (doubledPawns > 1) {
                    openingScore += (doubledPawns - 1) * DOUBLE_PAWN_PENALTY_OPENING;
                    endgameScore += (doubledPawns - 1) * DOUBLE_PAWN_PENALTY_ENDGAME;
                }

                // on isolated pawn
                if ((board.pieces[wP] & isolatedMasks[sq]) == 0) {
                    // give an isolated pawn penalty
                    openingScore += ISOLATED_PAWN_PENALTY_OPENING;
                    endgameScore += ISOLATED_PAWN_PENALTY_ENDGAME;
                }
                // on passed pawn
                if ((passedMasks[0][sq] & board.pieces[bP]) == 0) {
                    // give passed pawn bonus
                    openingScore += PASSED_PAWN_BONUS[GET_RANK[sq]];
                    endgameScore += PASSED_PAWN_BONUS[GET_RANK[sq]];
                }

                break;

            // evaluate white knights
            case wN:
                // get Opening/Endgame positional score
                openingScore += POSITIONAL_SCORE[Opening][KNIGHT][sq];
                endgameScore += POSITIONAL_SCORE[Endgame][KNIGHT][sq];

                break;

            // evaluate white bishops
            case wB:
                // get Opening/Endgame positional score
                openingScore += POSITIONAL_SCORE[Opening][BISHOP][sq];
                endgameScore += POSITIONAL_SCORE[Endgame][BISHOP][sq];

                // mobility
                openingScore += (countBits(getBishopAttack(sq, board.units[BOTH])) - BISHOP_UNIT) *
                                BISHOP_MOBILITY_OPENING;
                endgameScore += (countBits(getBishopAttack(sq, board.units[BOTH])) - BISHOP_UNIT) *
                                BISHOP_MOBILITY_ENDGAME;
                break;

            // evaluate white rooks
            case wR:
                // get Opening/Endgame positional score
                openingScore += POSITIONAL_SCORE[Opening][ROOK][sq];
                endgameScore += POSITIONAL_SCORE[Endgame][ROOK][sq];

                // semi open file
                if ((board.pieces[wP] & fileMasks[sq]) == 0) {
                    // add semi open file bonus
                    openingScore += SEMI_OPEN_FILE_SCORE;
                    endgameScore += SEMI_OPEN_FILE_SCORE;
                }

                // semi open file
                if (((board.pieces[wP] | board.pieces[bP]) & fileMasks[sq]) == 0) {
                    // add semi open file bonus
                    openingScore += OPEN_FILE_SCORE;
                    endgameScore += OPEN_FILE_SCORE;
                }

                break;

            // evaluate white queens
            case wQ:
                // get Opening/Endgame positional score
                openingScore += POSITIONAL_SCORE[Opening][QUEEN][sq];
                endgameScore += POSITIONAL_SCORE[Endgame][QUEEN][sq];

                // mobility
                openingScore += (countBits(getQueenAttack(sq, board.units[BOTH])) - QUEEN_UNIT) *
                                QUEEN_MOBILITY_OPENING;
                endgameScore += (countBits(getQueenAttack(sq, board.units[BOTH])) - QUEEN_UNIT) *
                                QUEEN_MOBILITY_ENDGAME;
                break;

            // evaluate white king
            case wK:
                // get Opening/Endgame positional score
                openingScore += POSITIONAL_SCORE[Opening][KING][sq];
                endgameScore += POSITIONAL_SCORE[Endgame][KING][sq];

                // semi open file
                if ((board.pieces[wP] & fileMasks[sq]) == 0) {
                    // add semi open file penalty
                    openingScore -= SEMI_OPEN_FILE_SCORE;
                    endgameScore -= SEMI_OPEN_FILE_SCORE;
                }

                // semi open file
                if (((board.pieces[wP] | board.pieces[bP]) & fileMasks[sq]) == 0) {
                    // add semi open file penalty
                    openingScore -= OPEN_FILE_SCORE;
                    endgameScore -= OPEN_FILE_SCORE;
                }

                // king safety bonus
                openingScore += countBits(kingAttacks[sq] & board.units[WHITE]) * KING_SHIELD_BONUS;
                endgameScore += countBits(kingAttacks[sq] & board.units[WHITE]) * KING_SHIELD_BONUS;

                break;

            // evaluate black pawns
            case bP:
                // get Opening/Endgame positional score
                openingScore -= POSITIONAL_SCORE[Opening][PAWN][MIRROR_SCORE[sq]];
                endgameScore -= POSITIONAL_SCORE[Endgame][PAWN][MIRROR_SCORE[sq]];

                // double pawn penalty
                doubledPawns = countBits(board.pieces[bP] & fileMasks[sq]);

                // on double pawns (tripple, etc)
                if (doubledPawns > 1) {
                    openingScore -= (doubledPawns - 1) * DOUBLE_PAWN_PENALTY_OPENING;
                    endgameScore -= (doubledPawns - 1) * DOUBLE_PAWN_PENALTY_ENDGAME;
                }

                // on isolated pawn
                if ((board.pieces[bP] & isolatedMasks[sq]) == 0) {
                    // give an isolated pawn penalty
                    openingScore -= ISOLATED_PAWN_PENALTY_OPENING;
                    endgameScore -= ISOLATED_PAWN_PENALTY_ENDGAME;
                }
                // on passed pawn
                if ((passedMasks[1][sq] & board.pieces[wP]) == 0) {
                    // give passed pawn bonus
                    openingScore -= PASSED_PAWN_BONUS[GET_RANK[sq]];
                    endgameScore -= PASSED_PAWN_BONUS[GET_RANK[sq]];
                }

                break;

            // evaluate black knights
            case bN:
                // get Opening/Endgame positional score
                openingScore -= POSITIONAL_SCORE[Opening][KNIGHT][MIRROR_SCORE[sq]];
                endgameScore -= POSITIONAL_SCORE[Endgame][KNIGHT][MIRROR_SCORE[sq]];

                break;

            // evaluate black bishops
            case bB:
                // get Opening/Endgame positional score
                openingScore -= POSITIONAL_SCORE[Opening][BISHOP][MIRROR_SCORE[sq]];
                endgameScore -= POSITIONAL_SCORE[Endgame][BISHOP][MIRROR_SCORE[sq]];

                // mobility
                openingScore -= (countBits(getBishopAttack(sq, board.units[BOTH])) - BISHOP_UNIT) *
                                BISHOP_MOBILITY_OPENING;
                endgameScore -= (countBits(getBishopAttack(sq, board.units[BOTH])) - BISHOP_UNIT) *
                                BISHOP_MOBILITY_ENDGAME;
                break;

            // evaluate black rooks
            case bR:
                // get Opening/Endgame positional score
                openingScore -= POSITIONAL_SCORE[Opening][ROOK][MIRROR_SCORE[sq]];
                endgameScore -= POSITIONAL_SCORE[Endgame][ROOK][MIRROR_SCORE[sq]];

                // semi open file
                if ((board.pieces[bP] & fileMasks[sq]) == 0) {
                    // add semi open file bonus
                    openingScore -= SEMI_OPEN_FILE_SCORE;
                    endgameScore -= SEMI_OPEN_FILE_SCORE;
                }

                // semi open file
                if (((board.pieces[wP] | board.pieces[bP]) & fileMasks[sq]) == 0) {
                    // add semi open file bonus
                    openingScore -= OPEN_FILE_SCORE;
                    endgameScore -= OPEN_FILE_SCORE;
                }

                break;

            // evaluate black queens
            case bQ:
                // get Opening/Endgame positional score
                openingScore -= POSITIONAL_SCORE[Opening][QUEEN][MIRROR_SCORE[sq]];
                endgameScore -= POSITIONAL_SCORE[Endgame][QUEEN][MIRROR_SCORE[sq]];

                // mobility
                openingScore -= (countBits(getQueenAttack(sq, board.units[BOTH])) - QUEEN_UNIT) *
                                QUEEN_MOBILITY_OPENING;
                endgameScore -= (countBits(getQueenAttack(sq, board.units[BOTH])) - QUEEN_UNIT) *
                                QUEEN_MOBILITY_ENDGAME;
                break;

            // evaluate black king
            case bK:
                // get Opening/Endgame positional score
                openingScore -= POSITIONAL_SCORE[Opening][KING][MIRROR_SCORE[sq]];
                endgameScore -= POSITIONAL_SCORE[Endgame][KING][MIRROR_SCORE[sq]];

                // semi open file
                if ((board.pieces[bP] & fileMasks[sq]) == 0) {
                    // add semi open file penalty
                    openingScore += SEMI_OPEN_FILE_SCORE;
                    endgameScore += SEMI_OPEN_FILE_SCORE;
                }

                // semi open file
                if (((board.pieces[wP] | board.pieces[bP]) & fileMasks[sq]) == 0) {
                    // add semi open file penalty
                    openingScore += OPEN_FILE_SCORE;
                    endgameScore += OPEN_FILE_SCORE;
                }

                // king safety bonus
                openingScore -= countBits(kingAttacks[sq] & board.units[BLACK]) * KING_SHIELD_BONUS;
                endgameScore -= countBits(kingAttacks[sq] & board.units[BLACK]) * KING_SHIELD_BONUS;
                break;
            }

            // pop ls1b
            popBit(bbCopy, sq);
        }
    }

    /*
        Now in order to calculate interpolated score
        for a given game phase we use this formula
        (same for material and positional scores):

        (
          openingScore * phase_score +
          endgameScore * (OPENING_PHASE_SCORE - phase_score)
        ) / OPENING_PHASE_SCORE

        E.g. the score for pawn on d4 at phase say 5000 would be
        interpolated_score = (12 * 5000 + (-7) * (6192 - 5000)) / 6192 = 8.342377261
    */

    // interpolate score in the middlegame
    if (game_phase == Middlegame)
        score = (openingScore * phaseScore + endgameScore * (OPENING_PHASE_SCORE - phaseScore)) /
                OPENING_PHASE_SCORE;

    // return pure Opening score in Opening
    else if (game_phase == Opening)
        score = openingScore;

    // return pure Endgame score in Endgame
    else if (game_phase == Endgame)
        score = endgameScore;

    // return final evaluation based on side
    return (board.side == WHITE) ? score : -score;
}
