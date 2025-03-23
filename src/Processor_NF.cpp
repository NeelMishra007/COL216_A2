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
int MEM[N];

typedef struct {
    int value;
} Register;

Register RegFile[32];

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

void process_IF(const vector<string> &instructions);
void process_ID(const vector<string> &instructions);
void process_EX();
void process_MEM();
void process_WB();

IFStage IF = {0, false, -1};
IDStage ID = {.RR1 = 0, .RR2 = 0, .WR = 0, .RD1 = 0, .RD2 = 0, .Imm = 0, .RegWrite = false, .RegDst = false, .Branch = false, .Jump = false, .MemRead = false, .MemWrite = false, .ALUSrc = false, .ALUOp = 0, .MemtoReg = false, .stall = false, .InStr = -1, .DM_stall_prev = 0};

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
                IF.PC = IF.PC + (ID.Imm / 4) - 2;
                cout << "Branch taken, new PC: " << IF.PC << endl;
                IF.InStr = -1; 
                ID.Branch = false;
            }
        }

        process_ID(instructions_hex);
        if (ID.InStr != -1 && ID.InStr < total_instructions && Output[1][ID.InStr] == -1)
            Output[1][ID.InStr] = cycle;

        process_IF(instructions_hex);
        if (IF.InStr != -1 && IF.InStr < total_instructions && Output[0][IF.InStr] == -1)
            Output[0][IF.InStr] = cycle;

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
void process_IF(const vector<string> &instructions)
{
    if (IF.stall)
    {
        cout << "IF stage stalled; PC remains at " << IF.PC << endl;
        IF.stall = false; 
        return;
    }
    if (IF.PC < instructions.size())
        IF.InStr = IF.PC;
    else
        IF.InStr = -1;
    IF.PC++;
}

void process_ID(const vector<string> &instructions)
{
    if (ID.stall)
    {
        cout << "ID stage stalled; holding instruction " << ID.InStr << endl;

        ID.stall = false;
        IF.stall = true;
        if (ID.DM_stall_prev == 1)
        {
            ID.DM_stall_prev = 2;
            DM.stall = true;
        }
        return;
    }
    int DM_stall_prev = 0;
    if (IF.InStr == -1 || IF.InStr >= instructions.size())
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
    EX.Branch = ID.Branch;
    EX.Jump = ID.Jump;
    EX.MemRead = ID.MemRead;
    EX.MemWrite = ID.MemWrite;
    EX.MemtoReg = ID.MemtoReg;
    EX.RegWrite = ID.RegWrite;
    EX.WriteData = ID.RD2;
    EX.WriteDataReg = ID.RR2;

    int arg1 = ID.RD1;
    int arg2 = ID.ALUSrc ? ID.Imm : ID.RD2;

    
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

    if (DM.MemWrite) {
        MEM[DM.Address] = DM.Write_data;
    }
    
    if (DM.MemRead) {
        DM.Read_data = MEM[DM.Address];
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