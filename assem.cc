/*
PAUL LEIMER

2/8/2018


NEXT UP:

	
	Now, compile to machine code (forget parser for now to verify syntax)
	
	NEXT:
		make immediate masks so that negative numbers do not overwrite instructions
		also make assem.h instead of including everything in sim.h to help with late projects
*/

#include "assem.h"
#include "sim_pipe.h"

using namespace std;

const char * WHITE_SPACE = " \t"; //space and tab

class eofException: public exception{
	virtual const char* what() const throw(){
		return "End of file.";
	}
} eofExc;

void assembler::assemble(string path, byte * inst_memory){
	
	//compile to machine code - check type, line and column to decide what equivalent machine
	//code and store into instruction memory
	
	vector<token> currInst;
	vector<token> labels;
	lexer lex(path);
	
	int instruction;
	int inst_mem_ptr = 0;
	
	while(true){ //build list of labels before compiling rest
		try{
			currInst = lex.get();
			for (token tok: currInst){ 
				if(tok.getType() == LABEL){
					labels.push_back(tok);
					break;	
				}
			}
		}
		catch(eofException){break;}	
	}
	
	lex.reset_file_pointer();
	
	while(true){
		try{
			currInst = lex.get(); //one line of instructions
			instruction = 0;
			
			for (token tok: currInst){ //compile instruction
				switch(tok.getType()){
					case OPCODE:{
						instruction = this->mapCode(tok);
						instruction = instruction << (INST_SIZE - OP_SIZE);
						break;
					}
					
					case REGISTER: {//registers
						string reg = tok.getCargo();
						//get register number
						for(unsigned int i=1;i<reg.length();i++){
							reg[i-1] = reg[i];
						}
						reg.pop_back();
						unsigned int regNum = stoi(reg);
						if(tok.getCol() == 1)
							regNum = regNum <<  (INST_SIZE - OP_SIZE - REG_REF_SIZE); //rd
						if(tok.getCol() == 2)
							regNum = regNum <<  (INST_SIZE - OP_SIZE - REG_REF_SIZE*2); //rs
						if(tok.getCol() == 3)
							regNum = regNum <<  (INST_SIZE - OP_SIZE - REG_REF_SIZE*3); //rt
						
						instruction = instruction | regNum;
					
						break;
					}
						
					case DECNUM: {//immediate
						if(tok.getCol() > 2){
							string strNum = tok.getCargo();

							int num = stoi(strNum);
							num &= IMM_MASK;
							
							instruction = instruction | num;
						}
						break;
					}
						
					case HEXNUM: {//immediate
						if(tok.getCol() > 2){
							string strNum = tok.getCargo();
							strNum = strNum.substr(2,strNum.length());	
							int num = stoi(strNum,nullptr,16);
							num &= IMM_MASK;
							
							instruction = instruction | num;
						}					
						break;
					}
					case BINUM:	{//immediate
						if(tok.getCol() > 2){
							string strNum = tok.getCargo();
							strNum = strNum.substr(2,strNum.length());	
							int num = stoi(strNum,nullptr,2);
							num &= IMM_MASK;
							
							instruction = instruction | num;
						}	
						break;
					}
						
					case REGOFFSET:{
						if(tok.getCol() > 1){
							string cargo = tok.getCargo();
						
							string regStr, numStr;
							int num;
						
							numStr = cargo.substr(0, cargo.find('('));
							regStr = cargo.substr(cargo.find('(')+1, cargo.find(')')-2);
								
							//determin register number
							unsigned int regNum = stoi(regStr.substr(1));
							
							regNum = regNum << (INST_SIZE - OP_SIZE - REG_REF_SIZE*2);
							
							//immediate
							num = stoi(numStr);

							instruction = instruction | regNum | num;
						}
						break;
					}
					
					/*case LABEL: {
						//this builds vector full of LABEL tokens that have all information for referencing later
						labels.push_back(tok);
						for(token t: labels){
							cout << "Token: " << t.getCargo() << " Line: " <<  t.getLine() << endl;
						}
						break;	
					}*/
					case REF_LABEL: {
						//find token in tokens vector, get linenumber, calculate offset
						string cargo = tok.getCargo();
						cargo += ":"; //Label token cargo contains the colon
						int offset=0;
						unsigned int pos = 0;
				
				
						for(token item: labels){
							if(cargo.compare(item.getCargo()) == 0)
								break;
							pos++;
						}

						//calculate offset
						cout << "Branch line is: " << tok.getLine() << endl;
						cout << "The label line is: "<< labels[pos].getLine() << endl; 
						offset = -1*(tok.getLine() - labels[pos].getLine());
						offset &= LABEL_MASK;

						cout << "Offset is: " << hex << offset << endl;
						instruction = instruction | offset;
						
						break;
					}
						
					default:
						break;
				}

			}
		
			//store instruction into instruction memory 
			//in four iterations, shift instruction into four bytes
			/*bitset<32> y(instruction);
			cout << y << endl;*/
		
			for (int i=3;i>=0;i--){ //four bytes in each instruction
				//opcode in first byte (order), operands after
				*(inst_memory + inst_mem_ptr) = instruction >> 8*i;
				
				inst_mem_ptr++;
			}
			
		}
		catch(eofException){break;}
	}	
}

