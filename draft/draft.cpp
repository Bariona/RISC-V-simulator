#include <bits/stdc++.h>
using namespace std;
typedef unsigned int uint;

int pc; uint rg[32]; 
unsigned char mem[1000005]; // unsigned char: 代表1个byte

template <typename T>
inline void print(T x) {
  for(int i = 31; ~i; --i)
    putchar('0' + (x >> i & 1));
  putchar('\n');
}
inline int ch(char c) {
  if(isdigit(c)) return c ^ '0';
  return c - 'A' + 10;
}
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

enum Instype {
  LUI, AUIPC, // U
  JAL, // J
  BEQ, BNE, BLT, BGE, BLTU, BGEU, // B
  SB, SH, SW, // S
  JALR, LB, LH, LW, LBU, LHU, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, // I
  ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND, // R
};
struct Instruction {
  Instype typ;
  uint imm;
  int rs1, rs2, rd;
  Instruction() {}
  Instruction(Instype typ, uint imm, int rs1, int rs2, int rd):
    typ(typ), imm(imm), rs1(rs1), rs2(rs2), rd(rd) {}
} cur;

inline void print(Instype x) {
  switch(x) {
    case 0: puts("LUI"); break;
    case 1: puts("AUIPC"); break;
    case 2: puts("JAL"); break;
    case 3: puts("BEQ"); break;
    case 4: puts("BNE"); break;
    case 5: puts("BLT"); break;
    case 6: puts("BGE"); break;
    case 7: puts("BLTU"); break;
    case 8: puts("BGEU"); break;
    case 9: puts("SB"); break;
    case 10: puts("SH"); break;
    case 11: puts("SW"); break;
    case 12: puts("JALR"); break;
    case 13: puts("LB"); break;
    case 14: puts("LH"); break;
    case 15: puts("LW"); break;
    case 16: puts("LBU"); break;
    case 17: puts("LHU"); break;
    case 18: puts("ADDI"); break;
    case 19: puts("SLTI"); break;
    case 20: puts("SLTIU"); break;
    case 21: puts("XORI"); break;
    case 22: puts("ORI"); break;
    case 23: puts("ANDI"); break;
    case 24: puts("SLLI"); break;
    case 25: puts("SRLI"); break;
    case 26: puts("SRAI"); break;
    case 27: puts("ADD"); break;
    case 28: puts("SUB"); break;
    case 29: puts("SLL"); break;
    case 30: puts("SLT"); break;
    case 31: puts("SLTU"); break;
    case 32: puts("XOR"); break;
    case 33: puts("SRL"); break;
    case 34: puts("SRA"); break;
    case 35: puts("OR"); break;
    case 36: puts("AND"); break;
  }
}

