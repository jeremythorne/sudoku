a simple sudoku solver

at each step find the location with least choices and make one. If no locations
with choices left, rewind until the last location we had more than one choice
and choose the next option. To speed up calculating available choices, keep a
list of the available choices for each row, column and square.
