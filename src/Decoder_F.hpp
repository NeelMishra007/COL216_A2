#ifndef DECODER_F_HPP
#define DECODER_F_HPP

#include <string>
#include "Processor.hpp" 

void Decoder_F(IDStage &ID, EXStage &EX, MEMStage &DM, WBStage &WB, std::string opcode, std::string instr);

#endif