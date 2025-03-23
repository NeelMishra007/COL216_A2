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

typedef struct
{
    int value;
} Register;

Register RegFile[32];

// Convert hex string to binary string
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

// Split a line by whitespace.
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

// Pipeline stage structures

struct IFStage
{
    int PC;
    bool stall;
    int InStr; // holds the index of the instruction fetched; -1 means bubble.
};

struct IDStage
{
    int RR1; // source register 1 (rs1)
    int RR2; // source register 2 (rs2)
    int WR;  // destination register (rd)
    int RD1; // read data from RegFile for RR1
    int RD2; // read data from RegFile for RR2
    int Imm; // sign-extended immediate

    // Control signals
    bool RegWrite; // write to register file
    bool RegDst;   // determines destination register (used for R-type vs I-type)
    bool Branch;   // branch instruction
    bool Jump;     // jump instruction
    bool MemRead;  // read from memory
    bool MemWrite; // write to memory
    bool ALUSrc;   // selects second ALU input (reg or imm)
    int ALUOp;     // operation code for the ALU
    bool MemtoReg; // selects data to write to register (ALU result or memory)

    // Additional fields from the code
    int MemSize;        // size of memory access (1, 2, or 4 bytes)
    bool MemSignExtend; // whether to sign-extend memory reads
    int BranchType;     // type of branch (BEQ, BNE, BLT, etc.)
    bool JumpAndLink;   // JAL or JALR instruction
    bool JumpReg;       // JALR instruction (jump to register)

    bool stall; // when true, hold the instruction (do not update)
    int InStr;  // index of instruction; -1 indicates bubble

    int DM_stall_prev;
};

struct EXStage
{
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

struct MEMStage
{
    int Address;
    int Write_data;
    int Read_data; // for loads
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
    int Read_data; // load data from memory
    int ALU_res;   // result from ALU
    int WriteReg;

