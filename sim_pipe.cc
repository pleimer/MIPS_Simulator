/*
	To do: 
	Implement the rest of the instructions
	
	Implement memory fetch delay
	
	Now, add data hazard handling

	Add hazard handling
	
	Up Next: 
		
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
	
	this->mem_latency = mem_latency;
	this->inst_executed = 0;
	this->clock_cycles = 0;
	this->num_NOPS = 0;
	this->total_NOPS = 0;
	this->mem_latency_tracker = mem_latency;
	
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
	int inst = get_inst(sp_registers[IF][PC] - 0x10000000);
	cout << "In IF: PC ->  "  << inst << endl;
	this->print_inst_memory(base_address - 0x10000000, base_address - 0x10000000 + 0x000F * 4);
}

void sim_pipe::run(unsigned cycles){
	//run for num clock cycles "cycles"
	//if "cycles" is 0, run until EOP
	
	if(cycles == 0) while(pipeline());
	else for(unsigned int i=0; i<cycles;i++){ pipeline();}	
}

bool sim_pipe::pipeline(){
	cout << "------------------------------------" << endl;
	cout << "Clock Cycle: " << dec << get_clock_cycles() << endl;
	//WB
	if(!exec_WB()) return false;
	
	//MEM
	exec_MEM();

	//EX
	exec_EX();
		
	exec_ID();

	//IF
	exec_IF();	
	cout << endl << "IR registers after clock cycle: " << endl;

	/*for(int i=WB;i != IF;i--){
		cout << stage_names[i] << ": " << hex <<  get_ir_reg((stage_t)i) << endl;
	}
	int inst = get_inst(sp_registers[IF][PC] - 0x10000000);
	cout << "PC -> " <<  inst << endl << endl;*/
	
	
	++clock_cycles;
	return true;
}



void sim_pipe::exec_IF(){
	int inst = get_inst(sp_registers[IF][PC] - 0x10000000);
	cout << "PC -> " <<  inst << endl << endl;
	cout << "Condition register: " << sp_registers[MEM][COND] << endl;
	if((sp_registers[MEM][COND]) && (sp_registers[MEM][COND] != UNDEFINED)){//BRANCH
		sp_registers[ID][NPC] = sp_registers[MEM][ALU_OUTPUT];
		sp_registers[IF][PC] = sp_registers[MEM][ALU_OUTPUT];
		//cout << "Jumping to: " << hex << sp_registers[MEM][ALU_OUTPUT] << endl;
	}

	if(!hazard){
		sp_registers[ID][IR] = get_inst(get_PC() - 0x10000000);
		if(OPCODE(get_ir_reg(ID)) != EOP){
				sp_registers[ID][NPC] = sp_registers[IF][PC] + 4;
				sp_registers[IF][PC] +=4;
		}
	}
}

void sim_pipe::exec_ID(){
		cout << "IR at ID is: " << hex << get_ir_reg(ID) << endl;
		int id_ir = get_ir_reg(ID);
		switch(get_inst_type(id_ir)){
			case ARITH:
				sp_registers[EX][A] = gp_registers[RS(get_ir_reg(ID))]; 
				sp_registers[EX][B] = gp_registers[RT(get_ir_reg(ID))];
				break;
			case ARITH_I:
				sp_registers[EX][A] = gp_registers[RS(get_ir_reg(ID))]; 
				//immediate
				if(get_ir_reg(ID) & IMM_SIGN) sp_registers[EX][IMM] = ((get_ir_reg(ID) & IMM_MASK) | IMM_SIGN_EXTEND); //negative number
				else sp_registers[EX][IMM] = (get_ir_reg(ID) & IMM_MASK);//positive number
				break;
			case MEMORY:
				sp_registers[EX][A] = gp_registers[RS(get_ir_reg(ID))]; 
				if(OPCODE(get_ir_reg(ID)) == SW) sp_registers[EX][B] = gp_registers[RD(get_ir_reg(ID))]; //in RD pos, but actually RT
				//immediate
				if(get_ir_reg(ID) & IMM_SIGN) sp_registers[EX][IMM] = ((get_ir_reg(ID) & IMM_MASK) | IMM_SIGN_EXTEND); //negative number
				else sp_registers[EX][IMM] = (get_ir_reg(ID) & IMM_MASK);//positive number
				break;
			case BRANCH:
				sp_registers[EX][A] = gp_registers[RD(get_ir_reg(ID))];
				//immediate
				if(get_ir_reg(ID) & IMM_SIGN) sp_registers[EX][IMM] = ((get_ir_reg(ID) & IMM_MASK) | IMM_SIGN_EXTEND); //negative number
				else sp_registers[EX][IMM] = (get_ir_reg(ID) & IMM_MASK);//positive number
				break;
				
			default: break;
			
		}
		
		hazard = isHazard();
		sp_registers[EX][NPC] = sp_registers[ID][NPC];
		sp_registers[EX][IR] = sp_registers[ID][IR]; //propagate
		if(hazard) insert_NOP();
}

