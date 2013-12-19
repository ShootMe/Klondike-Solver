#include"Random.h"
//fast and simple random number generator I put together
//randomness tested very well at http://www.cacert.at/random/
void Random::CalculateNext() {
	int y = value ^ twist - mix ^ value;
	y ^= twist ^ value ^ mix;
	mix ^= twist ^ value;
	value ^= twist - mix;
	twist ^= value ^ y;
	value ^= (twist << 7) ^ (mix >> 16) ^ (y << 8);
}
Random::Random() {
	SetSeed(101);
}
Random::Random(int seed) {
	SetSeed(seed);
}
void Random::SetSeed(int seed) {
	this->seed = seed;
	mix = 51651237;
	twist = 895213268;
	value = seed;

	for (int i = 0; i < 50; ++i) {
		CalculateNext();
	}

	seed ^= (seed >> 15);
	value = 0x9417B3AF ^ seed;

	for (int i = 0; i < 950; ++i) {
		CalculateNext();
	}
}
int Random::Next1() {
	CalculateNext();
	return value & 0x7fffffff;
}
int Random::Next2() {
	if (seed == 0) { seed = 0x12345987; }
	int k = seed / 127773;
	seed = 16807 * (seed - k * 127773) - 2836 * k;
	if ((int)seed < 0) { seed += 2147483647; }
	return seed & 0x7fffffff;
}