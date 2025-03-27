Design Decisions

1. Memory Representation: We have implemented MEM as a byte array (unsigned char), ensuring that it follows a Little Endian format. This decision allows for consistent memory access and alignment, making it easier to interpret multi-byte values in a way that aligns with typical processor architectures.

2. Memory Initialization: At the start of execution, all memory and registers are initialized to zero. This ensures predictable behavior and prevents undefined values from affecting computation.

3. Tracking and Branch Resolution: We maintain complete tracking of register values, ensuring that every register operation is correctly recorded and updated. Additionally, we have fully implemented branch resolution.

4. Branch Resolution Logic: We implement a single-cycle stall for all branches and jumps without using branch prediction. When a branch is detected in the instruction decode (ID) stage, IF.branchPC is set to the target address, signaling the IF stage to update the program counter (PC) to this new address in the next cycle. The implementation includes a stall mechanism where the pipeline pauses for one cycle to allow the branch target to be correctly loaded,

5. Forwarding Unit: Our implementation includes a fully functional forwarding unit that handles all possible data forwarding scenarios. This minimizes data hazards in the pipeline by forwarding results from later stages of execution to earlier dependent instructions, improving performance without requiring unnecessary stalls.

6. For the Processor_F, we stall in the case of a hazard till whenever necessary to correctly forward the result from a previous stage. For the Processor_NF, we stall till the WB stage of the depedent instruction.

Known issues in your implementation

1. Currently, we have not encountered any known issues in our implementation. 

Sources 

1. Course Textbook: We referred to the course textbook to set up the fundamental data structures and ensure correctness in architectural design decisions. This provided a strong theoretical foundation for structuring memory, registers, and instruction execution
2. We have used ChatGPT for setting up the control signals for a few of the instructions like LUI.