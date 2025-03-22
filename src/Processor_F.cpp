#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <bitset>
#include <cstdlib>

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
    int RR1; // source register 1 (e.g., rs)
    int RR2; // source register 2 (e.g., rt)
    int WR;  // destination register (could be rd or rt)
    int RD1; // read data from RegFile for RR1
    int RD2; // read data from RegFile for RR2
    int Imm; // sign-extended immediate
    // Control signals (assumed to be set by your RISCV decode)
    bool RegWrite;
    bool RegDst;
    bool Branch;
    bool Jump;
    bool MemRead;
    bool MemWrite;
    bool ALUSrc;
    int ALUOp;      // Operation code for the ALU (e.g., 2 for add, 3 for sub, etc.)
    bool MemtoReg;
    
    bool stall; // when true, hold the instruction (do not update)
    int InStr;  // index of instruction; -1 indicates bubble.
};

struct EXStage {
    int ALU_res;
    bool Zero;
    
    // Forwarding for store instructions:
    int WriteData;    // value from register used for store (RD2)
    int WriteDataReg; // register number for forwarding comparison
    
    int WriteReg; // destination register computed by RegDst mux
    
    // Control signals passed down
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
    int Read_data;   // for loads
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
    int Read_data;   // load data from memory
    int ALU_res;     // result from ALU
    int WriteReg;
    
    int InStr;
    bool stall;
};

// Function prototypes – note that the ID stage is assumed to fill in all needed fields.
void process_IF(IFStage &IF, const vector<string> &instructions);
void process_ID(IFStage &IF, IDStage &ID, const vector<string> &instructions);
void process_EX(IDStage &ID, EXStage &EX, const EXStage &prevEX, const MEMStage &DM, const WBStage &WB);
void process_MEM(EXStage &EX, MEMStage &DM);
void process_WB(MEMStage &DM, WBStage &WB);

