#include"Card.h"

void Card::Clear() {
	Rank = EMPTY;
	Suit = NONE;
}
void Card::Set(unsigned char value) {
	Value = value;
	Rank = (value % 13) + 1;
	Suit = value / 13;
	IsRed = Suit & 1;
	IsOdd = Rank & 1;
	Foundation = Suit + 9;
}