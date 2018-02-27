/*
	To do: Implement the rest of the instructions
	Implement data flow
	Add hazard handling
	
	figure out what to do with stages for get_sp_register()!
	
	make it such that address 0x10000000 is actually address 0 so we don't have to allocate so much memory
	
	Up Next: 
		Implement run() - get the stages rolling!
		Implement opcode control of A and B registers
*/
#include "sim_pipe.h"
#include <stdlib.h>
#include <iostream>
#include <iomanip>

using namespace std;

//used for debugging purposes
static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};

sim_pipe::sim_pipe(unsigned mem_size, unsigned mem_latency){
	
	//allocate memory
	//sp registers
	for(int i=0;i<NUM_STAGES;i++){
		fill_n(sp_registers[i], NUM_SP_REGISTERS, UNDEFINED);
	}
	
	//gp registers
	gp_registers = new int[NUM_GP_REGISTERS];
	fill_n(gp_registers, NUM_GP_REGISTERS, UNDEFINED);
	
	//data memory
	data_memory_size = mem_size;
	data_memory = new byte[data_memory_size];
	fill_n(data_memory, data_memory_size, 0xFF);
	
	//instruction memory
	inst_memory_size = 128;
	inst_memory = new byte[inst_memory_size];
	fill_n(inst_memory,inst_memory_size, 0xFF);
}
	
sim_pipe::~sim_pipe(){
	delete[] gp_registers;
	delete[] data_memory;
	delete[] inst_memory;
}

void sim_pipe::load_program(const char *filename, unsigned base_address){
	/*parse instructions - load into instruction memory*/
	as.assemble(filename, inst_memory);
	//start pc at base address
	sp_registers[IF][PC] = base_address; //start at 0
	this->print_inst_memory(base_address - 0x10000000, base_address - 0x10000000 + 0x000F * 4);
}

void sim_pipe::run(unsigned cycles){
	//run for num clock cycles "cycles"
	//if "cycles" is 0, run until EOP
	int ex_ir, mem_ir;
	
	//for now, just number of clock cycles
		for(unsigned int i=0; i<cycles;i++){ //each loop is a clock cycle

				//WB
				
				//MEM
				mem_ir = get_ir_reg(MEM);
				switch(get_inst_type(mem_ir)){
					case MEMORY:
						sp_registers[WB][IR] = get_ir_reg(MEM);
						//sp_registers[WB][LMD] //FINISH
						break;
					case ARITH:
						sp_registers[WB][IR] = get_ir_reg(MEM);
						sp_registers[WB][ALU_OUTPUT] = sp_registers[MEM][ALU_OUTPUT];
						break;
					case ARITH_I:
						sp_registers[WB][IR] = get_ir_reg(MEM);
						sp_registers[WB][ALU_OUTPUT] = sp_registers[MEM][ALU_OUTPUT];
						break;
					default:
						break;
				}
		
				//EX
				ex_ir = get_ir_reg(EX);
				switch(get_inst_type(ex_ir)){
					case MEMORY:
						sp_registers[MEM][IR] = sp_registers[EX][IR];
						sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A]  + sp_registers[EX][IMM];
						sp_registers[MEM][B] = sp_registers[EX][B];
						break;
					case ARITH:
						sp_registers[MEM][IR] = sp_registers[EX][IR];
						switch(OPCODE(ex_ir)){
							case ADD:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] + sp_registers[EX][B];
								break;
							case SUB:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] - sp_registers[EX][B];
								break;
							case XOR:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] ^ sp_registers[EX][B];
								break;
							case OR:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] | sp_registers[EX][B];
								break;
							case AND:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] & sp_registers[EX][B];
								break;
							case MULT:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] * sp_registers[EX][B];
								break;
							case DIV:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] / sp_registers[EX][B];
								break;
							default:
								break;
						}
						break;
					case ARITH_I:
						sp_registers[MEM][IR] = sp_registers[EX][IR];
						switch(OPCODE(ex_ir)){
							case ADDI:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] + sp_registers[EX][IMM];
								break;
							case SUBI:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] - sp_registers[EX][IMM];
								break;
							case XORI:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] ^ sp_registers[EX][IMM];
								break;
							case ORI:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] | sp_registers[EX][IMM];
								break;
							case ANDI:
								sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A] & sp_registers[EX][IMM];
								break;
							default:
								break;
						}
						break;
					case BRANCH:
						sp_registers[MEM][ALU_OUTPUT] = (sp_registers[EX][NPC] + (sp_registers[EX][IMM] << 2));
						sp_registers[MEM][COND] = sp_registers[EX][A];
						break;
					default:
						break;
				}
		
				//ID 
				if((unsigned) sp_registers[ID][IR] != UNDEFINED){ 
					sp_registers[EX][A] = gp_registers[(sp_registers[ID][IR] & RS_MASK) >> (INST_SIZE - OP_SIZE - REG_REF_SIZE*2)];//rs
					if(get_inst_type(get_ir_reg(ID)) == ARITH) {
						sp_registers[EX][B] = gp_registers[(sp_registers[ID][IR] & RT_MASK) >> (INST_SIZE - OP_SIZE - REG_REF_SIZE*3)];//rt
					}
					if((get_inst_type(get_ir_reg(ID)) == ARITH_I) || (get_inst_type(get_ir_reg(ID)) == MEMORY)){//immediate
						if(get_ir_reg(ID) & IMM_SIGN) sp_registers[EX][IMM] = ((get_ir_reg(ID) & IMM_MASK) | IMM_SIGN_EXTEND);
						else {
							cout << "ID.IR is: " << hex << get_ir_reg(ID) << endl;
							cout << "Instruction Mem is: " << hex << get_inst(0) << endl;
							sp_registers[EX][IMM] = (get_ir_reg(ID) & IMM_MASK);
						}
					}
					sp_registers[EX][NPC] = sp_registers[ID][NPC];
					sp_registers[EX][IR] = sp_registers[ID][IR];
				}
			
				//IF
				sp_registers[ID][IR] = get_inst(sp_registers[IF][PC] - 0x10000000);
				cout << "Instruction Mem in IF w/ offset is: " << hex << get_inst(sp_registers[IF][PC] - 0x10000000) << endl;
				cout << "ID.IR in IF: " << hex << get_ir_reg(ID) << endl;
				cout << "Instruction Mem in IF is: " << hex << get_inst(0) << endl;
				sp_registers[ID][NPC] = sp_registers[IF][PC] + 4;
				sp_registers[IF][PC] +=4; 
		}
}
	