    int InStr;
    bool stall;
};

// Function prototypes – note that the ID stage is assumed to fill in all needed fields.
void process_IF(const vector<string> &instructions);
void process_ID(const vector<string> &instructions);
void process_EX();
void process_MEM();
void process_WB();

IFStage IF = {0, false, -1};
IDStage ID = {
    .RR1 = 0, .RR2 = 0, .WR = 0, .RD1 = 0, .RD2 = 0, .Imm = 0, .RegWrite = false, .RegDst = false, .Branch = false, .Jump = false, .MemRead = false, .MemWrite = false, .ALUSrc = false, .ALUOp = 0, .MemtoReg = false, .stall = false, .InStr = -1, .DM_stall_prev = 0};

EXStage EX = {0, false, 0, 0, 0, false, false, false, false, false, false, -1, false};
MEMStage DM = {0, 0, 0, false, false, false, false, 0, 0, -1, false};
WBStage WB = {false, false, 0, 0, 0, -1, false};

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "Usage: " << argv[0] << " <input.txt> <num_cycles>" << endl;
        return 1;
    }

    // Initialize register file to 0.
    for (int i = 0; i < 32; i++)
    {
        RegFile[i].value = 0;
    }

    // Open the input file.
    ifstream file(argv[1]);
    if (!file)
    {
        cerr << "Error: Unable to open input file " << argv[1] << endl;
        return 1;
    }

    vector<string> instructions_hex;
    vector<string> instructions_print;
    string line;
    while (getline(file, line))
    {
        vector<string> words = splitLine(line);
        if (words.size() >= 3)
        {
            instructions_hex.push_back(words[1]);
            instructions_print.push_back(words[2]);
        }
    }
    file.close();

    int total_instructions = instructions_hex.size();
    int numCycles = atoi(argv[2]);

    // Initialize pipeline registers.

    // For recording which cycle each instruction appears in which stage.
    vector<vector<int>> Output(5, vector<int>(total_instructions, -1));

    // Main simulation loop.
    for (int cycle = 1; cycle <= numCycles; cycle++)
    {

        // Process WB stage.
        process_WB();
        if (WB.InStr != -1 && WB.InStr < total_instructions && Output[4][WB.InStr] == -1)
            Output[4][WB.InStr] = cycle;

        // Terminate when the last instruction completes WB.
        if (WB.InStr == total_instructions - 1)
        {
            cout << "Pipeline completed at cycle " << cycle << endl;
            break;
        }

        // Process MEM stage.
        process_MEM();
        if (DM.InStr != -1 && DM.InStr < total_instructions && Output[3][DM.InStr] == -1)
            Output[3][DM.InStr] = cycle;

        process_EX();
        if (EX.InStr != -1 && EX.InStr < total_instructions && Output[2][EX.InStr] == -1)
            Output[2][EX.InStr] = cycle;

        if (ID.Branch && !ID.stall)
        {
            bool branch_taken = false;

            switch (ID.BranchType)
            {
            case 0: // BEQ: Branch if Equal
                branch_taken = (RegFile[ID.RR1].value == RegFile[ID.RR2].value);
                break;
            case 1: // BNE: Branch if Not Equal
                branch_taken = (RegFile[ID.RR1].value != RegFile[ID.RR2].value);
                break;
            case 2: // BLT: Branch if Less Than (signed)
                branch_taken = (RegFile[ID.RR1].value < RegFile[ID.RR2].value);
                break;
            case 3: // BGE: Branch if Greater or Equal (signed)
                branch_taken = (RegFile[ID.RR1].value >= RegFile[ID.RR2].value);
                break;
            case 4: // BLTU: Branch if Less Than (unsigned)
                branch_taken = ((unsigned)RegFile[ID.RR1].value < (unsigned)RegFile[ID.RR2].value);
                break;
            case 5: // BGEU: Branch if Greater or Equal (unsigned)
                branch_taken = ((unsigned)RegFile[ID.RR1].value >= (unsigned)RegFile[ID.RR2].value);
                break;
            default:
                cout << "Unknown branch type encountered!" << endl;
                break;
            }

            if (branch_taken)
            {
                // Convert byte offset to instruction offset: assume PC increments by 1 per instruction.
                // ID.Imm is in bytes so divide by 4, then adjust offset as needed (-2 adjustment as in your design).
                IF.PC = IF.PC + (ID.Imm / 4) - 2;
                cout << "Branch taken, new PC: " << IF.PC << endl;
                IF.InStr = -1;     // Insert bubble in IF stage.
                ID.Branch = false; // Clear branch signal after taken.
            }
        }

        process_ID(instructions_hex);
        if (ID.InStr != -1 && ID.InStr < total_instructions && Output[1][ID.InStr] == -1)
            Output[1][ID.InStr] = cycle;

        // Process IF stage.
        process_IF(instructions_hex);
        if (IF.InStr != -1 && IF.InStr < total_instructions && Output[0][IF.InStr] == -1)
            Output[0][IF.InStr] = cycle;

        // (Optional) Debug print the stage instruction numbers.
        cout << "Cycle " << cycle << ": IF:" << IF.InStr << " ID:" << ID.InStr
             << " EX:" << EX.InStr << " MEM:" << DM.InStr << " WB:" << WB.InStr << endl;
    }

    // Print pipeline timing table.
    cout << "\nInstr\t";
    for (int cycle = 1; cycle <= numCycles; cycle++)
    {
        cout << cycle << "\t";
    }
    cout << endl;

    for (int i = 0; i < total_instructions; i++)
    {
        cout << instructions_print[i] << "\t";
        for (int cycle = 1; cycle <= numCycles; cycle++)
        {
            bool printed = false;
            for (int stage = 0; stage < 5; stage++)
            {
                if (Output[stage][i] == cycle)
                {
                    switch (stage)
                    {
                    case 0:
                        cout << "IF\t";
                        break;
                    case 1:
                        cout << "ID\t";
                        break;
                    case 2:
                        cout << "EX\t";
                        break;
                    case 3:
                        cout << "MEM\t";
                        break;
                    case 4:
                        cout << "WB\t";
                        break;
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
void process_IF(const vector<string> &instructions)
{
    if (IF.stall)
    {
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
void process_ID(const vector<string> &instructions)
{
    if (ID.stall)
    {
        cout << "ID stage stalled; holding instruction " << ID.InStr << endl;
        // Do not update; ID previous decode.
        ID.stall = false; // Clear stall for next cycle.
        IF.stall = true;
        if (ID.DM_stall_prev == 1)
        {
            ID.DM_stall_prev = 2;
            DM.stall = true;
        }
        return;
    }
    int DM_stall_prev = 0;
    // If IF holds a valid instruction, decode it.
    if (IF.InStr == -1 || IF.InStr >= instructions.size())
    {
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
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);  // rd
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 2; // Addition
            ID.MemtoReg = false;
        }
        // SUB: opcode = 0110011, funct7 = 0100000, funct3 = 000
        else if (instr.substr(0, 7) == "0100000" && instr.substr(17, 3) == "000")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 3; // Subtraction
            ID.MemtoReg = false;
        }
        // SLL: Shift Left Logical, funct7 = 0000000, funct3 = 001
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "001")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 7; // SLL
            ID.MemtoReg = false;
        }
        // SRL: Shift Right Logical, funct7 = 0000000, funct3 = 101
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "101")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 8; // SRL
            ID.MemtoReg = false;
        }
        // SRA: Shift Right Arithmetic, funct7 = 0100000, funct3 = 101
        else if (instr.substr(0, 7) == "0100000" && instr.substr(17, 3) == "101")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 9; // SRA
            ID.MemtoReg = false;
        }
        // SLT: Set Less Than (signed), funct7 = 0000000, funct3 = 010
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "010")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 10; // SLT
            ID.MemtoReg = false;
        }
        // SLTU: Set Less Than Unsigned, funct7 = 0000000, funct3 = 011
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "011")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 11; // SLTU
            ID.MemtoReg = false;
        }
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "111")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 4; // AND
            ID.MemtoReg = false;
        }
        // OR: opcode = 0110011, funct7 = 0000000, funct3 = 110
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "110")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 5; // OR
            ID.MemtoReg = false;
        }
        // XOR: opcode = 0110011, funct7 = 0000000, funct3 = 100
        else if (instr.substr(0, 7) == "0000000" && instr.substr(17, 3) == "100")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = true;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = false;
            ID.ALUOp = 6; // XOR
            ID.MemtoReg = false;
        }
    }
    // I-type instructions (non-load)
    else if (opcode == "0010011")
    {
        // ADDI: funct3 = "000"
        if (instr.substr(17, 3) == "000")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);

            // Extract 12-bit immediate and sign-extend it
            string imm_str = instr.substr(0, 12);
            int sign_bit = imm_str[0] - '0';
            int32_t imm_val = stoi(imm_str, nullptr, 2);
            // Sign extend if the MSB is 1
            if (sign_bit == 1)
            {
                imm_val |= 0xFFFFF000; // Set the upper 20 bits to 1
            }
            ID.Imm = imm_val;

            ID.RegWrite = true;
            ID.RegDst = false;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = true;
            ID.ALUOp = 2; // ADDI uses addition
            ID.MemtoReg = false;
        }
        // SLLI: Shift Left Logical Immediate, funct3 = "001"
        else if (instr.substr(17, 3) == "001" && instr.substr(0, 7) == "0000000")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            // For shift immediates, the shift amount comes from bits 7-11 (shamt)
            ID.Imm = stoi(instr.substr(7, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = false;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = true;
            ID.ALUOp = 7; // SLLI
            ID.MemtoReg = false;
        }
        // SRLI: Shift Right Logical Immediate, funct3 = "101", funct7 = "0000000"
        else if (instr.substr(17, 3) == "101" && instr.substr(0, 7) == "0000000")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            // Shift amount is in bits 7-11 (shamt)
            ID.Imm = stoi(instr.substr(7, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = false;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = true;
            ID.ALUOp = 8; // SRLI
            ID.MemtoReg = false;
        }
        // SRAI: Shift Right Arithmetic Immediate, funct3 = "101", funct7 = "0100000"
        else if (instr.substr(17, 3) == "101" && instr.substr(0, 7) == "0100000")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
            // Shift amount is in bits 7-11 (shamt)
            ID.Imm = stoi(instr.substr(7, 5), nullptr, 2);
            ID.RegWrite = true;
            ID.RegDst = false;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = true;
            ID.ALUOp = 9; // SRAI
            ID.MemtoReg = false;
        }
        // SLTI: Set Less Than Immediate (signed), funct3 = "010"
        else if (instr.substr(17, 3) == "010")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);

            // Extract and sign-extend the immediate
            string imm_str = instr.substr(0, 12);
            int sign_bit = imm_str[0] - '0';
            int32_t imm_val = stoi(imm_str, nullptr, 2);
            if (sign_bit == 1)
            {
                imm_val |= 0xFFFFF000;
            }
            ID.Imm = imm_val;

            ID.RegWrite = true;
            ID.RegDst = false;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = true;
            ID.ALUOp = 10; // SLTI
            ID.MemtoReg = false;
        }
        // SLTIU: Set Less Than Immediate Unsigned, funct3 = "011"
        else if (instr.substr(17, 3) == "011")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);

            // Sign-extend the immediate (even for SLTIU, the immediate is sign-extended)
            string imm_str = instr.substr(0, 12);
            int sign_bit = imm_str[0] - '0';
            int32_t imm_val = stoi(imm_str, nullptr, 2);
            if (sign_bit == 1)
            {
                imm_val |= 0xFFFFF000;
            }
            ID.Imm = imm_val;

            ID.RegWrite = true;
            ID.RegDst = false;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = true;
            ID.ALUOp = 11; // SLTIU
            ID.MemtoReg = false;
        }
        // ANDI: funct3 = "111"
        else if (instr.substr(17, 3) == "111")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);

            // Sign-extend the immediate
            string imm_str = instr.substr(0, 12);
            int sign_bit = imm_str[0] - '0';
            int32_t imm_val = stoi(imm_str, nullptr, 2);
            if (sign_bit == 1)
            {
                imm_val |= 0xFFFFF000;
            }
            ID.Imm = imm_val;

            ID.RegWrite = true;
            ID.RegDst = false;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = true;
            ID.ALUOp = 4; // ANDI
            ID.MemtoReg = false;
        }
        // ORI: funct3 = "110"
        else if (instr.substr(17, 3) == "110")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);

            // Sign-extend the immediate
            string imm_str = instr.substr(0, 12);
            int sign_bit = imm_str[0] - '0';
            int32_t imm_val = stoi(imm_str, nullptr, 2);
            if (sign_bit == 1)
            {
                imm_val |= 0xFFFFF000;
            }
            ID.Imm = imm_val;

            ID.RegWrite = true;
            ID.RegDst = false;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = true;
            ID.ALUOp = 5; // ORI
            ID.MemtoReg = false;
        }
        // XORI: funct3 = "100"
        else if (instr.substr(17, 3) == "100")
        {
            ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
            ID.WR = stoi(instr.substr(20, 5), nullptr, 2);

            // Sign-extend the immediate
            string imm_str = instr.substr(0, 12);
            int sign_bit = imm_str[0] - '0';
            int32_t imm_val = stoi(imm_str, nullptr, 2);
            if (sign_bit == 1)
            {
                imm_val |= 0xFFFFF000;
            }
            ID.Imm = imm_val;

            ID.RegWrite = true;
            ID.RegDst = false;
            ID.Branch = false;
            ID.Jump = false;
            ID.MemRead = false;
            ID.MemWrite = false;
            ID.ALUSrc = true;
            ID.ALUOp = 6; // XORI
            ID.MemtoReg = false;
        }
    }
    // I-type load instructions
    else if (opcode == "0000011")
    {
        ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2); // base register
        ID.WR = stoi(instr.substr(20, 5), nullptr, 2);  // destination register

        // Sign-extend the immediate
        string imm_str = instr.substr(0, 12);
        int sign_bit = imm_str[0] - '0';
        int32_t imm_val = stoi(imm_str, nullptr, 2);
        if (sign_bit == 1)
        {
            imm_val |= 0xFFFFF000;
        }
        ID.Imm = imm_val;

        ID.RegWrite = true;
        ID.RegDst = false;
        ID.Branch = false;
        ID.Jump = false;
        ID.MemRead = true;
        ID.MemWrite = false;
        ID.ALUSrc = true;
        ID.ALUOp = 2; // Address calculation uses addition
        ID.MemtoReg = true;

        // Different load types based on funct3
        string funct3 = instr.substr(17, 3);
        if (funct3 == "010")
        {                            // LW: Load Word
            ID.MemSize = 4;          // 4 bytes
            ID.MemSignExtend = true; // Not needed for word, but set for consistency
        }
        else if (funct3 == "001")
        {                            // LH: Load Halfword
            ID.MemSize = 2;          // 2 bytes
            ID.MemSignExtend = true; // Sign-extend
        }
        else if (funct3 == "101")
        {                             // LHU: Load Halfword Unsigned
            ID.MemSize = 2;           // 2 bytes
            ID.MemSignExtend = false; // Zero-extend
        }
        else if (funct3 == "000")
        {                            // LB: Load Byte
            ID.MemSize = 1;          // 1 byte
            ID.MemSignExtend = true; // Sign-extend
        }
        else if (funct3 == "100")
        {                             // LBU: Load Byte Unsigned
            ID.MemSize = 1;           // 1 byte
            ID.MemSignExtend = false; // Zero-extend
        }
    }
    // S-type: Store instructions
    else if (opcode == "0100011")
    {
        ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2); // base register
        ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);  // source register

        // S-type immediate: imm[11:5] is in bits 0-6, imm[4:0] is in bits 20-24
        string imm_str = instr.substr(0, 7) + instr.substr(20, 5);
        int sign_bit = imm_str[0] - '0';
        int32_t imm_val = stoi(imm_str, nullptr, 2);
        if (sign_bit == 1)
        {
            imm_val |= 0xFFFFF000; // Set the upper 20 bits to 1
        }
        ID.Imm = imm_val;

        ID.RegWrite = false;
        ID.RegDst = false;
        ID.Branch = false;
        ID.Jump = false;
        ID.MemRead = false;
        ID.MemWrite = true;
        ID.ALUSrc = true;
        ID.ALUOp = 2; // Address calculation uses addition

        // Different store types based on funct3
        string funct3 = instr.substr(17, 3);
        if (funct3 == "010")
        {                   // SW: Store Word
            ID.MemSize = 4; // 4 bytes
        }
        else if (funct3 == "001")
        {                   // SH: Store Halfword
            ID.MemSize = 2; // 2 bytes
        }
        else if (funct3 == "000")
        {                   // SB: Store Byte
            ID.MemSize = 1; // 1 byte
        }
    }
    // B-type: Branch instructions
    else if (opcode == "1100011")
    {
        ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2); // rs1
        ID.RR2 = stoi(instr.substr(7, 5), nullptr, 2);  // rs2

        // B-type immediate format: imm[12|10:5|4:1|11]
        // bit 0 is always 0 (half-word aligned addresses)
        string imm_str = "";
        imm_str += instr.substr(0, 1);  // imm[12]
        imm_str += instr.substr(24, 1); // imm[11] (stored at bit 24)
        imm_str += instr.substr(1, 6);  // imm[10:5]
        imm_str += instr.substr(20, 4); // imm[4:1]
        imm_str += "0";                 // imm[0] = 0

        // Sign-extend the immediate
        int sign_bit = imm_str[0] - '0';
        int32_t imm_val = stoi(imm_str, nullptr, 2);
        if (sign_bit == 1)
        {
            // Extend to the full 32 bits
            imm_val |= 0xFFFFE000; // Set the upper 19 bits to 1
        }
        ID.Imm = imm_val;

        ID.RegWrite = false;
        ID.RegDst = false;
        ID.Branch = true;
        ID.Jump = false;
        ID.MemRead = false;
        ID.MemWrite = false;
        ID.ALUSrc = false;
        bool ALU_stall_prev = false;
        bool DM_stall_prev2 = false;
        int arg1, arg2;
        if (EX.RegWrite && (EX.WriteReg == ID.RR1 || EX.WriteReg == ID.RR2) && !EX.MemtoReg) // forward last ALU one stall
        {
            ALU_stall_prev = true;
            ID.stall = true;
            return;
        }
        if (EX.RegWrite && (EX.WriteReg == ID.RR1 || EX.WriteReg == ID.RR2) && EX.MemtoReg) // forawrd last DM two stall
        {
            ID.DM_stall_prev = 1; // used at top in id.stall
            ID.stall = true;
            return;
        }
        if (DM.RegWrite && (DM.WriteReg == ID.RR1 || DM.WriteReg == ID.RR2) && !DM.MemtoReg) // forward last to last instr ALU, no stal
        {
            arg1 = DM.ALU_res;
        }
        if (DM.RegWrite && (DM.WriteReg == ID.RR1 || DM.WriteReg == ID.RR2) && DM.MemtoReg) // forward last to last DM, one stall
        {
            DM_stall_prev2 = true;
            ID.stall = true;
            return;
        }

        if (ALU_stall_prev)
        {
            arg1 = DM.ALU_res;
            ALU_stall_prev = false;
        }
        if (DM_stall_prev2)
        {
            arg1 = WB.Read_data;
            DM_stall_prev2 = false;
        }
        if (ID.DM_stall_prev == 2)
        {
            arg1 = WB.Read_data;
            ID.DM_stall_prev = 0;
        }
        // Different branch types based on funct3
        string funct3 = instr.substr(17, 3);
        if (funct3 == "000")
        {                      // BEQ: Branch if Equal
            ID.ALUOp = 3;      // SUB for comparison
            ID.BranchType = 0; // BEQ
        }
        else if (funct3 == "001")
        {                      // BNE: Branch if Not Equal
            ID.ALUOp = 3;      // SUB for comparison
            ID.BranchType = 1; // BNE
        }
        else if (funct3 == "100")
        {                      // BLT: Branch if Less Than
            ID.ALUOp = 10;     // SLT for comparison
            ID.BranchType = 2; // BLT
        }
        else if (funct3 == "101")
        {                      // BGE: Branch if Greater or Equal
            ID.ALUOp = 10;     // SLT for comparison
            ID.BranchType = 3; // BGE
        }
        else if (funct3 == "110")
        {                      // BLTU: Branch if Less Than (Unsigned)
            ID.ALUOp = 11;     // SLTU for comparison
            ID.BranchType = 4; // BLTU
        }
        else if (funct3 == "111")
        {                      // BGEU: Branch if Greater or Equal (Unsigned)
            ID.ALUOp = 11;     // SLTU for comparison
            ID.BranchType = 5; // BGEU
        }
    }
    // J-type: JAL (Jump and Link)
    else if (opcode == "1101111")
    {
        ID.WR = stoi(instr.substr(20, 5), nullptr, 2); // rd

        // J-type immediate format: imm[20|10:1|11|19:12]
        string imm_str = "";
        imm_str += instr.substr(0, 1);  // imm[20]
        imm_str += instr.substr(12, 8); // imm[19:12]
        imm_str += instr.substr(11, 1); // imm[11]
        imm_str += instr.substr(1, 10); // imm[10:1]
        imm_str += "0";                 // imm[0] = 0

        // Sign-extend the immediate
        int sign_bit = imm_str[0] - '0';
        int32_t imm_val = stoi(imm_str, nullptr, 2);
        if (sign_bit == 1)
        {
            // Extend to the full 32 bits
            imm_val |= 0xFFE00000; // Set the upper 11 bits to 1
        }
        ID.Imm = imm_val;

        ID.RegWrite = true;
        ID.RegDst = false;
        ID.Branch = false;
        ID.Jump = true;
        ID.MemRead = false;
        ID.MemWrite = false;
        ID.ALUSrc = false;
        ID.ALUOp = 2; // Addition for PC+4
        ID.MemtoReg = false;
        ID.JumpAndLink = true;
    }
    // I-type: JALR (Jump and Link Register)
    else if (opcode == "1100111" && instr.substr(17, 3) == "000")
    {
        ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2); // rs1
        ID.WR = stoi(instr.substr(20, 5), nullptr, 2);  // rd

        // Extract and sign-extend the immediate
        string imm_str = instr.substr(0, 12);
        int sign_bit = imm_str[0] - '0';
        int32_t imm_val = stoi(imm_str, nullptr, 2);
        if (sign_bit == 1)
        {
            imm_val |= 0xFFFFF000;
        }
        ID.Imm = imm_val;

        ID.RegWrite = true;
        ID.RegDst = false;
        ID.Branch = false;
        ID.Jump = true;
        ID.MemRead = false;
        ID.MemWrite = false;
        ID.ALUSrc = true;
        ID.ALUOp = 2; // Addition for base + offset
        ID.MemtoReg = false;
        ID.JumpAndLink = true;
        ID.JumpReg = true;
    }
}

