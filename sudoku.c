// a simple sudoku solver

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef uint16_t BitField;

struct Choice {
    uint8_t location; // 0 - 81
    uint8_t value; //1 - 9
    uint8_t choice_count; // 0 - 9
};

struct OptionalChoice {
    bool valid;
    struct Choice choice;
};

struct XYZ {
    uint8_t x;
    uint8_t y;
    uint8_t z;
};

struct Array {
    uint8_t *data;
    uint8_t len;
    uint8_t capacity;
};

struct ArrayChoice {
    struct Choice *data;
    uint8_t len;
    uint8_t capacity;
};

struct XYZ toxyz(uint8_t i) {
    assert(i < 81);
    uint8_t x = i % 9;
    uint8_t y = i / 9;
    uint8_t z = (x / 3) + (y / 3) * 3;
    struct XYZ p = { x, y, z };
    assert(p.x < 9);
    assert(p.y < 9);
    assert(p.z < 9);
    return p;
}

void locations_for_column(uint8_t column, struct Array *locations) {
    assert(locations->capacity >= locations->len + 9);
    uint8_t o = locations->len;
    for (int y = 0; y < 9; y++) {
        locations->data[o + y] = y * 9 + column;
    }
    locations->len += 9;
}

void locations_for_row(uint8_t row, struct Array *locations) {
    assert(locations->capacity >= locations->len + 9);
    uint8_t o = locations->len;
    for (int x = 0; x < 9; x++) {
        locations->data[o + x] = x + row * 9;
    }
    locations->len += 9;
}

void locations_for_square(uint8_t square, struct Array *locations) {
    assert(locations->capacity >= locations->len + 9);
    uint8_t x = (square % 3) * 3;
    uint8_t y = (square / 3) * 3;
    uint8_t off = x + y * 9;
    uint8_t o = locations->len;
    for (int i = 0; i < 9; i++) {
        locations->data[o + i] = off + (i % 3) + (i / 3) * 9;
    }
    locations->len += 9;
}

struct Board {
    struct Choice sequence_s[81];
    struct ArrayChoice sequence;
    uint8_t board_s[81];
    struct Array board;
    BitField column[9];
    BitField row[9];
    BitField square[9];
    uint8_t choice_counts_s[81];
    struct Array choice_counts;
};

void init_board(struct Board *board) {
    struct ArrayChoice c = { board->sequence_s, 0, 81 };
    board->sequence = c; 
    memset(board->board_s, 0, 81);
    struct Array b = { board->board_s, 81, 81 };
    board->board = b; 
    // keep list of available choices for each slice as a bit field
    // bits 2 - 10 
    for (int i = 0; i < 9; i++) {
        board->column[i] = 0x3fe;
        board->row[i] = 0x3fe;
        board->square[i] = 0x3fe;
    }
    // keep a count of choices for each square
    memset(board->choice_counts_s, 9, 81);
    struct Array b2 = { board->choice_counts_s, 81, 81 };
    board->choice_counts = b2; 
}

BitField bf_intersect(BitField a, BitField b) {
    return a & b;
}

void bf_add(BitField *a, uint8_t bit) {
    assert(bit <= 9);
    BitField v = 1 << bit;
    *a |= v;
}

void bf_remove(BitField *a, uint8_t bit) {
    assert(bit <= 9);
    BitField v = 1 << bit;
    *a &= ~v;
}

uint8_t bf_num_set(BitField a) {
    uint8_t count = 0;
    a = a & 0x3fe; // ignore anything outside bits 1-10
    while(a) {
        a = a & (a - 1);
        count++;
    }
    return count;
}

BitField bf_clear_less_than_equal(BitField a, uint8_t bit) {
    BitField v = 1 << bit;
    v |= (v - 1); // this bit and all bits lower
    return a & ~v;
}

uint8_t bf_min_bit(BitField a) {
    for(int i = 1; i <= 9; i++) {
        if (a & (1 << i)) {
            return i;
        }
    }
    return 0;
}

BitField get_choices(struct Board * board, uint8_t location) {
    assert(location < 81);
    struct XYZ xyz = toxyz(location);
    return bf_intersect(board->column[xyz.x], bf_intersect(board->row[xyz.y],
            board->square[xyz.z]));
}

void append_choice(struct ArrayChoice * array, struct Choice value) {
    assert(array->len < array->capacity);
    array->data[array->len++] = value;
}

void update_choices_for_locations(struct Board * board, struct Array * locations) {
    for(int i = 0; i < locations->len; i++) {
        uint8_t location = locations->data[i];
        if (board->board.data[location] == 0) {
            board->choice_counts.data[location] = bf_num_set(get_choices(board, location)); 
        } else {
            board->choice_counts.data[location] = 0;
        }
    }    
}