int main(int argc, char **argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input.txt> <num_cycles>" << endl;
        return 1;
    }

    // Initialize register file to 0.
    for (int i = 0; i < 32; i++) {
        RegFile[i].value = 0;
    }

    // Open the input file.
    ifstream file(argv[1]);
    if (!file) {
        cerr << "Error: Unable to open input file " << argv[1] << endl;
        return 1;
    }

    // Read instructions (assumed format: <line_number> <hex> <printable>)
    vector<string> instructions_hex;
    vector<string> instructions_print;
    string line;
    while (getline(file, line)) {
        vector<string> words = splitLine(line);
        if (words.size() >= 3) {
            instructions_hex.push_back(words[1]);
            instructions_print.push_back(words[2]);
        }
    }
    file.close();

    int total_instructions = instructions_hex.size();
    int numCycles = atoi(argv[2]);

    // Initialize pipeline registers.
    IFStage IF = {0, false, -1};
    IDStage ID = {
        .RR1 = 0, .RR2 = 0, .WR = 0,
        .RD1 = 0, .RD2 = 0, .Imm = 0,
        .RegWrite = false, .RegDst = false, .Branch = false,
        .Jump = false, .MemRead = false, .MemWrite = false,
        .ALUSrc = false, .ALUOp = 0, .MemtoReg = false,
        .stall = false, .InStr = -1
    };
    
    EXStage EX = {0, false, 0, 0, 0, false, false, false, false, false, false, -1, false};
    MEMStage DM = {0,0,0, false, false, false, false, 0, 0, -1, false};
    WBStage WB = {false, false, 0, 0, 0, -1, false};

    // For recording which cycle each instruction appears in which stage.
    vector<vector<int>> Output(5, vector<int>(total_instructions, -1));

    // Main simulation loop.
    for (int cycle = 1; cycle <= numCycles; cycle++) {
        // Process WB stage.
        process_WB(DM, WB);
        if (WB.InStr != -1 && WB.InStr < total_instructions && Output[4][WB.InStr] == -1)
            Output[4][WB.InStr] = cycle;

        // Terminate when the last instruction completes WB.
        if (WB.InStr == total_instructions - 1) {
            cout << "Pipeline completed at cycle " << cycle << endl;
            break;
        }

        // Process MEM stage.
        process_MEM(EX, DM);
        if (DM.InStr != -1 && DM.InStr < total_instructions && Output[3][DM.InStr] == -1)
            Output[3][DM.InStr] = cycle;

        // Process EX stage.
        {
            // Save a copy of the old EX stage if needed for forwarding.
            EXStage prevEX = EX;
            process_EX(ID, EX, prevEX, DM, WB);
        }
        if (EX.InStr != -1 && EX.InStr < total_instructions && Output[2][EX.InStr] == -1)
            Output[2][EX.InStr] = cycle;

        // Process ID stage.
        process_ID(IF, ID, instructions_hex);
        if (ID.InStr != -1 && ID.InStr < total_instructions && Output[1][ID.InStr] == -1)
            Output[1][ID.InStr] = cycle;

        // Process IF stage.
        process_IF(IF, instructions_hex);
        if (IF.InStr != -1 && IF.InStr < total_instructions && Output[0][IF.InStr] == -1)
            Output[0][IF.InStr] = cycle;

        // (Optional) Debug print the stage instruction numbers.
        cout << "Cycle " << cycle << ": IF:" << IF.InStr << " ID:" << ID.InStr 
             << " EX:" << EX.InStr << " MEM:" << DM.InStr << " WB:" << WB.InStr << endl;
    }

    // Print pipeline timing table.
    cout << "\nInstr\t";
    for (int cycle = 1; cycle <= numCycles; cycle++) {
        cout << cycle << "\t";
    }
    cout << endl;
    
    for (int i = 0; i < total_instructions; i++) {
        cout << instructions_print[i] << "\t";
        for (int cycle = 1; cycle <= numCycles; cycle++) {
            bool printed = false;
            for (int stage = 0; stage < 5; stage++) {
                if (Output[stage][i] == cycle) {
                    switch(stage) {
                        case 0: cout << "IF\t"; break;
                        case 1: cout << "ID\t"; break;
                        case 2: cout << "EX\t"; break;
                        case 3: cout << "MEM\t"; break;
                        case 4: cout << "WB\t"; break;
                    }
                    printed = true;
                    break;
                }
            }
            if (!printed)
                cout << "-\t";
        }
        cout << endl;
    }
    return 0;
}

// Instruction Fetch stage.
// If a stall is in effect, do not advance the PC.
void process_IF(IFStage &IF, const vector<string> &instructions) {
    if (IF.stall) {
        // Stall: do not fetch a new instruction.
        cout << "IF stage stalled; PC remains at " << IF.PC << endl;
        IF.stall = false; // Clear stall flag so that stall lasts one cycle.
        return;
    }
    if (IF.PC < instructions.size())
        IF.InStr = IF.PC;
    else
        IF.InStr = -1;
    IF.PC++;
}

