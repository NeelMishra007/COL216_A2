#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <bitset>

using namespace std;

const int N = 2e6 + 5;
int MEM[N];

typedef struct {
    int value;
} Register;

Register RegFile[32];

string hexToBin(const string &hex)
{
    string binary;
    for (char ch : hex)
    {
        int val = stoi(string(1, ch), nullptr, 16);
        binary += bitset<4>(val).to_string();
    }
    return binary;
}

vector<string> splitLine(const string &line)
{
    vector<string> words;
    stringstream ss(line);
    string word;

    while (ss >> word)
    {
        words.push_back(word);
    }

    return words;
}

typedef struct
{
    int PC;
    bool PCSrc;
    int InStr;
} IF;

typedef struct
{
    // Register read addresses
    int RR1; // e.g. rs
    int RR2; // e.g. rt

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
    int ALUOp; // 2 bits in classic MIPS (0=add, 1=sub, 2=R-type, etc.)
    bool MemtoReg;

    int InStr;
} ID;

typedef struct
{
    // ALU outputs
    int ALU_res;
    bool Zero; // ALU result == 0?

    // Forward the register data that might be used in MEM
    int WriteData; // e.g. RD2 if we need to store in memory
    int WriteDataReg;
    // Which register will be written in WB (after RegDst mux)
    int WriteReg;

    // Carry over control signals to MEM
    bool Branch;
    bool Jump;
    bool MemRead;
    bool MemWrite;
    bool MemtoReg;
    bool RegWrite;

    int InStr;
    int stalls;
} ALU;

typedef struct
{
    // Address to read/write in memory
    int Address;

    // Data to write (for store)
    int Write_data;

    // Data read from memory (for load)
    int Read_data;

    // Control signals used in MEM
    bool MemRead;
    bool MemWrite;

    // Control signals forwarded to WB
    bool MemtoReg;
    bool RegWrite;

    // Forward the ALU result
    int ALU_res;

    // The register number to be written in WB
    int WriteReg;

    int InStr;
    int stalls;
} DM;

typedef struct
{
    // Control signals
    bool MemtoReg;
    bool RegWrite;

    // Data from MEM
    int Read_data; // load result
    // Data from EX
    int ALU_res;

    // The final register to write back
    int WriteReg;

    int InStr;
    int stalls;
} WB;

// Function prototypes
void process_IF(IF &IF, ID &ID);
void process_ID(ALU &ALU, ID &ID, Register RegFile[32], vector<string> instructions);
void process_ALU(ALU &ALU, ID &ID, WB &WB, DM &DM);
void process_DM(DM &DM, ALU &ALU, WB &WB);
void process_WB(WB &WB, DM &DM);