void update_choices(struct Board *board, struct XYZ xyz) {
    uint8_t locations_s[27];
    struct Array locations = { locations_s, 0, 27 };
    locations_for_column(xyz.x, &locations);
    locations_for_row(xyz.x, &locations);
    locations_for_square(xyz.z, &locations);
    update_choices_for_locations(board, &locations);
}

void apply(struct Board * board, struct Choice choice) {
    // printf("apply %d %d %d\n", choice.location, choice.value, choice.choice_count);
    append_choice(&board->sequence, choice);
    board->board.data[choice.location] = choice.value;
    struct XYZ xyz = toxyz(choice.location);
    uint8_t v = choice.value;
    bf_remove(&board->column[xyz.x], v);
    bf_remove(&board->row[xyz.y], v);
    bf_remove(&board->square[xyz.z], v);
    update_choices(board, xyz);
    assert(board->choice_counts.data[choice.location] == 0);
}

struct OptionalChoice pop(struct Board *board) {
    if (board->sequence.len == 0) {
        struct OptionalChoice c = { false };
        return c;
    }
    struct OptionalChoice choice = {
        true,
        board->sequence.data[--board->sequence.len]
    };
    board->board.data[choice.choice.location] = 0;
    struct XYZ xyz = toxyz(choice.choice.location);
    uint8_t v = choice.choice.value;
    bf_add(&board->column[xyz.x], v);
    bf_add(&board->row[xyz.y], v); 
    bf_add(&board->square[xyz.z], v);
    update_choices(board, xyz);
    return choice;
}

bool all_done(const struct Board *board) {
    return board->sequence.len == 81;
}

bool verify(struct Board * board) {
    // check all rows, columns, squares have 9 different values
    for (int i = 0; i < 9; i++) {
        uint8_t column_locations_s[9];
        struct Array column_locations = { column_locations_s, 0, 9 };
        uint8_t row_locations_s[9];
        struct Array row_locations = { row_locations_s, 0, 9 };
        uint8_t square_locations_s[9];
        struct Array square_locations = { square_locations_s, 0, 9 };
        locations_for_column(i, &column_locations);
        locations_for_row(i, &row_locations);
        locations_for_square(i, &square_locations);
        BitField column = 0;
        BitField row = 0;
        BitField square = 0;
        for (int j = 0; j < 9; j++) {
            bf_add(&column, board->board.data[column_locations.data[j]]);
            bf_add(&row, board->board.data[row_locations.data[j]]);
            bf_add(&square, board->board.data[square_locations.data[j]]);
        }
        if (bf_num_set(column) != 9 ||
            bf_num_set(row) != 9 || 
            bf_num_set(square != 9)) {
            return false;
        }
    }
    return true;
}

void print_board(struct Board *board) {
    for (int i = 0; i < 9; i++) {
        int o = i * 9;
        uint8_t *p = board->board.data;
        printf("%d%d%d|%d%d%d|%d%d%d\n",
            p[o],     p[o + 1], p[o + 2],
            p[o + 3], p[o + 4], p[o + 5],
            p[o + 6], p[o + 7], p[o + 8]
        );
        if (i < 8 && i % 3 == 2) {
            printf("---+---+---\n");
        }
    }
}

void apply_fixed_values(struct Board *board, struct ArrayChoice *fixed_values) {
    // set the fixed values specified in the puzzle we're solving
    for (int i =0; i < fixed_values->len; i++) {
        struct Choice *c = &fixed_values->data[i];
        if (c->value > 0) {
            apply(board, *c);
        }
    }
}

bool make_choice(struct Board *board, uint8_t location, uint8_t last_choice) {
    // apply the first choice for the current location that is higher than the
    // last guess

    BitField choices = get_choices(board, location);
    choices = bf_clear_less_than_equal(choices, last_choice);
    uint8_t c = bf_min_bit(choices);
    if (c == 0) {
        return false;
    }
    struct Choice choice = { location, c, bf_num_set(choices)};
    apply(board, choice);
    printf(".");
    // struct XYZ xyz = toxyz(location);
    // printf("chose at %d,%d,%d value %d out of %d. Filled %d\n",
    //    xyz.x, xyz.y, xyz.z, choice.value, bf_num_set(choices),
    //    board->sequence.len);
    return true;
}

struct OptionalChoice board_rewind(struct Board *board) {
    // unapply last choice
    printf("r");
    return pop(board);
}

