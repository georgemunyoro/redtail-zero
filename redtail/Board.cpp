#include <string>
#include <vector>
#include <iostream>
#include <assert.h>
#include <algorithm>
#include "Utils.h"
#include "Board.h"

Board::Board()
{
	init_hash_keys();
	init_pv_table();
	init_mvv_lva();
	move_history = std::vector<int>();
	squares = std::vector<int>(128, OffBoard);
	clear_board();
}

Board::~Board() {
	free(pv_table->p_table);
}

// Reset board state such that on-board squares are marked empty,
// and offboard squares marked offboard.
void Board::clear_board() {
	for (int i = 0; i < 128; ++i) {
		if ((i & 0x88) == 0) squares[i] = Empty;
	}
}

// Set the board position using a FEN string
// https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
void Board::set_fen(std::string fen)
{
	clear_board();
	std::vector<std::string> split_fen = split(fen, ' ');

	if (split_fen.size() != 6) {
		std::cerr << "Incorrect FEN format, exiting..." << std::endl;
		exit(1);
	}

	int index = 0;
	for (char piece : split_fen[0])
	{
		switch (piece)
		{
		case 'R':
			squares[index] = WhiteRook;
			break;
		case 'N':
			squares[index] = WhiteKnight;
			break;
		case 'B':
			squares[index] = WhiteBishop;
			break;
		case 'Q':
			squares[index] = WhiteQueen;
			break;
		case 'K':
			squares[index] = WhiteKing;
			white_king_position = index;
			break;
		case 'P':
			squares[index] = WhitePawn;
			break;

		case 'r':
			squares[index] = BlackRook;
			break;
		case 'n':
			squares[index] = BlackKnight;
			break;
		case 'b':
			squares[index] = BlackBishop;
			break;
		case 'q':
			squares[index] = BlackQueen;
			break;
		case 'k':
			squares[index] = BlackKing;
			black_king_position = index;
			break;
		case 'p':
			squares[index] = BlackPawn;
			break;

		case '/':
			index += 7;
			break;
		case '2':
			++index;
			break;
		case '3':
			index += 2;
			break;
		case '4':
			index += 3;
			break;
		case '5':
			index += 4;
			break;
		case '6':
			index += 5;
			break;
		case '7':
			index += 6;
			break;
		case '8':
			index += 7;
			break;
		}
		++index;
	}

	// TODO: Implement complete FEN parser
	castling = split_fen[2];
	en_pas = split_fen[3];
	// fen_half_moves = split_fen[4]
	// fen_ply = split_fen[5]

	turn = split_fen[1] == "b" ? Black : White;
}

//
//  Print out a visual ASCII representation of the board:
//
//	r n b q k b n r
//	p p p p p p p p
//	. . . . . . . .
//	. . . . . . . .
//	. . . . . . . .
//	. . . . . . . .
//	P P P P P P P P
//	R N B Q K B N R
//
void Board::draw()
{
	int index = 0;
	for (int i = 0; i < squares.size(); ++i)
	{
		if (index == 8)
		{
			std::cout << std::endl;
			index = 0;
		}

		if ((i & 0x88) == 0)
		{
			printf("%2c", PIECE_CHAR_MAP[squares[i]]);
			++index;
		}
	}

	printf("\n");

	for (int i = 0; i < squares.size(); ++i)
	{
		if (index == 8)
		{
			std::cout << std::endl;
			index = 0;
		}

		if ((i & 0x88) == 0)
		{
			printf("%2c", is_square_attacked(i, White) ? '*' : '-');
			++index;
		}
	}

	std::cout << std::endl <<
		"turn: " << "WB "[turn] << std::endl;

	int w_king_position = std::find(squares.begin(), squares.end(), WhiteKing) - squares.begin();
	int b_king_position = std::find(squares.begin(), squares.end(), BlackKing) - squares.begin();

	std::cout << "white in check: " << is_square_attacked(w_king_position, Black) << " " << get_ref(w_king_position) << std::endl;
	std::cout << "black in check: " << is_square_attacked(b_king_position, White) << " " << get_ref(b_king_position) << std::endl;
	std::cout << "score: " << get_score() << std::endl;
	printf("%X\n", position_key());
}

int Board::switch_turn() {
	return turn ^= 1;
}

// Get the color of a piece
int Board::get_color(int piece) {
	if (piece <= WhiteKing) {
		return White;
	}
	else if (piece >= BlackPawn && piece <= BlackKing) {
		return Black;
	}
	return None;
}

std::string Board::get_ref(int position) {
	return "abcdefgh"[position % 8] + std::to_string((int)(8 - floor(position / 16)));
}

std::string Board::get_move_ref(int move) {
	return get_ref((move >> 20) & 0xff) + get_ref((move >> 12) & 0xff);
}

