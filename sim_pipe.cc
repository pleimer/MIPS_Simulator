/*
	To do: Implement the rest of the instructions
	Implement data flow
	Add hazard handling
	
	figure out what to do with stages for get_sp_register()!
	
	Up Next: 
		Implement run() - get the stages rolling!
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
	inst_memory_size = 65536;
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
	this->print_inst_memory(0x0000, 0x000F * 4);
}

void sim_pipe::run(unsigned cycles){
	//run for num clock cycles "cycles"
	//if "cycles" is 0, run until EOP
	
	switch(cycles){
		case 0://run to completion
		
		default://run for number of cycles
			for(int i=0;i<cycles;i++){
				
			}
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
	//s - stage to print it for????
	
	//for now, just return the value of that register - later,
	//I will have to implement the changes of the registers 
	//at the stage, assumingly in the run() method
	return sp_registers[s][reg];
}

int sim_pipe::get_gp_register(unsigned reg){
	return gp_registers[reg];
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
	for(int i=0; i<4;i++){
		data_memory[address + i] = (byte) (value >> 8*i);
	}
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
		if (get_gp_register(i)!=UNDEFINED) cout << "R" << dec << i << " = " << get_gp_register(i) << hex << " / 0x" << get_gp_register(i) << endl;
}