uint8_t best_location_by_number_of_choices(struct Array *choice_counts) {
    // takes a list of the count of available choices for every square on the
    //board and finds the lowest non-zero choice count i.e. not already filled
    assert(choice_counts->len == 81);
    uint8_t best_location = 81;
    uint8_t best_value = 10;
    for (int i = 0; i < 81; i++) {
        uint8_t n = choice_counts->data[i];
        if (n > 0 && n < best_value) {
            if (n == 1) {
                // can't do better than one choice
                return i;
            }
            best_value = n;
            best_location = i;
        }
    }
    return best_location;
}

bool solve(struct ArrayChoice *fixed_values) {
    // sort locations by ascending number of choices. Iterate through
    // locations, making guesses in numerical order. After each guess, update
    // number of choices for affected row, column, square and resort. If we get
    // stuck (no valid choices left), then rewind to the last guess and try the
    // next number
    
    struct Board board;
    init_board(&board);
    apply_fixed_values(&board, fixed_values);
    print_board(&board);
    if (all_done(&board)) {
        return true;
    }

    while (true) {
        uint8_t location = best_location_by_number_of_choices(&board.choice_counts);
        uint8_t last_choice = 0;
        if (location >= 81 || !make_choice(&board, location, last_choice)) {
            while (true) {
                struct OptionalChoice location_choice = board_rewind(&board);
                if (!location_choice.valid) {
                    // no where to go, give up
                    return false;
                }
                struct Choice *lc = &location_choice.choice;
                if (lc->choice_count == 0) {
                    // this was a fixed choice, give up
                    return false;
                }
                if (lc->choice_count == 1) {
                    // no other choices for this location, rewind again
                    continue;
                }
                if (make_choice(&board, lc->location, lc->value)) {
                   printf("\n");
                   break;
                }
            }
        }
        if (all_done(&board) && verify(&board)) {
            printf("\n");
            print_board(&board);
            return true;
        }
    }
}

void parse_str(const char *puzzle, struct ArrayChoice *fixed_values) {
    // read 9 lines of 9 characters, anything outside 1-9 is treated as 0 (aka
    // an empty square). Lines starting with # are skipped
    int n = strlen(puzzle);
    bool start_of_line = true;
    bool ignore_line = false;
    int j = 0;
    for(int i = 0; i < n && j < 81; i++) {
        if (start_of_line && puzzle[i] == '#') {
            ignore_line = true;
        }
        start_of_line = false;
        if (puzzle[i] == '\n') {
            start_of_line = true;
            ignore_line = false;
            continue;
        }
        if (ignore_line) {
            continue;
        }
        int value = puzzle[i] - '0';
        if (value < 0 || value > 9) {
            value = 0;
        }
        struct Choice choice = { j++, value, 0};
        append_choice(fixed_values, choice);
    }
}

void wikipedia(struct ArrayChoice *fixed_values) {
    //from https://en.wikipedia.org/wiki/Sudoku 
    char str[] =
    "53  7    \n"
    "6  195   \n"
    " 98    6 \n"
    "8   6   3\n"
    "4  8 3  1\n"
    "7   2   6\n"
    " 6    28 \n"
    "   419  5\n"
    "    8  79\n";
    parse_str(str, fixed_values);
    assert(fixed_values->len == 81);
}

void parse(const char *fname, struct ArrayChoice *fixed_values) {
    char buf[256];
    int n = 0;
    FILE * fp = fopen(fname, "rt");
    if (!fp) {
        return;
    }
    int c = fgetc(fp);
    while (c != EOF && n < sizeof(buf) - 2) {
        buf[n++] = c;
        c = fgetc(fp);
    }
    buf[n] = '\0';
    fclose(fp);
    parse_str(buf, fixed_values);
}

void self_test() {
    assert(bf_num_set(0x7) == 2); // ignore bit 0
    assert(bf_num_set(0x8) == 1);
    assert(bf_num_set(0x3fe) == 9);
    assert(bf_intersect(0x1, 0x1) == 0x1);
    assert(bf_intersect(0x3, 0x1) == 0x1);
    assert(bf_intersect(0x3fe, 0x3fe) == 0x3fe);
}

int main(char **argc, int argv) {
    bool r = false;
    struct Choice fixed_values_s[81];
    struct ArrayChoice fixed_values = { fixed_values_s, 0, 81 };
    self_test();
    if (argv > 1) {
        char * fname = argc[1];
        parse(fname, &fixed_values);
        if (fixed_values.len < 81) {
            printf("couldn't parse %s\n", fname);
            return -1;
        }
        printf("solving %s", fname);
    } else {
        printf("solving\n");
        wikipedia(&fixed_values);
    }
    r = solve(&fixed_values);
    printf("%s\n", r ? "success!" : "failed");
    return 0;
}
