#include "queue.hpp"
#include "Instruction.hpp"

#include <cstdio>
#include <cstring>
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

class Registerfile{
  private:
    struct Regfile {
      uint reg[32]; 
      int regState[32];
      
      Regfile() {
        memset(reg, 0, sizeof reg);
        memset(regState, 0, sizeof regState);
      }
    };
  Regfile preReg, nexReg;

  public: 
    void update() {
      preReg = nexReg;
    }
    inline uint & operator [] (int pos) { // 用于访问具体的reg值
      return preReg.reg[pos];
    }
    inline int & operator () (int pos) { // 用于方位reg的State状态
      return preReg.regState[pos];
    }
    inline void modify_state(int pos, int k) { // modify 都要对nexReg进行修改
      nexReg.regState[pos] = k;
    }
    inline void modify_value(int pos, int state, uint k) {
      // 需要当前reg[pos]的State与commit的指令编号相同才行!
      assert(state == nexReg.regState[pos]);
      if(state == nexReg.regState[pos]) {
        nexReg.regState[pos] = 0;
        nexReg.reg[pos] = k;
      }
    }
};

struct RS_node {
  bool busy; // check当前节点是否已经excute完毕 1: 表示还未执行完毕
  int Q1, Q2, target;
  uint V1, V2;
  Instruction ins;

  RS_node() {
    busy = 0;
    Q1 = Q2 = V1 = V2 = 0;
  }
  RS_node(bool busy, int Q1, int Q2, uint V1, uint V2, Instruction ins, int target):
    busy(busy), Q1(Q1), Q2(Q2), V1(V1), V2(V2), ins(ins), target(target) {}
  
  inline bool ready() { // ready 来执行的状态
    return !Q1 && !Q2; 
  }
  inline void clear() { 
    busy = Q1 = Q2 = V1 = V2 = 0; 
  }
  void modify(int id, uint val) {
    if(Q1 == id) V1 = val;
    if(Q2 == id) V2 = val;
  }
};

class ReservationStation { // 保留站实现
  private: 
    struct RSbuffer {
      int head, next[SIZE]; // head存的是第一个空的位置, next指向下一个空的位置
      RS_node a[SIZE];

      RSbuffer() { 
        head = 0;
        for(int i = 0; i < SIZE; ++i) 
          next[i] = i + 1; // 这里空的时候head就会是SIZE
      }

      inline bool isfull() { 
        return head == SIZE; 
      }
  };
  
  RSbuffer pre, nex; // 创建两个版本的RS

  public: 
    RS_node EXnode;

    void update() { 
      pre = nex; 
    }

    bool canEX() {
      for(int i = 0; i < SIZE; ++i) {
        if(pre.a[i].ready()) {
          EXnode = pre.a[i];
          remove(i);
          return true;
        }
      }
      return false;
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

    void modify(int id, uint val) {// 根据Ex/SLB的结果修改RS的值
      for(int i = 0; i < SIZE; ++i) {
        nex.a[i].modify(id, val); // 以此check是否能够修改
      }
    }
};

class SLBuffer{
  private: 
    queue<RS_node> preSLB, nexSLB;

  public:
    void update() {
      preSLB = nexSLB;
    }
    void insert(const RS_node x) {
      nexSLB.push(x);
    }
};

struct ROB_node {
  RS_node rs;
  bool ready;
  uint ans;

  ROB_node() { 
    ready = false; 
    ans = 0;
  }
  ROB_node(RS_node rs, bool ready = false, uint ans = 0): 
    rs(rs), ready(ready), ans(ans) {}
};

class ReorderBuffer {
  private:  
    queue<ROB_node> preROB, nexROB;

  public:
    int getpos() {
      return preROB.getTail();
    }

    void update() {
      preROB = nexROB;
    }
    void insert(const ROB_node x) {
      nexROB.push(x);
    }
    ROB_node & operator [] (int pos) { 
      return preROB[pos];
    }
};

void update(); 

Registerfile reg;
queue<Instruction> preIQ, nexIQ;
ReservationStation RS;
SLBuffer SLB;
ReorderBuffer ROB;

bool issue_flag = false, issue_flag_nex = false;
bool issue_to_rs = false, issue_to_slb = false;

struct Info {
  bool hasres;
  int pos;
  uint val;
  RS_node node;

  Info(bool hasres = 0, int pos = 0, uint val = 0):
    hasres(hasres), pos(pos), val(val) {}

