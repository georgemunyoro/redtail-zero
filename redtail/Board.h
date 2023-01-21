#include<string>
#include <vector>
#include <unordered_map>
#pragma once

static std::string PIECE_CHAR_MAP = "PNBRQKpnbrqk. *";

enum {
    WhitePawn,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,
    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing,
    Empty,
    OffBoard
};

const int VICTIM_SCORE[14] = { 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600, 0, 0 };

enum {
    White,
    Black,
    None
};

typedef struct {
    int move;
    int score;
} S_MOVE;

enum {
    TT_EXACT,
    TT_ALPHA,
    TT_BETA
};

typedef struct {
    int flag;
    int score;
    int depth;
} TT_ENTRY;

typedef struct {
    int starttime;
    int stoptime;
    int depth;
    int depthset;
    int timeset;
    int quit;
    int stopped;
    int movestogo;
    int infinite;
    long nodes;
    float fh;
    float fhf;
} S_SEARCHINFO;

typedef unsigned long long u64;

typedef struct
{
    u64 pos_key;
    int move;
} PV_ENTRY;

typedef struct
{
    PV_ENTRY* p_table;
    u64 num_entries;
} PV_TABLE;

class Board
{
private:
	std::string castling;
	std::string en_pas;

    int white_king_position;
    int black_king_position;

    // Useful utils
    bool is_square_attacked(int pos, int attacker);
    int get_color(int piece);

    std::vector<int> move_history;

    // Hash keys for position key generation
    u64 piece_keys[13][128];
    u64 turn_key;

    void init_hash_keys();

    int ply;
    int hisPly;

public:
	Board();
	~Board();

    // Most important property. This contains the actual board state
    // represented as a array os size 128.
    // en.wikipedia.org/wiki/0x88
    std::vector<int> squares;

	void set_fen(std::string fen);
	void draw();
    void clear_board();
    void make_move(int move);
    void undo_last_move();
    int perft(int depth);

    int nodes;

    // Move Generation
    std::vector<S_MOVE> generate_pseudo_moves();
    std::vector<S_MOVE> generate_moves();

    std::string get_ref(int position);
    std::string get_move_ref(int move);

	std::unordered_map<u64, int> trans_table;
    std::unordered_map<u64, TT_ENTRY> transposition_table;

    // Stores the color whose turn it is to play
    int turn;
    int switch_turn();

    bool in_check();
    bool is_opponent_in_check();

    // Evaluation
    // int search(S_SEARCHINFO *info);
    int search();
    int get_score();
    // int quiesce(int alpha, int beta, S_SEARCHINFO *info);
    int quiesce(int alpha, int beta);
    // int alpha_beta(int alpha, int beta, int depth, S_SEARCHINFO *info, bool do_null);
    int alpha_beta(int alpha, int beta, int depth);
    int search_position(S_SEARCHINFO *info);
    void clear_for_search(S_SEARCHINFO* info);

    u64 position_key();

    bool move_exists(int move);
    bool is_capture(int move);

    int mvv_lva_scores[13][13];
    void init_mvv_lva();

    // Principal Variation Table
	const int PV_SIZE = 0x100000 * 5;
    PV_TABLE pv_table[1];
    void init_pv_table();
    void clear_pv_table();
    void store_pv_move(int move);
    int probe_pv_table();
    int pv_array[64];
    int get_pv_line(int depth);
};

