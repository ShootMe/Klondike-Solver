#ifndef Card_h
#define Card_h
enum Cards {
	EMPTY = 0,
	ACE,
	TWO,
	THREE,
	FOUR,
	FIVE,
	SIX,
	SEVEN,
	EIGHT,
	NINE,
	TEN,
	JACK,
	QUEEN,
	KING
};

enum Suits {
	CLUBS = 0,
	DIAMONDS,
	SPADES,
	HEARTS,
	NONE = 255
};

struct Card {
	unsigned char Suit, Rank, IsOdd, IsRed, Foundation, Value;
	void Clear();
	void Set(unsigned char value);
};
#endif