''' a simple sudoku solver
'''

from dataclasses import dataclass
from typing import List, Optional, Tuple

@dataclass
class Choice:
    location: int # 0 - 81
    value: int # 1 - 9
    choice_count: int # 1 - 9


def toxyz(i:int) -> Tuple[int, int, int]:
    assert(i >=0 and i < 81)
    x = i % 9
    y = i // 9
    z = (x // 3) + (y // 3) * 3
    assert(x >= 0 and x < 9)
    assert(y >= 0 and y < 9)
    assert(z >= 0 and z < 9)
    return (x, y, z)


def locations_for_column(column: int) -> List[int]:
    return [y * 9 + column for y in range(9)]


def locations_for_row(row: int) -> List[int]:
    return [x + row * 9 for x in range(9)]


def locations_for_square(square: int) -> List[int]:
    x = (square % 3) * 3
    y = (square // 3) * 3
    o = x + y * 9
    return [o + (i % 3) + (i // 3) * 9 for i in range(9)]


class Board:
    def __init__(self):
        self.sequence = []
        self.board = [0] * 81
        # keep list of available choices for each slice
        self.column = [set(range(1, 10)) for _ in range(9)]
        self.row = [set(range(1, 10)) for _ in range(9)]
        self.square = [set(range(1, 10)) for _ in range(9)]
        # keep a count of choices for each square
        self.choice_counts = [9] * 81

    def choices(self, location) -> List[int]:
        assert(location >=0 and location < 81)
        x, y, z = toxyz(location)
        return self.column[x].intersection(self.row[y]
            .intersection(self.square[z]))

    def apply(self, choice:Choice):
        self.sequence.append(choice)
        self.board[choice.location] = choice.value
        x, y, z = toxyz(choice.location)
        assert(x + y * 9 == choice.location)
        v = choice.value
        assert(v > 0 and v <= 9)
        self.column[x].remove(v) 
        self.row[y].remove(v) 
        self.square[z].remove(v)
        self.update_choices(x, y, z)
        assert(self.choice_counts[choice.location] == 0)


    def update_choices(self, x: int, y: int, z: int):
        affected = set(locations_for_column(x)).union(
            set(locations_for_row(y)).union(
                set(locations_for_square(z))))
        for a in affected:
            if self.board[a] == 0:
                self.choice_counts[a] = len(self.choices(a))
            else:
                self.choice_counts[a] = 0

    def get_choice_counts(self) -> List[int]:
        return self.choice_counts

    def pop(self) -> Optional[Choice]:
        choice = self.sequence.pop()
        if choice is None:
            return None
        x, y, z = toxyz(choice.location)
        v = choice.value
        self.column[x].add(v) 
        self.row[y].add(v) 
        self.square[z].add(v)
        assert(len(self.column[x]) <= 9)
        assert(len(self.row[y]) <= 9)
        assert(len(self.square[z]) <= 9)
        self.update_choices(x, y, z)
        return choice

    def all_done(self) -> bool:
        return len(self.sequence) == 81

    def verify(self):
        '''check all rows, columns, squares have 9 different values'''
        for i in range(9):
            c = [self.board[j] for j in locations_for_column(i)]
            r = [self.board[j] for j in locations_for_row(i)]
            s = [self.board[j] for j in locations_for_square(i)]
            assert(len(set(c)) == 9)
            assert(len(set(r)) == 9)
            assert(len(set(s)) == 9)

    def __str__(self):
        lines = []
        for y in range(9):
            line = [str(v) for v in self.board[y * 9:y * 9 + 9]]
            lines.append("".join(line))

        return "\n".join(lines)


def apply_fixed_values(board, fixed_values):
    '''set the fixed values specified in the puzzle we're solving
    '''
    for i in range(len(fixed_values)):
        v = fixed_values[i]
        if v > 0:
            board.apply(Choice(i, v, 0))


def count_choices(board) -> List[int]:
    return board.get_choice_counts()


def all_done(board) -> bool:
    d = board.all_done()
    if d:
        board.verify()
    return d

def make_choice(board, location:int, last_choice:int) -> bool:
    '''apply the first choice for the current location that is higher than the
    last guess'''

    choices = board.choices(location)
    len_choices = len(choices)
    assert(len_choices == board.get_choice_counts()[location])
    choices = [choice for choice in choices if choice > last_choice]
    if len(choices) == 0:
        print("no choices available at {}".format(
            toxyz(location)))
        return False
    choices.sort()
    choice = choices[0]
    board.apply(Choice(location, choice, len_choices))
    print("chose at {} value {} out of {}".format(
        toxyz(location), choice, choices))
    return True


def rewind(board):
    '''unapply last choice'''
    print("rewind")
    return board.pop()


def sort_locations_by_number_of_choices(choice_counts: List[int]) -> List[int]:
    '''takes a list of the count of available choices for every square on the
    board and sorts by choice count, then returns only the locations for which
    the choice count is non zero (i.e. not already filled'''
    assert(len(choice_counts) == 81)
    locations = range(len(choice_counts))
    def f(x):
        return x[1]
    location_choices = list(zip(locations, choice_counts))
    location_choices.sort(key = f)
    return [lc[0] for lc in location_choices if lc[1] > 0]


def solve(fixed_values):
    '''sort locations by ascending number of choices. Iterate through
    locations, making guesses in numerical order. After each guess, update
    number of choices for affected row, column, square and resort. If we get
    stuck (no valid choices left), then rewind to the last guess and try the
    next number'''
    
    board = Board()
    apply_fixed_values(board, fixed_values)
    print(board)
    if all_done(board):
        return True

    while True:
        choice_counts = count_choices(board)
        locations = sort_locations_by_number_of_choices(choice_counts)
        assert(len(locations) > 0)
        location = locations[0]
        last_choice = 0
        if not make_choice(board, location, last_choice):
            while True:
                location_choice = rewind(board)
                if location_choice is None:
                    # no where to go, give up
                    return False
                lc = location_choice
                if lc.choice_count == 0:
                    # this was a fixed choice, give up
                    return False
                if lc.choice_count == 1:
                    # no other choices for this location, rewind again
                    continue
                if make_choice(board, lc.location, lc.value):
                   break
 
        if all_done(board):
            print(board)
            return True

def main():
    print("solving")
    r = solve(
          [0, 1, 6, 3, 7, 5, 8, 0, 4,
           5, 2, 4, 6, 8, 0, 9, 0, 3,
           7, 0, 0, 0, 0, 0, 1, 0, 5,
           0, 0, 0, 0, 9, 0, 0, 5, 0,
           0, 6, 9, 0, 0, 0, 0, 0, 8,
           8, 0, 2, 7, 0, 6, 0, 0, 0,
           2, 0, 0, 0, 0, 0, 0, 0, 6,
           0, 0, 0, 1, 6, 0, 0, 4, 7,
           0, 4, 0, 5, 0, 9, 0, 8, 0])
    if r:
        print("success!")
    else:
        print("failed")


if __name__ == "__main__":
    main()
