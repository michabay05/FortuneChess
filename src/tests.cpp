#include "tests.hpp"

namespace test
{

static void print_completion(const std::string& func_name)
{
    std::cout << "tests::" << func_name << "(); PASSED\n";
}

static const char* format_fail_str(const std::string& actual, const std::string& expected)
{
    std::stringstream ss;
    ss << "  Actual: " << actual << "\n";
    ss << "Expected: " << expected << "\n\n";
    return ss.str().c_str();
}

#define STR(x) std::to_string((x))

void parseFen()
{
    const Color SIDE[4] = {WHITE, WHITE, BLACK, WHITE};
    const Sq ENPASSANT[4] = {NOSQ, NOSQ, NOSQ, E6};
    const uint8_t CASTLING[4] = {0b1111, 0b1111, 0b0, 0b1111};

    Board b;
    for (int i = 0; i < 4; i++) {
        b = Board();
        b.parseFen(FEN_POSITIONS[i + 1]);

        _MY_ASSERT(b.side == SIDE[i], format_fail_str(STR(b.side), STR(SIDE[i])));
        _MY_ASSERT(b.enpassant == ENPASSANT[i], format_fail_str(STR(b.enpassant), STR(ENPASSANT[i])));
        _MY_ASSERT(b.castling == CASTLING[i], format_fail_str(STR(b.castling), STR(CASTLING[i])));
    }
    print_completion("parse_fen");
}

void moveGeneration()
{
    const std::string FEN_STRS[4] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    };
    const int NODE_COUNT[4] = {
        197'281,
        4'085'603,
        422'333,
        2'103'487,
    };
    const int PERFT_DEPTH = 4;

    for (int i = 0; i < 4; i++) {
        Board b;
        b.parseFen(FEN_POSITIONS[i]);
        uint64_t nodes = perftTest(b, PERFT_DEPTH, false);
        _MY_ASSERT(nodes == NODE_COUNT[i], format_fail_str(STR(nodes), STR(NODE_COUNT[i])));
    }
    print_completion("moveGeneration");
}

} // namespace test