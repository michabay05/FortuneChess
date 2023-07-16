#include "defs.hpp"

// Score layout (Black advantage -> 0 -> White advantage)
// -INFINITY < -MATE_VALUE < -MATE_SCORE < NORMAL(non - mating) score < MATE_SCORE < MATE_VALUE <
// INFINITY

// clang-format off
// [attacker][victim]
const int mvvLva[6][6] =
{
	{105, 205, 305, 405, 505, 605},
	{104, 204, 304, 404, 504, 604},
	{103, 203, 303, 403, 503, 603},
	{102, 202, 302, 402, 502, 602},
	{101, 201, 301, 401, 501, 601},
	{100, 200, 300, 400, 500, 600}
};
// clang-format on
int ply = 0;
// Total positions searched counter
int64_t nodes;

// Quiet moves that caused a beta-cutoff
int killerMoves[2][MAX_PLY]; // [id][ply]
// Quiet moves that updated the alpha value
int historyMoves[12][64];      // [piece][square]
int pvLength[MAX_PLY];         // [ply]
int pvTable[MAX_PLY][MAX_PLY]; // [ply][ply]

// PV flags
bool followPV, scorePV;

void getCPOrMateScore(int score);
int negamax(Board* board, int alpha, int beta, int depth);
int quiescence(Board* board, int alpha, int beta);
int scoreMoves(const Board& board, const int move);
void printMoveScores(const MoveList& moveList, const Board& board);
void sortMoves(MoveList& moveList, const Board& board);
void clearSearchTable();
void enablePVScoring(MoveList& moveList);
bool isRepetition(Board& board);

void clearSearchTable()
{
    nodes = 0LL;
    uciStop = false;
    followPV = false;
    scorePV = false;
    memset(killerMoves, 0, sizeof(killerMoves));
    memset(historyMoves, 0, sizeof(historyMoves));
    memset(pvTable, 0, sizeof(pvTable));
    memset(pvLength, 0, sizeof(pvLength));
}

void searchPos(Board& board, const int depth)
{
    long long startTime = getCurrTime();

    int score = 0;
    clearSearchTable();
    int alpha = -INF, beta = INF;
    for (int currDepth = 1; currDepth <= depth; currDepth++) {
        if (uciStop)
            break;
        // Enable followPV
        followPV = true;

        score = negamax(&board, alpha, beta, currDepth);
        // Aspiration window
        if ((score <= alpha) || (score >= beta)) {
            alpha = -INF;
            beta = INF;
            continue;
        }
        // Set up the window for the next iteration
        alpha = score - 50;
        beta = score + 50;
        if (pvLength[0]) {
            std::cout << "info score ";
            getCPOrMateScore(score);
            std::cout << " depth " << currDepth << " nodes " << nodes << " time "
                      << (getCurrTime() - startTime) << " pv";
            for (int i = 0; i < pvLength[0]; i++)
                std::cout << " " << moveToStr(pvTable[0][i]);
            std::cout << "\n";
        }
    }
    std::cout << "bestmove " << moveToStr(pvTable[0][0]) << "\n";
}

void getCPOrMateScore(int score)
{
    // Print information about current depth
    if (score > -MATE_VALUE && score < -MATE_SCORE) {
        std::cout << "mate " << (-(score + MATE_VALUE) / 2 - 1);
    } else if (score > MATE_SCORE && score < MATE_VALUE) {
        std::cout << "mate " << ((MATE_VALUE - score) / 2 + 1);
    } else {
        std::cout << "cp " << score;
    }
}

