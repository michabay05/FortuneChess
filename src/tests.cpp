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
        _MY_ASSERT(b.enpassant == ENPASSANT[i],
                   format_fail_str(STR(b.enpassant), STR(ENPASSANT[i])));
        _MY_ASSERT(b.castling == CASTLING[i], format_fail_str(STR(b.castling), STR(CASTLING[i])));
    }
    print_completion("parse_fen");
}

void polyKeyGeneration()
{
    const std::string FEN_LIST[9] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2",
        "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2",
        "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
        "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR b kq - 0 3",
        "rnbq1bnr/ppp1pkpp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR w - - 0 4",
        "rnbqkbnr/p1pppppp/8/8/PpP4P/8/1P1PPPP1/RNBQKBNR b KQkq c3 0 3",
        "rnbqkbnr/p1pppppp/8/8/P6P/R1p5/1P1PPPP1/1NBQKBNR b Kkq - 0 4",
    };

    const uint64_t KEY_LIST[9] = {
        0x463b96181691fc9c, 0x823c9b50fd114196, 0x0756b94461c50fb0,
        0x662fafb965db29d4, 0x22a48b5a8e47ff78, 0x652a607ca3f242c1,
        0x00fdd303c946bdd9, 0x3c8123ea7b067637, 0x5c3f9b829b279560,
    };

    Board b;

    for (int i = 0; i < 9; i++) {
        b.parseFen(FEN_LIST[i]);
        uint64_t generatedPolyKey = genPolyKey(b);
        _MY_ASSERT(generatedPolyKey == KEY_LIST[i], STR(i));
        std::cout << "Completed " << i << " checks! ("<< FEN_LIST[i] << ")\n";
    }
}

} // namespace test