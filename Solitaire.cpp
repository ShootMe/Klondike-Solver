#include<sstream>
#include<thread>
#include<iomanip>
#include<iostream>
#include<memory>
#include"Solitaire.h"
using namespace std;

SolitaireWorker::SolitaireWorker(Solitaire & solitaire, int maxClosedCount) {
	this->solitaire = &solitaire;
	this->maxClosedCount = maxClosedCount;
}
void SolitaireWorker::RunMinimalWorker(void * closedPointer) {
	HashMap<int> & closed = *reinterpret_cast<HashMap<int>*>(closedPointer);
	Move movesToMake[512];
	shared_ptr<MoveNode> firstNode = NULL;
	shared_ptr<MoveNode> node = NULL;
	Solitaire s = *solitaire;

	while (closed.Size() < maxClosedCount) {
		{
			unique_lock<mutex> lck(mtx);
			//Check for lowest score length
			int index = startMoves;
			while (index < 512 && open[index].size() == 0) { index++; }

			//End solver if no more states
			if (index >= 512) {
				if (numProcessing == 0) { break; }
				continue;
			}

			numProcessing++;

			//Get next state to evaluate
			openCount--;
			firstNode = open[index].top();
			open[index].pop();
		}

		//Initialize game to the found state
		s.ResetGame();
		int movesTotal = 0;
		node = firstNode;
		while (node != NULL) {
			movesToMake[movesTotal++] = node->Value;
			node = node->Parent;
		}
		while (movesTotal > 0) {
			s.MakeMove(movesToMake[--movesTotal]);
		}

		//Make any auto moves
		s.UpdateAvailableMoves();
		while (s.MovesAvailableCount() == 1) {
			Move move = s.GetMoveAvailable(0);
			s.MakeMove(move);
			firstNode = make_shared<MoveNode>(move, firstNode);
			s.UpdateAvailableMoves();
		}
		movesTotal = s.MovesMadeNormalizedCount();

		{
			unique_lock<mutex> lck(mtx);
			//Check for best solution to foundations
			if (s.FoundationCount() > maxFoundationCount || (s.FoundationCount() == maxFoundationCount && bestSolutionMoveCount > movesTotal)) {
				bestSolutionMoveCount = movesTotal;
				maxFoundationCount = s.FoundationCount();
				//cout << setw(3) << bestSolutionMoveCount << " - " << setw(2) << maxFoundationCount << " C: " << setw(9) << closed.Size() << " L: " << setw(4) << closed.MaxLength() << " U: " << setw(8) << closed.SlotsUsed() << " O: " << setw(9) << openCount << '\n';

				//Save solution
				for (int i = s.MovesMadeCount() - 1; i >= 0; i--) {
					bestSolution[i] = s[i];
				}
				bestSolution[s.MovesMadeCount()].Count = 255;
			} else if (maxFoundationCount == 52) {
				//Dont check state if above or equal to current best solution
				int helper = s.MinimumMovesLeft();
				helper += movesTotal;
				if (helper >= bestSolutionMoveCount) {
					numProcessing--;
					continue;
				}
			}
		}

		int movesAvailableCount = s.MovesAvailableCount();
		//Make available moves and add them to be evaulated
		for (int i = 0; i < movesAvailableCount; i++) {
			Move move = s.GetMoveAvailable(i);
			int movesAdded = s.MovesAdded(move);

			s.MakeMove(move);

			movesAdded += movesTotal;
			movesAdded += s.MinimumMovesLeft();
			if (maxFoundationCount < 52 || movesAdded < bestSolutionMoveCount) {
				int helper = movesAdded;
				helper += 52 - s.FoundationCount() + s.RoundCount();
				HashKey key = s.GameState();

				{
					unique_lock<mutex> lck(mtx);
					KeyValue<int> * result = closed.Add(key, movesAdded);
					if (result == NULL || result->Value > movesAdded) {
						node = make_shared<MoveNode>(move, firstNode);
						if (result != NULL) { result->Value = movesAdded; }

						openCount++;
						open[helper].push(node);
					}
				}
			}

			s.UndoMove();
		}

		{
			unique_lock<mutex> lck(mtx);
			numProcessing--;
		}
	}
}
SolveResult SolitaireWorker::Run(int numThreads) {
	solitaire->MakeAutoMoves();
	if (solitaire->MovesAvailableCount() == 0) { return solitaire->FoundationCount() == 52 ? SolvedMinimal : Impossible; }

	openCount = 1;
	maxFoundationCount = solitaire->FoundationCount();
	bestSolutionMoveCount = 512;
	bestSolution[0].Count = 255;
	startMoves = solitaire->MinimumMovesLeft() + solitaire->MovesMadeNormalizedCount();
	int powerOf2 = 1;
	while (maxClosedCount > (1 << (powerOf2 + 2))) {
		powerOf2++;
	}
	HashMap<int> * closed = new HashMap<int>(powerOf2);

	shared_ptr<MoveNode> firstNode = solitaire->MovesMadeCount() > 0 ? make_shared<MoveNode>(solitaire->GetMoveMade(solitaire->MovesMadeCount() - 1)) : NULL;
	shared_ptr<MoveNode> node = firstNode;
	for (int i = solitaire->MovesMadeCount() - 2; i >= 0; i--) {
		node->Parent = make_shared<MoveNode>(solitaire->GetMoveMade(i));
		node = node->Parent;
	}
	open[startMoves].push(firstNode);
	numProcessing = 0;

	thread * threads = new thread[numThreads];
	for (int i = 0; i < numThreads; i++) {
		threads[i] = thread(&SolitaireWorker::RunMinimalWorker, this, (void*)closed);
		this_thread::sleep_for(chrono::milliseconds(23));
	}

	for (int i = 0; i < numThreads; i++) {
		threads[i].join();
	}
	delete[] threads;

	//Reset game to best solution found
	solitaire->ResetGame();
	for (int i = 0; bestSolution[i].Count < 255; i++) {
		solitaire->MakeMove(bestSolution[i]);
	}

	//cout << setw(3) << bestSolutionMoveCount << " - " << setw(2) << maxFoundationCount << " C: " << setw(9) << closed->Size() << " L: " << setw(4) << closed->MaxLength() << " U: " << setw(8) << closed->SlotsUsed() << " O: " << setw(9) << openCount << '\n';
	SolveResult result = closed->Size() >= maxClosedCount ? (maxFoundationCount == 52 ? SolvedMayNotBeMinimal : CouldNotComplete) : (maxFoundationCount == 52 ? SolvedMinimal : Impossible);
	delete closed;
	return result;
}