int negamax(Board* board, int alpha, int beta, int depth)
{
    pvLength[ply] = ply;
    int score;
    TTFlag flag = F_ALPHA;

    // if position repetition occurs
    if (ply && isRepetition(*board)) {
        // return draw score
        return 0;
    }

    bool isPVNode = (beta - alpha) > 1;

    // Read score from transposition table if position already exists inside the
    // table
    if (ply && (score = readTTEntry(*board, alpha, beta, depth)) != NO_TT_ENTRY && !isPVNode)
        return score;

    // every 2047 nodes
    if ((nodes & 2047) == 0)
        // "listen" to the GUI/user input
        checkUp();

    // Escape condition
    if (depth == 0)
        return quiescence(board, alpha, beta);

    // Exit if ply > max ply; ply should be <= 63
    if (ply > MAX_PLY - 1)
        return evaluatePos(*board);

    // Increment nodes
    nodes++;

    // Is the king in check?
    // bool inCheck = board->inCheck();
    bool inCheck = board->sqAttacked((Sq)lsbIndex(board->pieces[board->side == WHITE ? wK : bK]),
                                     (Color)(board->side ^ 1));

    // Check extension
    if (inCheck) {
        depth++;
    }

    int legalMoves = 0;

    // NULL move pruning
    if (depth >= 3 && !inCheck && ply) {
        Board anotherClone = *board;
        ply++;

        board->repetitionIndex++;
        board->repetitionTable[board->repetitionIndex] = board->key;

        // Hash enpassant if available
        if (board->enpassant != NOSQ)
            updateZobristEnpassant(*board);
        // Reset enpassant
        board->enpassant = NOSQ;

        // Give opponent an extra move; 2 moves in one turn
        board->changeSide();
        // Hash the extra turn given by hashing the side one more time
        updateZobristSide(*board);

        // Search move with reduced depth to find beta-cutoffs
        score = -negamax(board, -beta, -beta + 1, depth - 1 - 2);

        ply--;
        board->repetitionIndex--;

        *board = anotherClone;
        if (uciStop)
            return 0;
        // Fail hard; beta-cutoffs
        if (score >= beta)
            return beta;
    }

    // Generate and sort moves
    MoveList moveList;
    genAllMoves(moveList, *board);
    if (followPV)
        enablePVScoring(moveList);
    sortMoves(moveList, *board);

    Board clone;
    int movesSearched = 0;
    // Loop over all the generated moves
    for (int i = 0; i < moveList.count; i++) {
        int mv = moveList.list[i];
        // Bookmark current of board
        clone = *board;

        // Increment half move
        ply++;

        board->repetitionIndex++;
        board->repetitionTable[board->repetitionIndex] = board->key;

        // Play move if move is legal
        if (!makeMove(board, mv, MoveType::AllMoves)) {
            // Decrement move and move onto next move
            ply--;
			board->repetitionIndex--;
            continue;
        }

        // Increment legal moves
        legalMoves++;

        // Full depth search
        if (movesSearched == 0) {
            // Do normal alpha-beta search
            score = -negamax(board, -beta, -alpha, depth - 1);
        } else {
            // Late move reduction (LMR)
            if (movesSearched >= FULL_DEPTH_MOVES && depth >= REDUCTION_LIMIT && !inCheck &&
                getPromoted(mv) == EMPTY && !isCapture(mv))
                score = -negamax(board, -alpha - 1, -alpha, depth - 2);
            else
                // Hack to ensure full depth search is done
                score = alpha + 1;

            // PVS (Principal Variation Search)
            if (score > alpha) {
                // re-search at full depth but with narrowed score bandwith
                score = -negamax(board, -alpha - 1, -alpha, depth - 1);

                // if LMR fails re-search at full depth and full score bandwith
                if ((score > alpha) && (score < beta))
                    score = -negamax(board, -beta, -alpha, depth - 1);
            }
        }
        // Decrement ply and restore board state
        ply--;
        board->repetitionIndex--;

        *board = clone;

        if (uciStop)
            return 0;

        movesSearched++;

        // If current move is better, update move
        if (score > alpha) {
            // Switch flag to EXACT(PV node) from ALPHA (fail-low node)
            flag = F_EXACT;
            if (!isCapture(mv))
                // Store history move
                historyMoves[getPiece(mv)][getTarget(mv)] += depth;

            // Principal Variation (PV) node
            alpha = score;
            // Write PV move
            pvTable[ply][ply] = mv;
            // Copy move from deeper ply into current ply
            for (int next = ply + 1; next < pvLength[ply + 1]; next++) {
                pvTable[ply][next] = pvTable[ply + 1][next];
            }
            // Adjust pv length
            pvLength[ply] = pvLength[ply + 1];

            // Fail-hard beta cutoff
            if (score >= beta) {
                // Store hash entry with score equal to beta
                writeTTEntry(*board, beta, depth, F_BETA);

                if (!isCapture(mv)) {
                    // Move 1st killer move to 2nd killer move
                    killerMoves[1][ply] = killerMoves[0][ply];
                    // Update 1st killer move to current move
                    killerMoves[0][ply] = mv;
                }
                // Move that fails high
                return beta;
            }
        }
    }

    // If no legal moves, it's either checkmate or stalemate
    if (legalMoves == 0) {
        if (inCheck) {
            // If check, return checkmate score
            return -MATE_VALUE + ply;
        } else {
            // If not check, return stalemate score
            return 0;
        }
    }
    // Store hash entry with score equal to alpha
    writeTTEntry(*board, alpha, depth, flag);
    // Move that failed low
    return alpha;
}

