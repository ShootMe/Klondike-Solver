#include"Pile.h"

void Pile::AddDown(Card card) {
	down[downSize++] = card;
	size++;
}
void Pile::AddUp(Card card) {
	up[upSize++] = card;
	size++;
}
void Pile::Flip() {
	if (upSize > 0) {
		down[downSize++] = up[--upSize];
	} else {
		up[upSize++] = down[--downSize];
	}
}
void Pile::Remove(Pile & to) {
	to.AddUp(up[--upSize]);
	size--;
}
void Pile::Remove(Pile & to, int count) {
	for (int i = upSize - count; i < upSize; ++i) {
		to.AddUp(up[i]);
	}

	upSize -= count;
	size -= count;
}
void Pile::RemoveTalon(Pile & to, int count) {
	int i = size - count;
	do {
		to.AddUp(up[--size]);
	} while (size > i);

	upSize = size;
}
void Pile::Reset() {
	size = 0;
	upSize = 0;
	downSize = 0;
}
void Pile::Initialize() {
	size = 0;
	upSize = 0;
	downSize = 0;
	for (int i = 0; i < 24; i++) {
		up[i].Clear();
		down[i].Clear();
	}
}
int Pile::Size() {
	return size;
}
int Pile::DownSize() {
	return downSize;
}
int Pile::UpSize() {
	return upSize;
}
Card Pile::operator[](int index) {
	return up[index];
}
Card Pile::Down(int index) {
	return down[index];
}
Card Pile::Up(int index) {
	return up[index];
}
Card Pile::Low() {
	return up[upSize - 1];
}
Card Pile::High() {
	return up[0];
}
int Pile::HighValue() {
	return upSize > 0 ? up[0].Value : 99;
}