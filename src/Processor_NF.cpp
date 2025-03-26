#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <bitset>
#include <cstdlib>
#include <cstdint>
#include "Decoder_NF.hpp"
#include "Processor.hpp"

using namespace std;

const int N = 2000005;
vector<unsigned char> MEM(N, 0);

Register RegFile[32];
IFStage IF;
IDStage ID;
EXStage EX;
MEMStage DM;
WBStage WB;

string hexToBin(const string &hex) {
    string binary;
    for (char ch : hex) {
        int val = stoi(string(1, ch), nullptr, 16);
        binary += bitset<4>(val).to_string();
    }
    return binary;
}

vector<string> splitLine(const string &line) {
    vector<string> words;
    stringstream ss(line);
    string word;
    while (ss >> word) {
        words.push_back(word);
    }
    return words;
}
string stageName(int stage) {
    switch (stage) {
        case 1: return "IF";
        case 2: return "ID";
        case 3: return "EX";
        case 4: return "MEM";
        case 5: return "WB";
        default : return "-";
    }
}

void process_IF(const vector<string> &instructions);
void process_ID(const vector<string> &instructions);
void process_EX();
void process_MEM();
void process_WB();

int main(int argc, char **argv)
{
    IF = {0, false, -1, -1, -1};
    ID = {
        .RR1 = 0, 
        .RR2 = 0, 
        .WR = 0, 
        .RD1 = 0, 
        .RD2 = 0, 
        .Imm = 0, 
        .RegWrite = false, 
        .RegDst = false, 
        .Branch = false, 
        .Jump = false, 
        .MemRead = false, 
        .MemWrite = false, 
        .ALUSrc = false, 
        .ALUOp = 0, 
        .MemtoReg = false, 
        .MemSize = 0,
        .MemSignExtend = false,
        .BranchType = 0,
        .JumpAndLink = false,
        .JumpReg = false,
        .stall = false, 
        .InStr = -1, 
        .DM_stall_prev = 0,
        .ALU_stall_prev = false,
        .DM_stall_prev2 = false
    };
    EX = {
        .ALU_res = 0, 
        .Zero = false, 
        .WriteData = 0, 
        .WriteDataReg = 0, 
        .WriteReg = 0, 
        .Branch = false, 
        .Jump = false, 
        .MemRead = false, 
        .MemWrite = false, 
        .MemtoReg = false, 
        .RegWrite = false,
        .MemSize = 0,
        .MemSignExtend = false,
        .InStr = -1, 
        .stall = false
    };
    DM = {
        .Address = 0, 
        .Write_data = 0, 
        .Read_data = 0, 
        .MemRead = false, 
        .MemWrite = false, 
        .MemtoReg = false, 
        .RegWrite = false, 
        .ALU_res = 0, 
        .WriteReg = 0,
        .MemSize = 0,
        .MemSignExtend = false,
        .InStr = -1, 
        .stall = false
    };
    WB = {
        .MemtoReg = false, 
        .RegWrite = false, 
        .Read_data = 0, 
        .ALU_res = 0, 
        .WriteReg = 0, 
        .InStr = -1, 
        .stall = false
    };
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
            string instruction;
            for (size_t i = 2; i < words.size(); i++) {
                // Stop concatenating if you hit a comment token.
                if (!words[i].empty() && words[i][0] == '#') {
                    break;
                }
                if (!instruction.empty()) {
                    instruction += " ";
                }
                instruction += words[i];
            }
            instructions_print.push_back(instruction);
            
        }
    }
    file.close();

    int total_instructions = instructions_hex.size();
    int numCycles = atoi(argv[2]);

    vector<vector<int>> Output(total_instructions, vector<int>(numCycles, -1));

    for (int cycle = 0; cycle < numCycles; cycle++)
    {

        process_WB();
        if (WB.InStr != -1 && WB.InStr < total_instructions) {
            Output[WB.InStr][cycle] = 5;
        }

        process_MEM();
        if (DM.InStr != -1 && DM.InStr < total_instructions) {
            Output[DM.InStr][cycle] = 4;
        }

        process_EX();
        if (EX.InStr != -1 && EX.InStr < total_instructions) {
            Output[EX.InStr][cycle] = 3;
        }

        process_ID(instructions_hex);
        if (ID.InStr != -1 && ID.InStr < total_instructions) {
            Output[ID.InStr][cycle] = 2;
        }

        process_IF(instructions_hex);
        if (IF.InStr != -1 && IF.InStr < total_instructions) {
            Output[IF.InStr][cycle] = 1;
        }

        //cout << "Cycle " << cycle << ": IF:" << IF.InStr << " ID:" << ID.InStr << " EX:" << EX.InStr << " MEM:" << DM.InStr << " WB:" << WB.InStr << endl;
    }
    string output_filename = "../outputfiles/_noforward_out.txt";
    ofstream outfile(output_filename);
    if (!outfile) {
        cerr << "Error: Unable to open output file " << output_filename << endl;
        return 1;
    }
    
    // Redirect cout to the file
    streambuf* orig_cout = cout.rdbuf(outfile.rdbuf());
    
    // Rest of the code remains the same...
    
    for (int i = 0; i < total_instructions; i++) {
        outfile << instructions_print[i];
        int lastCycle = -1;
        for (int cycle = 0; cycle < numCycles; cycle++) {
            if (Output[i][cycle] != -1) {
                lastCycle = cycle;
            }
        }    
        bool flag = true;
        for (int cycle = 0; cycle <= lastCycle; cycle++) {
            if (Output[i][cycle] == 5) flag = true;
            else if (Output[i][cycle] != -1) flag = false;
            
            if (Output[i][cycle] == 5) {
                outfile << ";" << "WB";
            }
            else if (flag) outfile << ";" << " ";
            else if (Output[i][cycle] == -1 || Output[i][cycle] == Output[i][cycle - 1]) outfile << ";" << "-";
            else outfile << ";" << stageName(Output[i][cycle]);
        }
        outfile << endl;
    }
    
    // Restore original cout
    cout.rdbuf(orig_cout);
    outfile.close();

}
void process_IF(const vector<string> &instructions)
{
    if (IF.stall)
    {
        //cout << "IF stage stalled; PC remains at " << IF.PC << endl;
        IF.stall = false;
        return;
    }
    if (IF.PC < instructions.size())
        IF.InStr = IF.PC;
    else
        IF.InStr = -1;
    if (IF.branch == 2)
    {
        IF.PC -= 1;
        IF.InStr = IF.PC;
        IF.branch = -1;
    }
    if (IF.branch == 3)
    {
        IF.PC = IF.branchPC;
        IF.InStr = IF.PC;
        IF.branch = -1;
        IF.branchPC = -1;
        //cout << IF.PC << endl;
    }
    if (IF.branch == 0)
    {
        IF.branch = 2;
    }
    if (IF.branch == 1)
    {
        IF.branch = 3;
    }

    IF.PC++;
}