void Board::make_move(int move) {
	bool is_white_promotion = ((move >> 8) & 0xf) == WhitePawn && floor(((move >> 12) & 0xf) / 16) == 0;
	bool is_black_promotion = ((move >> 8) & 0xf) == BlackPawn && floor(((move >> 12) & 0xf) / 16) == 7;

	int result_piece = (move >> 8) & 0xf;

	if (is_white_promotion) result_piece = WhiteQueen;
	if (is_black_promotion) result_piece = BlackQueen;

	move_history.push_back(move);
	squares[(move >> 12) & 0xff] = result_piece;
	squares[(move >> 20) & 0xff] = Empty;
	switch_turn();

	if (((move >> 8) & 0xf) == WhiteKing)
	{
		white_king_position = (move >> 12) & 0xff;
	}

	else if (((move >> 8) & 0xf) == BlackKing)
	{
		black_king_position = (move >> 12) & 0xff;
	}
}

void Board::undo_last_move() {
	int last_move = move_history[move_history.size() - 1];
	squares[(last_move >> 20) & 0xff] = (last_move >> 8) & 0xf;
	squares[(last_move >> 12) & 0xff] = (last_move >> 4) & 0xf;
	move_history.pop_back();
	switch_turn();
}

int Board::perft(int depth) {
	std::vector<S_MOVE> moves = generate_moves();
	int nodes = 0;

	if (depth == 0) return nodes + 1;

	for (S_MOVE move : moves)
	{
		make_move(move.move);
		nodes += perft(depth - 1);
		undo_last_move();
	}

	return nodes;
}

u64 rand_key()
{
	return (u64)rand() + ((u64)rand() << 15) + ((u64)rand() << 30) + ((u64)rand() << 45) + (((u64)rand() & 0xf) << 60);
}

void Board::init_hash_keys()
{
	for (int i = 0; i < 13; ++i)
	{
		for (int j = 0; j < 128; ++j)
		{
			piece_keys[i][j] = rand_key();
		}
	}
	turn_key = rand_key();
}

u64 Board::position_key()
{
	u64 final_key = 0;
	int piece = Empty;
	for (int i = 0; i < squares.size(); ++i)
	{
		piece = squares[i];
		if ((i & 0x88) == 0 && piece != Empty)
		{
			final_key ^= piece_keys[piece][i];
		}
	}
	if (turn == White) final_key ^= turn_key;
	return final_key;
}

bool Board::in_check()
{
	int king_position = std::find(squares.begin(), squares.end(), turn == White ? WhiteKing : BlackKing) - squares.begin();
	return is_square_attacked(king_position, turn ^ 1);
}

bool Board::is_opponent_in_check()
{
	int king_position = std::find(squares.begin(), squares.end(), turn == White ? BlackKing : WhiteKing) - squares.begin();
	return is_square_attacked(king_position, turn);
}

bool Board::move_exists(int move)
{
	for (S_MOVE c_move : generate_moves())
	{
		if (c_move.move == move) return true;
	}
	return false;
}

void Board::init_pv_table()
{
	pv_table->num_entries = PV_SIZE / sizeof(PV_ENTRY);
	pv_table->num_entries -= 5;
	if (pv_table->p_table != NULL) free(pv_table->p_table);
	pv_table->p_table = (PV_ENTRY*)malloc(pv_table->num_entries * sizeof(PV_ENTRY));
	clear_pv_table();

	std::cout << "pv table init complete with " << pv_table->num_entries << " entries\n";
}

void Board::clear_pv_table()
{
	PV_ENTRY* pv_entry;
	for (pv_entry = pv_table->p_table; pv_entry < pv_table->p_table + pv_table->num_entries; ++pv_entry)
	{
		pv_entry->pos_key = 0ULL;
		pv_entry->move = 0;
	}
}

void Board::store_pv_move(int move)
{
	int index = position_key() % pv_table->num_entries;

	assert(index >= 0 && index <= pv_table->num_entries - 1);

	pv_table->p_table[index].move = move;
	pv_table->p_table[index].pos_key = position_key();
}

int Board::probe_pv_table()
{
	int index = position_key() % pv_table->num_entries;
	if (pv_table->p_table[index].pos_key == position_key())
	{
		return pv_table->p_table[index].move;
	}
	return 0;
}

int Board::get_pv_line(int depth)
{
	assert(depth < 64);

	int move = probe_pv_table();
	int count = 0;

	while (move != 0 && count < depth)
	{
		if (move_exists(move))
		{
			make_move(move);
			pv_array[count++] = move;
		}
		else {
			break;
		}
		move = probe_pv_table();
	}

	for (int i = 0; i < count; ++i) undo_last_move();
	return count;
}

void Board::init_mvv_lva()
{
	for (int attacker = WhitePawn; attacker <= BlackKing; ++attacker)
	{
		for (int victim = WhitePawn; victim <= BlackKing; ++victim)
		{
			mvv_lva_scores[victim][attacker] = VICTIM_SCORE[victim] + 6 - (VICTIM_SCORE[attacker] / 100);
		}
	}
}

