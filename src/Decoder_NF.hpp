#ifndef DECODER_NF_HPP
#define DECODER_NF_HPP

#include <string>
#include "Processor.hpp" 

void Decoder_NF(IFStage &IF, IDStage &ID, EXStage &EX, MEMStage &DM, WBStage &WB, std::string opcode, std::string instr);

#endif