// Instruction Decode stage.
// If the ID stage is stalled, do not update the instruction.
void process_ID(IFStage &IF, IDStage &ID, const vector<string> &instructions) {
    if (ID.stall) {
        cout << "ID stage stalled; holding instruction " << ID.InStr << endl;
        // Do not update; ID previous decode.
        ID.stall = false; // Clear stall for next cycle.
        IF.stall = true;
        return;
    }
    // If IF holds a valid instruction, decode it.
    if (IF.InStr == -1 || IF.InStr >= instructions.size()) {
        ID.InStr = -1;
        return;
    }
    ID.InStr = IF.InStr;
    
    string instr = instructions[ID.InStr];
    instr = hexToBin(instr);
    
    // Get opcode from bits 25 to 31.
    string opcode = instr.substr(25, 7);

    if (opcode == "0110011")
    {
        // ADD: opcode = 0110011, funct7 = 0000000, funct3 = 000
        if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "000")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2); // rs1
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);  // rs2
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2); // rd
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 2;  // Addition
            ID.MemtoReg = false;
        }
        // SUB: opcode = 0110011, funct7 = 0100000, funct3 = 000
        else if (instr.substr(0, 7) == "0100000" && instr.substr(17, 3) == "000")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 3;  // Subtraction
            ID.MemtoReg = false;
        }
        // SLL: Shift Left Logical, funct7 = 0000000, funct3 = 001
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "001")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2); // shift amount in lower bits of rs2
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 7;  // SLL
            ID.MemtoReg = false;
        }
        // SRL: Shift Right Logical, funct7 = 0000000, funct3 = 101
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "101")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2); // shift amount
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 8;  // SRL
            ID.MemtoReg = false;
        }
        // SRA: Shift Right Arithmetic, funct7 = 0100000, funct3 = 101
        else if (instr.substr(0, 7) == "0100000" && instr.substr(17, 3) == "101")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2); // shift amount
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 9;  // SRA
            ID.MemtoReg = false;
        }
        // SLT: Set Less Than (signed), funct7 = 0000000, funct3 = 010
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "010")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 10; // SLT
            ID.MemtoReg = false;
        }
        // SLTU: Set Less Than Unsigned, funct7 = 0000000, funct3 = 011
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "011")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 11; // SLTU
            ID.MemtoReg = false;
        }
        // AND: opcode = 0110011, funct7 = 0000000, funct3 = 111
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "111")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 4;  // AND
            ID.MemtoReg = false;
        }
        // OR: opcode = 0110011, funct7 = 0000000, funct3 = 110
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "110")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 5;  // OR
            ID.MemtoReg = false;
        }
        // XOR: opcode = 0110011, funct7 = 0000000, funct3 = 100
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "100")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = true;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 6;  // XOR
            ID.MemtoReg = false;
        }
    }
    // I‑type instructions
    else if (opcode == "0010011")
    {
        // ADDI: funct3 = "000"
        if (instr.substr(17, 3) == "000")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            // For non‑shift I‑type instructions, immediate is from bits 0–11.
            ID.Imm = stoi(instr.substr(0, 12), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = false;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = true;    ID.ALUOp = 2;  // ADDI uses addition
            ID.MemtoReg = false;
        }
        // SLLI: Shift Left Logical Immediate, funct3 = "001"
        else if (instr.substr(17, 3) == "001")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            // For shift immediates, the shift amount comes from bits 20–24.
            ID.Imm = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = false;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = true;    ID.ALUOp = 7;  // SLLI
            ID.MemtoReg = false;
        }
        // SRLI / SRAI: Shift Right Logical/Arithmetic Immediate, funct3 = "101"
        else if (instr.substr(17, 3) == "101")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            // For shift immediates, shift amount is in bits 20–24.
            ID.Imm = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = false;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = true;
            // Determine if this is SRLI or SRAI by checking the higher bits (bits 0–7)
            if (instr.substr(0, 7) == "0000000")
                ID.ALUOp = 8;  // SRLI
            else if (instr.substr(0, 7) == "0100000")
                ID.ALUOp = 9;  // SRAI
            ID.MemtoReg = false;
        }
        // SLTI: Set Less Than Immediate (signed), funct3 = "010"
        else if (instr.substr(17, 3) == "010")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            // Immediate from bits 0–11.
            ID.Imm = stoi(instr.substr(0, 12), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = false;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = true;    ID.ALUOp = 10; // SLTI
            ID.MemtoReg = false;
        }
        // SLTIU: Set Less Than Immediate Unsigned, funct3 = "011"
        else if (instr.substr(17, 3) == "011")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.Imm = stoi(instr.substr(0, 12), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = false;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = true;    ID.ALUOp = 11; // SLTIU
            ID.MemtoReg = false;
        }
        // ANDI: funct3 = "111"
        else if (instr.substr(17, 3) == "111")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.Imm = stoi(instr.substr(0, 12), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = false;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = true;    ID.ALUOp = 4;  // ANDI
            ID.MemtoReg = false;
        }
        // ORI: funct3 = "110"
        else if (instr.substr(17, 3) == "110")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.Imm = stoi(instr.substr(0, 12), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = false;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = true;    ID.ALUOp = 5;  // ORI
            ID.MemtoReg = false;
        }
        // XORI: funct3 = "100"
        else if (instr.substr(17, 3) == "100")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR  = stoi(instr.substr(20, 5), nullptr, 2);
            ID.Imm = stoi(instr.substr(0, 12), nullptr, 2);
            ID.RegWrite = true;  ID.RegDst = false;
            ID.Branch = false;   ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = true;    ID.ALUOp = 6;  // XORI
            ID.MemtoReg = false;
        }
    }
    // I‑type load: LW
    else if (opcode == "0000011")
    {
        ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2); // base register
        ID.WR  = stoi(instr.substr(20, 5), nullptr, 2); // destination register
        ID.Imm = stoi(instr.substr(0, 12), nullptr, 2);
        ID.RegWrite = true;  ID.RegDst = false;
        ID.Branch = false;   ID.Jump = false;
        ID.MemRead = true;   ID.MemWrite = false;
        ID.ALUSrc = true;    ID.ALUOp = 2;  // Address calculation uses addition
        ID.MemtoReg = true;
    }
    // S‑type: Store Word (SW)
    else if (opcode == "0100011")
    {
        ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2); // base register
        ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);  // source register
        string imm_str = instr.substr(0, 7) + instr.substr(20, 5);
        ID.Imm = stoi(imm_str, nullptr, 2);
        ID.RegWrite = false; ID.RegDst = false;
        ID.Branch = false;   ID.Jump = false;
        ID.MemRead = false;  ID.MemWrite = true;
        ID.ALUSrc = true;    ID.ALUOp = 2;  // Address calculation uses addition
    }
    // B‑type: Branch Equal (BEQ)
    else if (opcode == "1100011")
    {
        if (instr.substr(17, 3) == "000")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            string imm_branch = "";
            imm_branch += instr.substr(0, 1);    // imm[12]
            imm_branch += instr.substr(1, 6);    // imm[10:5]
            imm_branch += instr.substr(7, 1);    // imm[11]
            imm_branch += instr.substr(20, 4);   // imm[4:1]
            imm_branch += "0";                   // branch addresses are 2-byte aligned
            ID.Imm = stoi(imm_branch, nullptr, 2);
            ID.RegWrite = false; ID.RegDst = false;
            ID.Branch = true;    ID.Jump = false;
            ID.MemRead = false;  ID.MemWrite = false;
            ID.ALUSrc = false;   ID.ALUOp = 3;  // Subtraction for comparison
        }
    }
}

