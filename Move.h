#ifndef Move_h
#define Move_h
#include<memory>
using namespace std;

struct Move {
	unsigned char From, To, Count, Extra;

	Move();
	Move(unsigned char from, unsigned char to, unsigned char count, unsigned char extra);
	void Set(unsigned char from, unsigned char to, unsigned char count, unsigned char extra);
};

struct MoveNode {
	shared_ptr<MoveNode> Parent;
	Move Value;

	MoveNode(Move move);
	MoveNode(Move move, shared_ptr<MoveNode> const& parent);
};
#endif