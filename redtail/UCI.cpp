#include "Board.h"
#include <iostream>
#include "Utils.h"
#include <thread>

int parse_uci_move(std::string uci_move, Board *board)
{
	std::string FILES = "abcdefgh";
	std::string RANKS = "12345678";

	int from_row = FILES.find(uci_move[0]);
	int from_col = RANKS.find(uci_move[1]);

	int target_row = FILES.find(uci_move[2]);
	int target_col = RANKS.find(uci_move[3]);

	int from_pos = (16 * (7 - from_col)) + from_row;
	int target_pos = (16 * (7 - target_col)) + target_row;

	if (uci_move.size() == 5)
	{
		if (uci_move[4] == 'q') board->squares[from_pos] = board->turn == White ? WhiteQueen : BlackQueen;
		if (uci_move[4] == 'b') board->squares[from_pos] = board->turn == White ? WhiteQueen : BlackQueen;
		if (uci_move[4] == 'n') board->squares[from_pos] = board->turn == White ? WhiteQueen : BlackQueen;
		if (uci_move[4] == 'r') board->squares[from_pos] = board->turn == White ? WhiteQueen : BlackQueen;
	}

	int move = (from_pos << 20) | (target_pos << 12) | (board->squares[from_pos] << 8) | (board->squares[target_pos] << 4) | 0;

	return move;
}

int main() {
	Board board = Board();
	const std::string START_POS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	// const std::string START_POS = "rnbqkb2/p1pp1p1p/1p2p2n/6Q1/2BPP3/8/PPP2PPP/RN2K1NR b KQq - 0 8";
	board.set_fen(START_POS);
	// position startpos moves e2e4 g8h6 d1f3 h8g8 f1c4 g7g5 d2d4 g8g7 f3g3 b7b6 c1g5 g7g5 g3g5


	int counter = 0;

	std::cout
		<< "id name Redtail\n"
		<< "id author George Munyoro\n"
		<< "id country South Africa\n"
		<< "uciok\n";

	for (;;)
	{
		std::string comm;
		std::getline(std::cin, comm);

		std::vector<std::string> comms = split(comm, ' ');

		if (comm == "isready")
		{
			std::cout << "readyok" << std::endl;
		}

		else if (comm == "uci")
		{
			std::cout << "uciok" << std::endl;
		}

		else if (comms[0] == "position")
		{
			std::string position = comms[1];

			if (position == "startpos")
			{
				board.set_fen(START_POS);

				// Series of moves from the initial starting position
				if (comms[2] == "moves")
				{
					for (int i = 3; i < comms.size(); ++i)
					{
						std::string uci_move = comms[i];

						if (uci_move.size() < 4) continue;

						int move = parse_uci_move(uci_move, &board);

						board.store_pv_move(move);
						board.make_move(move);
					}
				}
				// Absolute positioning using a FEN string
				else {

				}
			}
			else if (position == "fen") {
				std::string pos = comms[2];
				std::string col = comms[3];
				std::string cas = comms[4];
				std::string enp = comms[5];

				// std::cin >> col >> enp >> cas;

				// std::cout << pos << " " << col << " " << enp << " " << cas << std::endl;

				if (col.size() + enp.size() + cas.size() != 0)
				{
					std::string fen = pos + " " + col + " " + cas + " " + enp + " - -";
					board.set_fen(fen);
				}
			}
		}

		else if (comms[0] == "go" || comms[0] == "stop")
		{
			int move = board.search();
			std::cout << "bestmove " << board.get_move_ref(move) << std::endl;
		}

		else if (comm == "quit")
		{
			break;
		}

		// Custom UCI debugging extensions

		else if (comm == "draw")
		{
			board.draw();
		}

		else if (comm == "listmoves")
		{
			std::vector<S_MOVE> moves = board.generate_moves();
			for (S_MOVE move : moves)
			{
				board.make_move(move.move);

				// S_SEARCHINFO search_info;
				// board.clear_for_search(&search_info);

				// std::cout << board.get_move_ref(move.move) << " " << board.get_score() << " " << board.alpha_beta(-999999999, 999999999, 0) << std::endl;
				std::cout << board.get_move_ref(move.move) << " " << move.score << std::endl;
				board.undo_last_move();
			}
		}

		else if (comm == "perft")
		{
			int depth;
			std::cin >> depth;
			std::cout << board.perft(depth) << std::endl;
		}

		else if (comm == "t")
		{
			board.undo_last_move();
			board.draw();
		}

		else if (comm == "m")
		{
			std::string uci_move;
			std::cin >> uci_move;
			int move = parse_uci_move(uci_move, &board);

			board.store_pv_move(move);
			board.make_move(move);
			board.draw();

			++counter;
		}

		else if (comm == "pv")
		{
			int pv_moves = board.get_pv_line(counter);
			printf("pv (%d) ", pv_moves);
			for (int pv_num = 0; pv_num < pv_moves; ++pv_num)
			{
				printf(" %s", board.get_move_ref(board.pv_array[pv_num]).c_str());
			}
			printf("\n");
		}
	}
}