void process_ID(const vector<string> &instructions)
{
    if (ID.stall)
    {
        //cout << "ID stage stalled; holding instruction " << ID.InStr << endl;
        ID.stall = false;
        IF.stall = true;
        return;
    }
    if (IF.InStr == -1 || IF.InStr >= instructions.size())
    {
        ID.InStr = -1;
        return;
    }
    if (IF.branch == 2 || IF.branch == 3)
    {
        ID.InStr = -1;
        return;
    }
    ID.InStr = IF.InStr;

    string instr = instructions[ID.InStr];
    instr = hexToBin(instr);

    string opcode = instr.substr(25, 7);

    Decoder_NF(IF, ID, EX, DM, WB, opcode, instr);
}

void process_EX()
{
    if (ID.InStr == -1 || ID.stall)
    {
        EX.InStr = -1;
        EX.MemRead = false;
        EX.MemWrite = false;
        EX.RegWrite = false;
        EX.MemtoReg = false;
        EX.Branch = false;
        EX.Jump = false;

        return;
    }

    EX.InStr = ID.InStr;
    EX.WriteReg = ID.WR;

    //cout << EX.WriteReg << endl;
    EX.Branch = ID.Branch;
    EX.Jump = ID.Jump;
    EX.MemRead = ID.MemRead;
    EX.MemWrite = ID.MemWrite;
    EX.MemtoReg = ID.MemtoReg;
    EX.RegWrite = ID.RegWrite;
    EX.WriteData = ID.RD2;
    EX.WriteDataReg = ID.RR2;

    EX.MemSize = ID.MemSize;
    EX.MemSignExtend = ID.MemSignExtend;

    int arg1 = ID.RD1;
    int arg2 = ID.ALUSrc ? ID.Imm : ID.RD2;

    
    switch (ID.ALUOp)
    {
    case 2: // ADD (also used for address calculation)
        EX.ALU_res = arg1 + arg2;
        break;
    case 3: // SUB
        EX.ALU_res = arg1 - arg2;
        break;
    case 4: // AND
        EX.ALU_res = arg1 & arg2;
        break;
    case 5: // OR
        EX.ALU_res = arg1 | arg2;
        break;
    case 6: // XOR
        EX.ALU_res = arg1 ^ arg2;
        break;
    case 7: // SLL (Shift Left Logical)
        EX.ALU_res = arg1 << arg2;
        break;
    case 8: // SRL (Shift Right Logical)
        EX.ALU_res = (unsigned int)arg1 >> arg2;
        break;
    case 9: // SRA (Shift Right Arithmetic)
        EX.ALU_res = arg1 >> arg2;
        break;
    case 10: // SLT (Set Less Than, signed)
        EX.ALU_res = (arg1 < arg2) ? 1 : 0;
        break;
    case 11: // SLTU (Set Less Than Unsigned)
        EX.ALU_res = ((unsigned int)arg1 < (unsigned int)arg2) ? 1 : 0;
        break;
    case 12: // MUL (Signed multiplication, lower 32 bits)
        EX.ALU_res = (int32_t)(arg1 * arg2);
        break;
    case 13: // MULH (Signed x Signed, upper 32 bits)
        EX.ALU_res = (int32_t)((int64_t)arg1 * (int64_t)arg2 >> 32);
        break;
    case 14: // MULHU (Unsigned x Unsigned, upper 32 bits)
        EX.ALU_res = (uint32_t)((uint64_t)arg1 * (uint64_t)arg2 >> 32);
        break;
    case 15: // MULHSU (Signed x Unsigned, upper 32 bits)
        EX.ALU_res = (int32_t)((int64_t)arg1 * (uint64_t)arg2 >> 32);
        break;
    case 16: // DIV (Signed division)
        if (arg2 == 0) {
            // Handle division by zero (implementation-specific)
            EX.ALU_res = -1; // or some other error value
        } else {
            EX.ALU_res = arg1 / arg2;
        }
        break;
    case 17: // DIVU (Unsigned division)
        if (arg2 == 0) {
            // Handle division by zero (implementation-specific)
            EX.ALU_res = (unsigned int)-1; // or some other error value
        } else {
            EX.ALU_res = (unsigned int)arg1 / (unsigned int)arg2;
        }
        break;
    case 18: // REM (Signed remainder)
        if (arg2 == 0) {
            // Handle division by zero (implementation-specific)
            EX.ALU_res = arg1; // or some other error value
        } else {
            EX.ALU_res = arg1 % arg2;
        }
        break;
    case 19: // REMU (Unsigned remainder)
        if (arg2 == 0) {
            // Handle division by zero (implementation-specific)
            EX.ALU_res = (unsigned int)arg1; // or some other error value
        } else {
            EX.ALU_res = (unsigned int)arg1 % (unsigned int)arg2;
        }
        break;
    case 20: // LUI (Load Upper Immediate)
        EX.ALU_res = arg2; // Pass through immediate value
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
        DM.MemRead = false;
        DM.MemWrite = false;
        DM.RegWrite = false;
        DM.MemtoReg = false;
        DM.Address = 0;

        return;
    }
    
    DM.InStr = EX.InStr;
    DM.ALU_res = EX.ALU_res;
    DM.Write_data = EX.WriteData;
    DM.WriteReg = EX.WriteReg;
    DM.MemRead = EX.MemRead;
    DM.MemWrite = EX.MemWrite;
    DM.MemtoReg = EX.MemtoReg;
    DM.RegWrite = EX.RegWrite;
    DM.Address = EX.ALU_res;
    
    // Additional memory size and sign extend information
    DM.MemSize = EX.MemSize;
    DM.MemSignExtend = EX.MemSignExtend;

    if (DM.MemRead)
    {
        // Load operations with different memory sizes and sign extension
        switch (DM.MemSize)
        {
        case 1: // Byte
        {
            uint8_t byte_val = MEM[DM.Address];
            DM.Read_data = DM.MemSignExtend ? (int8_t)byte_val : byte_val;
            break;
        }
        case 2: // Halfword (16-bit)
        {
            uint16_t halfword_val = MEM[DM.Address] | (MEM[DM.Address + 1] << 8);
            DM.Read_data = DM.MemSignExtend ? (int16_t)halfword_val : halfword_val;
            break;
        }
        case 4: // Word (32-bit)
        {
            DM.Read_data = MEM[DM.Address] |
                            (MEM[DM.Address + 1] << 8) |
                            (MEM[DM.Address + 2] << 16) |
                            (MEM[DM.Address + 3] << 24);
            break;
        }
        }
    }
    else if (DM.MemWrite)
    {
        // Forwarding logic for store instructions
        if (WB.RegWrite && WB.WriteReg == EX.WriteDataReg)
        {
            if (WB.MemtoReg)
            {
                switch (DM.MemSize)
                {
                case 1: // Store Byte
                    MEM[DM.Address] = WB.Read_data & 0xFF;
                    break;
                case 2: // Store Halfword
                    MEM[DM.Address]     = WB.Read_data & 0xFF;
                    MEM[DM.Address + 1] = (WB.Read_data >> 8) & 0xFF;
                    break;
                case 4: // Store Word
                    MEM[DM.Address]     = WB.Read_data & 0xFF;
                    MEM[DM.Address + 1] = (WB.Read_data >> 8) & 0xFF;
                    MEM[DM.Address + 2] = (WB.Read_data >> 16) & 0xFF;
                    MEM[DM.Address + 3] = (WB.Read_data >> 24) & 0xFF;
                    break;
                }
            }
            else
            {
                switch (DM.MemSize)
                {
                case 1: // Store Byte
                    MEM[DM.Address] = WB.ALU_res & 0xFF;
                    break;
                case 2: // Store Halfword
                    MEM[DM.Address]     = WB.ALU_res & 0xFF;
                    MEM[DM.Address + 1] = (WB.ALU_res >> 8) & 0xFF;
                    break;
                case 4: // Store Word
                    MEM[DM.Address]     = WB.ALU_res & 0xFF;
                    MEM[DM.Address + 1] = (WB.ALU_res >> 8) & 0xFF;
                    MEM[DM.Address + 2] = (WB.ALU_res >> 16) & 0xFF;
                    MEM[DM.Address + 3] = (WB.ALU_res >> 24) & 0xFF;
                    break;
                }
            }
        }
        else
        {
            // Original store logic
            switch (DM.MemSize)
            {
            case 1: // Store Byte
                MEM[DM.Address] = DM.Write_data & 0xFF;
                break;
            case 2: // Store Halfword
                MEM[DM.Address]     = DM.Write_data & 0xFF;
                MEM[DM.Address + 1] = (DM.Write_data >> 8) & 0xFF;
                break;
            case 4: // Store Word
                MEM[DM.Address]     = DM.Write_data & 0xFF;
                MEM[DM.Address + 1] = (DM.Write_data >> 8) & 0xFF;
                MEM[DM.Address + 2] = (DM.Write_data >> 16) & 0xFF;
                MEM[DM.Address + 3] = (DM.Write_data >> 24) & 0xFF;
                break;
            }
        }
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
        cout << WB.WriteReg << " " << RegFile[WB.WriteReg].value << endl;
        
    }
}