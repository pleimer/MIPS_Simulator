#ifndef SIM_PIPE_H_
#define SIM_PIPE_H_

#include <stdio.h>
#include <math.h>
#include "assem.h"

#define UNDEFINED (signed)0xFFFFFFFF //constant used to initialize registers
#define NUM_SP_REGISTERS 9
#define NUM_GP_REGISTERS 32
#define NUM_OPCODES 23
#define NUM_STAGES 5

#define RD_MASK 0x03E00000
#define RS_MASK 0x001F0000
#define RT_MASK 0x0000F800
#define IMM_SIGN 0x0008000
#define IMM_SIGN_EXTEND 0xFFFF0000

#define OPCODE(X) ((opcode_t)((X & 0xFC000000) >> (INST_SIZE-OP_SIZE)))
//gets reference of register as int
#define RD(X) 	((X & RD_MASK) >> (INST_SIZE - OP_SIZE - REG_REF_SIZE * 1)) 
#define RS(X) 	((X & RS_MASK) >> (INST_SIZE - OP_SIZE - REG_REF_SIZE * 2))
#define RT(X) 	((X & RT_MASK) >> (INST_SIZE - OP_SIZE - REG_REF_SIZE * 3))

typedef enum {PC, NPC, IR, A, B, IMM, COND, ALU_OUTPUT, LMD} sp_register_t;

// The NOP instruction should be automatically inserted by the processor to implement pipeline bubbles
typedef enum {LW, SW, ADD, ADDI, SUB, SUBI, XOR, XORI, OR, ORI, AND, ANDI, MULT, DIV, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, NOP} opcode_t;
typedef enum {ARITH_I, ARITH, MEMORY, BRANCH, UNDEF_INST} inst_t;

typedef enum {IF, ID, EX, MEM, WB} stage_t;


class sim_pipe{
	
	unsigned mem_latency;
	unsigned clock_cycles;
	unsigned inst_executed;
	unsigned num_NOPS; //keeps track of NOPS to insert
	unsigned total_NOPS;

	/* Add the data members required by your simulator's implementation here */
	int *gp_registers;
	int sp_registers[NUM_STAGES][NUM_SP_REGISTERS];
	
	//data memory - should be initialize to all 0xFF
	byte *data_memory;
	
	//instruction memory
	byte *inst_memory;

	//memory size in bytes
	unsigned data_memory_size;
	unsigned inst_memory_size;
	
	//assembler declaration
	assembler as;

public:

	//instantiates the simulator with a data memory of given size (in bytes) and latency (in clock cycles)
	/* Note: 
           - initialize the registers to UNDEFINED value 
	   - initialize the data memory to all 0xFF values
	*/
	sim_pipe(unsigned data_mem_size, unsigned data_mem_latency);
	
	//de-allocates the simulator
	~sim_pipe();

	//loads the assembly program in file "filename" in instruction memory at the specified address
	void load_program(const char *filename, unsigned base_address=0x0);

	//runs the simulator for "cycles" clock cycles (run the program to completion if cycles=0) 
	void run(unsigned cycles=0);
	
	//resets the state of the simulator
        /* Note: 
	   - registers should be reset to UNDEFINED value 
	   - data memory should be reset to all 0xFF values
	*/
	
	bool pipeline(); //contains the pipeline logic
	
	void reset();

	// returns value of the specified special purpose register for a given stage (at the "entrance" of that stage)
        // if that special purpose register is not used in that stage, returns UNDEFINED
        //
        // Examples (refer to page C-37 in the 5th edition textbook, A-32 in 4th edition of textbook)::
        // - get_sp_register(PC, IF) returns the value of PC
        // - get_sp_register(NPC, ID) returns the value of IF/ID.NPC
        // - get_sp_register(NPC, EX) returns the value of ID/EX.NPC
        // - get_sp_register(ALU_OUTPUT, MEM) returns the value of EX/MEM.ALU_OUTPUT
        // - get_sp_register(ALU_OUTPUT, WB) returns the value of MEM/WB.ALU_OUTPUT
	// - get_sp_register(LMD, ID) returns UNDEFINED
	/* Note: you are allowed to use a custom format for the IR register.
           Therefore, the test cases won't check the value of IR using this method. 
	   You can add an extra method to retrieve the content of IR */
	unsigned get_sp_register(sp_register_t reg, stage_t stage);

	//returns value of the specified general purpose register
	int get_gp_register(unsigned reg);
	
	//returns ir register for stage
	int get_ir_reg(stage_t s);

	// set the value of the given general purpose register to "value"
	void set_gp_register(unsigned reg, int value);
	
	//returns instruction at location in instruction memory
	int get_inst(unsigned base_address);

	//returns the IPC
	float get_IPC();

	//returns the number of instructions fully executed
	unsigned get_instructions_executed();

	//returns the number of stalls added by processor
	unsigned get_stalls();

	//returns the number of clock cycles
	unsigned get_clock_cycles();

	//prints the content of the data memory within the specified address range
	void print_memory(unsigned start_address, unsigned end_address);
	
	//print instruction memory for debugging
	void print_inst_memory(unsigned start_address, unsigned end_address);

	// writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness)
	void write_memory(unsigned address, unsigned value);
	
	//read integer starting at base value
	int read_memory(unsigned address);

	//prints the values of the registers 
	void print_registers();
	
	//get instruction type from IR register
	int get_inst_type(int inst);
	
	//detect hazards
	bool isHazard();
	
	//inject NOP into pipeline
	void insert_NOP();

};

#endif /*SIM_PIPE_H_*/
