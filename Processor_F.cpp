#include <iostream>

typedef struct {
    int value;
} Register;

typedef struct {
    int PC;
    bool PCSrc;
} IF;

typedef struct {
    int RR1;
    int RR2;
    int WR;
    int RD1;
    int RD2;
    bool RegWrite;

} ID;

typedef struct {
    int ALU_res;
    bool ALUSrc;
    bool ALUOp;
    bool Zero;
} ALU;

typedef struct {
    int Address;
    int Write_data;
    int Read_data;
    bool MemWrite;
    bool MemRead;
} DM;

typedef struct {
    bool MemtoReg;
} WB;