SolveResult Solitaire::SolveFast(int maxMovesToCheckPerBranch, int maxClosedCount) {
	if (maxMovesToCheckPerBranch <= 1 || maxMovesToCheckPerBranch > 5) { return CouldNotComplete; }
	MakeAutoMoves();
	if (movesAvailableCount == 0) { return foundationCount == 52 ? SolvedMinimal : Impossible; }

	int openCount = 1;
	int maxFoundationCount = foundationCount;
	int bestSolutionMoveCount = 512;
	int totalOpenCount = 1;

	int powerOf2 = 1;
	while (maxClosedCount > (1 << (powerOf2 + 2))) {
		powerOf2++;
	}
	HashMap<int> closed(powerOf2);
	stack<shared_ptr<MoveNode>> open[512];
	Move movesToMake[512];
	Move bestSolution[512];
	bestSolution[0].Count = 255;
	int startMoves = MinimumMovesLeft() + MovesMadeNormalizedCount();

	shared_ptr<MoveNode> firstNode = movesMadeCount > 0 ? make_shared<MoveNode>(movesMade[movesMadeCount - 1]) : NULL;
	shared_ptr<MoveNode> node = firstNode;
	for (int i = movesMadeCount - 2; i >= 0; i--) {
		node->Parent = make_shared<MoveNode>(movesMade[i]);
		node = node->Parent;
	}
	open[startMoves].push(firstNode);
	while (closed.Size() < maxClosedCount) {
		//Check for lowest score length
		int index = startMoves;
		while (index < 512 && open[index].size() == 0) { index++; }

		//End solver if no more states
		if (index >= 512) { break; }

		//Get next state to evaluate
		openCount--;
		firstNode = open[index].top();
		open[index].pop();

		//Initialize game to the found state
		ResetGame(drawCount);
		int movesTotal = 0;
		node = firstNode;
		while (node != NULL) {
			movesToMake[movesTotal++] = node->Value;
			node = node->Parent;
		}
		while (movesTotal > 0) {
			MakeMove(movesToMake[--movesTotal]);
		}

		//Make any auto moves
		UpdateAvailableMoves();
		while (movesAvailableCount == 1) {
			Move move = movesAvailable[0];
			MakeMove(move);
			firstNode = make_shared<MoveNode>(move, firstNode);
			UpdateAvailableMoves();
		}
		movesTotal = MovesMadeNormalizedCount();

		//Check for best solution to foundations
		if (foundationCount > maxFoundationCount || (foundationCount == maxFoundationCount && bestSolutionMoveCount > movesTotal)) {
			bestSolutionMoveCount = movesTotal;
			maxFoundationCount = foundationCount;

			//Save solution
			for (int i = 0; i < movesMadeCount; i++) {
				bestSolution[i] = movesMade[i];
			}
			bestSolution[movesMadeCount].Count = 255;
		} else if (maxFoundationCount == 52) {
			//Dont check state if above or equal to current best solution
			int helper = MinimumMovesLeft();
			helper += movesTotal;
			if (helper >= bestSolutionMoveCount) { continue; }
		}

		//Make available moves and add them to be evaulated
		int movesToAdd = movesAvailableCount > maxMovesToCheckPerBranch ? maxMovesToCheckPerBranch : movesAvailableCount;
		int movesTried[5] = { -1, -1, -1, -1, -1 };
		while (movesToAdd-- > 0) {
			int i = random.Next() % movesAvailableCount;
			while (movesTried[0] == i || movesTried[1] == i || movesTried[2] == i || movesTried[3] == i || movesTried[4] == i) {
				i = random.Next() % movesAvailableCount;
			}
			movesTried[movesToAdd] = i;

			Move move = movesAvailable[i];
			int movesAdded = MovesAdded(move);

			MakeMove(move);

			movesAdded += movesTotal;
			movesAdded += MinimumMovesLeft();
			if (maxFoundationCount < 52 || movesAdded < bestSolutionMoveCount) {
				int helper = movesAdded;
				helper += 52 - foundationCount + roundCount;
				HashKey key = GameState();
				KeyValue<int> * result = closed.Add(key, movesAdded);
				if (result == NULL || result->Value > movesAdded) {
					node = make_shared<MoveNode>(move, firstNode);
					if (result != NULL) { result->Value = movesAdded; }

					totalOpenCount++;
					openCount++;
					open[helper].push(node);
				}
			}

			UndoMove();
		}
	}

	//Reset game to best solution found
	ResetGame(drawCount);
	for (int i = 0; bestSolution[i].Count < 255; i++) {
		MakeMove(bestSolution[i]);
	}
	return maxFoundationCount == 52 ? SolvedMayNotBeMinimal : CouldNotComplete;
}
SolveResult Solitaire::SolveMinimalMultithreaded(int numThreads, int maxClosedCount) {
	SolitaireWorker worker(*this, maxClosedCount);
	return worker.Run(numThreads);
}
SolveResult Solitaire::SolveMinimal(int maxClosedCount) {
	MakeAutoMoves();
	if (movesAvailableCount == 0) { return foundationCount == 52 ? SolvedMinimal : Impossible; }

	int openCount = 1;
	int maxFoundationCount = foundationCount;
	int bestSolutionMoveCount = 512;
	int totalOpenCount = 1;

	int powerOf2 = 1;
	while (maxClosedCount > (1 << (powerOf2 + 2))) {
		powerOf2++;
	}
	HashMap<int> closed(powerOf2);
	stack<shared_ptr<MoveNode>> open[512];
	Move movesToMake[512];
	Move bestSolution[512];
	bestSolution[0].Count = 255;
	int startMoves = MinimumMovesLeft() + MovesMadeNormalizedCount();

	shared_ptr<MoveNode> firstNode = movesMadeCount > 0 ? make_shared<MoveNode>(movesMade[movesMadeCount - 1]) : NULL;
	shared_ptr<MoveNode> node = firstNode;
	for (int i = movesMadeCount - 2; i >= 0; i--) {
		node->Parent = make_shared<MoveNode>(movesMade[i]);
		node = node->Parent;
	}
	open[startMoves].push(firstNode);
	while (closed.Size() < maxClosedCount) {
		//Check for lowest score length
		int index = startMoves;
		while (index < 512 && open[index].size() == 0) { index++; }

		//End solver if no more states
		if (index >= 512) { break; }

		//Get next state to evaluate
		openCount--;
		firstNode = open[index].top();
		open[index].pop();

		//Initialize game to the found state
		ResetGame(drawCount);
		int movesTotal = 0;
		node = firstNode;
		while (node != NULL) {
			movesToMake[movesTotal++] = node->Value;
			node = node->Parent;
		}
		while (movesTotal > 0) {
			MakeMove(movesToMake[--movesTotal]);
		}

		//Make any auto moves
		UpdateAvailableMoves();
		while (movesAvailableCount == 1) {
			Move move = movesAvailable[0];
			MakeMove(move);
			firstNode = make_shared<MoveNode>(move, firstNode);
			UpdateAvailableMoves();
		}
		movesTotal = MovesMadeNormalizedCount();

		//Check for best solution to foundations
		if (foundationCount > maxFoundationCount || (foundationCount == maxFoundationCount && bestSolutionMoveCount > movesTotal)) {
			bestSolutionMoveCount = movesTotal;
			maxFoundationCount = foundationCount;
			//cout << setw(3) << bestSolutionMoveCount << " - " << setw(2) << maxFoundationCount << " C: " << setw(9) << closed.Size() << " L: " << setw(4) << closed.MaxLength() << " U: " << setw(8) << closed.SlotsUsed() << " O: " << setw(9) << openCount << '\n';

			//Save solution
			for (int i = 0; i < movesMadeCount; i++) {
				bestSolution[i] = movesMade[i];
			}
			bestSolution[movesMadeCount].Count = 255;
		} else if (maxFoundationCount == 52) {
			//Dont check state if above or equal to current best solution
			int helper = MinimumMovesLeft();
			helper += movesTotal;
			if (helper >= bestSolutionMoveCount) { continue; }
		}

		//Make available moves and add them to be evaulated
		for (int i = 0; i < movesAvailableCount; i++) {
			Move move = movesAvailable[i];
			int movesAdded = MovesAdded(move);

			MakeMove(move);

			movesAdded += movesTotal;
			movesAdded += MinimumMovesLeft();
			if (maxFoundationCount < 52 || movesAdded < bestSolutionMoveCount) {
				int helper = movesAdded;
				helper += 52 - foundationCount + roundCount;
				HashKey key = GameState();
				KeyValue<int> * result = closed.Add(key, movesAdded);
				if (result == NULL || result->Value > movesAdded) {
					node = make_shared<MoveNode>(move, firstNode);
					if (result != NULL) { result->Value = movesAdded; }

					totalOpenCount++;
					openCount++;
					open[helper].push(node);
				}
			}

			UndoMove();
		}
	}
	//cout << setw(3) << bestSolutionMoveCount << " - " << setw(2) << maxFoundationCount << " C: " << setw(9) << closed.Size() << " L: " << setw(4) << closed.MaxLength() << " U: " << setw(8) << closed.SlotsUsed() << " O: " << setw(9) << openCount << " T: " << setw(9) << totalOpenCount << '\n';

	//Reset game to best solution found
	ResetGame(drawCount);
	for (int i = 0; bestSolution[i].Count < 255; i++) {
		MakeMove(bestSolution[i]);
	}
	return closed.Size() >= maxClosedCount ? (maxFoundationCount == 52 ? SolvedMayNotBeMinimal : CouldNotComplete) : (maxFoundationCount == 52 ? SolvedMinimal : Impossible);
}
void Solitaire::UpdateAvailableMoves() {
	movesAvailableCount = 0;
	int foundationMin = FoundationMin();

	//Check tableau to foundation, Check tableau to tableau
	for (int i = TABLEAU1; i <= TABLEAU7; ++i) {
		Pile & pile1 = piles[i];
		int pile1Size = pile1.Size();

		if (pile1Size == 0) { continue; }

		int pile1UpSize = pile1.UpSize();
		Card card1 = pile1.Low();
		int cardFoundation = card1.Foundation;

		if (card1.Rank - piles[cardFoundation].Size() == 1) {
			//logic used to tell if we can safely move a card to its foundation
			if (card1.Rank < foundationMin) {
				movesAvailable[0].Set(i, cardFoundation, 1, pile1UpSize == 1 && pile1Size > 1 ? 1 : 0);
				movesAvailableCount = 1;
				return;
			}

			movesAvailable[movesAvailableCount++].Set(i, cardFoundation, 1, pile1UpSize == 1 && pile1Size > 1 ? 1 : 0);
		}

		Card card2 = pile1.High();
		int pile1Length = card2.Rank - card1.Rank + 1;
		bool kingMoved = false;

		for (int j = TABLEAU1; j <= TABLEAU7; ++j) {
			if (i == j) { continue; }

			Pile & pile2 = piles[j];

			if (pile2.Size() == 0) {
				if (card2.Rank == KING && pile1Size != pile1Length && !kingMoved) {
					movesAvailable[movesAvailableCount++].Set(i, j, pile1Length, 1);
					//only create one move for a blank spot
					kingMoved = true;
				}
				continue;
			}

			Card card3 = pile2.Low();
			//logic used to determine if a pile of cards can be moved ontop of another pile of cards
			if (card1.Rank >= card3.Rank || card2.Rank + 1 < card3.Rank || ((card3.IsRed ^ card1.IsRed) ^ (card3.IsOdd ^ card1.IsOdd)) != 0) {
				continue;
			}

			int pile1Moved = card3.Rank - card1.Rank;

			if (pile1Moved == pile1Length) {//we are moving all face up cards
				movesAvailable[movesAvailableCount++].Set(i, j, pile1Moved, pile1Size > pile1Moved ? 1 : 0);
				continue;
			}

			//look to see if we are covering a card that can be moved to the foundation
			Card card4 = pile1[pile1UpSize - pile1Moved - 1];
			if (card4.Rank - piles[card4.Foundation].Size() == 1) {
				movesAvailable[movesAvailableCount++].Set(i, j, pile1Moved, 0);
			}
		}
	}

	//Check top of waste to foundation and waste to tableau
	int wasteSize = piles[WASTE].Size();

	if (wasteSize > 0) {
		Card wasteCard = piles[WASTE].Low();
		int wasteFoundation = wasteCard.Foundation;

		if (wasteCard.Rank - piles[wasteFoundation].Size() == 1) {
			if (wasteCard.Rank <= foundationMin) {
				if (drawCount == 1) {
					movesAvailable[0].Set(WASTE, wasteFoundation, 1, 0);
					movesAvailableCount = 1;
				} else {
					movesAvailable[movesAvailableCount++].Set(WASTE, wasteFoundation, 1, 0);
				}
				return;
			}

			movesAvailable[movesAvailableCount++].Set(WASTE, wasteFoundation, 1, 0);
		}

		for (int i = TABLEAU1; i <= TABLEAU7; ++i) {
			Pile & pile = piles[i];

			if (pile.Size() != 0) {
				Card tableauCard = pile.Low();

				if (tableauCard.Rank - wasteCard.Rank != 1 || tableauCard.IsRed == wasteCard.IsRed) {
					continue;
				}

				movesAvailable[movesAvailableCount++].Set(WASTE, i, 1, 0);
			} else if (wasteCard.Rank == KING) {
				movesAvailable[movesAvailableCount++].Set(WASTE, i, 1, 0);
				break;
			}
		}
	}

	//Check cards waiting to be turned over from stock
	Pile & stock = piles[STOCK];
	int stockSize = stock.Size();
	for (int j = (stockSize > 0 && stockSize - drawCount <= 0) ? 0 : stockSize - drawCount; j >= 0; j -= drawCount) {
		Card card1 = stock.Up(j);
		int stockFoundation = card1.Foundation;

		if (card1.Rank - piles[stockFoundation].Size() == 1) {
			if (card1.Rank <= foundationMin) {
				movesAvailable[movesAvailableCount++].Set(WASTE, stockFoundation, 1, stockSize - j);
				if (drawCount == 1) {
					return;
				} else {
					continue;
				}
			}

			movesAvailable[movesAvailableCount++].Set(WASTE, stockFoundation, 1, stockSize - j);
		}


		for (int i = TABLEAU1; i <= TABLEAU7; ++i) {
			Pile & pile2 = piles[i];

			if (pile2.Size() != 0) {
				Card card = pile2.Low();

				if (card.Rank - card1.Rank != 1 || card.IsRed == card1.IsRed) {
					continue;
				}

				movesAvailable[movesAvailableCount++].Set(WASTE, i, 1, stockSize - j);
			} else if (card1.Rank == KING) {
				movesAvailable[movesAvailableCount++].Set(WASTE, i, 1, stockSize - j);
				break;
			}
		}

		if (j > 0 && j < drawCount) { j = drawCount; }
	}

	//Check cards already turned over in the waste, meaning we have to "redeal" the deck to get to it
	Pile & waste = piles[WASTE];
	int amountToDraw = stockSize;
	amountToDraw += stockSize;
	amountToDraw += wasteSize;
	amountToDraw++;
	wasteSize--;

	int lastIndex = drawCount - 1;
	for (int j = drawCount - 1; j < wasteSize; j += drawCount, lastIndex = j) {
		Card card1 = waste.Up(j);
		int stockFoundation = card1.Foundation;

		if (card1.Rank - piles[stockFoundation].Size() == 1) {
			if (card1.Rank <= foundationMin) {
				movesAvailable[movesAvailableCount++].Set(WASTE, stockFoundation, 1, amountToDraw + j);
				if (drawCount == 1) {
					return;
				} else {
					continue;
				}
			}

			movesAvailable[movesAvailableCount++].Set(WASTE, stockFoundation, 1, amountToDraw + j);
		}

		for (int i = TABLEAU1; i <= TABLEAU7; ++i) {
			Pile & pile2 = piles[i];

			if (pile2.Size() != 0) {
				Card card = pile2.Low();

				if (card.Rank - card1.Rank != 1 || card.IsRed == card1.IsRed) {
					continue;
				}

				movesAvailable[movesAvailableCount++].Set(WASTE, i, 1, amountToDraw + j);
			} else if (card1.Rank == KING) {
				movesAvailable[movesAvailableCount++].Set(WASTE, i, 1, amountToDraw + j);
				break;
			}
		}
	}

	//Check cards in stock after a "redeal". Only happens when draw count > 1 and you have access to more cards in the talon
	if (lastIndex > wasteSize && wasteSize > -1) {
		amountToDraw += wasteSize;
		amountToDraw += stockSize;
		for (int j = (stockSize > 0 && stockSize - lastIndex + wasteSize <= 0) ? 0 : stockSize - lastIndex + wasteSize; j > 0; j -= drawCount) {
			Card card1 = stock.Up(j);
			int stockFoundation = card1.Foundation;

			if (card1.Rank - piles[stockFoundation].Size() == 1) {
				if (card1.Rank <= foundationMin) {
					movesAvailable[movesAvailableCount++].Set(WASTE, stockFoundation, 1, amountToDraw - j);
					if (drawCount == 1) {
						return;
					} else {
						continue;
					}
				}

				movesAvailable[movesAvailableCount++].Set(WASTE, stockFoundation, 1, amountToDraw - j);
			}


			for (int i = TABLEAU1; i <= TABLEAU7; ++i) {
				Pile & pile2 = piles[i];

				if (pile2.Size() != 0) {
					Card card = pile2.Low();

					if (card.Rank - card1.Rank != 1 || card.IsRed == card1.IsRed) {
						continue;
					}

					movesAvailable[movesAvailableCount++].Set(WASTE, i, 1, amountToDraw - j);
				} else if (card1.Rank == KING) {
					movesAvailable[movesAvailableCount++].Set(WASTE, i, 1, amountToDraw - j);
					break;
				}
			}
		}
	}

	if (foundationCount == 0) { return; }
	//Check foundation to tableau, very rarely needed to solve optimally
	Move lastMove = movesMade[movesMadeCount - 1];
	for (int i = FOUNDATION1C; i <= FOUNDATION4H; ++i) {
		Pile & pile1 = piles[i];
		int foundationRank = pile1.Size();
		if (foundationRank == 0 || foundationRank <= foundationMin) { continue; }

		for (int j = TABLEAU1; j <= TABLEAU7; ++j) {
			Pile & pile2 = piles[j];

			if (pile2.Size() != 0) {
				Card card = pile2.Low();

				if ((card.Foundation & 1) == (i & 1) || card.Rank - foundationRank != 1) {
					continue;
				}

				if (lastMove.From != j && lastMove.To != i) {
					movesAvailable[movesAvailableCount++].Set(i, j, 1, 0);
				}
			} else if (foundationRank == KING) {
				if (lastMove.From != j && lastMove.To != i) {
					movesAvailable[movesAvailableCount++].Set(i, j, 1, 0);
				}
				break;
			}
		}
	}
}
void Solitaire::ResetGame() {
	ResetGame(drawCount);
}
void Solitaire::ResetGame(int drawCount) {
	this->drawCount = drawCount;
	roundCount = 0;
	foundationCount = 0;
	movesMadeCount = 0;
	movesAvailableCount = 0;

	for (int i = 0; i < 13; ++i) {
		piles[i].Reset();
	}

	for (int j = TABLEAU1, i = 0; j <= TABLEAU7; ++j) {
		piles[j].AddUp(cards[i++]);
		for (int k = j + 1; k <= TABLEAU7; ++k, ++i) {
			piles[k].AddDown(cards[i]);
		}
	}

	for (int i = 51; i >= 28; --i) {
		piles[STOCK].AddUp(cards[i]);
	}
}
int Solitaire::Shuffle(int dealNumber) {
	if (dealNumber != -1) {
		random.SetSeed(dealNumber);
	} else {
		dealNumber = random.Next();
		random.SetSeed(dealNumber);
	}

	Card temp;

	for (int i = 0; i < 52; i++) {
		cards[i].Set(i);
	}

	for (int x = 0; x < 269; ++x) {
		int k = random.Next() % 52;
		int j = random.Next() % 52;
		temp = cards[k];
		cards[k] = cards[j];
		cards[j] = temp;
	}

	return dealNumber;
}
int Solitaire::MinimumMovesLeft() {
	Pile & waste = piles[WASTE];
	int wasteSize = waste.Size();
	int win = piles[STOCK].Size();
	int stockCount = win / drawCount;
	stockCount += (win % drawCount) == 0 ? 0 : 1;
	win += stockCount;
	win += wasteSize;

	for (int i = wasteSize - 1; i > 0; --i) {
		Card card1 = waste.Up(i);

		for (int j = i - 1; j >= 0; --j) {
			Card card2 = waste.Up(j);

			if (card1.Suit == card2.Suit && card1.Rank > card2.Rank) {
				++win;
				break;
			}
		}
	}

	for (int i = TABLEAU1; i <= TABLEAU7; ++i) {
		Pile & pile = piles[i];
		int pileSize = pile.Size();
		int downSize = pile.DownSize();
		win += pileSize;
		win += downSize;
		if (downSize == 0) { continue; }

		pileSize -= downSize;
		unsigned char mins[28] = {};

		for (int j = pileSize - 1; j >= 0; j--) {
			Card card1 = pile.Up(j);

			mins[card1.Suit] = card1.Rank;
		}

		for (int j = downSize - 1; j >= 0; j--) {
			Card card1 = pile.Down(j);

			unsigned char & rank = mins[card1.Suit];
			unsigned char cardRank = card1.Rank;
			if (mins[card1.Suit + 4] == EMPTY) {
				if (rank > cardRank) {
					win++;
				}
				rank = cardRank;
				continue;
			} else if (rank > cardRank) {
				do {
					win++;
					rank = *(&rank + 4);
				} while (rank > cardRank);
			}

			do {
				unsigned char temp = rank;
				rank = cardRank;
				cardRank = temp;
				rank = *(&rank + 4);
			} while (rank < cardRank);
		}
	}

	return win;
}
void Solitaire::Initialize() {
	drawCount = 1;
	for (int i = 0; i < 52; i++) {
		cards[i].Set(i);
	}
	for (int i = 0; i < 13; ++i) {
		piles[i].Initialize();
	}
}
void Solitaire::MakeAutoMoves() {
	UpdateAvailableMoves();
	while (movesAvailableCount == 1) {
		MakeMove(movesAvailable[0]);
		UpdateAvailableMoves();
	}
}
void Solitaire::MakeMove(int index) {
	MakeMove(movesAvailable[index]);
}
void Solitaire::MakeMove(Move move) {
	movesMade[movesMadeCount++] = move;

	if (move.Count == 1) {
		if (move.From == WASTE && move.Extra > 0) {
			int stockSize = piles[STOCK].Size();
			if (move.Extra <= stockSize) {
				piles[STOCK].RemoveTalon(piles[WASTE], move.Extra);
			} else {
				roundCount++;
				stockSize += stockSize;
				int wasteSize = piles[WASTE].Size();
				stockSize += wasteSize;
				stockSize += wasteSize;
				stockSize -= move.Extra;
				if (stockSize > 0) {
					piles[WASTE].RemoveTalon(piles[STOCK], stockSize);
				} else {
					piles[STOCK].RemoveTalon(piles[WASTE], -stockSize);
				}
			}
		}
		piles[move.From].Remove(piles[move.To]);

		if (move.To >= FOUNDATION1C) {
			++foundationCount;
		} else if (move.From >= FOUNDATION1C) {
			--foundationCount;
		}
	} else {
		piles[move.From].Remove(piles[move.To], move.Count);
	}

	if (move.From != WASTE && move.Extra > 0) {
		piles[move.From].Flip();
	}
}
void Solitaire::UndoMove() {
	Move move = movesMade[--movesMadeCount];

	if (move.From != WASTE && move.Extra > 0) {
		piles[move.From].Flip();
	}

	if (move.Count == 1) {
		piles[move.To].Remove(piles[move.From]);

		if (move.To >= FOUNDATION1C) {
			--foundationCount;
		} else if (move.From >= FOUNDATION1C) {
			++foundationCount;
		}

		if (move.From == WASTE && move.Extra > 0) {
			int wasteSize = piles[WASTE].Size();
			if (move.Extra <= wasteSize) {
				piles[WASTE].RemoveTalon(piles[STOCK], move.Extra);
			} else {
				roundCount--;
				wasteSize += wasteSize;
				int stockSize = piles[STOCK].Size();
				wasteSize += stockSize;
				wasteSize += stockSize;
				wasteSize -= move.Extra;
				if (wasteSize > 0) {
					piles[STOCK].RemoveTalon(piles[WASTE], wasteSize);
				} else {
					piles[WASTE].RemoveTalon(piles[STOCK], -wasteSize);
				}
			}
		}
	} else {
		piles[move.To].Remove(piles[move.From], move.Count);
	}
}
int Solitaire::FoundationMin() {
	int one = piles[FOUNDATION2D].Size();
	int two = piles[FOUNDATION4H].Size();
	int redFoundationMin = one <= two ? one : two;
	one = piles[FOUNDATION1C].Size();
	two = piles[FOUNDATION3S].Size();
	int blackFoundationMin = one <= two ? one : two;
	return 2 + (blackFoundationMin <= redFoundationMin ? blackFoundationMin : redFoundationMin);
}
Move Solitaire::GetMoveAvailable(int index) {
	return movesAvailable[index];
}
Move Solitaire::GetMoveMade(int index) {
	return movesMade[index];
}
bool Solitaire::LoadSolitaire(string const& cardSet) {
	int used[52] = {};
	if (cardSet.size() < 156) { return false; }
	for (int i = 0; i < 52; i++) {
		int suit = (cardSet[i * 3 + 2] ^ 0x30) - 1;
		if (suit < CLUBS || suit > HEARTS) { return false; }

		if (suit >= SPADES) {
			suit = (suit == SPADES) ? HEARTS : SPADES;
		}

		int rank = (cardSet[i * 3] ^ 0x30) * 10 + (cardSet[i * 3 + 1] ^ 0x30);
		if (rank < ACE || rank > KING) { return false; }

		int value = suit * 13 + rank - 1;
		if (used[value] == 1) { return false; }
		used[value] = 1;
		cards[i].Set(value);
	}

	return true;
}
string Solitaire::GetSolitaire() {
	stringstream cardSet;
	for (int i = 0; i < 52; i++) {
		Card c = cards[i];
		unsigned char suit = c.Suit;

		if (suit >= 2) {
			suit = (suit == 2) ? 3 : 2;
		}
		suit++;

		if (c.Rank < 10) {
			cardSet << '0' << (char)(c.Rank ^ 0x30) << (char)(suit ^ 0x30);
		} else {
			cardSet << '1' << (char)((c.Rank - 10) ^ 0x30) << (char)(suit ^ 0x30);
		}
	}
	return cardSet.str();
}
bool Solitaire::LoadPysol(string const& cardSet) {
	int used[52] = {};
	if (cardSet.size() < 211) { return false; }
	unsigned int j = 7;
	for (int i = 28; i < 52; i++) {
		int rank = cardSet[j] == 'A' ? ACE : (cardSet[j] == 'T' ? TEN : (cardSet[j] == 'J' ? JACK : (cardSet[j] == 'Q' ? QUEEN : (cardSet[j] == 'K' ? KING : cardSet[j] ^ 0x30))));
		if (rank < ACE || rank > KING) { return false; }
		j++;

		int suit = cardSet[j] == 'C' ? CLUBS : (cardSet[j] == 'D' ? DIAMONDS : (cardSet[j] == 'S' ? SPADES : HEARTS));
		if (suit < CLUBS || suit > HEARTS) { return false; }
		j += 2;

		int value = suit * 13 + rank - 1;
		if (used[value] == 1) { return false; }
		used[value] = 1;
		cards[i].Set(value);
	}

	const int order[28] = { 0, 1, 7, 2, 8, 13, 3, 9, 14, 18, 4, 10, 15, 19, 22, 5, 11, 16, 20, 23, 25, 6, 12, 17, 21, 24, 26, 27 };
	for (int i = 0; i < 28; i++) {
		while (j < cardSet.size() && (cardSet[j] == '\r' || cardSet[j] == '\n' || cardSet[j] == '\t' || cardSet[j] == ' ' || cardSet[j] == ':' || cardSet[j] == '<')) { j++; }
		if (j + 1 >= cardSet.size()) { return false; }

		int rank = cardSet[j] == 'A' ? ACE : (cardSet[j] == 'T' ? TEN : (cardSet[j] == 'J' ? JACK : (cardSet[j] == 'Q' ? QUEEN : (cardSet[j] == 'K' ? KING : cardSet[j] ^ 0x30))));
		if (rank < ACE || rank > KING) { return false; }
		j++;

		int suit = cardSet[j] == 'C' ? CLUBS : (cardSet[j] == 'D' ? DIAMONDS : (cardSet[j] == 'S' ? SPADES : HEARTS));
		if (suit < CLUBS || suit > HEARTS) { return false; }
		j += 3;

		int value = suit * 13 + rank - 1;
		if (used[value] == 1) { return false; }
		used[value] = 1;
		cards[order[i]].Set(value);
	}
	return true;
}
string Solitaire::GetPysol() {
	const char RANKS[] = { "XA23456789TJQK" };
	const char SUITS[] = { "CDSH" };
	stringstream cardSet;
	cardSet << "Talon: ";
	for (int i = 28; i < 52; i++) {
		Card c = cards[i];

		cardSet << RANKS[c.Rank] << SUITS[c.Suit];
		if (i < 51) { cardSet << " "; }
	}

	const int order[28] = { 0, 1, 7, 2, 8, 13, 3, 9, 14, 18, 4, 10, 15, 19, 22, 5, 11, 16, 20, 23, 25, 6, 12, 17, 21, 24, 26, 27 };
	for (int i = 0, j = 0; j < 7; j++) {
		cardSet << '\n';
		for (int k = 0; k <= j; k++, i++) {
			Card c = cards[order[i]];

			if (k < j) {
				cardSet << '<' << RANKS[c.Rank] << SUITS[c.Suit] << "> ";
			} else {
				cardSet << RANKS[c.Rank] << SUITS[c.Suit];
			}
		}
	}
	return cardSet.str();
}
void Solitaire::SetDrawCount(int drawCount) {
	this->drawCount = drawCount;
}
HashKey Solitaire::GameState() {
	int order[7] = { TABLEAU1, TABLEAU2, TABLEAU3, TABLEAU4, TABLEAU5, TABLEAU6, TABLEAU7 };
	int current = 1;
	//sort the piles
	while (current < 7) {
		int search = current;

		do {
			if (piles[order[search - 1]].HighValue() <= piles[order[search]].HighValue()) {
				break;
			}

			int temp = order[--search];
			order[search] = order[search + 1];
			order[search + 1] = temp;
		} while (search > 0);

		++current;
	}

	HashKey key;
	int z = 0;
	key[z++] = (piles[FOUNDATION1C].Size() << 4) | (piles[FOUNDATION2D].Size() + 1);
	key[z++] = (piles[FOUNDATION3S].Size() << 4) | piles[FOUNDATION4H].Size();

	int bits = 5;
	int mask = roundCount;
	for (int i = 0; i < 7; ++i) {
		Pile & pile = piles[order[i]];
		int upSize = pile.UpSize();

		int added = 10;
		bits += 10;
		mask <<= 6;
		mask |= upSize > 0 ? pile.Up(0).Value + 1 : 0;
		mask <<= 4;
		mask |= upSize;
		for (int j = 1; j < upSize; ++j) {
			bits++;
			added++;
			mask <<= 1;
			mask |= pile.Up(j).Suit >> 1;
		}

		bits += 21 - added;
		mask <<= 21 - added;
		do {
			bits -= 8;
			key[z++] = (mask >> bits) & 255;
		} while (bits >= 8);
	}
	if (bits > 0) {
		key[z] = mask & 255;
	}

	return key;
}
string Solitaire::GameDiagram() {
	stringstream ss;
	const char RANKS[] = { "XA23456789TJQK" };
	const char SUITS[] = { "CDSH" };
	for (int i = 0; i < 13; i++) {
		if (i < 10) {
			ss << ' ';
		}
		ss << i << ": ";
		Pile & p = piles[i];
		int size = p.UpSize();
		for (int j = size - 1; j >= 0; j--) {
			Card c = p.Up(j);
			char rank = RANKS[c.Rank];
			char suit = c.Suit != NONE ? SUITS[c.Suit] : 'X';
			ss << rank << suit << ' ';
		}
		size = p.DownSize();
		for (int j = size - 1; j >= 0; j--) {
			Card c = p.Down(j);
			char rank = RANKS[c.Rank];
			char suit = c.Suit != NONE ? SUITS[c.Suit] : 'X';
			ss << '-' << rank << suit;
		}
		ss << '\n';
	}
	ss << "Minimum Moves Needed: " << MinimumMovesLeft();
	return ss.str();
}
string Solitaire::GameDiagramPysol() {
	const char RANKS[] = { "0A23456789TJQK" };
	const char SUITS[] = { "CDSH" };
	stringstream ss;
	ss << "Foundations: H-" << RANKS[piles[FOUNDATION4H].Size()] << " C-" << RANKS[piles[FOUNDATION1C].Size()] << " D-" << RANKS[piles[FOUNDATION2D].Size()] << " S-" << RANKS[piles[FOUNDATION3S].Size()];
	ss << "\nTalon: ";

	Pile & waste = piles[WASTE];
	int size = waste.UpSize();
	for (int j = size - 1; j >= 0; j--) {
		Card c = waste.Up(j);
		char rank = RANKS[c.Rank];
		char suit = c.Suit != NONE ? SUITS[c.Suit] : 'X';
		ss << rank << suit << ' ';
	}
	ss << "==> ";

	Pile & stock = piles[STOCK];
	size = stock.UpSize();
	for (int j = size - 1; j >= 0; j--) {
		Card c = stock.Up(j);
		char rank = RANKS[c.Rank];
		char suit = c.Suit != NONE ? SUITS[c.Suit] : 'X';
		ss << rank << suit << ' ';
	}
	ss << "<==";

	for (int i = TABLEAU1; i <= TABLEAU7; i++) {
		ss << "\n:";
		Pile & p = piles[i];
		size = p.DownSize();
		for (int j = 0; j < size; j++) {
			Card c = p.Down(j);
			char rank = RANKS[c.Rank];
			char suit = c.Suit != NONE ? SUITS[c.Suit] : 'X';
			ss << " <" << rank << suit << ">";
		}
		size = p.UpSize();
		for (int j = 0; j < size; j++) {
			Card c = p.Up(j);
			char rank = RANKS[c.Rank];
			char suit = c.Suit != NONE ? SUITS[c.Suit] : 'X';
			ss << ' ' << rank << suit;
		}
	}

	return ss.str();
}
void AddMove(stringstream & ss, Move m, int stockSize, int wasteSize, int drawCount, bool combine) {
	const char PILES[] = { "W1234567GCDSH" };
	if (m.Extra > 0) {
		if (m.From != WASTE) {
			if (m.Count > 1) {
				if (!combine) {
					ss << PILES[m.From] << PILES[m.To] << '-' << (int)m.Count << " F" << (int)m.From << ' ';
				} else {
					ss << '[' << PILES[m.From] << PILES[m.To] << '-' << (int)m.Count << " F" << (int)m.From << "] ";
				}
			} else if (!combine) {
				ss << PILES[m.From] << PILES[m.To] << " F" << (int)m.From << ' ';
			} else {
				ss << '[' << PILES[m.From] << PILES[m.To] << " F" << (int)m.From << "] ";
			}
		} else if (!combine) {
			if (m.Extra <= stockSize) {
				ss << "DR" << (m.Extra / drawCount + ((m.Extra % drawCount) == 0 ? 0 : 1)) << ' ' << PILES[m.From] << PILES[m.To] << ' ';
			} else {
				int temp = stockSize / drawCount + ((stockSize % drawCount) == 0 ? 0 : 1);
				if (temp != 0) { ss << "DR" << temp << ' '; }
				ss << "NEW ";
				temp = m.Extra - stockSize - stockSize - wasteSize;
				temp = temp / drawCount + ((temp % drawCount) == 0 ? 0 : 1);
				ss << "DR" << temp << ' ' << PILES[m.From] << PILES[m.To] << ' ';
			}
		} else if (m.Extra <= stockSize) {
			ss << "[DR" << (m.Extra / drawCount + ((m.Extra % drawCount) == 0 ? 0 : 1)) << ' ' << PILES[m.From] << PILES[m.To] << "] ";
		} else {
			int temp = m.Extra - stockSize - stockSize - wasteSize;
			temp = temp / drawCount + ((temp % drawCount) == 0 ? 0 : 1);
			temp += stockSize / drawCount + ((stockSize % drawCount) == 0 ? 0 : 1);
			ss << "[DR" << temp << ' ' << PILES[m.From] << PILES[m.To] << "] ";
		}
	} else if (m.Count > 1) {
		ss << PILES[m.From] << PILES[m.To] << '-' << (int)m.Count << ' ';
	} else {
		ss << PILES[m.From] << PILES[m.To] << ' ';
	}
}
string Solitaire::MovesAvailable() {
	//Returns moves available for the current state. Flip moves are combined with the move that caused it in []. See below for move representation.
	stringstream ss;
	for (int i = 0; i < movesAvailableCount; i++) {
		Move m = movesAvailable[i];
		AddMove(ss, m, piles[STOCK].Size(), piles[WASTE].Size(), drawCount, true);
	}
	return ss.str();
}
string Solitaire::MovesMade() {
	//Returns moves made so far in the current game.
	//DR# is a draw move that is done # number of times. ie) DR2 means draw twice, if draw count > 1 it is still DR2.
	//NEW is to represent the moving of cards from the Waste pile back to the stock pile. A New round.
	//F# means to flip the card on tableau pile #. 
	//XY means to move the top card from pile X to pile Y.
	//X will be 1 through 7, W for Waste, or a foundation suit character. 'C'lubs, 'D'iamonds, 'S'pades, 'H'earts
	//Y will be 1 through 7 or the foundation suit character.
	//XY-# is the same as above except you are moving # number of cards from X to Y.
	stringstream ss;
	int moves = movesMadeCount;
	ResetGame(drawCount);
	for (int i = 0; i < moves; i++) {
		Move m = movesMade[i];
		AddMove(ss, m, piles[STOCK].Size(), piles[WASTE].Size(), drawCount, false);
		MakeMove(m);
	}
	return ss.str();
}
int Solitaire::MovesAvailableCount() {
	return movesAvailableCount;
}
int Solitaire::MovesMadeNormalizedCount() {
	int movesTotal = 0;
	int stockSize = 24;
	int wasteSize = 0;
	for (int i = 0; i < movesMadeCount; i++) {
		Move m = movesMade[i];
		movesTotal++;
		if (m.Extra > 0) {
			if (m.From == WASTE) {
				int temp = stockSize;
				if (m.Extra <= stockSize) {
					temp = m.Extra;
					stockSize -= temp;
					wasteSize += temp - 1;
				} else {
					movesTotal += temp / drawCount;
					movesTotal += (temp % drawCount) == 0 ? 0 : 1;
					temp = m.Extra;
					temp -= wasteSize;
					temp -= stockSize;
					temp -= stockSize;
					stockSize += wasteSize - temp;
					wasteSize = temp - 1;
				}
				movesTotal += temp / drawCount;
				movesTotal += (temp % drawCount) == 0 ? 0 : 1;
			} else {
				movesTotal++;
			}
		} else if (m.From == WASTE) {
			wasteSize--;
		}
	}
	return movesTotal;
}
int Solitaire::MovesMadeCount() {
	return movesMadeCount;
}
int Solitaire::FoundationCount() {
	return foundationCount;
}
int Solitaire::RoundCount() {
	return roundCount;
}
int Solitaire::DrawCount() {
	return drawCount;
}
int Solitaire::MovesAdded(Move const& move) {
	int movesAdded = 1;
	int wasteSize = piles[WASTE].Size();
	int stockSize = piles[STOCK].Size();
	if (move.Extra > 0) {
		if (move.From == WASTE) {
			if (move.Extra <= stockSize) {
				movesAdded += move.Extra / drawCount;
				movesAdded += (move.Extra % drawCount) == 0 ? 0 : 1;
			} else {
				movesAdded += stockSize / drawCount;
				movesAdded += (stockSize % drawCount) == 0 ? 0 : 1;
				int temp = move.Extra;
				temp -= wasteSize;
				temp -= stockSize;
				temp -= stockSize;
				movesAdded += temp / drawCount;
				movesAdded += (temp % drawCount) == 0 ? 0 : 1;
			}
		} else {
			movesAdded++;
		}
	}
	return movesAdded;
}
Move Solitaire::operator[](int index) {
	return movesMade[index];
}