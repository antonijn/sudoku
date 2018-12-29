#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

int popcnt(unsigned u);
int lsb(unsigned u);

typedef int sku_board[9][9];

typedef struct {
	sku_board board;
	unsigned cell_opts[9][9];
	unsigned left_in_seg[3][3];
	unsigned left_in_row[9];
	unsigned left_in_col[9];
} sku_board_ex;

struct {
	bool verbose, smart_segments;
}
opts = {
	.verbose = false,
	.smart_segments = true
};

void clear_sku(sku_board *sku);
int load_sku_file(sku_board *sku, FILE *f);
void fprint_sku_board(FILE *f, sku_board *sku);
int solve_sku(sku_board *sku);

int
main(int argc, char **argv)
{
	sku_board sku;

	for (int i = 1; i < argc; ++i) {
		char *arg = argv[i];
		char opt;
		while ((opt = *arg++)) {
			switch (opt) {
			case 'v':
				opts.verbose = true;
				break;
			case 'd':
				opts.smart_segments = false;
				break;
			}
		}
	}

	if (load_sku_file(&sku, stdin) < 0)
		exit(1);

	if (solve_sku(&sku) < 0) {
		fprintf(stderr, "could not solve sudoku\n");
		fprintf(stderr, "I commit seppuku\n");
		exit(1);
	}

	fprint_sku_board(stdout, &sku);

	return 0;
}

int
popcnt(unsigned u)
{
	int res = 0;
	while (u != 0) {
		if (u & 1)
			++res;

		u >>= 1;
	}
	return res;
}

int
lsb(unsigned u)
{
	for (int res = 1; u != 0; ++res) {
		if (u & 1)
			return res;

		u >>= 1;
	}

	return 0;
}

void clear_sku(sku_board *sku)
{
	memset(sku, 0, sizeof(sku_board));
}

int load_sku_row(int *row, FILE *f);

int load_sku_file(sku_board *sku, FILE *f)
{
	clear_sku(sku);

	for (int row = 0; row < 9; ++row)
		if (load_sku_row((*sku)[row], f) < 0)
			return -1;

	return 0;
}

int load_sku_row(int *row, FILE *f)
{
	int ch;

	for (int col = 0; col < 9; ++col) {
		ch = fgetc(f);
		if (ch == '\n' || ch == EOF)
			return 0;

		if (!isdigit(ch))
			continue;

		int number = ch - '0';
		if (number >= 1 && number <= 9)
			row[col] = number;
	}

	while ((ch = fgetc(f)) != '\n' && ch != EOF)
		;

	return 0;
}

void
fprint_sku_board(FILE *f, sku_board *sku)
{
	for (int i = 0; i < 9; ++i) {
		for (int j = 0; j < 9; ++j) {
			if ((*sku)[i][j] == 0)
				fputc(' ', f);
			else
				fputc((*sku)[i][j] + '0', f);
		}

		fputc('\n', f);
	}
}

bool is_sku_full(sku_board *sku);
bool sku_has_contradiction(sku_board_ex *sku, int *row, int *col);
int solve_sku_ex(sku_board_ex *sku);
int make_sku_board_ex(sku_board_ex *res, sku_board *orig);
int mask_cell_opts(sku_board_ex *sku, int row, int col, unsigned mask);
int input_sku_number(sku_board_ex *sku, int row, int col, int number, bool cascade);
int rethink_segments(sku_board_ex *sku, int seg_row_origin, int seg_col_origin, int number, bool cascade);
int rethink_segment(sku_board_ex *sku, int seg_row, int seg_col, int number, bool cascade);
int find_most_certain_cell(sku_board_ex *sku, int *row, int *col);
void sku_log(const char *fmt, ...);

int log_depth = 0;

int solve_sku(sku_board *board)
{
	sku_board_ex sku;
	if (make_sku_board_ex(&sku, board) < 0)
		return -1;

	sku_log("SOLVING\n");

	if (solve_sku_ex(&sku) < 0)
		return -1;

	memcpy(board, &sku.board, sizeof(sku_board));
	return 0;
}

bool
is_sku_full(sku_board *sku)
{
	for (int i = 0; i < 9; ++i)
		for (int j = 0; j < 9; ++j)
			if ((*sku)[i][j] == 0)
				return false;

	return true;
}

bool
sku_has_contradiction(sku_board_ex *sku, int *row, int *col)
{
	for (*row = 0; *row < 9; ++*row)
		for (*col = 0; *col < 9; ++*col)
			if (sku->cell_opts[*row][*col] == 0 && sku->board[*row][*col] == 0)
				return true;

	return false;
}

int
solve_sku_ex(sku_board_ex *sku)
{
	int row, col;
	assert(!sku_has_contradiction(sku, &row, &col));

	if (find_most_certain_cell(sku, &row, &col) < 0) {
		assert(is_sku_full(&sku->board));
		return 0;
	}

	unsigned cell_opts = sku->cell_opts[row][col];
	int choices = popcnt(cell_opts);

	if (choices == 1) {
		int attempt = lsb(cell_opts) - 1;
		if (input_sku_number(sku, row, col, attempt, true) < 0)
			return -1;

		return solve_sku_ex(sku);
	}

	while (cell_opts != 0) {
		int attempt = lsb(cell_opts) - 1;
		cell_opts &= ~(1 << attempt);

		sku_board_ex new_board = *sku;

		sku_log("GUESS (%d choices for (%d,%d))\n", choices, row, col);

		if (input_sku_number(&new_board, row, col, attempt, true) < 0)
			return -1;

		++log_depth;
		int rval = solve_sku_ex(&new_board);
		--log_depth;

		if (rval < 0) {
			sku_log("BAD_GUESS %d at (%d,%d)\n", attempt, row, col);
			continue;
		}

		sku_log("GOOD_GUESS %d at (%d,%d)\n", attempt, row, col);

		*sku = new_board;
		return 0;
	}

	return -1;
}

