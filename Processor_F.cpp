#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

const int N = 2e6 + 5;

vector<string> splitLine(const string& line) {
    vector<string> words;
    stringstream ss(line); 
    string word;

    while (ss >> word) { 
        words.push_back(word);
    }

    return words;
}

typedef struct {
    int value;
} Register;


typedef struct {
    int PC;
    bool PCSrc;

    int InStr = -1;
} IF;

typedef struct {
    // Register read addresses
    int RR1;        // e.g. rs
    int RR2;        // e.g. rt
    
    // Register write address (which might be rd or rt, chosen by RegDst)
    int WR;
    
    // Register file read data
    int RD1;        
    int RD2;
    
    // Sign-extended immediate
    int Imm;
    
    // Control signals
    bool RegWrite;  
    bool RegDst;    
    bool Branch;    
    bool Jump;      
    bool MemRead;   
    bool MemWrite;  
    bool ALUSrc;    
    int  ALUOp;     // 2 bits in classic MIPS (0=add, 1=sub, 2=R-type, etc.)
    bool MemtoReg;  

    int InStr = -1;

} ID;

typedef struct {
    // ALU outputs
    int ALU_res;
    bool Zero;        // ALU result == 0?

    // Forward the register data that might be used in MEM
    int WriteData;    // e.g. RD2 if we need to store in memory
    
    // Which register will be written in WB (after RegDst mux)
    int WriteReg;
    
    // Carry over control signals to MEM
    bool Branch;
    bool Jump;
    bool MemRead;
    bool MemWrite;
    bool MemtoReg;
    bool RegWrite;

    int InStr = -1;
} ALU;

typedef struct {
    // Address to read/write in memory
    int Address;

    // Data to write (for store)
    int Write_data;
    
    // Data read from memory (for load)
    int Read_data;
    
    // Control signals used in MEM
    bool MemRead;
    
    // Control signals forwarded to WB
    bool MemtoReg;
    bool RegWrite;
    
    // Forward the ALU result
    int ALU_res;
    
    // The register number to be written in WB
    int WriteReg;

    int InStr = -1;
} DM;

typedef struct {
    // Control signals
    bool MemtoReg;
    bool RegWrite;
    
    // Data from MEM
    int Read_data;  // load result
    // Data from EX
    int ALU_res;
    
    // The final register to write back
    int WriteReg;

    int InStr = -1;
} WB;

int main() {
    Register RegFile[32];
    int MEM[N];
    for (int i = 0; i < 32; i++) {
        RegFile[i].value = 0;  
    }

    ifstream file("input.txt");
    if (!file) {
        cerr << "Error: Unable to open input.txt" << endl;
        return 1;
    }

    vector<int> instructions;

    int instr;
    while (file >> instr) {
        instructions.push_back(instr);
    }
    file.close();

    IF IF = {0, false, -1};
    ID ID = {0, 0, 0, 0, 0, false, -1};
    ALU ALU = {0, false, false, false, -1};
    DM DM = {0, 0, 0, false, false, -1};
    WB WB = {false, -1};

    vector<int> Output[5];

    int num = instructions.size();

    for (int i = 0; i < num; i++) {

        if (WB.InStr != -1) {
            Output[4].push_back(WB.InStr);

            if (WB.RegWrite == true) {
                if (WB.MemtoReg == true) {
                    RegFile[WB.WriteReg].value = WB.Read_data;
                }
                else {
                    RegFile[WB.WriteReg].value = WB.ALU_res;
                }
            }

        }
        if (DM.InStr != -1) {
            Output[3].push_back(DM.InStr);

            if (DM.MemRead == true) {
                DM.Read_data = MEM[DM.Address];

            }
            else {
                MEM[DM.Address] = DM.Write_data;
            }

            WB.Read_data = DM.Read_data;
            WB.MemtoReg = DM.MemtoReg;
            WB.RegWrite = DM.RegWrite;
            WB.ALU_res = DM.ALU_res;
            WB.InStr = DM.InStr;
        }
        if (ALU.InStr != -1) {
            Output[2].push_back(ALU.InStr);
        }
        if (ID.InStr != -1) {
            Output[1].push_back(ID.InStr);
        }
        if (IF.InStr != -1) {
            Output[0].push_back(IF.InStr);
        }


        




    }





}