// EX (ALU) stage.
// In addition to performing the ALU operation, we check for load–use hazards.
// If a hazard is detected, we insert a bubble (by setting EX.InStr = –1) and
// signal the IF and ID stages to stall.
void process_EX()
{
    // If no valid instruction, pass a bubble.
    if (ID.InStr == -1 || ID.stall)
    {
        EX.InStr = -1;
        // Clear control signals to avoid persistent hazard.
        EX.MemRead = false;
        EX.MemWrite = false;
        EX.RegWrite = false;
        EX.MemtoReg = false;
        EX.Branch = false;
        EX.Jump = false;

        return;
    }

    // Hazard detection: if DM holds a load whose destination is used by the current instruction.
    bool loadHazard = false;
    if (DM.RegWrite && DM.MemtoReg)
    {
        if (DM.WriteReg == ID.RR1 || (!ID.ALUSrc && DM.WriteReg == ID.RR2))
        {
            loadHazard = true;
        }
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
    bool branchHazard = false;

    // if (ID.Branch)
    // {
    //     cout << ID.RR1 << " " << ID.RR2 << endl;
    //     cout << EX.WriteReg << endl;
    //     // Case 1: Hazard from EX stage (e.g., a load in EX)
    //     if (EX.RegWrite && (EX.WriteReg == ID.RR1 || EX.WriteReg == ID.RR2))
    //     {
    //         branchHazard = true;
    //         cout << "ID stage detected branch hazard from EX stage at instruction "
    //              << ID.InStr << ". Inserting bubble." << endl;
    //     }
    //     // Case 2: Hazard from DM stage (e.g., a load in DM)
    //     else if (DM.RegWrite && (DM.WriteReg == ID.RR1 || DM.WriteReg == ID.RR2))
    //     {
    //         branchHazard = true;
    //         cout << "ID stage detected branch hazard from DM stage at instruction "
    //              << ID.InStr << ". Inserting bubble." << endl;
    //     }
    // }

    if (loadHazard || branchHazard)
    {
        cout << "EX stage detected load-use hazard at instruction " << ID.InStr
             << ". Inserting bubble." << endl;
        // Insert a bubble by setting EX.InStr to -1.
        EX.InStr = -1;
        // Signal the ID stage to stall for one cycle.
        ID.stall = true;
        return;
    }
    int arg1 = ID.RD1;
    if (DM.RegWrite && DM.WriteReg == ID.RR1)
        arg1 = (DM.MemtoReg ? DM.Read_data : DM.ALU_res);
    else if (WB.RegWrite && WB.WriteReg == ID.RR1)
        arg1 = (WB.MemtoReg ? WB.Read_data : WB.ALU_res);

    int arg2 = ID.ALUSrc ? ID.Imm : ID.RD2;
    if (!ID.ALUSrc)
    {
        if (DM.RegWrite && DM.WriteReg == ID.RR2)
            arg2 = (DM.MemtoReg ? DM.Read_data : DM.ALU_res);
        else if (WB.RegWrite && WB.WriteReg == ID.RR2)
            arg2 = (WB.MemtoReg ? WB.Read_data : WB.ALU_res);
    }

    switch (ID.ALUOp)
    {
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

void process_MEM()
{
    if (EX.InStr == -1)
    {
        DM.InStr = -1;
        // Clear control signals to avoid persistent hazard.
        DM.MemRead = false;
        DM.MemWrite = false;
        DM.RegWrite = false;
        DM.MemtoReg = false;
        DM.Address = 0;

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

    if (DM.MemRead)
    {
        DM.Read_data = MEM[DM.Address];
    }
    else if (DM.MemWrite)
    {
        MEM[DM.Address] = DM.Write_data;
    }
}

void process_WB()
{
    if (DM.InStr == -1)
    {
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

    if (WB.RegWrite)
    {
        RegFile[WB.WriteReg].value = (WB.MemtoReg ? WB.Read_data : WB.ALU_res);
    }
}