uint fet;
void IF() {
  // printf("pc = %4d, %02x%02x%02x%02x\t", pc, mem[pc], mem[pc + 1], mem[pc + 2], mem[pc + 3]);
  fet = (uint) mem[pc] | ((uint) mem[pc + 1] << 8) | ((uint) mem[pc + 2] << 16) | ((uint) mem[pc + 3] << 24);
  // print(fet);
  pc += 4;
}
Instruction decode(uint fet) {
  Instype typ;
  uint imm = 0;
  int rd = -1, rs1 = -1, rs2 = -1;

  //print(fet);
  switch(fet & 0x7f) {
    case 0x37: { // U lui
      typ = LUI;
      rd = fet >> 7 & 0x1f;
      imm = (fet >> 12 & 0xfffff) << 12;
      break;
    }
    case 0x17: { // U auipc
      typ = AUIPC;
      rd = fet >> 7 & 0x1f;
      imm = (fet >> 12 & 0xfffff) << 12;
      break;
    }
    case 0x6f: { // J jal
      typ = JAL;
      rd = fet >> 7 & 0x1f;
      imm = ((fet >> 21 & 0x3ff) << 1) | ((fet >> 20 & 1) << 11) | ((fet >> 12 & 0xff) << 12); if(fet >> 31) imm |= 0xfff00000;
      break;
    }
    case 0x63: { // B 
      rs1 = fet >> 15 & 0x1f;
      rs2 = fet >> 20 & 0x1f;
      imm = ((fet >> 7 & 1) << 11) | ((fet >> 8 & 0xf) << 1) | ((fet >> 25 & 0x3f) << 5); if(fet >> 31) imm |= 0xfffff800;
      switch(fet >> 12 & 7) {
        case 0: typ = BEQ; break;
        case 1: typ = BNE; break;
        case 4: typ = BLT; break;
        case 5: typ = BGE; break;
        case 6: typ = BLTU; break;
        case 7: typ = BGEU; break;
        default: assert(0);
      }
      break;
    }
    case 0x23: { // S
      rs1 = fet >> 15 & 0x1f;
      rs2 = fet >> 20 & 0x1f;
      imm = (fet >> 7 & 0x1f) | ((fet >> 25 & 0x3f) << 5); if(fet >> 31) imm |= 0xfffff800;
      switch(fet >> 12 & 7) {
        case 0: typ = SB; break;
        case 1: typ = SH; break;
        case 2: typ = SW; break;
        default: assert(0);
      }
      break;
    }
    case 0x67: { // I jalr
      rd = fet >> 7 & 0x1f;
      rs1 = fet >> 15 & 0x1f;
      imm = fet >> 20 & 0x7ff; if(fet >> 31) imm |= 0xfffff800;
      typ = JALR;
      break;
    }
    case 0x3: { // I
      rd = fet >> 7 & 0x1f;
      rs1 = fet >> 15 & 0x1f;
      imm = fet >> 20 & 0x7ff; if(fet >> 31) imm |= 0xfffff800;
      switch(fet >> 12 & 7) {
        case 0: typ = LB; break;
        case 1: typ = LH; break;
        case 2: typ = LW; break;
        case 4: typ = LBU; break;
        case 5: typ = LHU; break;
        default: assert(0);
      }
      break;
    }
    case 0x13: { // I
      rd = fet >> 7 & 0x1f;
      rs1 = fet >> 15 & 0x1f;
      imm = fet >> 20 & 0x7ff; if(fet >> 31) imm |= 0xfffff800;
      switch(fet >> 12 & 7) {
        case 0: typ = ADDI; break;
        case 2: typ = SLTI; break;
        case 3: typ = SLTIU; break;
        case 4: typ = XORI; break;
        case 6: typ = ORI; break;
        case 7: typ = ANDI; break;
        case 1: typ = SLLI; break;
        case 5: {
          typ = (fet >> 30 & 1) ? SRAI : SRLI; // !
          break;
        }
        default: assert(0);
      }
      break;
    }
    case 0x33: { // R
      rd = fet >> 7 & 0x1f;
      rs1 = fet >> 15 & 0x1f;
      rs2 = fet >> 20 & 0x1f;
      int syb = fet >> 30 & 1;
      switch(fet >> 12 & 7) {
        case 0: typ = !syb ? ADD : SUB; break;
        case 1: typ = SLL; break;
        case 2: typ = SLT; break;
        case 3: typ = SLTU; break;
        case 4: typ = XOR; break;
        case 5: typ = !syb ? SRL : SRA; break;
        case 6: typ = OR; break;
        case 7: typ = AND; break;
      }
      break;
    }
    default: {
      print(fet);
      assert(0); 
    }
  }
  return Instruction(typ, imm, rs1, rs2, rd);
}

Instype typ;
uint imm; int rd, rs1, rs2, shamt;

