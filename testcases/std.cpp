#include <iostream>
//
// Created by 86137 on 2021/6/29.
//

#ifndef RISC_V_ALL_HEADER_HPP
#define RISC_V_ALL_HEADER_HPP

//#define debug
//#define debug_single_run

#include <iostream>

using std::cin ;
using std::cout ;
using std::endl ;
using std::string ;

const int MEMORY_SIZE = 500000 ;

enum codeType{ R , I , S , B , U , J };

struct Memory{
    unsigned char all_data[MEMORY_SIZE] = {0} ;
    void write_word( int pos , unsigned int word ){ // 4 - byte
        *( unsigned int * )( all_data + pos ) = word ;
    }
    void write_ushort( int pos , unsigned short word ){ // 2 - byte
        *( unsigned short *)( all_data + pos ) = word ;
    }
    void write_short( int pos , unsigned char word ){ // 1 - byte
        all_data[pos] = word ;
    }
    unsigned int read_word( int pos ){
        return *( unsigned int * ) ( all_data + pos ) ;
    }
    unsigned short read_ushort( int pos ){
        return *( unsigned short *)( all_data + pos ) ;
    }
    unsigned char read_short( int pos ){
        return all_data[pos] ;
    }
};

class description
{
public:

    unsigned int opcode = 0 , rs1 = 0 , rs2 = 0 , rd = 0 , func3 = 0 , func7 = 0 ;
    unsigned int origin_code[4] = {0} , real_code = 0 ;
    int immediate = 0 , shamt = 0 ;
    codeType code_type ;

    int reg[32] = {0} ;
    Memory my_memory ;
    int pc = 0 ;

public:

    static int get_bits( int num , int low , int high ){
        int small_int = ( 1 << ( high - low + 1 ) ) - 1 ;
        return ( num >> low ) & small_int ;
    }

    // todo 检查 immediate 正确性

    // todo extend 符号位肯定有问题

    // todo pc 增长的地方全部 -4

    void maintain_reg0(){ reg[0] = 0 ; };

    void LoadMemory(){
        int memory_pointer = 0 ;
        string temp_input ;
        while( cin >> temp_input ){
            if ( temp_input[0] == '@' ){
                memory_pointer = strtol(temp_input.c_str()+1,NULL,16) ;
                continue;
            }
            my_memory.all_data[memory_pointer] = strtol(temp_input.c_str(),NULL,16) ;
            memory_pointer++ ;
        }
    }

    int char_extend_sign( unsigned char word ){
        int sign_temp = get_bits(word,7,7) ;
        if ( sign_temp == 1 ){
            sign_temp = ( sign_temp << 24 ) - 1 ;
            sign_temp = sign_temp << 8 ;
        }
        sign_temp += word ;
        return sign_temp ;
    }

    int short_extend_sign( unsigned char word ){
        int sign_temp = get_bits(word,15,15) ;
        if ( sign_temp == 1 ){
            sign_temp = ( sign_temp << 16 ) - 1 ;
            sign_temp = sign_temp << 16 ;
        }
        sign_temp += word ;
        return sign_temp ;
    }

    int I_get_immediate(){
        int temp = get_bits(real_code,31,31) ;
        if ( temp == 1 ) {
            temp = ( temp << 21 ) - 1 ;
            temp = temp << 11 ;
        }
        temp += get_bits(real_code,20,30) ;
        return temp ;
    }

    int S_get_immediate(){
        int temp = get_bits(real_code,31,31) ;
        if ( temp == 1 ) {
            temp = ( temp << 21 ) - 1 ;
            temp = temp << 6 ;
        }
        temp += get_bits(real_code,25,30) ;
        temp = temp << 5 ;
        temp += get_bits(real_code,7,11) ;
        return temp ;
    }

    int B_get_immediate(){
        int temp = get_bits(real_code,31,31) ;
        if ( temp == 1 ) {
            temp = ( temp << 20 ) - 1 ;
            temp = temp << 1 ;
        }
        temp += get_bits(real_code,7,7) ;
        temp = temp << 6 ;
        temp += get_bits(real_code,25,30) ;
        temp = temp << 4 ;
        temp += get_bits(real_code,8,11) ;
        temp = temp << 1 ;
        return temp ;
    }

    int U_get_immediate(){
        int temp = get_bits(real_code,12,31) ;
        temp = temp << 12 ;
        return temp ;
    }