int quiescence(Board* board, int alpha, int beta)
{
    // every 2047 nodes
    if ((nodes & 2047) == 0)
        // "listen" to the GUI/user input
        checkUp();

    // Increment nodes
    nodes++;

    // Escape condition - fail-hard beta cutoff
    int evaluation = evaluatePos(*board);

    // Exit if ply > max ply; ply should be <= 63
    if (ply > MAX_PLY - 1)
        return evaluation;

    // Fail-hard beta cutoff
    if (evaluation >= beta)
        // Move that fails high
        return beta;

    // If current move is better, update move
    if (evaluation > alpha)
        // Principal Variation (PV) node
        alpha = evaluation;

    // Generate and sort moves
    MoveList moveList;
    genAllMoves(moveList, *board);
    sortMoves(moveList, *board);

    Board clone;
    // Loop over all the generated moves
    for (int i = 0; i < moveList.count; i++) {
        // Bookmark current of board
        clone = *board;

        // Increment half move
        ply++;

        board->repetitionIndex++;
        board->repetitionTable[board->repetitionIndex] = board->key;

        // Play move if move is legal
        if (!makeMove(board, moveList.list[i], MoveType::OnlyCaptures)) {
            // Decrement move and move onto next move
            ply--;
			board->repetitionIndex--;
            continue;
        }

        // Score current move
        int score = -quiescence(board, -beta, -alpha);

        // Decrement ply and restore board state
        ply--;
		board->repetitionIndex--;

        *board = clone;

        if (uciStop)
            return 0;

        // If current move is better, update move
        if (score > alpha) {
            alpha = score;

            // Fail-hard beta cutoff
            if (score >= beta)
                // Move that fails high
                return beta;
        }
    }
    // Move that failed low
    return alpha;
}

void printMoveScores(const MoveList& moveList, const Board& board)
{
    std::cout << "Move scores: \n";
    for (int i = 0; i < moveList.count; i++)
        std::cout << "    " << moveToStr(moveList.list[i]).c_str() << ": "
                  << scoreMoves(board, moveList.list[i]) << "\n";
}

void sortMoves(MoveList& moveList, const Board& board)
{
    int* moveScores = new int[moveList.count];
    // Initialize moveScores with move scores
    for (int i = 0; i < moveList.count; i++)
        moveScores[i] = scoreMoves(board, moveList.list[i]);

    // Sort moves based on scores
    for (int curr = 0; curr < moveList.count; curr++) {
        for (int next = curr + 1; next < moveList.count; next++) {
            if (moveScores[curr] < moveScores[next]) {
                // Swap moves
                int temp = moveList.list[curr];
                moveList.list[curr] = moveList.list[next];
                moveList.list[next] = temp;
                // Swap scores
                temp = moveScores[curr];
                moveScores[curr] = moveScores[next];
                moveScores[next] = temp;
            }
        }
    }
    delete[] moveScores;
}

/*
        Move Scoring Order or Priority
          1. PV moves        ( = 20,000 pts)
          2. MVV LVA move    (>= 10,000 pts)
          3. 1st killer move ( =  9,000 pts)
          4. 2nd killer move ( =  8,000 pts)
          5. History move
          6. Unsorted move
*/
int scoreMoves(const Board& board, const int move)
{
    // PV (Principal variation move) scoring
    if (scorePV && pvTable[0][ply] == move) {
        scorePV = false;
        return 20'000;
    }
    // Capture move scoring
    if (isCapture(move)) {
        int victimPiece = wP;
        for (int bbPiece = (board.side == WHITE ? bP : wP);
             bbPiece <= (board.side == WHITE ? bK : wK); bbPiece++) {
            if (getBit(board.pieces[bbPiece], getTarget(move))) {
                victimPiece = bbPiece;
                break;
            }
        }
        return mvvLva[getPiece(move) % 6][victimPiece % 6] + 10'000;
    }
    // Quiet move scoring
    else {
        // Score 1st killer move
        if (killerMoves[0][ply] == move)
            return 9'000;
        // Score 2nd killer move
        else if (killerMoves[1][ply] == move)
            return 8'000;
        // Score history moves
        else
            return historyMoves[getPiece(move)][getTarget(move)];
    }
}

void enablePVScoring(MoveList& moveList)
{
    followPV = false;
    for (int i = 0; i < moveList.count; i++) {
        if (pvTable[0][ply] == moveList.list[i]) {
            // Enable PV scoring and following
            scorePV = true;
            followPV = true;
        }
    }
}

bool isRepetition(Board& board)
{
    for (int i = 0; i < board.repetitionIndex; i++) {
        if (board.repetitionTable[i] == board.key) {
            return true;
        }
    }
    return false;
}