void sim_pipe::exec_EX(){
	int ex_ir;
	ex_ir = get_ir_reg(EX);
	cout << "IR at EX is: " << hex << ex_ir << endl;
	if(get_inst_type(ex_ir) != BRANCH) sp_registers[MEM][COND] = 0;//branch will set this true
	sp_registers[MEM][B] = sp_registers[EX][B];
	sp_registers[MEM][IR] = ex_ir;//propagate IR
	
	
	
	switch(get_inst_type(ex_ir)){
		case MEMORY:
			sp_registers[MEM][ALU_OUTPUT] = sp_registers[EX][A]  + sp_registers[EX][IMM];
			break;
		case ARITH:
		
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
			sp_registers[MEM][ALU_OUTPUT] = ((sp_registers[EX][NPC]) + (sp_registers[EX][IMM] << 2) - 4);
			switch(OPCODE(get_ir_reg(EX))){
			 case BNEZ:
				if(sp_registers[EX][A] != 0) sp_registers[MEM][COND] = 1;
				break;
			default:
				break;
			}
			break;
		default:
			break;
	}	
}


void sim_pipe::exec_MEM(){
		int mem_ir;
		mem_ir = get_ir_reg(MEM);
		cout << "IR at MEM is: " << hex << mem_ir << endl;
		sp_registers[WB][IR] = get_ir_reg(MEM); //propagate IR to next stage
		
		switch(get_inst_type(mem_ir)){
			case MEMORY:
					//doesn't execute until after mem_latncy cycles
				if(!mem_latency_tracker){
					sp_registers[WB][ALU_OUTPUT] = sp_registers[MEM][ALU_OUTPUT];
					//sp_registers[WB][IR] = get_ir_reg(MEM);
					if(OPCODE(mem_ir) == LW) sp_registers[WB][LMD] = read_memory(sp_registers[MEM][ALU_OUTPUT]);
					else if (OPCODE(mem_ir) == SW) write_memory(sp_registers[MEM][ALU_OUTPUT], sp_registers[MEM][B]);
				}
				else {
					num_NOPS += mem_latency ;
					hazard_type = STRUCTURAL;
					insert_NOP();
					mem_latency_tracker--;
				}
				break;
			case ARITH:
				sp_registers[WB][ALU_OUTPUT] = sp_registers[MEM][ALU_OUTPUT];
				//sp_registers[WB][IR] = get_ir_reg(MEM);
				break;
			case ARITH_I:
				sp_registers[WB][ALU_OUTPUT] = sp_registers[MEM][ALU_OUTPUT];
				//sp_registers[WB][IR] = get_ir_reg(MEM);
				break;
			case BRANCH:
				break;
			default:
				break;
		}
}