int main(int argc, char **argv)
{
    // Initialize register file
    for (int i = 0; i < 32; i++)
    {
        RegFile[i].value = 0;
    }

    // Open the input file
    ifstream file(argv[1]);
    if (!file)
    {
        cerr << "Error: Unable to open input.txt" << endl;
        return 1;
    }

    // Read instructions from file
    vector<string> instructions_hex;
    vector<string> instructions_print;

    string instr;
    while (getline(file, instr))
    {
        vector<string> words = splitLine(instr);
        instructions_hex.push_back(words[1]);
        instructions_print.push_back(words[2]);
    }

    // Initialize pipeline stages
    IF IF = {0, false, -1};
    ID ID = {0, 0, 0, 0, 0, 0, false, false, false, false, false, false, false, 0, false, -1};
    ALU ALU = {0, false, 0, 0, 0, false, false, false, false, false, false, -1, 0};
    DM DM = {0, 0, 0, false, false, false, false, 0, 0, -1, 0};
    WB WB = {false, false, 0, 0, 0, -1, 0};

    // Arrays to store cycle information for each instruction
    vector<int> Output[5]; // [IF, ID, ALU, DM, WB] cycles for each instruction
    
    // Initialize Output arrays
    for (int i = 0; i < 5; i++) {
        Output[i].resize(instructions_hex.size(), -1);
    }

    int num = atoi(argv[2]); // Number of cycles to simulate
    int total_instructions = instructions_hex.size();

    cout << total_instructions << endl;
    // Pipeline simulation loop
    for (int cycle = 1; cycle <= num; cycle++)
    {
        // Process stages in reverse order to avoid overwriting
        // Write-Back stage
        if (DM.InStr != -1) {
            process_WB(WB, DM);
            if (WB.InStr != -1) {
                Output[4][WB.InStr] = cycle; // Record WB cycle
            }
        }
        
        // Memory Access stage
        if (ALU.InStr != -1) {
            process_DM(DM, ALU, WB);
            if (DM.InStr != -1 && DM.stalls == 0) {
                Output[3][DM.InStr] = cycle; // Record DM cycle
            }
        }
        
        // Execute stage
        if (ID.InStr != -1) {
            process_ALU(ALU, ID, WB, DM);
            if (ALU.InStr != -1) {
                Output[2][ALU.InStr] = cycle; // Record ALU cycle
            }
        }
        
        // Instruction Decode stage
        if (IF.InStr != -1) {
            process_ID(ALU, ID, RegFile, instructions_hex);
            if (ID.InStr != -1) {
                Output[1][ID.InStr] = cycle; // Record ID cycle
            }
        }
        
        // Instruction Fetch stage
        if (IF.PC < total_instructions) {
            process_IF(IF, ID);
            if (IF.InStr != -1 && IF.InStr < total_instructions) {
                Output[0][IF.InStr] = cycle; // Record IF cycle
            }
        }
        
        // Check if all instructions have completed the pipeline
        bool all_done = true;
        for (int i = 0; i < total_instructions; i++) {
            if (Output[4][i] == -1) { // If any instruction hasn't reached WB
                all_done = false;
                break;
            }
        }
        
        if (all_done && IF.PC >= total_instructions) {
            break; // Exit loop if all instructions have completed
        }
    }

    // Print results
    cout << "Instruction\tIF\tID\tALU\tDM\tWB" << endl;
    for (int i = 0; i < total_instructions; i++) {
        cout << instructions_print[i] << "\t";
        for (int j = 0; j < 5; j++) {
            if (Output[j][i] == -1) {
                cout << "-\t";
            } else {
                cout << Output[j][i] << "\t";
            }
        }
        cout << endl;
    }

    file.close();
    return 0;
}