  inline void clear() {
    hasres = pos = val = 0;
    node.clear();
  }
  void excute() {
    // excute ins这条指令, 将答案存储在val中
    node.ins.doit(val, node.V1, node.V2);
  }
};

Info preEXres, curEXres, preSLBres, curSLBres;
Info curEX, nexEX;

inline uint getcommand(int pos) {
  return (uint) mem[pos] | ((uint) mem[pos + 1] << 8) | ((uint) mem[pos + 2] << 16) | ((uint) mem[pos + 3] << 24);
}

void run_InstructionQueue() {
  if(issue_flag) {
    assert(!nexIQ.isempty());
    nexIQ.pop(); // 发送指令"直接"issue
  }

  // 判断现在是否可以再从内存中获取一条指令
  if(!nexIQ.isfull()) { 
    uint fet = getcommand(pc);
    Instruction ins;

    ins.decode(fet);
    ins.pc = pc;

    if(ins.typ == JAL) { // JAL
      ins.predict_pc = pc + ins.imm;
    } else if((fet & 0x7f) == 0b1100011) { // B-type
      // to: branch predict 
      ins.predict_pc = pc + 4; // 默认不跳转
    } else {
      ins.predict_pc = pc + 4;
    }
    nexIQ.push(ins);
  }
}

RS_node Issue_ins;

void Issue() {
  if(issue_flag) { // 当前的IQ中可以issue一个指令
    assert(!preIQ.isempty());

    Instruction ins = preIQ.front();
    int id = ROB.getpos(); // ROB中要插入的位置

    // 指令插入SLbuffer 或者 RS中
    int Q1 = 0, Q2 = 0, V1 = 0, V2 = 0;
    {
      if(~ins.rs1) { // 得到Qj/Vj
        Q1 = reg(ins.rs1);
        if(!Q1) V1 = reg[ins.rs1];
        else if(ROB[Q1].ready) { // 可以从ROB中获取到值
          V1 = ROB[Q1].ans;
          Q1 = 0;
        } else {
          V1 = 0;
        }
      }
      if(~ins.rs2) { // 得到Qk/Vk
        Q2 = reg(ins.rs2);
        if(!Q2) V2 = reg[ins.rs2];
        else if(ROB[Q2].ready) {
          V2 = ROB[Q2].ans;
          Q2 = 0;
        } else {
          V2 = 0;
        }
      }
      if(~ins.rd) { // 修改目标寄存器 reg[rd] 的 State 状态, 改为id
        reg.modify_state(ins.rd, id); 
      }
      
      // true表示busy, target = -1表示没有目标寄存器, 为branch指令
      Issue_ins = RS_node(true, Q1, Q2, V1, V2, ins, (~ins.rd) ? id : -1); 

      issue_to_rs = issue_to_slb = false;
      if(9 <= ins.typ && ins.typ <= 11 || 13 <= ins.typ && ins.typ <= 17) {
      // load / stroe 指令
        issue_to_slb = 1;
        // SLB.insert(tmp);
      } else {  
        issue_to_rs = 1;
        // 发送给RS RS.insert(tmp);
      }
    }

    // 指令插入到ROB中
    ROB.insert(ROB_node(Issue_ins));
  }
}

void run_ReservationStation() {
  if(issue_to_rs && Issue_ins.ready()) {
    // 下一个周期做 Issue_ins, 存储在curEX中
    curEX.hasres = 1; 
    curEX.node = Issue_ins;
    curEX.pos = Issue_ins.target;
  } else {
    RS.insert(Issue_ins);
    if(RS.canEX()) {
      // 下一个周期做 RS.EXnode;
      curEX.hasres = 1;
      curEX.node = RS.EXnode;
      curEX.pos = RS.EXnode.target;
    }
  }

  if(preEXres.hasres) { // 根据Excute的结果来修改RS_nex的值
    RS.modify(preEXres.pos, preEXres.val);
  }
  if(preSLBres.hasres) { // 根据SLBuffer的结果来修改RS_nex的值
    RS.modify(preSLBres.pos, preSLBres.val);
  }
}

void EX() {
  // 将当前处理好的指令存在val中, 并且存储EXres
  if(curEX.hasres) { // 当先存在可执行命令
    curEX.excute(); 
    
    curEXres.hasres = true;
    curEXres.val = curEX.val;
    curEXres.pos = curEX.pos;
  } else { // 当前没有要excute的指令
    curEXres.hasres = false;
  }
}

void run_SLBuffer() {

}



int main() {
  input();
  while(true) {
    update();

    run_InstructionQueue();
    Issue();
    
    run_ReservationStation();
    run_SLBuffer();
    EX();

    exit(0);
  }
  return 0;
}

void update() {
  pc = next_pc;
  issue_flag = issue_flag_nex;
  issue_flag_nex = true;

  preEXres = curEXres; curEXres.clear();
  preSLBres = curSLBres; curSLBres.clear();
  curEX = nexEX, nexEX.clear();

  reg.update();
  preIQ = nexIQ; // Instruction Queue
  RS.update(); // Resevation Station
  SLB.update();
  ROB.update();
}