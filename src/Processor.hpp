#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

typedef struct
{
    int value;
} Register;

extern Register RegFile[32];

struct IFStage
{
    int PC;
    bool stall;
    int InStr;
    int branch;
    int branchPC;
};

struct IDStage
{
    int RR1; // Source register 1 (rs1)
    int RR2; // Source register 2 (rs2)
    int WR;  // Destination register (rd)
    int RD1; // Read data from RegFile for RR1
    int RD2; // Read data from RegFile for RR2
    int Imm; // Sign-extended immediate

    // Control signals
    bool RegWrite; // Write to register file
    bool RegDst;   // Determines destination register (used for R-type vs I-type)
    bool Branch;   // Branch instruction
    bool Jump;     // Jump instruction
    bool MemRead;  // Read from memory
    bool MemWrite; // Write to memory
    bool ALUSrc;   // Selects second ALU input (reg or imm)
    int ALUOp;     // Operation code for the ALU
    bool MemtoReg; // Selects data to write to register (ALU result or memory)

    // Additional control fields
    int MemSize;        // Size of memory access (1, 2, or 4 bytes)
    bool MemSignExtend; // Whether to sign-extend memory reads
    int BranchType;     // Type of branch (BEQ, BNE, BLT, etc.)
    bool JumpAndLink;   // JAL or JALR instruction
    bool JumpReg;       // JALR instruction (jump to register)

    bool stall; // When true, hold the instruction (do not update)
    int InStr;  // Index of instruction; -1 indicates bubble

    int DM_stall_prev; // Counter for memory stall propagation
};

// Execute stage
struct EXStage
{
    int ALU_res;
    bool Zero;

    int WriteData;    // Value from register used for store (RD2)
    int WriteDataReg; // Register number for forwarding comparison

    int WriteReg;

    bool Branch;
    bool Jump;
    bool MemRead;
    bool MemWrite;
    bool MemtoReg;
    bool RegWrite;

    int InStr;

    bool stall;
};

// Memory stage
struct MEMStage
{
    int Address;
    int Write_data;
    int Read_data; // For loads
    bool MemRead;
    bool MemWrite;
    bool MemtoReg;
    bool RegWrite;
    int ALU_res;
    int WriteReg;

    int InStr;

    bool stall;
};

struct WBStage
{
    bool MemtoReg;
    bool RegWrite;
    int Read_data;
    int ALU_res;
    int WriteReg;

    int InStr;
    bool stall;
};

extern IFStage IF;
extern IDStage ID;
extern EXStage EX;
extern MEMStage DM;
extern WBStage WB;

#endif