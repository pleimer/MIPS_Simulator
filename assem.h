#ifndef ASSEM_H
#define ASSEM_H


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <exception>
#include <map>
#include <vector>

using namespace std;

typedef unsigned char byte;

#define INST_SIZE 32
#define OP_SIZE 6
#define REG_REF_SIZE 5

#define LABEL_MASK 0x001FFFFF
#define IMM_MASK 0x0000FFFF

//typedef enum {LW, SW, ADD, ADDI, SUB, SUBI, XOR, BLTZ, BNEZ, EOP} opcode_t;

typedef enum {
	LABEL,
	REF_LABEL,
	OPCODE,
	REGISTER,
	DECNUM,
	HEXNUM,
	BINUM,
	COMMENT,
	REGOFFSET,
	UNDEF
} tokenType;

class token{
	//contains information about each element in .asm file (each word)
	string cargo;
	string sourceText;
	unsigned int lineIndex;
	unsigned int column;
	
	tokenType type;
	
	
	public:
		token(string cargo, unsigned int lineIndex, unsigned int column, tokenType type);
		void show();
		
		friend ostream& operator<< (ostream& stream, const token&);
		
		tokenType getType();
		unsigned int getCol();
		unsigned int getLine();
		string getCargo();
};

class assembler{
	//assembles all tokens to machine code and store in instruction memory
	ifstream* openFile(string path);
	
	public:
		int test;
		void assemble(string path, byte *);
		byte mapCode(token tok);
};


class lexer{
	//used to get the next token in the .asm file 
	//	Parses through whitespace, determines lines, passes comments, etc
	//	until it encounters EOF
		
		static unsigned int cursor;
		ifstream asmFile;
		
		void split(const string &s, const char* delim, vector<string> & v);
		
		unsigned lineIndex;
		
		
	public:
		lexer(string path);
		~lexer();
		vector<token> get();
		tokenType detType(string token_str, unsigned int column);
		void reset_file_pointer();

};

//#include "sim.h"

#endif
