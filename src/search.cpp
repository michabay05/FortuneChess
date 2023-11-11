#include "defs.hpp"
#include "tinycthread.h"

// Score layout (Black advantage -> 0 -> White advantage)
// -INFINITY < -MATE_VALUE < -MATE_SCORE < NORMAL(non - mating) score < MATE_SCORE < MATE_VALUE <
// INFINITY

const int INF = 50'000;
const int MATE_VALUE = 49'000;
const int MATE_SCORE = 48'000;

const int FULL_DEPTH_MOVES = 4;
const int REDUCTION_LIMIT = 3;

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

SearchTable mainSearchTable;

/*
        Move Scoring Order or Priority
          1. PV moves        ( = 20,000 pts)
          2. MVV LVA move    (>= 10,000 pts)
          3. 1st killer move ( =  9,000 pts)
          4. 2nd killer move ( =  8,000 pts)
          5. History move
          6. Unsorted move
*/
static int scoreMoves(const Board& board, SearchTable& sTable, const int move)
{
    // PV (Principal variation move) scoring
    if (sTable.scorePV && sTable.pvTable[0][sTable.ply] == move) {
        sTable.scorePV = false;
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
        if (sTable.killerMoves[0][sTable.ply] == move)
            return 9'000;
        // Score 2nd killer move
        else if (sTable.killerMoves[1][sTable.ply] == move)
            return 8'000;
        // Score history moves
        else
            return sTable.historyMoves[getPiece(move)][getTarget(move)];
    }
}

static void printMoveScores(const MoveList& moveList, const Board& board, SearchTable& sTable)
{
    std::cout << "Move scores: \n";
    for (int i = 0; i < moveList.count; i++)
        std::cout << "    " << moveToStr(moveList.list[i]).c_str() << ": "
                  << scoreMoves(board, sTable, moveList.list[i]) << "\n";
}

static void enablePVScoring(MoveList& moveList, SearchTable& sTable)
{
    sTable.followPV = false;
    for (int i = 0; i < moveList.count; i++) {
        if (sTable.pvTable[0][sTable.ply] == moveList.list[i]) {
            // Enable PV scoring and following
            sTable.scorePV = true;
            sTable.followPV = true;
        }
    }
}

