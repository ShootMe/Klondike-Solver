Klondike-Solver
===============

Klondike (Patience) Solver that finds minimal length solutions.

KlondikeSolver [/DC] [/D] [/G] [/O] [/M] [/S] [/F] [/R] [FilePath]

/DRAW # [/DC #] - Sets the draw count to use when solving. Defaults to 1.

/DECK str [/D str] - Loads the deck specified by the string.

/GAME # [/G #] - Loads a random game with seed #.

FilePath - Solves deals specified in the file.

/R - Replays solution to output if one is found.

/MOVES /MVS - Will output a compact list of moves made when a solution is found.

/MULTI # [/M #] - Uses # threads to solve deals. Only works when solving minimally.

/OUT # [/O #] - Sets the output method of the solver. Defaults to 0, 1 for Pysol, 2 for minimal output.

/STATES # [/S #] - Sets the maximum number of game states to evaluate before terminating. Defaults to 5,000,000.

/FAST [/F] - Run the solver in a best attempt mode, which is faster, but not guaranteed to give minimal solution or one at all.