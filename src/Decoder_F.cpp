#include "Processor.hpp"
#include <string>
#include <iostream>
#include <cstdint>
using namespace std;


void Decoder_F(string opcode, string instr)
{
    bool temp = false;
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
            ID.RR2 = -1;
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
            ID.RR2 = -1;
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
            ID.RR2 = -1;
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
            ID.RR2 = -1;
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
            ID.RR2 = -1;
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
            ID.RR2 = -1;
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
            ID.RR2 = -1;
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
            ID.RR2 = -1;
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
            ID.RR2 = -1;
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
    // LUI: Load Upper Immediate (U-type instruction)
    else if (opcode == "0110111")
    {
        ID.RR1 = -1;
        ID.RR2 = -1;
        ID.WR = stoi(instr.substr(20, 5), nullptr, 2);

        // Extract the 20-bit immediate and shift left by 12 bits
        string imm_str = instr.substr(0, 20);
        int32_t imm_val = stoi(imm_str, nullptr, 2) << 12;
        ID.Imm = imm_val;

        ID.RegWrite = true;
        ID.RegDst = false;
        ID.Branch = false;
        ID.Jump = false;
        ID.MemRead = false;
        ID.MemWrite = false;
        ID.ALUSrc = true;  // Uses immediate value
        ID.ALUOp = 20;     // LUI operation (likely pass immediate)
        ID.MemtoReg = false;
    }
    else if (opcode == "0110011" && instr.substr(0, 7) == "0000001")
    {
        // MUL: funct3 = 000
        if (instr.substr(17, 3) == "000")
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
            ID.ALUOp = 12; // MUL
            ID.MemtoReg = false;
        }
        // MULH: Signed x Signed, Upper product, funct3 = 001
        else if (instr.substr(17, 3) == "001")
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
            ID.ALUOp = 13; // MULH
            ID.MemtoReg = false;
        }
        // MULHU: Unsigned x Unsigned, Upper product, funct3 = 011
        else if (instr.substr(17, 3) == "011")
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
            ID.ALUOp = 14; // MULHU
            ID.MemtoReg = false;
        }
        // MULHSU: Signed x Unsigned, Upper product, funct3 = 010
        else if (instr.substr(17, 3) == "010")
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
            ID.ALUOp = 15; // MULHSU
            ID.MemtoReg = false;
        }
    }
    // Division and Remainder Instructions 
    // (R-type, opcode = 0110011, funct7 = 0000001)
    else if (opcode == "0110011" && instr.substr(0, 7) == "0000001")
    {
        // DIV: Signed division, funct3 = 100
        if (instr.substr(17, 3) == "100")
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
            ID.ALUOp = 16; // DIV
            ID.MemtoReg = false;
        }
        // DIVU: Unsigned division, funct3 = 101
        else if (instr.substr(17, 3) == "101")
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
            ID.ALUOp = 17; // DIVU
            ID.MemtoReg = false;
        }
        // REM: Signed remainder, funct3 = 110
        else if (instr.substr(17, 3) == "110")
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
            ID.ALUOp = 18; // REM
            ID.MemtoReg = false;
        }
        // REMU: Unsigned remainder, funct3 = 111
        else if (instr.substr(17, 3) == "111")
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
            ID.ALUOp = 19; // REMU
            ID.MemtoReg = false;
        }
    }
    // I-type load instructions
    else if (opcode == "0000011")
    {
        ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2); // base register
        ID.RR2 = -1;
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
        //cout << "branch";
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
        int arg1 = RegFile[ID.RR1].value, arg2 = RegFile[ID.RR2].value;

        if (EX.RegWrite && (EX.WriteReg == ID.RR1 || EX.WriteReg == ID.RR2) && !EX.MemtoReg) // forward last ALU one stall
        {
            ID.ALU_stall_prev = true;
            IF.stall = true;
            ID.InStr = -1;
            //cout << "hi";
            return;
        }
        if (EX.RegWrite && (EX.WriteReg == ID.RR1 || EX.WriteReg == ID.RR2) && EX.MemtoReg) // forawrd last DM two stall
        {
            ID.DM_stall_prev = 1; // used at top in id.stall
            ID.stall = true;
            IF.stall = true;
            ID.InStr = -1;
            return;
        }
        if (DM.RegWrite && (DM.WriteReg == ID.RR1 || DM.WriteReg == ID.RR2) && !DM.MemtoReg) // forward last to last instr ALU, no stall
        {
            if (DM.WriteReg == ID.RR1)
                arg1 = DM.ALU_res;
            else
                arg2 = DM.ALU_res;
        }
        if (DM.RegWrite && (DM.WriteReg == ID.RR1 || DM.WriteReg == ID.RR2) && DM.MemtoReg) // forward last to last DM, one stall
        {
            ID.DM_stall_prev2 = true;
            IF.stall = true;
            ID.InStr = -1;
            return;
        }

        if (ID.ALU_stall_prev)
        {
            //cout << "hi";
            if (DM.WriteReg == ID.RR1)
                arg1 = DM.ALU_res;
            else
                arg2 = DM.ALU_res;
            ID.ALU_stall_prev = false;
        }
        if (ID.DM_stall_prev2)
        {
            if (DM.WriteReg == ID.RR1)
                arg1 = WB.Read_data;
            else
                arg2 = WB.Read_data;
            ID.DM_stall_prev2 = false;
        }
        if (ID.DM_stall_prev == 1)
        {
            //cout << "hi2";
            if (WB.WriteReg == ID.RR1)
                arg1 = WB.Read_data;
            else
                arg2 = WB.Read_data;
            ID.DM_stall_prev = 0;
        }
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
        //cout << "hi";
        switch (ID.BranchType)
        {
        case 0: // BEQ: Branch if Equal
            IF.branch = (arg1 == arg2);
            break;
        case 1: // BNE: Branch if Not Equal
            IF.branch = (arg1 != arg2);
            break;
        case 2: // BLT: Branch if Less Than (signed)
            IF.branch = (arg1 < arg2);
            break;
        case 3: // BGE: Branch if Greater or Equal (signed)
            IF.branch = (arg1 >= arg2);
            break;
        case 4: // BLTU: Branch if Less Than (unsigned)
            IF.branch = ((unsigned)arg1 < (unsigned)arg2);
            break;
        case 5: // BGEU: Branch if Greater or Equal (unsigned)
            IF.branch = ((unsigned)arg1 >= (unsigned)arg2);
            break;
        default:
            cout << "Unknown branch type encountered!" << endl;
            break;
        }
        if (IF.branch == 1)
            IF.branchPC = IF.PC + (ID.Imm / 4) - 1;
            //cout << "yes" << " " << IF.branchPC << endl;
    }
    // J-type: JAL (Jump and Link)
    else if (opcode == "1101111")
    {
        ID.RR1 = -1;
        ID.RR2 = -1;
        ID.WR = stoi(instr.substr(20, 5), nullptr, 2); // rd
        // Extract the 20-bit immediate: imm[20|19:12|11|10:1]
        string imm_str = instr.substr(0, 1) + instr.substr(12, 8) + instr.substr(11, 1) + instr.substr(1, 10);
        int32_t imm_val = stoi(imm_str, nullptr, 2);
        if (imm_str[0] == '1')
        {
            imm_val -= (1 << 20); // Correctly sign-extend by subtracting 2^20
        }
        ID.Imm = imm_val << 1; // Shift left by 1 to get byte offset
        // if (ID.WR != 0)
        //     RegFile[ID.WR].value = IF.PC; // Save the return address in rd
        //cout << ID.Imm << endl;
        IF.branchPC = IF.PC + ID.Imm / 4 - 1; // Set jump target (instruction index)
        IF.branch = 1;
        
        temp = true; // for WB write
        ID.RegWrite = true;
        ID.Jump = false;
        ID.Branch = false;
        ID.MemRead = false;
        ID.MemWrite = false;
        ID.ALUSrc = true;
        ID.ALUOp = 2; // No ALU op needed
        ID.MemtoReg = false;
        ID.JumpAndLink = false; // Optional
    }
    else if (opcode == "1100111" && instr.substr(17, 3) == "000")
    {
        ID.RR1 = stoi(instr.substr(12, 5), nullptr, 2); // rs1
        ID.RR2 = -1;
        ID.WR = stoi(instr.substr(20, 5), nullptr, 2);  // rd
        string imm_str = instr.substr(0, 12);
        int32_t imm_val = stoi(imm_str, nullptr, 2);
        if (imm_str[0] == '1')
        {
            imm_val |= 0xFFFFF000;
        }
        ID.Imm = imm_val;
        //cout << ID.Imm << "gi" << endl;
        int arg1 = RegFile[ID.RR1].value;

        if (EX.RegWrite && (EX.WriteReg == ID.RR1) && !EX.MemtoReg) // forward last ALU one stall
        {
            ID.ALU_stall_prev = true;
            IF.stall = true;
            ID.InStr = -1;
            //cout << "hi";
            return;
        }
        if (EX.RegWrite && (EX.WriteReg == ID.RR1) && EX.MemtoReg) // forawrd last DM two stall
        {
            ID.DM_stall_prev = 1; // used at top in id.stall
            ID.stall = true;
            IF.stall = true;
            ID.InStr = -1;
            return;
        }
        if (DM.RegWrite && (DM.WriteReg == ID.RR1) && !DM.MemtoReg) // forward last to last instr ALU, no stall
        {
            if (DM.WriteReg == ID.RR1)
                arg1 = DM.ALU_res;
        }
        if (DM.RegWrite && (DM.WriteReg == ID.RR1) && DM.MemtoReg) // forward last to last DM, one stall
        {
            ID.DM_stall_prev2 = true;
            IF.stall = true;
            ID.InStr = -1;
            return;
        }

        if (ID.ALU_stall_prev)
        {
            //cout << "hi";
            arg1 = DM.ALU_res;
            ID.ALU_stall_prev = false;
        }
        if (ID.DM_stall_prev2)
        {
           arg1 = WB.Read_data;
            ID.DM_stall_prev2 = false;
        }
        if (ID.DM_stall_prev == 1)
        {
            //cout << "hi2";
            arg1 = WB.Read_data;
            ID.DM_stall_prev = 0;
        }

        IF.branchPC = arg1 + ID.Imm/4;  // Jump target
        IF.branch = 1;
        // if (ID.WR != 0)
        //     RegFile[ID.WR].value = IF.PC; // Save the return address in rd

        ID.RegWrite = true;  // Write PC + 4 to rd
        ID.Jump = false;      // Jump instruction
        ID.Branch = false;
        ID.MemRead = false;
        ID.MemWrite = false;
        ID.ALUSrc = true;    // ALU uses immediate
        ID.ALUOp = 2;        // Addition for rs1 + imm
        ID.MemtoReg = false;
        ID.JumpAndLink = false; // Optional, for consistency with JAL
        ID.JumpReg = false;   // Optional, to indicate register-based jump
    }
    ID.RD1 = RegFile[max(0, ID.RR1)].value;
    ID.RD2 = RegFile[max(0, ID.RR2)].value;
    if (temp){    // wb of jal jalr
        ID.Imm = 0;
        ID.RD1 = IF.PC; 
    }
    //cout << ID.RR1 << " " << ID.RD1 << " " << ID.RR2 << " " << ID.RD2 << " " << ID.Imm << endl;
    if (ID.WR == 0) ID.RegWrite = false;
}