#include "queue.hpp"
#include "Instruction.hpp"

#include <cstdio>
#include <iostream>
typedef unsigned int uint;

const int SIZE = 32;

namespace {
  template <typename T>
  void print(T x) {
    std :: cout << x << std :: endl;
  }
  template <typename T1, typename T2>
  void print(T1 x1, T2 x2) {
    std :: cout << x1 << ' ' << x2 << std :: endl;
  }
}

int pc; uint rg[32]; 
unsigned char mem[1000005]; // unsigned char: 代表1个byte

void input() {
  char s[100];
  int address;
  while(~scanf("%s", s)) {
    if(s[0] == '@') {
      sscanf(s + 1, "%x", &address);
    } else {
      sscanf(s, "%x", mem + address);
      ++address;
    }
  }
}

struct ReservationStation {
  struct RS_node {
    bool busy;
    int Q1, Q2;
    uint V1, V2;

    RS_node(bool busy = 0, int Q1 = 0, int Q2 = 0, uint V1 = 0, uint V2 = 0):
      busy(busy), Q1(Q1), Q2(Q2), V1(V1), V2(V2) {}
    
    inline bool ready() { return !Q1 && !Q2; }
    inline void clear() { busy = Q1 = Q2 = V1 = V2 = 0; }
  };

  struct RS {
    int head, next[SIZE]; // head存的是第一个空的位置, next指向下一个空的位置
    RS_node a[SIZE];

    RS() { 
      head = 0;
      for(int i = 0; i < SIZE; ++i) 
        next[i] = i + 1;
    }
    
    inline bool isfull() { return head == SIZE; }
  } pre, nex;

  // insert,remove都是对nex进行操作
  void insert(const RS_node x) { 
    nex.a[nex.head] = x;
    nex.head = nex.next[nex.head];
  }
  void remove(int pos) {
    nex.a[pos].clear();
    nex.next[pos] = nex.head;
    nex.head = pos;
  }
  void update() { pre = nex; }
};

int main() {

  return 0;
}