    int J_get_immediate(){
        int temp = get_bits(real_code,31,31) ;
        if ( temp == 1 ) {
            temp = ( temp << 12 ) - 1 ;
            temp = temp << 8 ;
        }
        temp += get_bits(real_code,12,19) ;
        temp = temp << 1 ;
        temp += get_bits(real_code,20,20) ;
        temp = temp << 10 ;
        temp += get_bits(real_code,21,30) ;
        temp = temp << 1 ;
        return temp ;
    }

    int tag = 0;
    void fetch_code(){
        real_code = 0 ;
        for ( int i = 0 ; i < 4 ; i++ ){
            origin_code[i] = my_memory.all_data[pc+3-i] ;
        }
        if(pc == 4200) {
            cout << "@";
            tag = 1;
        } else tag = 0;

        pc += 4 ;
        for ( int i = 0 ; i < 4 ; i++ ){
            real_code = real_code << 8 ;
            real_code += origin_code[i] ;
        }
    }

    void decode_code(){
        opcode = get_bits(real_code,0,6) ;
        switch(opcode){
            case 0b0110111 :
            case 0b0010111 : code_type = U ; rd = get_bits( real_code , 7 , 11 ) ; immediate = U_get_immediate() ; break ;
            case 0b1101111 : code_type = J ; rd = get_bits( real_code , 7, 11 ) ; immediate = J_get_immediate() ; break ;
            case 0b1100111 :
            case 0b0000011 :
            case 0b0010011 : code_type = I ; rd = get_bits( real_code , 7 , 11 ) ; rs1 = get_bits(real_code,15,19) ; func3 = get_bits(real_code,12,14) ; immediate = I_get_immediate() ; shamt = get_bits(real_code,20,24) ; func7 = get_bits(real_code,25,31) ; break ; // todo 此处 func7 并非真实func7,而是用来区分 srli srai
            case 0b1100011 : code_type = B ; rs1 = get_bits( real_code , 15 , 19 ) ; rs2 = get_bits( real_code , 20 , 24 ) ; func3 = get_bits(real_code,12,14) ; immediate = B_get_immediate() ; break ;
            case 0b0100011 : code_type = S ; rs1 = get_bits( real_code , 15 , 19 ) ; rs2 = get_bits( real_code , 20 , 24 ) ; func3 = get_bits(real_code,12,14) ; immediate = S_get_immediate() ; break ;
            case 0b0110011 : code_type = R ; rd = get_bits(real_code,7,11) ; rs1 = get_bits( real_code , 15 , 19 ) ; rs2 = get_bits( real_code , 20 , 24 ) ; func3 = get_bits(real_code,12,14) ; func7 = get_bits(real_code,25,31) ; break ;
        }
        if(tag) cout << code_type << endl;
    }

