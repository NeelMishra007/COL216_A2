#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <bitset>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include "Decoder_F.hpp"
#include "Processor.hpp"

using namespace std;


const int N = 2000005;
int MEM[N];

Register RegFile[32];
IFStage IF;
IDStage ID;
EXStage EX;
MEMStage DM;
WBStage WB;

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
    ID = {.RR1 = 0, .RR2 = 0, .WR = 0, .RD1 = 0, .RD2 = 0, .Imm = 0, .RegWrite = false, .RegDst = false, .Branch = false, .Jump = false, .MemRead = false, .MemWrite = false, .ALUSrc = false, .ALUOp = 0, .MemtoReg = false, .stall = false, .InStr = -1, .DM_stall_prev = 0};
    EX = {0, false, 0, 0, 0, false, false, false, false, false, false, -1, false};
    DM = {0, 0, 0, false, false, false, false, 0, 0, -1, false};
    WB = {false, false, 0, 0, 0, -1, false};
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

        cout << "Cycle " << cycle << ": IF:" << IF.InStr << " ID:" << ID.InStr << " EX:" << EX.InStr << " MEM:" << DM.InStr << " WB:" << WB.InStr << endl;
    }
    string output_filename = "../outputfiles/_forward_out.txt";
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
    // cout << opcode;
    Decoder_F(opcode, instr);
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
    bool loadHazard = false;
    if (DM.RegWrite && DM.MemtoReg)
    {
        if (DM.WriteReg == ID.RR1 || (!ID.ALUSrc && DM.WriteReg == ID.RR2))
        {
            loadHazard = true;
        }
    }

    if (loadHazard)
    {
        // cout << "EX stage detected load-use hazard at instruction " << ID.InStr << ". Inserting bubble." << endl;
        EX.InStr = -1;
        ID.stall = true;
        return;
    }

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

    EX.MemSize = ID.MemSize;
    EX.MemSignExtend = ID.MemSignExtend;

    // For store instructions, prepare the write data and check for forwarding.
    EX.WriteData = ID.RD2;
    EX.WriteDataReg = ID.RR2;
    if (EX.MemWrite == true) // Forwarding for DM last-to-last instruction.
    {
        if (WB.RegWrite == true && WB.WriteReg == EX.WriteDataReg)
        {
            if (WB.MemtoReg == true)
                EX.WriteData = WB.Read_data;
            else
                EX.WriteData = WB.ALU_res;
        }
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
    case 2:  // ADD (also used for address calculation)
        EX.ALU_res = arg1 + arg2;
        break;
    case 3:  // SUB
        EX.ALU_res = arg1 - arg2;
        break;
    case 4:  // AND
        EX.ALU_res = arg1 & arg2;
        break;
    case 5:  // OR
        EX.ALU_res = arg1 | arg2;
        break;
    case 6:  // XOR
        EX.ALU_res = arg1 ^ arg2;
        break;
    case 7:  // SLL (Shift Left Logical)
        EX.ALU_res = arg1 << arg2;
        break;
    case 8:  // SRL (Shift Right Logical)
        EX.ALU_res = (unsigned int)arg1 >> arg2;
        break;
    case 9:  // SRA (Shift Right Arithmetic)
        EX.ALU_res = arg1 >> arg2;
        break;
    case 10: // SLT (Set Less Than, signed)
        EX.ALU_res = (arg1 < arg2) ? 1 : 0;
        break;
    case 11: // SLTU (Set Less Than Unsigned)
        EX.ALU_res = ((unsigned int)arg1 < (unsigned int)arg2) ? 1 : 0;
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
            if (DM.MemSignExtend)
                DM.Read_data = (int8_t)byte_val; // Sign extend
            else
                DM.Read_data = byte_val; // Zero extend
            break;
        }
        case 2: // Halfword
        {
            uint16_t halfword_val = *((uint16_t*)&MEM[DM.Address]);
            if (DM.MemSignExtend)
                DM.Read_data = (int16_t)halfword_val; // Sign extend
            else
                DM.Read_data = halfword_val; // Zero extend
            break;
        }
        case 4: // Word
            DM.Read_data = *((int32_t*)&MEM[DM.Address]);
            break;
        }
    }
    else if (DM.MemWrite == true)
    {
        // Restore the original forwarding logic for store instructions
        if (WB.RegWrite == true && WB.WriteReg == EX.WriteDataReg)
        { // previous instruction
            if (WB.MemtoReg == true)
            {
                switch (DM.MemSize)
                {
                case 1: // Store Byte
                    MEM[DM.Address] = WB.Read_data & 0xFF;
                    break;
                case 2: // Store Halfword
                    *((uint16_t*)&MEM[DM.Address]) = WB.Read_data & 0xFFFF;
                    break;
                case 4: // Store Word
                    *((int32_t*)&MEM[DM.Address]) = WB.Read_data;
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
                    *((uint16_t*)&MEM[DM.Address]) = WB.ALU_res & 0xFFFF;
                    break;
                case 4: // Store Word
                    *((int32_t*)&MEM[DM.Address]) = WB.ALU_res;
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
                *((uint16_t*)&MEM[DM.Address]) = DM.Write_data & 0xFFFF;
                break;
            case 4: // Store Word
                *((int32_t*)&MEM[DM.Address]) = DM.Write_data;
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
        //cout << WB.WriteReg << " " << RegFile[WB.WriteReg].value << endl;
    }
}