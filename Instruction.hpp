# ifndef INS
# define INS

#include <iostream>
#include <assert.h>

typedef unsigned int uint;

// # define Debug

enum Instype {
  LUI, AUIPC, // U [0, 1]
  JAL, // J [2, 2]
  BEQ, BNE, BLT, BGE, BLTU, BGEU, // B [3, 8]
  SB, SH, SW, // S [9, 11]
  JALR, LB, LH, LW, LBU, LHU, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, // I [12, 26]
  ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND, // R [27, 26]
  ERROR
};

struct Instruction {
  Instype typ;
  uint imm, fetch;
	int pc, npc, predict_pc;
  int rs1, rs2, rd, shamt;
  
	Instruction() { 
    typ = ERROR;
    rs1 = rs2 = rd = shamt = 0;
    imm = 0u; 
  }
  Instruction(Instype typ, uint imm, int rs1, int rs2, int rd, int shamt):
    typ(typ), imm(imm), rs1(rs1), rs2(rs2), rd(rd), shamt(shamt) {}
  
  void clear() {
    typ = ERROR;
    rs1 = rs2 = rd = shamt = 0;
    imm = 0u; 
  }
  void decode(uint fet) {
    fetch = fet;
		typ = ERROR;
		rs1 = rs2 = rd = shamt = -1;
		imm = 0u;
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
      default: ERROR; 
    }

    if(typ == SLLI || typ == SRLI || typ == SRAI) {
      shamt = imm;
      if(typ == SRAI)
        shamt ^= 1 << 10;
    }
  }
  void doit(uint &reg_rd, uint reg1, uint reg2) {
    npc = -1;
    switch(typ) {
      // ---- U ----
      case LUI: reg_rd = imm; break;
      case AUIPC: reg_rd = pc + imm; break;
      case JAL: reg_rd = pc + 4, npc = pc + imm; break;
      // ---- B ----
      case BEQ: npc = (reg1 == reg2) ? pc + imm : pc + 4; break; 
      case BNE: npc = (reg1 != reg2) ? pc + imm : pc + 4; break;
      case BLT: npc = ((int) reg1 < (int) reg2) ? pc + imm : pc + 4; break;
      case BGE: npc = ((int) reg1 >= (int) reg2) ? pc + imm : pc + 4; break;
      case BLTU: npc = (reg1 < reg2) ? pc + imm : pc + 4; break;
      case BGEU: npc = (reg1 >= reg2) ? pc + imm : pc + 4; break;
      // ---- S ----
      // store
      case SB: 
      case SH: 
      case SW: reg_rd = reg1 + imm; break;
      // ---- I ----
      case JALR: npc = (reg1 + imm) & ~1, reg_rd = pc + 4; break;

      // load
      case LB: 
      case LH: 
      case LW: 
      case LBU:
      case LHU: reg_rd = reg1 + imm; break;

      case ADDI: reg_rd = reg1 + imm; break;
      case SLTI: reg_rd = (int) reg1 < (int) imm; break;
      case SLTIU: reg_rd = reg1 < imm; break;
      case XORI: reg_rd = reg1 ^ imm; break;
      case ORI: reg_rd = reg1 | imm; break;
      case ANDI: reg_rd = reg1 & imm; break;
      case SLLI: {
        // if(rd == 13)
        //   std :: cout << reg1 << ' ' << shamt << std :: endl, exit(0);
        reg_rd = reg1 << shamt; break; 
      }
      case SRLI: reg_rd = reg1 >> shamt; break;
      case SRAI: reg_rd = (int) reg1 >> (int) shamt; break;
      // ---- R ----
      case ADD: reg_rd = reg1 + reg2; break;
      case SUB: reg_rd = reg1 - reg2; break;
      case SLL: reg_rd = reg1 << reg2; break;
      case SLT: reg_rd = (int) reg1 < (int) reg2; break;
      case SLTU: reg_rd = reg1 < reg2; break;
      case XOR: reg_rd = reg1 ^ reg2; break;
      case SRL: reg_rd = reg1 >> reg2; break;
      case SRA: reg_rd = (int) reg1 >> (int) reg2; break;
      case OR: reg_rd = reg1 | reg2; break;
      case AND: reg_rd = reg1 & reg2; break;
      default: {
        print();
        std :: cout << rs1 << ' ' << rs2 << ' ' << rd << ' ' << std :: endl ;
        assert(0);
      }
    }
  }

  inline void print() {
    switch(typ) {
			case 0: std :: cout << "LUI" ; break;
			case 1: std :: cout << "AUIPC" ; break;
			case 2: std :: cout << "JAL" ; break;
			case 3: std :: cout << "BEQ" ; break;
			case 4: std :: cout << "BNE" ; break;
			case 5: std :: cout << "BLT" ; break;
			case 6: std :: cout << "BGE" ; break;
			case 7: std :: cout << "BLTU" ; break;
			case 8: std :: cout << "BGEU" ; break;
			case 9: std :: cout << "SB" ; break;
			case 10: std :: cout << "SH" ; break;
			case 11: std :: cout << "SW" ; break;
			case 12: std :: cout << "JALR" ; break;
			case 13: std :: cout << "LB" ; break;
			case 14: std :: cout << "LH" ; break;
			case 15: std :: cout << "LW" ; break;
			case 16: std :: cout << "LBU" ; break;
			case 17: std :: cout << "LHU" ; break;
			case 18: std :: cout << "ADDI" ; break;
			case 19: std :: cout << "SLTI" ; break;
			case 20: std :: cout << "SLTIU" ; break;
			case 21: std :: cout << "XORI" ; break;
			case 22: std :: cout << "ORI" ; break;
			case 23: std :: cout << "ANDI" ; break;
			case 24: std :: cout << "SLLI" ; break;
			case 25: std :: cout << "SRLI" ; break;
			case 26: std :: cout << "SRAI" ; break;
			case 27: std :: cout << "ADD" ; break;
			case 28: std :: cout << "SUB" ; break;
			case 29: std :: cout << "SLL" ; break;
			case 30: std :: cout << "SLT" ; break;
			case 31: std :: cout << "SLTU" ; break;
			case 32: std :: cout << "XOR" ; break;
			case 33: std :: cout << "SRL" ; break;
			case 34: std :: cout << "SRA" ; break;
			case 35: std :: cout << "OR" ; break;
			case 36: std :: cout << "AND" ; break;
			default: std :: cout << "ERROR" ;
  	}
	}
  inline void println() {
    print(); puts("");
  }
};

#endif