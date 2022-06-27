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

int pc, next_pc;
uint rg[32]; 
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

struct ReservationStation { // 保留站实现
  struct RS_node {
    bool busy;
    int Q1, Q2;
    uint V1, V2;

    RS_node(bool busy = 0, int Q1 = 0, int Q2 = 0, uint V1 = 0, uint V2 = 0):
      busy(busy), Q1(Q1), Q2(Q2), V1(V1), V2(V2) {}
    
    inline bool ready() { return !Q1 && !Q2; }
    inline void clear() { busy = Q1 = Q2 = V1 = V2 = 0; }
  };

  struct RSbuffer {
    int head, next[SIZE]; // head存的是第一个空的位置, next指向下一个空的位置
    RS_node a[SIZE];

    RSbuffer() { 
      head = 0;
      for(int i = 0; i < SIZE; ++i) 
        next[i] = i + 1;
    }
    
    inline bool isfull() { return head == SIZE; }
  } pre, nex;

  void update() { 
    pre = nex; 
  }

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
  
} RS;


void update(); 

queue<Instruction> preIQ, nexIQ;
bool fet_ins_flag = false;

inline uint getcommand(int pos) {
  return (uint) mem[pos] | ((uint) mem[pos + 1] << 8) | ((uint) mem[pos + 2] << 16) | ((uint) mem[pos + 3] << 24);
}
void run_InstructionQueue() {
  if(fet_ins_flag) nexIQ.pop(); // 发送指令"直接"issue

  if(!preIQ.isfull()) { // 判断现在是否可以再从内存中获取一条指令
    uint fet = getcommand(pc);
    Instruction ins; 
    ins.decode(fet);
    ins.pc = pc;
    if(ins.typ == JAL) { // JAL
      ins.npc = pc + ins.imm;
    } else if((fet & 0x7f) == 0b1100011) { // B-type
      // to: branch predict 
      ins.npc = pc + 4; // 默认不跳转
    } else {
      ins.npc = pc + 4;
    }
    nexIQ.push(ins);
  }
}
void Issue() {
  if(!preIQ.isempty()) { // 当前的IQ中可以issue一个指令
    Instruction ins = preIQ.front();
    
    
    if(9 <= ins.typ && ins.typ <= 11 || 13 <= ins.typ && ins.typ <= 17) {
      // load / stroe 指令

    } else {
      // 发送给RS
    }
  }
}

int main() {
  input();
  while(true) {
    update();

    run_InstructionQueue();
    Issue();

    exit(0);
  }
  return 0;
}

void update() {
  pc = next_pc;
  preIQ = nexIQ; // Instruction Queue
  RS.update(); // Resevation Station
}