static void sortMoves(MoveList& moveList, const Board& board, SearchTable& sTable)
{
    int* moveScores = new int[moveList.count];
    // Initialize moveScores with move scores
    for (int i = 0; i < moveList.count; i++)
        moveScores[i] = scoreMoves(board, sTable, moveList.list[i]);

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

static bool isRepetition(Board& board)
{
    for (int i = 0; i < board.repetitionIndex; i++) {
        if (board.repetitionTable[i] == board.key) {
            return true;
        }
    }
    return false;
}

static void clearSearchTable(HashTable* tt, SearchInfo* sInfo, SearchTable* sTable)
{
    sTable->nodes = 0LL;
    sInfo->stop = false;
    sTable->followPV = false;
    sTable->scorePV = false;
    memset(sTable->killerMoves, 0, sizeof(sTable->killerMoves));
    memset(sTable->historyMoves, 0, sizeof(sTable->historyMoves));
    memset(sTable->pvTable, 0, sizeof(sTable->pvTable));
    memset(sTable->pvLength, 0, sizeof(sTable->pvLength));

    tt->currentAge++;
}

static int quiescence(Board* board, SearchInfo* sInfo, SearchTable* sTable, int alpha, int beta)
{
    // every 2047 nodes
    if ((sTable->nodes & 2047) == 0)
        // "listen" to the GUI/user input
        checkUp(sInfo);

    // Increment nodes
    sTable->nodes++;

    // Escape condition - fail-hard beta cutoff
    int evaluation = evaluatePos(*board);

    // Exit if ply > max ply; ply should be <= 63
    if (sTable->ply > MAX_PLY - 1)
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
    genCaptureMoves(moveList, *board);
    sortMoves(moveList, *board, *sTable);

    Board clone;
    // Loop over all the generated moves
    for (int i = 0; i < moveList.count; i++) {
        // Bookmark current of board
        clone = *board;

        // Increment half move
        sTable->ply++;

        board->repetitionIndex++;
        board->repetitionTable[board->repetitionIndex] = board->key;

        // Play move if move is legal
        if (!makeMove(board, moveList.list[i], MoveType::OnlyCaptures)) {
            // Decrement move and move onto next move
            sTable->ply--;
            board->repetitionIndex--;
            continue;
        }

        // Score current move
        int score = -quiescence(board, sInfo, sTable, -beta, -alpha);

        // Decrement ply and restore board state
        sTable->ply--;
        board->repetitionIndex--;

        *board = clone;

        if (sInfo->stop)
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

static int negamax(Board* board, HashTable* tt, SearchInfo* sInfo, SearchTable* sTable, int alpha, int beta, int depth)
{
    sTable->pvLength[sTable->ply] = sTable->ply;
    int score;
    TTFlag flag = F_ALPHA;

    // if position repetition occurs
    if (sTable->ply && isRepetition(*board)) {
        // return draw score
        return 0;
    }

    bool isPVNode = (beta - alpha) > 1;

    // Read score from transposition table if position already exists inside the
    // table
    if (sTable->ply && (score = tt->read(*board, *sTable, alpha, beta, depth)) != NO_TT_ENTRY &&
        !isPVNode)
        return score;

    // every 2047 nodes
    if ((sTable->nodes & 2047) == 0)
        // "listen" to the GUI/user input
        checkUp(sInfo);

    // Escape condition
    if (depth == 0)
        return quiescence(board, sInfo, sTable, alpha, beta);

    // Exit if ply > max ply; ply should be <= 63
    if (sTable->ply > MAX_PLY - 1)
        return evaluatePos(*board);

    // Increment nodes
    sTable->nodes++;

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
    if (depth >= 3 && !inCheck && sTable->ply) {
        Board anotherClone = *board;
        sTable->ply++;

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
        score = -negamax(board, tt, sInfo, sTable, -beta, -beta + 1, depth - 1 - 2);

        sTable->ply--;
        board->repetitionIndex--;

        *board = anotherClone;
        if (sInfo->stop)
            return 0;
        // Fail hard; beta-cutoffs
        if (score >= beta)
            return beta;
    }

    // Generate and sort moves
    MoveList moveList;
    genAllMoves(moveList, *board);
    if (sTable->followPV)
        enablePVScoring(moveList, *sTable);
    sortMoves(moveList, *board, *sTable);

    Board clone;
    int movesSearched = 0;
    // Loop over all the generated moves
    for (int i = 0; i < moveList.count; i++) {
        int mv = moveList.list[i];
        // Bookmark current of board
        clone = *board;

        // Increment half move
        sTable->ply++;

        board->repetitionIndex++;
        board->repetitionTable[board->repetitionIndex] = board->key;

        // Play move if move is legal
        if (!makeMove(board, mv, MoveType::AllMoves)) {
            // Decrement move and move onto next move
            sTable->ply--;
            board->repetitionIndex--;
            continue;
        }

        // Increment legal moves
        legalMoves++;

        // Full depth search
        if (movesSearched == 0) {
            // Do normal alpha-beta search
            score = -negamax(board, tt, sInfo, sTable, -beta, -alpha, depth - 1);
        } else {
            // Late move reduction (LMR)
            if (movesSearched >= FULL_DEPTH_MOVES && depth >= REDUCTION_LIMIT && !inCheck &&
                getPromoted(mv) == EMPTY && !isCapture(mv))
                score = -negamax(board, tt, sInfo, sTable, -alpha - 1, -alpha, depth - 2);
            else
                // Hack to ensure full depth search is done
                score = alpha + 1;

            // PVS (Principal Variation Search)
            if (score > alpha) {
                // re-search at full depth but with narrowed score bandwith
                score = -negamax(board, tt, sInfo, sTable, -alpha - 1, -alpha, depth - 1);

                // if LMR fails re-search at full depth and full score bandwith
                if ((score > alpha) && (score < beta))
                    score = -negamax(board, tt, sInfo, sTable, -beta, -alpha, depth - 1);
            }
        }
        // Decrement ply and restore board state
        sTable->ply--;
        board->repetitionIndex--;

        *board = clone;

        if (sInfo->stop)
            return 0;

        movesSearched++;

        // If current move is better, update move
        if (score > alpha) {
            // Switch flag to EXACT(PV node) from ALPHA (fail-low node)
            flag = F_EXACT;
            if (!isCapture(mv))
                // Store history move
                sTable->historyMoves[getPiece(mv)][getTarget(mv)] += depth;

            // Principal Variation (PV) node
            alpha = score;
            // Write PV move
            sTable->pvTable[sTable->ply][sTable->ply] = mv;
            // Copy move from deeper ply into current ply
            for (int next = sTable->ply + 1; next < sTable->pvLength[sTable->ply + 1]; next++) {
                sTable->pvTable[sTable->ply][next] = sTable->pvTable[sTable->ply + 1][next];
            }
            // Adjust pv length
            sTable->pvLength[sTable->ply] = sTable->pvLength[sTable->ply + 1];

            // Fail-hard beta cutoff
            if (score >= beta) {
                // Store hash entry with score equal to beta
                tt->store(*board, *sTable, beta, depth, F_BETA);

                if (!isCapture(mv)) {
                    // Move 1st killer move to 2nd killer move
                    sTable->killerMoves[1][sTable->ply] = sTable->killerMoves[0][sTable->ply];
                    // Update 1st killer move to current move
                    sTable->killerMoves[0][sTable->ply] = mv;
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
            return -MATE_VALUE + sTable->ply;
        } else {
            // If not check, return stalemate score
            return 0;
        }
    }
    // Store hash entry with score equal to alpha
    tt->store(*board, *sTable, alpha, depth, flag);
    // Move that failed low
    return alpha;
}

static void getCPOrMateScore(int score)
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

void workerSearchPos(SearchWorkerData* data)
{
    int score = 0;
    // clearSearchTable(data->tt, data->sInfo);
    int alpha = -INF, beta = INF;
    for (int currDepth = 1; currDepth <= data->sInfo->searchDepth; currDepth++) {
        if (data->sInfo->stop)
            break;
        // Enable followPV
        data->sTable->followPV = true;

        score = negamax(data->board, data->tt, data->sInfo, data->sTable, alpha, beta, currDepth);
        // Aspiration window
        if ((score <= alpha) || (score >= beta)) {
            alpha = -INF;
            beta = INF;
            continue;
        }
        // Set up the window for the next iteration
        alpha = score - 50;
        beta = score + 50;
        if (data->threadID == 0) {
            if (data->sTable->pvLength[0]) {
                std::cout << "info score ";
                getCPOrMateScore(score);
                std::cout << " depth " << currDepth << " nodes " << data->sTable->nodes << " time "
                          << (getCurrTime() - data->sInfo->startTime) << " pv";
                for (int i = 0; i < data->sTable->pvLength[0]; i++)
                    std::cout << " " << moveToStr(data->sTable->pvTable[0][i]);
                std::cout << "\n";
            }
        }
    }
}

void searchPos(Board* board, HashTable* tt, SearchInfo* sInfo, SearchTable* sTable)
{
    if (sInfo->useBook && !outOfBookMoves) {
        int bookMove = getBookMove(*board);
        if (bookMove != 0) {
            std::cout << "bestmove " << moveToStr(bookMove) << "\n";
            return;
        }
    }

    createSearchWorkers(board, sInfo, sTable, tt);
    //std::cout << "Created " << sInfo->threadCount << " thread(s)...\n";

	joinSearchWorkers(sInfo);
	//std::cout << "Joined " << sInfo->threadCount << " thread(s)...\n";
}

#if 0
// This is the previous single threaded searching function
void searchPos(Board* board, HashTable* tt, SearchInfo* sInfo)
{
    if (sTable->useBook && !outOfBookMoves) {
        int bookMove = getBookMove(*board);
        if (bookMove != 0) {
            std::cout << "bestmove " << moveToStr(bookMove) << "\n";
            return;
        }
    }

    int score = 0;
    clearSearchTable(tt, sInfo);
    int alpha = -INF, beta = INF;
    for (int currDepth = 1; currDepth <= sTable->searchDepth; currDepth++) {
        if (sTable->stop)
            break;
        // Enable followPV
        followPV = true;

        score = negamax(board, tt, sInfo, alpha, beta, currDepth);
        // Aspiration window
        if ((score <= alpha) || (score >= beta)) {
            alpha = -INF;
            beta = INF;
            continue;
        }
        // Set up the window for the next iteration
        alpha = score - 50;
        beta = score + 50;
        if (sTable->pvLength[0]) {
            std::cout << "info score ";
            getCPOrMateScore(score);
            std::cout << " depth " << currDepth << " nodes " << sTable->nodes << " time "
                      << (getCurrTime() - sTable->startTime) << " pv";
            for (int i = 0; i < sTable->pvLength[0]; i++)
                std::cout << " " << moveToStr(sTable->pvTable[0][i]);
            std::cout << "\n";
        }
    }
}

#endif