void sim_pipe::reset(){ //reset all memory and registers
	for(int i=0;i<NUM_STAGES;i++){
		fill_n(sp_registers[i], NUM_SP_REGISTERS, UNDEFINED);
	}
	fill_n(gp_registers, NUM_GP_REGISTERS, UNDEFINED);
	fill_n(data_memory, data_memory_size, 0xFF);
	fill_n(inst_memory,inst_memory_size, 0xFF);
}

unsigned sim_pipe::get_sp_register(sp_register_t reg, stage_t s){
	//reg - register to print 
	return sp_registers[s][reg];
}

int sim_pipe::get_gp_register(unsigned reg){
	return gp_registers[reg];
}

int sim_pipe::get_ir_reg(stage_t s){
	return sp_registers[s][IR];
}

int sim_pipe::get_inst(unsigned base_address){
	int instruction;
	for(int i=0;i<4;i++)
		instruction |= (inst_memory[base_address + i] << (3-i)*8);
	return instruction;
}

int sim_pipe::get_inst_type(int inst){
	if((OPCODE(inst) == ADDI) || (OPCODE(inst) == SUBI) || (OPCODE(inst) == XORI) || (OPCODE(inst) == ORI) || (OPCODE(inst) == ANDI))
		return ARITH_I;
	else if((OPCODE(inst) == LW) || (OPCODE(inst) == SW))
		return MEMORY;
	else if ((OPCODE(inst) >= ADD) && (OPCODE(inst) <= DIV))
		return ARITH;
	else if ((OPCODE(inst) >= BEQZ) && (OPCODE(inst) <= JUMP))
		return BRANCH;
	return UNDEF_INST;
}

void sim_pipe::set_gp_register(unsigned reg, int value){
	gp_registers[reg] = value;
}

float sim_pipe::get_IPC(){
	return 0; //please modify
}

unsigned sim_pipe::get_instructions_executed(){
	return 0; //please modify
}

unsigned sim_pipe::get_stalls(){
	return 0; //please modify
}

unsigned sim_pipe::get_clock_cycles(){
	return 0; //please modify
}

void sim_pipe::print_memory(unsigned start_address, unsigned end_address){
	cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	unsigned i;
	for (i=start_address; i<end_address; i++){
		if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
		cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
		if (i%4 == 3) cout << endl;
	} 
}

void sim_pipe::print_inst_memory(unsigned start_address, unsigned end_address){
	cout << "inst_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	unsigned i;
	for (i=start_address; i<end_address; i++){
		if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
		cout << hex << setw(2) << setfill('0') << int(inst_memory[i]) << " ";
		if (i%4 == 3) cout << endl;
	} 
}

void sim_pipe::write_memory(unsigned address, unsigned value){
	//store to data memory in little endian at address
	for(int i=0; i<4;i++) data_memory[address + i] = (byte) (value >> 8*i);
}

void sim_pipe::print_registers(){
	cout << "Special purpose registers:" << endl;
        unsigned i, s;
	for (s=0; s<NUM_STAGES; s++){
		cout << "Stage: " << stage_names[s] << endl;  
		for (i=0; i< NUM_SP_REGISTERS; i++)
			if ((sp_register_t)i != IR && (sp_register_t)i != COND && get_sp_register((sp_register_t)i, (stage_t)s)!=UNDEFINED) cout << reg_names[i] << " = " << dec <<  get_sp_register((sp_register_t)i, (stage_t)s) << hex << " / 0x" << get_sp_register((sp_register_t)i, (stage_t)s) << endl;
	}
	cout << "General purpose registers:" << endl;
	for (i=0; i< NUM_GP_REGISTERS; i++)
		if ((unsigned) get_gp_register(i)!= UNDEFINED) cout << "R" << dec << i << " = " << get_gp_register(i) << hex << " / 0x" << get_gp_register(i) << endl;
}

