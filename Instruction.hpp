# ifndef INS
# define INS

#include <assert.h>
typedef unsigned int uint;

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
  uint imm;
  int rs1, rs2, rd, shamt;
  Instruction() { typ = ERROR, rs1 = rs2 = rd = shamt = 0, imm = 0u; }
  Instruction(Instype typ, uint imm, int rs1, int rs2, int rd, int shamt):
    typ(typ), imm(imm), rs1(rs1), rs2(rs2), rd(rd), shamt(shamt) {}

  void decode(uint fet) {
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
      default: assert(0); 
    }
  }
  inline void print() {
    switch(typ) {
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
			default: puts("ERROR");
  	}
	}
};


#endif