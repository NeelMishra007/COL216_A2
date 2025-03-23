#ifndef DECODER_HPP
#define DECODER_HPP

#include <string>
#include "Processor_F.hpp" 

void Decoder(IDStage &ID, EXStage &EX, MEMStage &DM, WBStage &WB, std::string opcode, std::string instr);

#endif