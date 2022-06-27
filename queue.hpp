# ifndef QUEUE
# define QUEUE

#include <assert.h>
#include <iostream>

const int SIZE = 32;

template<typename T, int array_size = SIZE + 1>
class queue {
  private: 
		int head, tail, size;
		T a[array_size + 1];
	public:
		queue() { size = array_size; }
		
		inline bool isempty() { return head == tail; }
		inline bool isfull() { return (tail + 1) % size == head; }
		inline T front() { return a[(head + 1) % size]; }
		void push(const T &x) {
			if(isfull()) {
				std :: cerr << "queue_size_has_full\n";
				assert(0);
			}
			++tail; if(tail == size) tail = 0;
			a[tail] = x;
		}
		T pop() {
			++head; if(head == size) head = 0;
			return a[head];
		}
};

# endif