// EX (ALU) stage.
// In addition to performing the ALU operation, we check for load–use hazards.
// If a hazard is detected, we insert a bubble (by setting EX.InStr = –1) and
// signal the IF and ID stages to stall.
void process_EX(IDStage &ID, EXStage &EX, const EXStage &prevEX, const MEMStage &DM, const WBStage &WB) {
    // If no valid instruction, pass a bubble.
    if (ID.InStr == -1) {
        EX.InStr = -1;
        return;
    }
    
    // Hazard detection: if DM holds a load whose destination is used by the current instruction.
    bool loadHazard = false;
    if (DM.RegWrite && DM.MemtoReg) {
        if (DM.WriteReg == ID.RR1 || (!ID.ALUSrc && DM.WriteReg == ID.RR2)) {
            loadHazard = true;
        }
    }
    
    if (loadHazard) {
        cout << "EX stage detected load-use hazard at instruction " << ID.InStr
             << ". Inserting bubble." << endl;
        // Insert a bubble by setting EX.InStr to -1.
        EX.InStr = -1;
        // Signal the ID stage to stall for one cycle.
        ID.stall = true;
        return;
    }
    
    // Normal propagation from ID to EX.
    EX.InStr = ID.InStr;
    EX.WriteReg = ID.WR;
    EX.Branch = ID.Branch;
    EX.Jump = ID.Jump;
    EX.MemRead = ID.MemRead;
    EX.MemWrite = ID.MemWrite;
    EX.MemtoReg = ID.MemtoReg;
    EX.RegWrite = ID.RegWrite;
    EX.WriteData = ID.RD2;
    EX.WriteDataReg = ID.RR2;
    
    int arg1 = ID.RD1;
    if (DM.RegWrite && DM.WriteReg == ID.RR1)
        arg1 = (DM.MemtoReg ? DM.Read_data : DM.ALU_res);
    else if (WB.RegWrite && WB.WriteReg == ID.RR1)
        arg1 = (WB.MemtoReg ? WB.Read_data : WB.ALU_res);
    
    int arg2 = ID.ALUSrc ? ID.Imm : ID.RD2;
    if (!ID.ALUSrc) {
        if (DM.RegWrite && DM.WriteReg == ID.RR2)
            arg2 = (DM.MemtoReg ? DM.Read_data : DM.ALU_res);
        else if (WB.RegWrite && WB.WriteReg == ID.RR2)
            arg2 = (WB.MemtoReg ? WB.Read_data : WB.ALU_res);
    }
    
    switch (ID.ALUOp) {
        case 2:
            EX.ALU_res = arg1 + arg2;
            break;
        case 3:
            EX.ALU_res = arg1 - arg2;
            break;
        case 4:
            EX.ALU_res = arg1 & arg2;
            break;
        case 5:
            EX.ALU_res = arg1 | arg2;
            break;
        default:
            EX.ALU_res = arg1 + arg2;
            break;
    }
    EX.Zero = (EX.ALU_res == 0);
}

