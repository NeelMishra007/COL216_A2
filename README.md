Design Decisions

1. Pipeline Architecture
Implemented a five-stage pipeline: Instruction Fetch (IF), Instruction Decode (ID), Execute (EX), Memory (MEM), and Write Back (WB)
Each stage maintains its own state structure with control signals and instruction tracking
Pipeline stages are processed in reverse order to simulate actual hardware behavior

2. Memory and Register Management
Used a vector of unsigned char (vector<unsigned char>) for memory representation
Implemented a fixed-size memory of 2,000,005 bytes
Initialized all 32 registers to zero at the start of execution
Support for different memory access sizes (byte, halfword, word)
Implemented sign and zero extension for memory loads

3. Instruction Decoding and Control
Used a hexadecimal to binary conversion for instruction parsing
Implemented a comprehensive decoder (Decoder_F) to set control signals based on opcode
Supports a wide range of RISC-V instructions including arithmetic, logical, memory, and control flow operations

4. Memory Representation: We have implemented MEM as a byte array (unsigned char), ensuring that it follows a Little Endian format. This decision allows for consistent memory access and alignment, making it easier to interpret multi-byte values in a way that aligns with typical processor architectures.

5. Memory Initialization: At the start of execution, all memory and registers are initialized to zero. This ensures predictable behavior and prevents undefined values from affecting computation.

6. Tracking and Branch Resolution: We maintain complete tracking of register values, ensuring that every register operation is correctly recorded and updated. Additionally, we have fully implemented branch resolution.

7. Branch Resolution Logic: We implement a single-cycle stall for all branches and jumps without using branch prediction. When a branch is detected in the instruction decode (ID) stage, IF.branchPC is set to the target address, signaling the IF stage to update the program counter (PC) to this new address in the next cycle. The implementation includes a stall mechanism where the pipeline pauses for one cycle to allow the branch target to be correctly loaded,

8. Forwarding Unit: Our implementation includes a fully functional forwarding unit that handles all possible data forwarding scenarios. This minimizes data hazards in the pipeline by forwarding results from later stages of execution to earlier dependent instructions, improving performance without requiring unnecessary stalls.

9. For the Processor_F, we stall in the case of a hazard till whenever necessary to correctly forward the result from a previous stage. For the Processor_NF, we stall till the WB stage of the depedent instruction.

10. Instruction Decoding Strategy
The Decoder_F function implements a comprehensive instruction decoding mechanism that meticulously breaks down RISC-V instructions across multiple instruction formats:
Instruction Formats Supported
R-type (Register-Register Operations)
I-type (Immediate and Load Instructions)
S-type (Store Instructions)
B-type (Branch Instructions)
J-type (Jump and Link Instructions)
U-type (Upper Immediate Instructions)

11. Instruction Type Specific Handling
Arithmetic and Logical Instructions

Supports operations like ADD, SUB, SLL, SRL, SRA, SLT, SLTU
Handles both R-type and I-type variants
Differentiates between signed and unsigned comparisons

Memory Instructions

Supports various load and store instructions (LW, LH, LB, SW, SH, SB)
Handles different memory access sizes
Implements sign and zero extension for loads

Branch and Jump Instructions

Comprehensive branch condition evaluation
Supports all RISC-V branch types (BEQ, BNE, BLT, BGE, BLTU, BGEU)
Implements jump and link instructions (JAL, JALR)

12. Register and Operand Management
Extracts source and destination register indices
Manages special cases like x0 (hardwired zero register)

Known issues in your implementation

1. Currently, we have not encountered any known issues in our implementation. 

Sources 

1. Course Textbook: We referred to the course textbook to set up the fundamental data structures and ensure correctness in architectural design decisions. This provided a strong theoretical foundation for structuring memory, registers, and instruction execution
2. We have used ChatGPT for setting up the control signals for a few of the instructions like LUI.