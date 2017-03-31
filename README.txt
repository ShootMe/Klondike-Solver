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

========================
NOTES:

The Deck format is in the order a deck of cards is dealt to the board.  Each card is represented by a 3 digit long numerical character.  The first two digits are the value of the card:
01 for an ace, 02 for a 2, 11 for a jack, 13 for a king.  The third digit represents the suit. 1 for clubs, 2 for diamonds, 3 for hearts, 4 for spades.
Therefore an Ace of spaces is 014.  A 4 of diamonds is represented by 042.
You can more quickly convert a deck of cards by using the createadeckstring.xls excel sheet.

When using the /MOVES command, the program will produce the moves neccesary such that you could execute the winning condition.  The codex for moves is as follows:
	DR# is a draw move that is done # number of times. ie) DR2 means draw twice, if draw count > 1 it is still DR2.
	NEW is to represent the moving of cards from the Waste pile back to the stock pile. A New round.
	F# means to flip the card on tableau pile #. 
	XY means to move the top card from pile X to pile Y.
		X will be 1 through 7, W for Waste, or a foundation suit character. 'C'lubs, 'D'iamonds, 'S'pades, 'H'earts
		Y will be 1 through 7 or the foundation suit character.
	XY-# is the same as above except you are moving # number of cards from X to Y.
	
