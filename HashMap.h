#ifndef HashMap_h
#define HashMap_h
using namespace std;

#define KEYSIZE 21
#define KEYSIZEM1 KEYSIZE - 1

struct HashKey {
	char Key[KEYSIZE];

	HashKey() {
		for (int i = 0; i < KEYSIZE; i++) {
			Key[i] = 0;
		}
	}

	int ComputeHash() const {
		unsigned long hash = 0;
		int i = 0;

		while (i < KEYSIZE) {
			hash = (int)Key[i++] + (hash << 7) + (hash << 16) - hash;
		}

		return hash;
	}
	bool operator==(HashKey const& key) const {
		int i = 0;
		while (i < KEYSIZEM1 && Key[i] == key.Key[i]) { i++; }
		return Key[i] == key.Key[i];
	}
	char & operator[](int index) {
		return Key[index];
	}
};

template <typename T> struct KeyValue {
	KeyValue<T> * Next;
	HashKey Key;
	T Value;
	int Hash;

	KeyValue();
	KeyValue(int hash, HashKey const& key, T const& value, KeyValue<T> * next);
	~KeyValue();
};

template <typename T> KeyValue<T>::KeyValue() {
	Next = NULL;
}
template <typename T> KeyValue<T>::KeyValue(int hash, HashKey const& key, T const& value, KeyValue<T> * next) {
	Next = next;
	Key = key;
	Hash = hash;
	Value = value;
}
template <typename T> KeyValue<T>::~KeyValue() {
	delete Next;
}

template <typename T> class HashMap {
private:
	KeyValue<T> * table;
	int size, capacity, powerOf2, maxLength, slotsUsed;

public:
	HashMap(int powerOf2);
	~HashMap();

	int Size();
	int Capacity();
	int SlotsUsed();
	int MaxLength();
	void Clear();
	KeyValue<T> * Add(HashKey const& key, T const& value);
};

template <typename T> HashMap<T>::HashMap(int powerOf2) {
	this->powerOf2 = powerOf2;
	size = 0;
	slotsUsed = 0;
	maxLength = 0;
	capacity = (1 << powerOf2) - 1;
	table = new KeyValue<T>[capacity + 1];
}
template <typename T> HashMap<T>::~HashMap() {
	delete[] table;
}
template <typename T> int HashMap<T>::Size() {
	return size;
}
template <typename T> int HashMap<T>::MaxLength() {
	return maxLength;
}
template <typename T> int HashMap<T>::SlotsUsed() {
	return slotsUsed;
}
template <typename T> int HashMap<T>::Capacity() {
	return capacity + 1;
}
template <typename T> KeyValue<T> * HashMap<T>::Add(HashKey const& key, T const& value) {
	int hash = key.ComputeHash();
	int i = hash;
	i ^= (hash >> 16);
	i &= capacity;
	KeyValue<T> * node = &table[i];

	int length = 1;
	while (node != NULL && node->Key[0] != 0) {
		if (node->Hash == hash && key == node->Key) { return node; }
		node = node->Next;
		length++;
	}

	++size;
	node = &table[i];
	if (node->Key[0] != 0) {
		node->Next = new KeyValue<T>(node->Hash, node->Key, node->Value, node->Next);
	}
	node->Hash = hash;
	node->Key = key;
	node->Value = value;

	if (length > maxLength) { maxLength = length; }
	if (length == 1) { slotsUsed++; }
	return NULL;
}
template <typename T> void HashMap<T>::Clear() {
	KeyValue<T> * old = table;
	delete[] old;
	table = new KeyValue<T>[capacity + 1];
	size = 0;
	slotsUsed = 0;
	maxLength = 0;
}
#endif