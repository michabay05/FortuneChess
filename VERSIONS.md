# CHESS ENGINE

## v1.1.1
- Improved the speed of quiescence search by generating and sorting capture moves only instead of all possible moves

## v1.1.0
- Implemented a Polyglot opening book and integrated it with the UCI protocol

## v1.0
List of implemented features
- Bitboard representation
- Precomputed attack tables(for every piece on every square)
- Use of magic bitboards for sliding pieces(bishop, rook, and queen)
- Move generation
- Move making via the Copy/Make approach
- Performance tests for move generations (perft)
- Zobrist hashing
- Material and Positional square table evaluations
- Evaluation bonus and penalties based on rules of thumbs
	- BONUS: controlling open/semi-open files, passed pawns, bishop pair
	- PENALTY: isolated pawns, doubled pawns, exposed king
- Tapered evaluation(Interpolation between opening and endgame evaluations based the phase of the game)
- Negamax search algorithm with alpha-beta pruning
- Search Optimizations
	- PV line
	- Late Move Reduction
	- Null Move Pruning
	- Transposition table
- UCI protocol
	- List of implemented UCI commands
		- `quit`
		- `ucinewgame`
		- `uci`
		- `go`
		- `position`