    void execute_code(){
        if ( real_code == 0x0ff00513 ){
            cout << (unsigned int) ( reg[rd] & 255u ) << endl ;
            exit(0) ;
        }
#ifdef debug_single_run
        if ( pc == 5732 )
            std::cerr << "check" << endl ;
#endif
        int temp = 0 ; // todo 特判 reg[0] = 0
        switch (opcode) {
            case 0b0110111 : reg[rd] = immediate ; return ; // lui // todo 立即数不能为负数未实现
            case 0b0010111 : reg[rd] = pc - 4 + immediate ; return ; // auipc
            case 0b1101111 : reg[rd] = pc  - 4 + 4 ; pc += immediate - 4 ; return ; // jal
            case 0b1100111 : {
                temp = pc - 4 + 4 ; 
                pc = ( reg[rs1] + immediate ) & ~1 ; 
                //cout << rs1 << ' ' << reg[rs1] << ' ' << immediate << endl;
                reg[rd] = temp ; return ; // jalr
            }
            case 0b1100011 :
                switch (func3) {
                    case 0b000: if ( reg[rs1] == reg[rs2] ) pc += immediate - 4 ; return ; // beq
                    case 0b001: if ( reg[rs1] != reg[rs2] ) pc += immediate - 4 ; return ; // bne
                    case 0b100: if ( reg[rs1] < reg[rs2] ) pc += immediate - 4 ; return ; // blt
                    case 0b101: if ( reg[rs1] >= reg[rs2] ) pc += immediate - 4 ; return ; // bge
                    case 0b110: if ( reg[rs1] < unsigned ( reg[rs2] ) ) pc += immediate - 4 ; return ; // bltu
                    case 0b111: if ( reg[rs1] >= unsigned ( reg[rs2] )) pc += immediate - 4 ; return ; // bgeu
                }
            case 0b0000011 :
                switch (func3) { // todo 这里无符号没有体现? / 按第一位扩展
                    case 0b000: reg[rd] = char_extend_sign(my_memory.read_short(reg[rs1]+immediate)) ; return ; // lb
                    case 0b001: reg[rd] = short_extend_sign(my_memory.read_ushort(reg[rs1]+immediate)) ; return ; // lh
                    case 0b010: {
                        //if(rd == 1) cout << "ohhhhh " << rs1 << ' ' << immediate << endl;
                        reg[rd] = my_memory.read_word(reg[rs1]+immediate) ;  // lw
                        // if(rd == 1) cout << "onext "<< reg[rd] << endl;
                        return ;
                    }
                    case 0b100: reg[rd] = my_memory.read_short(reg[rs1]+immediate) ; return ; // lbu
                    case 0b101: reg[rd] = my_memory.read_ushort(reg[rs1]+immediate) ; return ; // lhu
                }
            case 0b0100011 :
                switch (func3) {
                    case 0b000: my_memory.write_short(reg[rs1]+immediate,reg[rs2]) ; return ; // sb
                    case 0b001: my_memory.write_ushort(reg[rs1]+immediate,reg[rs2]) ; return ; // sh
                    case 0b010: my_memory.write_word(reg[rs1]+immediate,reg[rs2]) ;  return ; // sw
                }
            case 0b0010011 :
                switch (func3) {
                    case 0b000: reg[rd] = reg[rs1] + immediate ; return ; // addi
                    case 0b010: reg[rd] = reg[rs1] < immediate ; return ; // slti
                    case 0b011: reg[rd] = ( reg[rs1] < (unsigned ) immediate ) ; return ; // sltiu
                    case 0b100: reg[rd] = reg[rs1] xor immediate ; return ; // xori
                    case 0b110: reg[rd] = reg[rs1] | immediate ; return ; // ori
                    case 0b111: reg[rd] = reg[rs1] & immediate ; return ; // andi
                    case 0b001: reg[rd] = reg[rs1] << shamt ; return ; // slli
                    case 0b101:
                        if ( func7 == 0 ){ // srli
                            reg[rd] = ( (unsigned int) reg[rs1] >> shamt ) ;
                            return ;
                        }else{ // srai
                            reg[rd] = ( reg[rs1] >> shamt ) ;
                            return ;
                        }
                }
            case 0b0110011 :
                switch (func3) {
                    case 0b000:
                        if ( func7 == 0 ){ // add
                            reg[rd] = reg[rs1] + reg[rs2] ;
                            return ;
                        }else{ // sub
                            reg[rd] = reg[rs1] - reg[rs2] ;
                            return ;
                        }
                    case 0b001: reg[rd] = reg[rs1] << reg[rs2] ; return ; // sll
                    case 0b010: reg[rd] = reg[rs1] < reg[rs2] ; return ; // slt
                    case 0b011: reg[rd] = reg[rs1] < (unsigned) reg[rs2] ; return ; // sltu
                    case 0b100: reg[rd] = reg[rs1] xor reg[rs2] ; return ; // xor
                    case 0b101:
                        if ( func7 == 0 ){ // srl
                            reg[rd] = reg[rs1] >> ((unsigned) reg[rs2]) ; // todo << >> 都需要看一下
                            return ;
                        }else{ // sra
                            reg[rd] = reg[rs1] >> reg[rs2] ;
                            return ;
                        }
                    case 0b110: reg[rd] = reg[rs1] | reg[rs2] ; return ; // or
                    case 0b111: reg[rd] = reg[rs1] & reg[rs2] ; return ; // and
                }
        }

    }

    void debug_show(){
        cout << "<------------------------------->" << endl ;
        cout << "pc: " << pc << endl ;
        for ( int i = 0 ; i < 32 ; i++ ){
            cout << "register " << i << " : " << (unsigned int) reg[i] << endl ;
        }
        cout << "<------------------------------->" << endl ;
    }

};

#endif //RISC_V_ALL_HEADER_HPP

description riscv ;

int main()
{
//    freopen("my_output.txt","w",stdout) ;
    riscv.LoadMemory() ;
    while ( true ){
        riscv.fetch_code() ;
        riscv.decode_code() ;
        riscv.execute_code() ;
        cout << riscv.pc << endl;
        //if(riscv.reg[0]) cout << '@' << riscv.reg[0] << endl;
        riscv.maintain_reg0() ;
        
#ifdef debug
        riscv.debug_show() ;
#endif
    }
}