void ID() {
  cur = decode(fet);
  typ = cur.typ;
  imm = cur.imm;
  rd = cur.rd, rs1 = cur.rs1, rs2 = cur.rs2;
  if(typ == SLLI || typ == SRLI || typ == SRAI) {
    shamt = imm;
    if(typ == SRAI)
      shamt ^= 1 << 10;
  }
}
void EX() {
  
  int p0 = pc - 4;
  // printf("pc = %4d, %02x%02x%02x%02x\t", pc - 4, mem[pc - 1], mem[pc - 2], mem[pc - 3], mem[pc - 4]);
  print(typ);
  // cout << "rs1= " << rs1 << " rs2=" << rs2 << " rd=" << rd << ' ' << imm << endl;
  if(fet == 0x0ff00513) {
    printf("%u\n", rg[10] & 255u);
    exit(0);
  }
  switch(typ) {
    case LUI: rg[rd] = imm; break;
    case AUIPC: rg[rd] = p0 + imm; break;
    case JAL: rg[rd] = p0 + 4, pc = p0 + imm; break;
    // ---- B ----
    case BEQ: if(rg[rs1] == rg[rs2]) pc = p0 + imm; break; 
    case BNE: if(rg[rs1] != rg[rs2]) pc = p0 + imm; break;
    case BLT: if((int) rg[rs1] < (int) rg[rs2]) pc = p0 + imm; break;
    case BGE: if((int) rg[rs1] >= (int) rg[rs2]) pc = p0 + imm; break;
    case BLTU: {
      cout << rg[rs1] << ' ' << rg[rs2] << endl;
      if(rg[rs1] < rg[rs2]) pc = p0 + imm; break;
    }
    case BGEU: if(rg[rs1] >= rg[rs2]) pc = p0 + imm; break;
    // ---- S ----
            // imm要转化成int类型！
    case SB: mem[int(rg[rs1] + imm)] = (unsigned char) rg[rs2]; break;  // 取低位[7:0] (1byte) 
    case SH: *(unsigned short *)(mem + int(rg[rs1] + imm)) = (unsigned short) rg[rs2]; break; // [15:0]
    case SW: *(unsigned int *)(mem + int(rg[rs1] + imm)) = rg[rs2]; break;
    // ---- I ----
    case JALR: pc = (rg[rs1] + imm) & ~1, rg[rd] = p0 + 4; break;
    case LB: { // 符号位拓展[7:0]
      uint x = (uint) mem[rg[rs1] + imm]; 
      if(x >> 7 & 1) x |= 0xffffff00;
      rg[rd] = x;
      break;
    }
    case LH: { // 符号位拓展[15:0]
      uint x = (uint) mem[rg[rs1] + imm] | ((uint) mem[rg[rs1] + imm + 1] << 8);
      if(x >> 15 & 1) x |= 0xffff0000;
      rg[rd] = x;
      break;
    }
    case LW: { // 符号位拓展[31:0]
      int idx = rg[rs1] + imm;
      rg[rd] = (uint) mem[idx] | ((uint) mem[idx + 1] << 8) | ((uint) mem[idx + 2] << 16) | ((uint) mem[idx + 3] << 24);
      break;
    }
    case LBU: rg[rd] = (uint) mem[rg[rs1] + imm]; break;
    case LHU: rg[rd] = (uint) mem[rg[rs1] + imm] | ((uint) mem[rg[rs1] + imm + 1] << 8); break;
    case ADDI: rg[rd] = rg[rs1] + imm; break;
    case SLTI: rg[rd] = (int) rg[rs1] < (int) imm; break;
    case SLTIU: rg[rd] = rg[rs1] < imm; break;
    case XORI: rg[rd] = rg[rs1] ^ imm; break;
    case ORI: rg[rd] = rg[rs1] | imm; break;
    case ANDI: rg[rd] = rg[rs1] & imm; break;
    case SLLI: rg[rd] = rg[rs1] << shamt; break; 
    case SRLI: rg[rd] = rg[rs1] >> shamt; break;
    case SRAI: rg[rd] = (int) rg[rs1] >> (int) shamt; break;
    // ---- R ----
    case ADD: rg[rd] = rg[rs1] + rg[rs2]; break;
    case SUB: rg[rd] = rg[rs1] - rg[rs2]; break;
    case SLL: rg[rd] = rg[rs1] << rg[rs2]; break;
    case SLT: rg[rd] = (int) rg[rs1] < (int) rg[rs2]; break;
    case SLTU: rg[rd] = rg[rs1] < rg[rs2]; break;
    case XOR: rg[rd] = rg[rs1] ^ rg[rs2]; break;
    case SRL: rg[rd] = rg[rs1] >> rg[rs2]; break;
    case SRA: rg[rd] = (int) rg[rs1] >> (int) rg[rs2]; break;
    case OR: rg[rd] = rg[rs1] | rg[rs2]; break;
    case AND: rg[rd] = rg[rs1] & rg[rs2]; break;
    default: assert(0);
  }
}
inline void SET() {
  if(rg[0]) rg[0] = 0;
}

int main() {
  input();
  int clk = 0;
  while(true) {
    ++clk;
    IF();
    ID();
    EX();
    SET();
    // for(int i = 1; i < 20; ++i)
    //   cout << rg[i] << ' '; puts("");
  }
  return 0;
}