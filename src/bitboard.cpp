#include "defs.hpp"

void printBits(const uint64_t bitboard)
{
    std::cout << "\n";
    for (int r = 0; r < 8; r++) {
        std::cout << "  " << 8 - r << " |";
        for (int f = 0; f < 8; f++)
            std::cout << " " << (getBit(bitboard, r * 8 + f) ? "1" : ".");
        std::cout << "\n";
    }
    std::cout << "      - - - - - - - -\n      a b c d e f g h\n";
    std::cout << "\n\n      Decimal: " << bitboard << "\n      Hexadecimal: 0x" << std::hex
              << bitboard << std::dec << "\n";
}
