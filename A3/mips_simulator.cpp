#include <bits/stdc++.h>

using namespace std;

int MAX_MEMORY = (1<<20), 				// Maximum memory
	MAX_VALUE_IN_MEMORY = (1<<8)-1,		// Each memory address can store 1 byte
    PC = 0,								// PC stores the address of current instruction
    cycle = 0,							// Number of clock cycle
    registers[32] = {0};			// Values of registers

short int memory[(1<<20)+1] = {0};			// Array to hold memory


unordered_map<string, int> reg_name_to_number, 	// Map of (conventional register name, register number)
						   instructions_count,		// Map to hold count of instructions
						   branches;			// Map of (branch label, pointer to the branch in instructions)

string input_file;						// input file
vector<string> instructions;			// stores the input instructions

void initialize() {
	vector<string> regs = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
						   "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
						   "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
						   "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};		// All the valid registers

	vector<string> instructions_name = {"add", "sub", "mul", "beq", "bne", 
										"slt", "j", "lw", "sw", "addi"};		// All the valid instructions

	for (int i=0; i<regs.size(); i++) {
		reg_name_to_number.insert({regs[i], i});
	}

	for (int i=0; i<instructions_name.size(); i++) {
		instructions_count.insert({instructions_name[i], 0});
	}

}

bool isNumber(string str) {
	int i = (str[0]=='-');
	while (i<str.length()) {
		if (str[i]<'0' || str[i]>'9') return false;
		i++;
	}
	return true;
}

bool isValidRegister(string str) {
	if (str[0] != '$') return false;
	str = str.substr(1);
	cout << str << endl;
	return reg_name_to_number.find(str) != reg_name_to_number.end() || 
		   (isNumber(str) && stoi(str) >= 0 && stoi(str) < 32);
}

bool isValidInstruction(string str) {
	return instructions_count.find(str) != instructions_count.end();
}


bool isValidOffset(string s) {
	// To do
	return true;
}

bool lexParse() {
	int i = 0, n = instructions.size();

	while (i<n) {
		// cout << "ha" << endl;
		PC += 4;
		if (instructions[i] == "add" || instructions[i] == "sub" || instructions[i] == "mul" || instructions[i] == "slt") {
			if (i+3 >= n) return false;
			if (!isValidRegister(instructions[i+1]) ||
				!isValidRegister(instructions[i+2]) ||
				!isValidRegister(instructions[i+3])) return false;

			i += 4;
		}
		else if (instructions[i] == "beq" || instructions[i] == "bne") {
			if (i+3 >= n) return false;
			if (!isValidRegister(instructions[i+1]) ||
				!isValidRegister(instructions[i+2]) ||
				isValidRegister(instructions[i+3]) || 
				isValidInstruction(instructions[i+3])) return false;

			branches.insert({instructions[i+3], i+3});
			i += 4;
		}
		else if (instructions[i] == "j") {
			if (i+1 >= n) return false;
			if (isValidRegister(instructions[i+1]) || 
				isValidInstruction(instructions[i+1])) return false;

			branches.insert({instructions[i+1], i+1});
			i += 2;
		}
		else if (instructions[i] == "lw" || instructions[i] == "sw") {
			if (i+2 >= n) return false;
			if (!isValidRegister(instructions[i+1]) ||
				!(isValidRegister(instructions[i+2]) || 
				  isValidOffset(instructions[i+2]))) return false;

			i += 3;
		}
		else if (instructions[i] == "addi") {
			if (i+3 >= n) return false;
			if (!isValidRegister(instructions[i+1]) ||
				!isValidRegister(instructions[i+2]) ||
				!isNumber(instructions[i+3])) return false;

			i += 4;
		}
		else {
			if (instructions[i].back() == ':') i += 1;
			else {
				if (i+1 >= n) return false;
				if (instructions[i+1] != ":") return false;
				i += 2;
			}
			PC -= 4;
		}
	}

	return true;
}

int main() 
{
	cout << "Enter file name: ";
	cin >> input_file;

	freopen(input_file.c_str(), "r", stdin);

	initialize(); 	// initiallize varaibles

	// Reading the input file
	string instruction;
	while (cin >> instruction) {
		instructions.push_back(instruction);	
	}

	// lex and parse the input instructions
	if (!lexParse()) {
		cout << "Invalid syntax" << endl;
		return 0;
	}



}

