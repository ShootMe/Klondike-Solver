#ifndef Pile_h
#define Pile_h
#include"Card.h"

enum Piles {
	WASTE = 0,
	TABLEAU1,
	TABLEAU2,
	TABLEAU3,
	TABLEAU4,
	TABLEAU5,
	TABLEAU6,
	TABLEAU7,
	STOCK,
	FOUNDATION1C,
	FOUNDATION2D,
	FOUNDATION3S,
	FOUNDATION4H
};

class Pile {
private:
	Card down[24], up[24];
	int size, downSize, upSize;
public:
	void AddDown(Card card);
	void AddUp(Card card);
	void Remove(Pile & to);
	void Remove(Pile & to, int count);
	void RemoveTalon(Pile & to, int count);
	void Flip();
	void Reset();
	void Initialize();
	int HighValue();
	int Size();
	int DownSize();
	int UpSize();
	Card operator[](int index);
	Card Down(int index);
	Card Up(int index);
	Card Low();
	Card High();
};
#endif