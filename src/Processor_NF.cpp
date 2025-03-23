#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <bitset>
#include <cstdlib>
#include <cstdint>

using namespace std;

const int N = 2000005;
int MEM[N];

typedef struct {
    int value;
} Register;

Register RegFile[32];

// Convert hex string to binary string
string hexToBin(const string &hex) {
    string binary;
    for (char ch : hex) {
        int val = stoi(string(1, ch), nullptr, 16);
        binary += bitset<4>(val).to_string();
    }
    return binary;
}

// Split a line by whitespace.
vector<string> splitLine(const string &line) {
    vector<string> words;
    stringstream ss(line);
    string word;
    while (ss >> word) {
        words.push_back(word);
    }
    return words;
}

// Pipeline stage structures

struct IFStage {
    int PC;
    bool stall;
    int InStr; // holds the index of the instruction fetched; -1 means bubble.
};

struct IDStage {
    int RR1;        // source register 1 (rs1)
    int RR2;        // source register 2 (rs2)
    int WR;         // destination register (rd)
    int RD1;        // read data from RegFile for RR1
    int RD2;        // read data from RegFile for RR2
    int Imm;        // sign-extended immediate
    
    bool RegWrite;  // write to register file
    bool RegDst;    // determines destination register (used for R-type vs I-type)
    bool Branch;    // branch instruction
    bool Jump;      // jump instruction
    bool MemRead;   // read from memory
    bool MemWrite;  // write to memory
    bool ALUSrc;    // selects second ALU input (reg or imm)
    int ALUOp;      // operation code for the ALU
    bool MemtoReg;  // selects data to write to register (ALU result or memory)
    
    int MemSize;            // size of memory access (1, 2, or 4 bytes)
    bool MemSignExtend;     // whether to sign-extend memory reads
    int BranchType;         // type of branch (BEQ, BNE, BLT, etc.)
    bool JumpAndLink;       // JAL or JALR instruction
    bool JumpReg;           // JALR instruction (jump to register)
    
    bool stall;   
    int InStr;    
};

struct EXStage {
    int ALU_res;
    bool Zero;
    
    int WriteData;    
    int WriteDataReg; 
    
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

struct MEMStage {
    int Address;
    int Write_data;
    int Read_data;   
    bool MemRead;
    bool MemWrite;
    bool MemtoReg;
    bool RegWrite;
    int ALU_res;
    int WriteReg;
    
    int InStr;

    bool stall;
};

struct WBStage {
    bool MemtoReg;
    bool RegWrite;
    int Read_data;   
    int ALU_res;    
    int WriteReg;
    
    int InStr;
    bool stall;
};

void process_IF(IFStage &IF, const vector<string> &instructions);
void process_ID(IFStage &IF, IDStage &ID, const vector<string> &instructions);
void process_EX(IDStage &ID, EXStage &EX, const MEMStage &DM, const WBStage &WB);
void process_MEM(EXStage &EX, MEMStage &DM);
void process_WB(MEMStage &DM, WBStage &WB);
