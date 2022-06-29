#include "queue.hpp"
#include "Instruction.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>
typedef unsigned int uint;

# define debug(x) std :: cout << #x << ": " << (x) << std :: endl;

const int SIZE = 32;

# ifdef Debug
  using std :: cout;
  using std :: endl;
# endif

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

int clk = 0, pc, next_pc;
unsigned char mem[1000005]; // unsigned char: 代表1个byte

void input() {
  char s[100];
  int address, getn;
  while(~scanf("%s", s)) {
    if(s[0] == '@') {
      sscanf(s + 1, "%x", &address);
    } else {
      sscanf(s, "%x", &getn);
      mem[address] = getn;
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
    void clear() {
      memset(preReg.regState, 0, sizeof preReg.regState);
      memset(nexReg.regState, 0, sizeof nexReg.regState);
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
    busy = false;
    Q1 = Q2 = V1 = V2 = 0; 
  }
  void modify(int id, uint val) {
    if(Q1 == id) {
      V1 = val;
      Q1 = 0;
    }
    if(Q2 == id) {
      V2 = val;
      Q2 = 0;
    }
  }
};

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
Info curCommit, nexCommit;

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
      void clear() {
        head = 0;
        for(int i = 0; i < SIZE; ++i) {
          a[i].clear();
          next[i] = i + 1;
        }
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
        if(pre.a[i].busy && pre.a[i].ready()) {
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
    void clear() {
      pre.clear();
      nex.clear();
      EXnode.clear();
    }
};

struct SLB_node {
  RS_node node;
  bool hascommit;
  // cnt 用来计数: 3个周期一次excute

  SLB_node() { hascommit = 0; }
  SLB_node(RS_node node, int hascommit):
    node(node), hascommit(hascommit) {}
  
  inline bool ready() {
    if(node.ins.typ == SB || node.ins.typ == SW || node.ins.typ == SH) 
      return node.ready() && hascommit;
    return node.ready();
  }
  void modify(int id, uint val) {
    node.modify(id, val);
  }
  void clear() {
    hascommit = 0;
    node.clear();
  }
};

class SLBuffer{
  friend void run_SLBuffer();
  private: 
    queue<SLB_node> preSLB, nexSLB;

  public:
    void update() {
      preSLB = nexSLB;
    }
    inline bool empty() {
      return preSLB.isempty();
    }
    void run() {
      curSLBres.hasres = false;

      SLB_node &front = preSLB.getfront();
      if(front.ready()) {
        uint dst;
        front.node.ins.doit(dst, front.node.V1, front.node.V2);
        switch(front.node.ins.typ) {
          // store type
          case SB: mem[int(dst)] = (unsigned char) front.node.V2; break;  // 取低位[7:0] (1byte) 
          case SH: *(unsigned short *)(mem + int(dst)) = (unsigned short) front.node.V2; break; // [15:0]
          case SW: *(unsigned int *)(mem + int(dst)) = front.node.V2; break;

          // load type
          case LB: { // 符号位拓展[7:0]
            uint x = (uint) mem[dst]; 
            if(x >> 7 & 1) x |= 0xffffff00;
            front.node.V2 = x;
            break;
          }
          case LH: { // 符号位拓展[15:0]
            uint x = (uint) mem[dst] | ((uint) mem[dst + 1] << 8);
            if(x >> 15 & 1) x |= 0xffff0000;
            front.node.V2 = x;
            break;
          }
          case LW: { // 符号位拓展[31:0]
            int idx = dst;
            front.node.V2 = (uint) mem[idx] | ((uint) mem[idx + 1] << 8) | ((uint) mem[idx + 2] << 16) | ((uint) mem[idx + 3] << 24);
            break;
          }
          case LBU: front.node.V2 = (uint) mem[dst]; break;
          case LHU: front.node.V2 = (uint) mem[dst] | ((uint) mem[dst + 1] << 8); break;
        }
        curSLBres.hasres = true;
        curSLBres.pos = front.node.target;
        curSLBres.val = front.node.V2;
        nexSLB.pop();
        
      }
    }
    void modify(int id, uint val) {
      for(int i = 1; i <= SIZE; ++i) {
        nexSLB[i].modify(id, val);
      }
    }
    void insert(const SLB_node x) {
      nexSLB.push(x);
    }
    inline void can_commit() {
      nexSLB.getfront().hascommit = true;
    }

    void clear() {
      preSLB.clear();
      nexSLB.clear();
    }
};

struct ROB_node {
  RS_node rs;
  bool ready;
  uint val;

  ROB_node() { 
    ready = false; 
    val = 0;
  }
  ROB_node(RS_node rs, bool ready = false, uint val = 0): 
    rs(rs), ready(ready), val(val) {}
  void clear() {
    ready = false;
    val = 0;
    rs.clear();
  }
};

class ReorderBuffer {
  friend void run_ROB();

  private:  
    queue<ROB_node> preROB, nexROB;

  public:
    void update() {
      preROB = nexROB;
    }
    inline int getpos() {
      return preROB.getTail();
    }
    inline bool isfull() {
      return nexROB.isfull();
    }
    void insert(const ROB_node x) {
      nexROB.push(x);
    }
    bool can_commit() {
      return preROB.front().ready;
    }
    void modify(int pos, uint val) {
      assert(pos >= 1 && pos <= 32);
      nexROB[pos].val = val;
      nexROB[pos].ready = true;
    }
    ROB_node & operator [] (int pos) { 
      return preROB[pos];
    }

    void clear() {
      preROB.clear();
      nexROB.clear();
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
    // 此处跳转到预测pc的位置
    next_pc = ins.predict_pc;

    nexIQ.push(ins);
  }
}

RS_node Issue_ins;

void Issue() {

  issue_to_rs = issue_to_slb = false;

  if(issue_flag) { // 当前的IQ中可以issue一个指令
    assert(!preIQ.isempty());

    Instruction ins = preIQ.front();
    
    int id = ROB.getpos(); // ROB中要插入的位置来作为regState[rd]的id
  # ifdef Debug
    // debug(issue_flag);
    printf("issue instruct: "); 
    ins.print();
    printf("put it to ROB: %d\n", id);
  # endif

    // 指令插入SLbuffer 或者 RS中
    // 在本次作业中，我们认为相应寄存器的值已在ROB中存储但尚未commit的情况是可以直接获得的，即你需要实现这个功能

    int Q1 = 0, Q2 = 0, V1 = 0, V2 = 0;
    {
      if(~ins.rs1) { // 得到Qj/Vj
        Q1 = reg(ins.rs1);
        if(!Q1) V1 = reg[ins.rs1];
        else if(ROB[Q1].ready) { // 可以从ROB中获取到值
          V1 = ROB[Q1].val;
          Q1 = 0;
        } else {
          V1 = 0;
        }
      }
      if(~ins.rs2) { // 得到Qk/Vk
        Q2 = reg(ins.rs2);
        if(!Q2) V2 = reg[ins.rs2];
        else if(ROB[Q2].ready) {
          V2 = ROB[Q2].val;
          Q2 = 0;
        } else {
          V2 = 0;
        }
      }
      if(~ins.rd) { 
        // 修改目标寄存器 nexReg[rd] 的 State 状态, 改为id
        reg.modify_state(ins.rd, id); 
      }
      
      // true表示busy, target = -1表示没有目标寄存器, 为branch指令
      Issue_ins = RS_node(true, Q1, Q2, V1, V2, ins, (~ins.rd) ? id : -1); 
      
      if((9 <= ins.typ && ins.typ <= 11) || (13 <= ins.typ && ins.typ <= 17)) {
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
    nexEX.hasres = true; 
    nexEX.node = Issue_ins;
    nexEX.pos = Issue_ins.target;
  } else {
    if(issue_to_rs) {
      RS.insert(Issue_ins);
    }

    if(RS.canEX()) { 
      // 下一个周期做 RS.EXnode;
    # ifdef Debug
      // cerr 
    # endif
    
      nexEX.hasres = true;
      nexEX.node = RS.EXnode;
      nexEX.pos = RS.EXnode.target;
    } else {
      nexEX.hasres = false; // 没有指令可以执行
    }
  }
  # ifdef Debug
    cout << clk << ' ' << nexEX.hasres << endl;
    cout << issue_to_rs << endl;
  # endif
  

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
  # ifdef Debug
    cout << "current Excute: "; curEX.node.ins.print();
  # endif

    curEX.excute(); 
    
    curEXres.hasres = true;
    curEXres.val = curEX.val;
    curEXres.pos = curEX.pos;
  } else { // 当前没有要excute的指令
    curEXres.hasres = false;
  }
}

void run_SLBuffer() {
  if(issue_to_slb) {
    SLB_node tmp = SLB_node(Issue_ins, false); // 还没有commit
    // if(SLB.empty() && tmp.node.ready()) {
    //   ++tmp.cnt;
    // }
    SLB.insert(tmp);
  }
  if(!SLB.empty()) {
    SLB.run();
  }
  //  同时SLBUFFER还需根据上个周期EX和SLBUFFER的计算结果遍历SLBUFFER进行数据的更新。
  if(preEXres.hasres) {
    SLB.modify(preEXres.pos, preEXres.val);
  }
  if(preSLBres.hasres) {
    SLB.modify(preSLBres.pos, preSLBres.val);
  }
  
}

void run_ROB() {
  if(ROB.isfull()) issue_flag_nex = false;
  if(!ROB.preROB.isempty()) {
    ROB_node & front = ROB.preROB.getfront();
    if(front.ready) {
      nexCommit.hasres = true;
      nexCommit.pos = front.rs.target;
      nexCommit.node = front.rs;
      nexCommit.val = front.val;
    } else {
      nexCommit.hasres = false;
    }
  }
  if(preEXres.hasres) {
    ROB.modify(preEXres.pos, preEXres.val);
  }
  if(preSLBres.hasres) {
    ROB.modify(preSLBres.pos, preSLBres.val);
  }
}
void CLEAR();
void Commit() {
  if(curCommit.hasres) {
    Instruction ins = curCommit.node.ins;
    if(ins.fetch == 0x0ff00513) {
      print(reg[0] & 255u);
      exit(0);
    }
    if(ins.typ == JALR || (ins.typ >= 3 && ins.typ <= 8)) { 
      // 分支预测指令: JALR 或者 branch指令
      bool predict_res = true;
      if(ins.typ == JALR) {
        predict_res = false;
      } else if(ins.npc != ins.predict_pc) {
        predict_res = false;
      }

      if(predict_res == false) {
        CLEAR();
        next_pc = ins.npc;
      }
    } else {
      // 其他指令
      if(~curCommit.node.ins.rd) {
        reg.modify_value(curCommit.node.ins.rd, curCommit.pos, curCommit.val);
      }
      if(ins.typ == SW || ins.typ == SH || ins.typ == SB) {// store指令 
        SLB.can_commit();
        // assert();
      }
    }
  }
}

int main() {
  input();

  while(true) {
    ++clk;
    # ifdef Debug
      printf("==== cycle %d: begin ====\n", clk);
    # endif

    update();

    run_InstructionQueue();
    Issue();
    
    run_ReservationStation();
    run_SLBuffer();
    EX();

    run_ROB();
    Commit();

    # ifdef Debug
      puts("");
    # endif
    // exit(0);
  }
  return 0;
}

void update() {
  pc = next_pc;
  issue_flag = issue_flag_nex, issue_flag_nex = true;

  preEXres = curEXres, curEXres.clear();
  preSLBres = curSLBres, curSLBres.clear();
  curEX = nexEX, nexEX.clear();
  curCommit = nexCommit, nexCommit.clear();

  reg.update();
  preIQ = nexIQ; // Instruction Queue
  RS.update(); // Resevation Station
  SLB.update();
  ROB.update();
}

void CLEAR() {
  preEXres.clear(), curEXres.clear();
  preSLBres.clear(), curSLBres.clear();
  curEX.clear(), nexEX.clear();
  curCommit.clear(), nexCommit.clear();

  reg.clear();
  preIQ.clear(), nexIQ.clear();
  RS.clear();
  SLB.clear();
  ROB.clear();
}