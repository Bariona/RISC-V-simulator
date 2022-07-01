# ifndef QUEUE
# define QUEUE

#include <assert.h>
#include <iostream>

template<typename T, int array_size = 32>
class queue {
	// 维护的是下标[1:32]的循环队列
  	private: 
		int head, tail, size;
		T a[array_size + 5];
	public:
		// tail指向空位置
		queue() { head = tail = 0, size = array_size; }
		
		inline bool isempty() { return head == tail; }
		inline bool isfull() { return (tail + 1) % size == head; }
		inline T front() { return a[head]; }
		void push(const T &x) {
			if(isfull()) {
				std :: cerr << "queue_size_has_full\n";
				assert(0);
			}
			a[tail] = x;
			++tail; if(tail == size) tail = 0;
		}
		T pop() {
			T tmp = a[head];
			++head; if(head == size) head = 0;
			return tmp;
		}
		void print() {
			int idx = head;
			while(idx != tail) {
				a[idx].print();
				idx = (idx + 1) % size;
			}
		}
		
		// 主要用于ROB中, 返回队列的下一个空位置, 注意这里下标整体右移1位 
		inline int getTail() { return tail + 1; } 
		inline T & getfront() { return a[head]; }
		T & operator [] (int pos) {
			//assert(pos >= 1 && pos <= 32);
			return a[pos - 1];
		}
		void clear() {
			for(int i = 0; i < size; ++i)
				a[i].clear();
			head = tail = 0;
		}
		
};

# endif