void process_IF(IF &IF, ID &ID)
{
    ID.InStr = IF.PC;
    IF.InStr = IF.PC;
    IF.PC += 1;
}
void process_ID(ALU &ALU, ID &ID, Register RegFile[32], vector<string> instructions)
{
    // Fetch instruction and convert from hex to binary.
    string instr = instructions[ID.InStr];
    instr = hexToBin(instr);
    
    // Get opcode from bits 25 to 31.
    string opcode = instr.substr(25, 7);
    cout << opcode << endl;
    // R‑type instructions
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

void process_ALU(ALU &ALU, ID &ID, WB &WB, DM &DM)
{
    if (ID.InStr == -1)
        return;
        
    // Set ALU control signals from the ID stage.
    ALU.InStr       = ID.InStr;
    ALU.WriteReg    = ID.WR;
    ALU.Branch      = ID.Branch;
    ALU.Jump        = ID.Jump;
    ALU.MemRead     = ID.MemRead;
    ALU.MemWrite    = ID.MemWrite;
    ALU.MemtoReg    = ID.MemtoReg;
    ALU.RegWrite    = ID.RegWrite;
    
    // For store instructions, prepare the write data and check for forwarding.
    ALU.WriteData   = ID.RD2;
    ALU.WriteDataReg = ID.RR2;
    if (ALU.MemWrite == true) // Forwarding for DM last-to-last instruction.
    {
        if (WB.RegWrite == true && WB.WriteReg == ALU.WriteDataReg)
        {
            if (WB.MemtoReg == true)
                ALU.WriteData = WB.Read_data;
            else
                ALU.WriteData = WB.ALU_res;
        }
    }
    
    // Determine first operand (arg1) with hazard forwarding.
    int arg1 = 0;
    if (DM.RegWrite == true && DM.WriteReg == ID.RR1)
    {
        if (DM.MemtoReg == true)
        {
            if (DM.stalls == 0)
            {
                ALU.InStr = -1;
                return;
            }
            else
                arg1 = DM.Read_data;
        }
        else
            arg1 = DM.ALU_res;
    }
    else if (WB.RegWrite == true && WB.WriteReg == ID.RR1)
    {
        if (WB.MemtoReg == true)
            arg1 = WB.Read_data;
        else
            arg1 = WB.ALU_res;
    }
    else
    {
        arg1 = ID.RD1;
    }
    
    // Determine second operand (arg2) – immediate or register with forwarding.
    int arg2 = 0;
    if (ID.ALUSrc == true)
    {
        arg2 = ID.Imm;
    }
    else
    {
        if (DM.RegWrite == true && DM.WriteReg == ID.RR2)
        {
            if (DM.MemtoReg == true)
            {
                if (DM.stalls == 0)
                {
                    ALU.InStr = -1;
                    return;
                }
                else
                    arg2 = DM.Read_data;
            }
            else
            {
                arg2 = DM.ALU_res;
            }
        }
        else if (WB.RegWrite == true && WB.WriteReg == ID.RR2)
        {
            if (WB.MemtoReg == true)
                arg2 = WB.Read_data;
            else
                arg2 = WB.ALU_res;
        }
        else
        {
            arg2 = ID.RD2;
        }
    }
    
    // Perform ALU operation based on ALUOp.
    // Mapping of ALUOp values:
    //  2: Addition, 3: Subtraction, 4: AND, 5: OR, 6: XOR,
    //  7: SLL, 8: SRL, 9: SRA, 10: SLT, 11: SLTU.
    switch (ID.ALUOp)
    {
        case 2: // ADD / ADDI / LW / SW effective address calculation
            ALU.ALU_res = arg1 + arg2;
            break;
        case 3: // SUB / BEQ
            ALU.ALU_res = arg1 - arg2;
            break;
        case 4: // AND / ANDI
            ALU.ALU_res = arg1 & arg2;
            break;
        case 5: // OR / ORI
            ALU.ALU_res = arg1 | arg2;
            break;
        case 6: // XOR / XORI
            ALU.ALU_res = arg1 ^ arg2;
            break;
        case 7: // SLL / SLLI (shift left logical)
            ALU.ALU_res = arg1 << (arg2 & 0x1F);
            break;
        case 8: // SRL / SRLI (logical right shift)
            ALU.ALU_res = ((unsigned int)arg1) >> (arg2 & 0x1F);
            break;
        case 9: // SRA / SRAI (arithmetic right shift)
            ALU.ALU_res = arg1 >> (arg2 & 0x1F);
            break;
        case 10: // SLT / SLTI (set less than, signed)
            ALU.ALU_res = (arg1 < arg2) ? 1 : 0;
            break;
        case 11: // SLTU / SLTIU (set less than, unsigned)
            ALU.ALU_res = (((unsigned int)arg1) < ((unsigned int)arg2)) ? 1 : 0;
            break;
        default:
            // If ALUOp is not recognized, default to addition.
            ALU.ALU_res = arg1 + arg2;
            break;
    }
    
    // Set the zero flag.
    ALU.Zero = (ALU.ALU_res == 0);
}


void process_DM(DM &DM, ALU &ALU, WB &WB)
{
    if (ALU.InStr == -1)
    {
        DM.stalls++;
        return;
    }
    DM.stalls = 0;
    DM.InStr = ALU.InStr;
    DM.ALU_res = ALU.ALU_res;
    DM.Write_data = ALU.WriteData;
    DM.WriteReg = ALU.WriteReg;
    DM.MemRead = ALU.MemRead;
    DM.MemWrite = ALU.MemWrite;
    DM.MemtoReg = ALU.MemtoReg;
    DM.RegWrite = ALU.RegWrite;
    DM.Address = ALU.ALU_res;

    if (DM.MemRead == true)
    {
        DM.Read_data = MEM[DM.Address];
    }
    else if (DM.MemWrite == true)
    {
        if (WB.RegWrite == true && WB.WriteReg == ALU.WriteDataReg)
        { // previous instruction
            if (WB.MemtoReg == true)
            {
                MEM[DM.Address] = WB.Read_data;
            }
            else
            {
                MEM[DM.Address] = WB.ALU_res;
            }
        }
        else
        {
            MEM[DM.Address] = DM.Write_data;
        }
    }
}

void process_WB(WB &WB, DM &DM)
{

    WB.InStr = DM.InStr;
    WB.Read_data = DM.Read_data;
    WB.MemtoReg = DM.MemtoReg;
    WB.RegWrite = DM.RegWrite;
    WB.ALU_res = DM.ALU_res;
    WB.WriteReg = DM.WriteReg;
    if (WB.InStr == -1)
        return;
    if (WB.RegWrite == true)
    {
        if (WB.MemtoReg == true)
        {
            RegFile[WB.WriteReg].value = WB.Read_data;
        }
        else
        {
            RegFile[WB.WriteReg].value = WB.ALU_res;
        }
    }
}