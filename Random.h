#ifndef Random_h
#define Random_h

class Random {
private:
	int value, mix, twist;
	unsigned int seed;

	void CalculateNext();
public:
	Random();
	Random(int seed);
	void SetSeed(int seed);
	int Next1();
	int Next2();
};
#endif