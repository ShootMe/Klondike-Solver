Klondike-Solver
===============

Klondike (Patience) Solver that finds minimal length solutions.

KlondikeSolver [/DC] [/D] [/G] [/O] [/MP] [/S] [/T] [/R] [Path]

  /DRAW # [/DC #]       Sets the draw count to use when solving. Defaults to 1.

  /DECK str [/D str]    Loads the deck specified by the string.

  /GAME # [/G #]        Loads a random game with seed #.

  FilePath              Solves deals specified in the file.

  /R                    Replays solution to output if one is found.

  /MULTI [/MP]          Uses 2 threads to solve deals. Only works when solving minimally.

  /OUT # [/O #]         Sets the output method of the solver. Defaults to 0, 1 for Pysol, 2 for minimal output.

  /STATES # [/S #]      Sets the maximum number of game states to evaluate before terminating. Defaults to 1,000,000.

  /TRY # [/T #]         Run the solver # of times in a best attempt mode, which is faster, but not guaranteed to give minimal solution.