# ifndef QUEUE
# define QUEUE

#include <assert.h>
#include <iostream>

template<typename T, int array_size = 32 + 1>
class queue {
	// 维护的是下标[1:32]的循环队列
  	private: 
		int head, tail, size;
		T a[array_size + 1];
	public:
		queue() { head = tail = 0, size = array_size; }
		
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
	
		inline int getTail() { return (tail + 1) % size + 1; } // 返回队列的下一个空位置
		T & operator [] (int pos) {
			assert(pos >= 1 && pos <= 32);
			return a[pos - 1];
		}
		
};

# endif