bool sim_pipe::exec_WB(){
	int wb_ir = get_ir_reg(WB);
	cout << endl << "IR at WB is: " << hex << wb_ir << endl;
	inst_executed++;
	switch(get_inst_type(wb_ir)){
		case MEMORY:
			if(OPCODE(wb_ir) == LW) gp_registers[RD(wb_ir)] = sp_registers[WB][LMD];
			break;
		case ARITH:
			gp_registers[RD(wb_ir)] = sp_registers[WB][ALU_OUTPUT];
			break;
		case ARITH_I:
			gp_registers[RD(wb_ir)] = sp_registers[WB][ALU_OUTPUT];
			break;
		case BRANCH:
			break;
		default:
			inst_executed--;
			if(OPCODE(wb_ir) == EOP) return false;
			break;
	}
	return true;
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

int sim_pipe::get_PC(){
	return sp_registers[IF][PC];
}

int sim_pipe::get_inst(unsigned base_address){
	int instruction = 0;
	for(int i=0;i<4;i++){
		instruction = instruction | (inst_memory[base_address + i] << (3-i)*8);
	}
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
	float IPC = (float)inst_executed/(float)clock_cycles;
	return IPC;
}

unsigned sim_pipe::get_instructions_executed(){
	return inst_executed;
}

unsigned sim_pipe::get_stalls(){
	return total_NOPS;
}

unsigned sim_pipe::get_clock_cycles(){
	return clock_cycles; //please modify
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
	std::ofstream out("assem_out.txt");
    std::streambuf *coutbuf = std::cout.rdbuf(); 
    std::cout.rdbuf(out.rdbuf()); 
	
	cout << "inst_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	unsigned i;
	for (i=start_address; i<end_address; i++){
		if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
		cout << hex << setw(2) << setfill('0') << int(inst_memory[i]) << " ";
		if (i%4 == 3) cout << endl;
	}
	std::cout.rdbuf(coutbuf);
	
}

void sim_pipe::write_memory(unsigned address, unsigned value){
	//store to data memory in little endian at address
	for(int i=0; i<4;i++) data_memory[address + i] = (byte) (value >> 8*i);
}

int sim_pipe::read_memory(unsigned address){
	//read data from mem (little endian)
	int data;
	for(int i=0; i<4;i++) data |= (data_memory[i+address] << i*8);
	return data;
}

void sim_pipe::print_registers(){
	cout << "Special purpose registers:" << endl;
        unsigned i, s;
	for (s=0; s<NUM_STAGES; s++){
		cout << "Stage: " << stage_names[s] << endl;  
		for (i=0; i< NUM_SP_REGISTERS; i++)
			if ((sp_register_t)i != IR && (sp_register_t)i != COND && get_sp_register((sp_register_t)i, (stage_t)s)!= (unsigned)UNDEFINED) cout << reg_names[i] << " = " << dec <<  get_sp_register((sp_register_t)i, (stage_t)s) << hex << " / 0x" << get_sp_register((sp_register_t)i, (stage_t)s) << endl;
	}
	cout << "General purpose registers:" << endl;
	for (i=0; i< NUM_GP_REGISTERS; i++)
		if (get_gp_register(i)!= UNDEFINED) cout << "R" << dec << i << " = " << get_gp_register(i) << hex << " / 0x" << get_gp_register(i) << endl;
}

bool sim_pipe::isHazard(){

	//data hazard
	int op_id = OPCODE(get_ir_reg(ID));
	int op_ex = OPCODE(get_ir_reg(EX)); // MEM because of way pipeline is set up -> don't ask questions
	int op_mem = OPCODE(get_ir_reg(WB));
	int id_rt_sw = RD(get_ir_reg(ID)); //special case for SW since RT position is different
	int id_rs = RS(get_ir_reg(ID));
	int id_rt = RT(get_ir_reg(ID));
	int ex_rd = RD(get_ir_reg(EX));
	int mem_rd =  RD(get_ir_reg(WB));
	
	//data hazard ID-EX
	if((get_ir_reg(EX) != UNDEFINED) && (op_id != EOP) && (op_ex != NOP)){
		if((op_id == SW) ){
			if( (id_rt_sw == ex_rd)){
					cout << "EOP inserted bc SW in EX" << endl;
					num_NOPS = mem_latency + 2;
					hazard_type = DATA;
			}
		}
		else if ((op_id == LW) ){
			if( (id_rs == ex_rd)){
				cout << "EOP inserted bc LW in EX" << endl;
				num_NOPS = mem_latency + 2;
				hazard_type = DATA;
			}
		}

		if( (((id_rs == ex_rd)|| (id_rt == ex_rd))) && (op_ex != SW)){
			num_NOPS =  mem_latency + 2;
			cout << "id_rs: " << id_rs << endl;
			cout << "id_rt: " << id_rt << endl;
			cout << "ex_rd: " << ex_rd << endl;
			cout << "EX.IR is: " << get_ir_reg(EX) << endl;
			cout << "EOP inserted bc RD in ARITH in EX" << endl;
			hazard_type = DATA;
		}
	}
	
	//data hazard ID-MEM
	if((get_ir_reg(MEM) != UNDEFINED) && (op_id != EOP) && (op_mem != NOP)){
		if((op_id == SW) ){
			cout << "id_rt_sw: " << id_rt_sw << endl;
			cout << "mem_rd: " << mem_rd << endl;
			if( (id_rt_sw == mem_rd)){
					cout << "EOP inserted bc SW at MEM" << endl;
					num_NOPS = mem_latency + 1;
					hazard_type = DATA;
			}
		}
		else if ((op_id == LW) ){
			if( (id_rs == mem_rd)){
				cout << "EOP inserted bc LW in MEM" << endl;
				num_NOPS = mem_latency + 1;
				hazard_type = DATA;
				
			}
		}
		
		if( (((id_rs == mem_rd)|| (id_rt == mem_rd))) && (op_mem != SW)){
			num_NOPS = mem_latency + 1;
			cout << "id_rs: " << id_rs << endl;
			cout << "id_rt: " << id_rt << endl;
			cout << "mem_rd: " << mem_rd << endl;
			cout << "op_mem: " << op_mem << endl;
			cout << "EOP inserted bc RD in ARITH in MEM" << endl;
			hazard_type = DATA;
		}
	}
	
		//control hazard
	if(get_inst_type(get_ir_reg(ID)) == BRANCH){
			num_NOPS = 2;
			cout << "EOP inserted bc control hazard" << endl;
			hazard_type = CONTROL;
	}
	
	if((num_NOPS > 0) && (op_id != EOP)){
		cout << "EOP inserted bc num_NOPS" << endl;
		return true;
	}
	
		
	return false;
}

void sim_pipe::insert_NOP(){
	//unsigned pos = (unsigned)log2((unsigned) NOP);
	num_NOPS--;
	total_NOPS++;
	
	switch(hazard_type){
		case DATA: 
			sp_registers[EX][IR] = (NOP << (INST_SIZE - OP_SIZE));
			break;
		case CONTROL:
			sp_registers[ID][IR] = (NOP << (INST_SIZE - OP_SIZE));
			break;
		case STRUCTURAL:
			sp_registers[EX][IR] = (NOP << (INST_SIZE - OP_SIZE));
			break;
		default: break;
	}
}