void process_MEM(EXStage &EX, MEMStage &DM) {
    if (EX.InStr == -1) {
        DM.InStr = -1;
        // Clear control signals to avoid persistent hazard.
        DM.MemRead = false;
        DM.MemWrite = false;
        DM.RegWrite = false;
        DM.MemtoReg = false;
        DM.ALU_res = 0;
        DM.Address = 0;
        DM.Write_data = 0;
        return;
    }
    // Normal propagation when there is a valid instruction.
    DM.InStr = EX.InStr;
    DM.ALU_res = EX.ALU_res;
    DM.Write_data = EX.WriteData;
    DM.WriteReg = EX.WriteReg;
    DM.MemRead = EX.MemRead;
    DM.MemWrite = EX.MemWrite;
    DM.MemtoReg = EX.MemtoReg;
    DM.RegWrite = EX.RegWrite;
    DM.Address = EX.ALU_res;
    
    if (DM.MemRead) {
        DM.Read_data = MEM[DM.Address];
    } else if (DM.MemWrite) {
        MEM[DM.Address] = DM.Write_data;
    }
}

void process_WB(MEMStage &DM, WBStage &WB) {
    if (DM.InStr == -1) {
        WB.InStr = -1;
        WB.RegWrite = false;
        WB.MemtoReg = false;
        WB.ALU_res = 0;
        WB.Read_data = 0;
        WB.WriteReg = 0;
        return;
    }
    WB.InStr = DM.InStr;
    WB.Read_data = DM.Read_data;
    WB.MemtoReg = DM.MemtoReg;
    WB.RegWrite = DM.RegWrite;
    WB.ALU_res = DM.ALU_res;
    WB.WriteReg = DM.WriteReg;
    
    if (WB.RegWrite) {
        RegFile[WB.WriteReg].value = (WB.MemtoReg ? WB.Read_data : WB.ALU_res);
    }
}