void clear_sku_board_ex(sku_board_ex *res)
{
	unsigned all_nums = 0x3FE;

	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			res->left_in_seg[i][j] = all_nums;

	for (int row = 0; row < 9; ++row) {
		for (int col = 0; col < 9; ++col) {
			res->board[row][col] = 0;
			res->cell_opts[row][col] = all_nums;
		}

		res->left_in_row[row] = all_nums;
		res->left_in_col[row] = all_nums;
	}
}

int
make_sku_board_ex(sku_board_ex *res, sku_board *orig)
{
	clear_sku_board_ex(res);

	for (int row = 0; row < 9; ++row) {
		for (int col = 0; col < 9; ++col) {
			int number = (*orig)[row][col];
			if (input_sku_number(res, row, col, number, false) < 0)
				return -1;
		}
	}

	return 0;
}

int
find_most_certain_cell(sku_board_ex *sku, int *row, int *col)
{
	int most_certain = INT_MAX;
	for (int i = 0; i < 9; ++i) {
		for (int j = 0; j < 9; ++j) {
			int certainty = popcnt(sku->cell_opts[i][j]);
			if (certainty == 0)
				continue;

			if (certainty < most_certain) {
				*row = i;
				*col = j;
				most_certain = certainty;
			}

			if (certainty == 1)
				return 0;
		}
	}

	if (most_certain == INT_MAX)
		return -1;

	return 0;
}

int
rethink_segments(sku_board_ex *sku, int seg_row_origin, int seg_col_origin, int number, bool cascade)
{
	for (int i = 1; i < 3; ++i) {
		if (rethink_segment(sku, (seg_row_origin + i) % 3, seg_col_origin, number, cascade) < 0)
			return -1;
		if (rethink_segment(sku, seg_row_origin, (seg_col_origin + i) % 3, number, cascade) < 0)
			return -1;
	}

	return 0;
}

int
rethink_segment(sku_board_ex *sku, int seg_row, int seg_col, int number, bool cascade)
{
	unsigned num_bit = 1 << number;

	/* number already present in segment */
	if ((sku->left_in_seg[seg_row][seg_col] & num_bit) == 0)
		return 0;

	bool found = false;
	int pls_row, pls_col;
	for (int i = 0; i < 3; ++i) {
		int row = seg_row*3 + i;
		for (int j = 0; j < 3; ++j) {
			int col = seg_col*3 + j;

			if ((sku->cell_opts[row][col] & num_bit) == 0)
				continue;

			if (found)
				return 0;

			pls_row = row;
			pls_col = col;
			found = true;
		}
	}

	if (!found)
		return 0;

	sku_log("SMART_SEGMENTS %d at (%d,%d)\n", number, pls_row, pls_col);
	if (mask_cell_opts(sku, pls_row, pls_col, num_bit) < 0)
		return -1;

	if (cascade)
		return input_sku_number(sku, pls_row, pls_col, number, cascade);

	return 0;
}

int
mask_cell_opts(sku_board_ex *sku, int row, int col, unsigned mask)
{
	unsigned new_opts = (sku->cell_opts[row][col] &= mask);
	if (new_opts == 0 && sku->board[row][col] == 0) {
		sku_log("CONTRADICTION at (%d,%d)\n", row, col);
		return -1;
	}

	return 0;
}

int input_sku_number(sku_board_ex *sku, int row, int col, int number, bool cascade)
{
	if (number == 0)
		return 0;

	sku_log("INPUT %d at (%d,%d)\n", number, row, col);

	unsigned num_bit = 1 << number;

	if (sku->board[row][col] != 0) {
		sku_log("ERROR (%d,%d) already filled\n", row, col);
		return -1;
	}

	if ((sku->cell_opts[row][col] & num_bit) == 0) {
		sku_log("ERROR %d not allowed at (%d,%d)\n", number, row, col);
		return -1;
	}

	assert(sku->left_in_seg[row / 3][col / 3] & num_bit);
	assert(sku->left_in_row[row] & num_bit);
	assert(sku->left_in_col[col] & num_bit);

	sku->board[row][col] = number;
	sku->cell_opts[row][col] = 0x00;

	int seg_row = row / 3;
	int seg_col = col / 3;
	sku->left_in_seg[seg_row][seg_col] &= ~num_bit;
	sku->left_in_row[row] &= ~num_bit;
	sku->left_in_col[col] &= ~num_bit;

	for (int i = 0; i < 9; ++i) {
		if (mask_cell_opts(sku, row, i, ~num_bit) < 0)
			return -1;
		if (mask_cell_opts(sku, i, col, ~num_bit) < 0)
			return -1;
	}

	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			if (mask_cell_opts(sku, seg_row*3 + i, seg_col*3 + j, ~num_bit) < 0)
				return -1;

	if (opts.smart_segments && rethink_segments(sku, seg_row, seg_col, number, cascade) < 0)
		return -1;

	return 0;
}

void
sku_log(const char *fmt, ...)
{
	if (!opts.verbose)
		return;

	va_list ap;
	va_start(ap, fmt);

	for (int i = 0; i < log_depth; ++i)
		fputs("  ", stderr);

	vfprintf(stderr, fmt, ap);

	va_end(ap);
}