byte assembler::mapCode(token tok){ //return binary opcode value from token string
	string cargo = tok.getCargo();
	if(cargo.compare("LW")==0)
		return (byte)(LW);
	if(cargo.compare("SW")==0)
		return (byte)(SW);
	if(cargo.compare("ADD")==0)
		return (byte)(ADD);
	if(cargo.compare("ADDI")==0)
		return (byte)(ADDI);
	if(cargo.compare("SUB")==0)
		return (byte)(SUB);
	if(cargo.compare("SUBI")==0)
		return (byte)(SUBI);
	if(cargo.compare("XOR")==0)
		return (byte)(XOR);
	if(cargo.compare("XORI")==0)
		return (byte)(XORI);
	if(cargo.compare("OR")==0)
		return (byte)(OR);
	if(cargo.compare("ORI")==0)
		return (byte)(ORI);
	if(cargo.compare("AND")==0)
		return (byte)(AND);
	if(cargo.compare("ANDI")==0)
		return (byte)(ANDI);
	if(cargo.compare("BLTZ")==0)
		return (byte)(BLTZ);
	if(cargo.compare("BEQZ")==0)
		return (byte)(BEQZ);
	if(cargo.compare("BNEZ")==0)
		return (byte)(BNEZ);
	if(cargo.compare("BLTZ")==0)
		return (byte)(BLTZ);
	if(cargo.compare("BGTZ")==0)
		return (byte)(BGTZ);
	if(cargo.compare("BLEZ")==0)
		return (byte)(BLEZ);
	if(cargo.compare("BGEZ")==0)
		return (byte)(BGEZ);
	if(cargo.compare("JUMP")==0)
		return (byte)(JUMP);
	if(cargo.compare("EOP")==0)
		return (byte)(EOP);
	
	
	
	return 0xFC; //bad value
}

lexer::lexer(string path){
	lineIndex = 0;
	istringstream iss (path);
	string token;
	while(getline(iss,token,'.'));
	
	if (token.compare("asm") != 0) { //check if valid file
		cout << "Invalid file. Exiting..." << endl;
		return;
	}
	
	asmFile.open(path.c_str());
}

lexer::~lexer(){
	
}

void lexer::reset_file_pointer(){
	lineIndex = 0;
	asmFile.clear(); //return to beginning of file
	asmFile.seekg(0, ios::beg);
}

vector<token> lexer::get(){
	//get next line and divide into tokens, return vector of tokens
	string line;
	
	vector<token> tokens;
	tokenType type;
	unsigned int column = 0;

	getline(asmFile, line);

	if(!asmFile){
		if(!asmFile.eof()) cout << "Error in file\n";
		throw eofExc;
	}
	
	vector<string> token_strs; //this needs to be deleted before refilling
	
	split(line, WHITE_SPACE, token_strs); //get token strings for one line
	
	//create token for each entry in token_strs
	for (string token_str : token_strs){
		type = detType(token_str, column);
		
		if (type == UNDEF){
			cout << "There was an undefined type: " << token_str << endl;
		}
		else{
			tokens.push_back(token(token_str, lineIndex, column, type));
			if (type != LABEL){
				++column;
			}
		}
	}	

	++lineIndex;
	return tokens;
}

tokenType lexer::detType(string token_str, unsigned int column){

	//check all that must start with alphabetic letter
	if (isalpha(token_str[0]) != 0){
		if (column == 0 && token_str.back() == ':') //check for label (check column and end character)
			return LABEL;
		else { //check for register
			if (token_str[0] == 'R'){
				string regNum;
				for(unsigned int i=1;i<token_str.length();i++){
					regNum.push_back(token_str[i]);
				}
				int reg = stoi(regNum);
				if ((0 <= reg) && (reg < NUM_GP_REGISTERS)) return REGISTER;
			}
		}
		
		if (column > 1) return REF_LABEL;
		
		
		//check for opcode type
		if(token_str.back() != ':'){
			return OPCODE;
		}
		
	}
	else {
		//check for DECNUM
		bool isNum = true;
		for (char c: token_str){
			if(!isdigit(c)){
				isNum = false;
			}
		}
		if(isNum) return DECNUM;
	
		//check for HEXNUM
		if((token_str[0] == '0') && (toupper(token_str[1]) == 'X'))
			return HEXNUM;
		
		//check for REGOFFSET
		if (token_str.find("(") != string::npos)
			return REGOFFSET;
		
	}

	
	return UNDEF; //MUST BE CHANGED
}

void lexer::split(const string &s, const char* delim, vector<string> & v){
    char * dup = strdup(s.c_str());
    char * token = strtok(dup, delim);
    while(token != NULL){
        v.push_back(string(token));
        token = strtok(NULL, delim);
    }
    free(dup);
}

token::token(string cargo, unsigned int lineIndex, unsigned int column, tokenType type){
	//create token variables
	/*
	string cargo;
	string sourceText;
	unsigned int lineIndex;
	unsigned int column;
	
	string type;*/
	
	this->cargo = cargo;
	this->lineIndex = lineIndex;
	this->column = column;
	this->type = type;
}

tokenType token::getType(){
	return this->type;
}

unsigned int token::getLine(){
	return this->lineIndex;
}

unsigned int token::getCol(){
	return this->column;
}

string token::getCargo(){
	return this->cargo;
}

//token print statement
ostream& operator<< (ostream& stream, const token& tok){
	//display function for each token
	
	
	const char* s = 0;
	
	#define PROCESS_VAL(p) case(p): s = #p; break;
	switch(tok.type){
		PROCESS_VAL(LABEL);
		PROCESS_VAL(REF_LABEL);
		PROCESS_VAL(OPCODE);
		PROCESS_VAL(REGISTER);
		PROCESS_VAL(DECNUM);
		PROCESS_VAL(HEXNUM);
		PROCESS_VAL(BINUM);
		PROCESS_VAL(COMMENT);
		PROCESS_VAL(REGOFFSET);
		PROCESS_VAL(UNDEF);
	}
	#undef PROCESS_VAL
	
	stream << tok.cargo << " " << s << " " << tok.lineIndex << " " << tok.column;
	return stream;
}

	

