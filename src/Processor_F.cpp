#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <bitset>

using namespace std;

const int N = 2e6 + 5;
int MEM[N];
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
    int value;
} Register;

typedef struct
{
    int PC;
    bool PCSrc;

    int InStr = -1;
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

    int InStr = -1;

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

    int InStr = -1;
    int stalls = 0;
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

    int InStr = -1;
    int stalls = 0;
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

    int InStr = -1;
    int stalls = 0;
} WB;

int main(int argv, char **argc)
{
    for (int i = 0; i < 32; i++)
    {
        RegFile[i].value = 0;
    }

    ifstream file(argc[1]);
    if (!file)
    {
        cerr << "Error: Unable to open input.txt" << endl;
        return 1;
    }

    vector<string> instructions_hex;
    vector<string> instructions_print;

    string instr;
    while (getline(file, instr))
    {
        vector<string> words = splitLine(instr);
        instructions_hex.push_back(words[1]);
        instructions_print.push_back(words[2]);
    }

    IF IF = {0, false, -1};
    ID ID = {0, 0, 0, 0, 0, false, -1};
    ALU ALU = {0, false, false, false, -1};
    DM DM = {0, 0, 0, false, false, -1};
    WB WB = {false, -1};

    vector<int> Output[5];

    int num = atoi(argc[2]);

    for (int i = 0; i < num; i++)
    {
        if (WB.InStr != -1)
        {
            Output[4].push_back(WB.InStr);

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
        if (DM.InStr != -1)
        {
            Output[3].push_back(DM.InStr);

            if (DM.MemRead == true)
            {
                DM.Read_data = MEM[DM.Address];
            }
            else
            {
                MEM[DM.Address] = DM.Write_data;
            }

            WB.Read_data = DM.Read_data;
            WB.MemtoReg = DM.MemtoReg;
            WB.RegWrite = DM.RegWrite;
            WB.ALU_res = DM.ALU_res;
            WB.InStr = DM.InStr;
        }
        if (ALU.InStr != -1)
        {
            Output[2].push_back(ALU.InStr);
        }
        if (ID.InStr != -1)
        {
            Output[1].push_back(ID.InStr);
        }
        if (IF.InStr != -1)
        {
            Output[0].push_back(IF.InStr);
        }
    }

    file.close();
}

void process_IF(IF &IF, ID &ID)
{
    ID.InStr = IF.PC;
    IF.InStr = IF.PC;
    IF.PC += 1;
}

void proces_ID(ALU &ALU, ID &ID, Register RegFile[32], vector<string> instructions)
{
    string instr = instructions[ID.InStr];

    instr = hexToBin(instr);
    string opcode = instr.substr(25, 7);
    if (opcode == "0110011")
    {
        if (instr.substr(0, 7) == "0000000")
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
            ID.ALUOp = 2;
            ID.MemtoReg = false;
        }
        else if (instr.substr(0, 7) == "0100000")
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
            ID.ALUOp = 3;
        }
    }
    else if (opcode == "0000011")
    {
        ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2);
        ID.WR = stoi(instr.substr(20, 5), nullptr, 2);
        ID.Imm = stoi(instr.substr(0, 12), nullptr, 2);
        ID.RegWrite = true;
        ID.RegDst = false;
        ID.Branch = false;
        ID.Jump = false;
        ID.MemRead = true;
        ID.MemWrite = false;
        ID.ALUSrc = true;
        ID.ALUOp = 0;
        ID.MemtoReg = true;
    }
}

void process_ALU(ALU &ALU, ID &ID, WB &WB, DM &DM)
{
    if (ID.InStr == -1)
        return;
    ALU.InStr = ID.InStr;
    ALU.WriteData = ID.RD2;
    ALU.WriteDataReg = ID.RR2;
    ALU.WriteReg = ID.WR;
    ALU.Branch = ID.Branch;
    ALU.Jump = ID.Jump;
    ALU.MemRead = ID.MemRead;
    ALU.MemWrite = ID.MemWrite;
    ALU.MemtoReg = ID.MemtoReg;
    ALU.RegWrite = ID.RegWrite;

    if (ALU.MemWrite == true) // forwarding for DM LAST TO LAST INSTRUCTION THROUGH ALU
    {
        if (WB.RegWrite == true && WB.WriteReg == ALU.WriteDataReg)
        {
            if (WB.MemtoReg == true)
            {
                ALU.WriteData = WB.Read_data;
            }
            else
            {
                ALU.WriteData = WB.ALU_res;
            }
        }
    }
    if (ID.ALUOp == 0)
    {
        if (ID.ALUSrc == true)
        {
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
                        ALU.ALU_res = DM.Read_data + ID.Imm;
                }
                else
                {
                    ALU.ALU_res = DM.ALU_res + ID.Imm;
                }
            }
            else if (WB.RegWrite == true && WB.WriteReg == ID.RR1)
            {
                if (WB.MemtoReg == true)
                {
                    ALU.ALU_res = WB.Read_data + ID.Imm;
                }
                else
                {
                    ALU.ALU_res = WB.ALU_res + ID.Imm;
                }
            }
            else
            {
                ALU.ALU_res = ID.RD1 + ID.Imm;
            }
        }
        else
        {
            ALU.ALU_res = ID.RD1 + ID.RD2;
        }
    }
    else if (ID.ALUOp == 1)
    {
        ALU.ALU_res = ALU.WriteData - ID.Imm;
    }
    else if (ID.ALUOp == 2)
    {
        ALU.ALU_res = ALU.WriteData + ID.RD1;
    }
    else if (ID.ALUOp == 3)
    {
        ALU.ALU_res = ALU.WriteData - ID.RD1;
    }
    if (ALU.ALU_res == 0)
    {
        ALU.Zero = true;
    }
    else
    {
        ALU.Zero = false;
    }
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