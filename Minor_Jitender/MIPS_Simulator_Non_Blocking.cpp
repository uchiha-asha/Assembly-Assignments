#include <bits/stdc++.h>

using namespace std;

int MAX_MEMORY = (1<<20), 					// Maximum memory
	MAX_INSTRUCTIONS_MEMORY = (1000),		// Maximum memory instructions can occupy
	MAX_VALUE_IN_MEMORY = (1<<8)-1,			// Each memory address can store 1 byte
	MAX_VALUE_IN_REGISTER = (1ll<<32)-1,	// Max value in memory
    PC = 0,									// PC stores the address of current instruction
    cycle = 0,								// Number of cycle cycle
    registers[32] = {0},					// Values of registers
	row_buffer = -1,						// Provides the row that is being loaded
	ROW_ACCESS_DELAY, COL_ACCESS_DELAY;
string instruction;							// Store the current instruction
short int memory[(1<<20)+1] = {0};			// Array to hold memory
unordered_map<string, int> reg_name_to_number, 		// Map of (conventional register name, register number)
						   instructions_count,		// Map to hold count of instructions
						   branches;				// Map of (branch label, pointer to the branch in instructions)
string input_file;
vector<string> instructions;

int reg_progress = -1, clock_start = -1, clock_end = -1;
string last_instruction = "";

void initialize() {
	vector<string> regs = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3",
						   "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
						   "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};										// valid registers
	vector<string> instructions_name = {"add", "sub", "mul", "beq", "bne", "slt", "j", "lw", "sw", "addi"};		// valid instructions
	for (int i=0; i<regs.size(); i++) {
		reg_name_to_number.insert({regs[i], i});
	}
	for (int i=0; i<instructions_name.size(); i++) {
		instructions_count.insert({instructions_name[i], 0});
	}
}

bool isNumber(string str) {
	// Check if a string is valid integer
	int i = (str[0]=='-');
	while (i<str.length()) {
		if (str[i]<'0' || str[i]>'9') return false;
		i++;
	}
	return true;
}

bool isValidRegister(string str) {
	// Check if a string represents valid register
	if (str[0] != '$') return false;
	str = str.substr(1);
	return reg_name_to_number.find(str) != reg_name_to_number.end() || (isNumber(str) && stoi(str) >= 0 && stoi(str) < 32);
}

bool isValidInstruction(string str) {
	// check if a string represents valid instruction
	return instructions_count.find(str) != instructions_count.end();
}

pair<string,int> offset(string s) {
	// parse the offset instructions
	string off = "", reg = "";
	int i=0, n=s.length();
	while (i<n && s[i] != '(') off += s[i], i++;
	i++;
	while (i<n && s[i] != ')') reg += s[i], i++;
	if (i!=n-1 || !isNumber(off)) return {"",0};
	return {reg, stoi(off)};
}

int lexParse() {
	// Function to check the syntax of the input file and load instructions to memory
	int i = 0, n = instructions.size();
	while (i<n) {
		PC += 4;
		if (PC > MAX_INSTRUCTIONS_MEMORY) {
			return -2;
		}
		if (instructions[i] == "add" || instructions[i] == "sub" || instructions[i] == "mul" || instructions[i] == "slt") {
			if (i+3 >= n) return -1;
			if (!isValidRegister(instructions[i+1]) ||
				!isValidRegister(instructions[i+2]) ||
				!isValidRegister(instructions[i+3])) return -1;
			memory[PC-4] = i;
			i += 4;
		}
		else if (instructions[i] == "beq" || instructions[i] == "bne") {
			if (i+3 >= n) return -1;
			if (!isValidRegister(instructions[i+1]) ||
				!isValidRegister(instructions[i+2]) ||
				isValidRegister(instructions[i+3]) || 
				isValidInstruction(instructions[i+3])) return -1;
			memory[PC-4] = i;
			i += 4;
		}
		else if (instructions[i] == "j") {
			if (i+1 >= n) return -1;
			if (isValidRegister(instructions[i+1]) || 
				isValidInstruction(instructions[i+1])) return -1;
			memory[PC-4] = i;
			i += 2;
		}
		else if (instructions[i] == "lw" || instructions[i] == "sw") {
			if (i+2 >= n) return -1;
			if (!isValidRegister(instructions[i+1]) ||
				!isValidRegister(offset(instructions[i+2]).first)) return -1;
			memory[PC-4] = i;
			i += 3;
		}
		else if (instructions[i] == "addi") {
			if (i+3 >= n) return -1;
			if (!isValidRegister(instructions[i+1]) ||
				!isValidRegister(instructions[i+2]) ||
				!isNumber(instructions[i+3])) return -1;
			memory[PC-4] = i;
			i += 4;
		}
		else {
			memory[PC-4] = i;
			string s = instructions[i];
			if (instructions[i].back() == ':') s = s.substr(0, int(s.length())-1), i += 1;
			else {
				if (i+1 >= n) return -1;
				if (instructions[i+1] != ":") return -1;
				i += 2;
			}
			if (isNumber(s)) return -1;
			if (branches.find(s) != branches.end()) return -1;
			branches.insert({s, PC});
		}
	}
	memory[PC] = n;
	PC += 4;
	return 0;
}

int getRegister(int ind) {
	// INV :- instructions[ind] is a valid register
	string s = instructions[ind].substr(1);
	if (isNumber(s)) return stoi(s);
	else return reg_name_to_number[s];
}

int getInstructionMemory(int ind) {
	int mem = isNumber(instructions[ind]) ? stoi(instructions[ind]) : branches[instructions[ind]];
	if (mem >= PC || mem%4) return -1;
	return mem;
}

void add(int ind) {
	long long int sum = registers[getRegister(ind+2)] + 0ll + registers[getRegister(ind+3)];
	registers[getRegister(ind+1)] = sum & MAX_VALUE_IN_REGISTER;
}

void sub(int ind) {
	long long int dif = registers[getRegister(ind+2)] + 0ll - registers[getRegister(ind+3)];
	registers[getRegister(ind+1)] = dif & MAX_VALUE_IN_REGISTER;
}

void mul(int ind) {
	long long int prod = registers[getRegister(ind+2)] * 1ll * registers[getRegister(ind+3)];
	registers[getRegister(ind+1)] = prod & MAX_VALUE_IN_REGISTER;
}

int beq(int ind) {
	if (getInstructionMemory(ind+3) == -1) return -1; 
	return (registers[getRegister(ind+1)] == registers[getRegister(ind+2)]) ? memory[getInstructionMemory(ind+3)] : ind+4;
}

int bne(int ind) {
	if (getInstructionMemory(ind+3) == -1) return -1; 
	return (registers[getRegister(ind+1)] != registers[getRegister(ind+2)]) ? memory[getInstructionMemory(ind+3)] : ind+4;
}

void slt(int ind) {
	registers[getRegister(ind+1)] = registers[getRegister(ind+2)] < registers[getRegister(ind+3)];
}

int j(int ind) {
	int mem = getInstructionMemory(ind+1);
	return (mem==-1) ? -1 : memory[mem];
}

int lw(int ind) {
	pair<string,int> temp = offset(instructions[ind+2]);
	int reg;
	if (!isNumber(temp.first.substr(1))) reg = reg_name_to_number[temp.first.substr(1)];
	else reg = stoi(temp.first.substr(1));
	int mem = registers[reg] + temp.second;
	if (mem < MAX_INSTRUCTIONS_MEMORY || mem > MAX_MEMORY-3) return -1;
	registers[getRegister(ind+1)] = memory[mem] + (memory[mem+1] << 8) + (memory[mem+2] << 16) + (memory[mem+3] << 24);
	return 0;
}

int sw(int ind) {
	pair<string,int> temp = offset(instructions[ind+2]);
	int reg;
	if (!isNumber(temp.first.substr(1))) reg = reg_name_to_number[temp.first.substr(1)];
	else reg = stoi(temp.first.substr(1));
	int mem = registers[reg] + temp.second;
	if (mem < MAX_INSTRUCTIONS_MEMORY || mem > MAX_MEMORY-3) return -1;
	int val = registers[getRegister(ind+1)];
	memory[mem] = val & MAX_VALUE_IN_MEMORY, memory[mem+1] = (val >> 8) & MAX_VALUE_IN_MEMORY, 
	memory[mem+2] = (val >> 16) & MAX_VALUE_IN_MEMORY, memory[mem+3] = (val >> 24) & MAX_VALUE_IN_MEMORY;
	return 0;
}

void addi(int ind) {
	long long int sum = registers[getRegister(ind+2)] + 0ll + stoi(instructions[ind+3]);
	registers[getRegister(ind+1)] = sum & MAX_VALUE_IN_REGISTER;
}

void printRegisters(int cycle, string instruction) {
	if (cycle == 1) cout << "----------------------------------------------------------------------------------" << endl;
	if (cycle != -1) cout << "Cycle Number: " << dec << cycle << endl;
	else cout << "Final Values" << endl;
	if (cycle != -1) cout << instruction << endl;
	for (int i=0; i<31; i++) {
		cout << hex << registers[i] << " ";
	}
	if (cycle != -1) cout << hex << registers[31] << endl << "----------------------------------------------------------------------------------" << endl;
	else{
		cout << hex << registers[31] << endl;
		cout << "\n";
		cout << "Memory Contents" << endl;
		for (int i=1000; i<MAX_MEMORY; i+=4) {
			int res = 0, mask = 0;
			for (int j=0; j<4; j++) res +=  (memory[j+i] << mask), mask += 8;
			if (res) cout << dec << i/1024 << "," << i%1024 << "-" << i%1024+3 << "\t:\t" << res << endl;
		}
		cout << endl;
		cout << "**********************************************************************************" << endl;
	}
}

int main(int argc, char *argv[]) 
{
	input_file = argv[1];
	ROW_ACCESS_DELAY = stoi(argv[2]);
	COL_ACCESS_DELAY = stoi(argv[3]);
	freopen(input_file.c_str(), "r", stdin);
	initialize(); 	// initiallize varaibles

	// Reading the input file
	string instruction;
	while (cin >> instruction) {
		string s = "";
		for (auto c : instruction) {
			if (c==',') {
				if (s!="") instructions.push_back(s);
				s = "";
			}
			else s += c;
		}
		if (s != "") instructions.push_back(s);	
	}
	
	// lex and parse the input instructions
	int res = lexParse();
	if (res == -1) {
		cout << "Invalid syntax" << endl;
		return -1;
	}
	else if (res == -2) {
		cout << "Memory is full! Don't write too many instructions, noob." << endl;
		return -1;
	}
	
	int i=0, n=instructions.size(), cnt = 0;
	while (i<n) {
		if (instructions[i]=="add") {
			if((getRegister(i+1) == reg_progress || getRegister(i+2) == reg_progress || getRegister(i+3) == reg_progress)){
				printRegisters(clock_end, last_instruction);
				cycle = clock_end;
				reg_progress = -1;
			}
			add(i);
			instruction = "";
			for (int j=0; j<4; j++) instruction += instructions[i+j] + " ";
			i += 4;
			instructions_count["add"]++, cycle++;
			printRegisters(cycle, instruction);
		}
		else if (instructions[i]=="sub") {
			sub(i);
			instruction = "";
			for (int j=0; j<4; j++) instruction += instructions[i+j] + " ";
			i += 4;
			instructions_count["sub"]++, cycle++;
		}
		else if (instructions[i]=="mul") {
			mul(i);
			instruction = "";
			for (int j=0; j<4; j++) instruction += instructions[i+j] + " ";
			i += 4;
			instructions_count["mul"]++, cycle++;
		}
		else if (instructions[i]=="beq") {
			instruction = "";
			for (int j=0; j<4; j++) instruction += instructions[i+j] + " ";
			i = beq(i);
			if (i==-1) {
				cout << "Trying to access invalid instruction in memory" << endl;
				return -1;
			}
			instructions_count["beq"]++, cycle++;
		}
		else if (instructions[i]=="bne") {
			instruction = "";
			for (int j=0; j<4; j++) instruction += instructions[i+j] + " ";
			i = bne(i);
			if (i==-1) {
				cout << "Trying to access invalid instruction in memory" << endl;
				return -1;
			}
			instructions_count["bne"]++, cycle++;
		}
		else if (instructions[i]=="slt") {
			instruction = "";
			for (int j=0; j<4; j++) instruction += instructions[i+j] + " ";
			slt(i);
			i += 4;
			instructions_count["slt"]++, cycle++;
		}
		else if (instructions[i]=="j") {
			instruction = "";
			for (int j=0; j<2; j++) instruction += instructions[i+j] + " ";
			i = j(i);
			if (i==-1) {
				cout << "Trying to access invalid instruction in memory" << endl;
				return -1;
			}
			instructions_count["j"]++, cycle++;
		}
		else if (instructions[i]=="lw") {
			if(reg_progress != -1){
				printRegisters(clock_end, last_instruction);
			}
			cycle = clock_end;
			instruction = "";
			for (int j=0; j<3; j++) instruction += instructions[i+j] + " ";
			int res = lw(i);
			if (res == -1) {
				cout << "Trying to load from invalid memory location" << endl;
				return -1;
			}
			pair<string,int> temp = offset(instructions[i+2]);
			int reg;
			if (!isNumber(temp.first.substr(1))) reg = reg_name_to_number[temp.first.substr(1)];
			else reg = stoi(temp.first.substr(1));
			int mem = registers[reg] + temp.second;
			cycle++;
			cout << "Cycle Number: " << dec << cycle << endl;
			cout << "DRAM Request Initiated" << endl;
			cout << "----------------------------------------------------------------------------------" << endl;
			clock_start = cycle;
			last_instruction = instruction;
			reg_progress = getRegister(i+1);
			if (row_buffer == -1){
				row_buffer = mem/1024;
				clock_end = cycle + ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
			}
			else if(row_buffer == mem/1024){
				clock_end = cycle + COL_ACCESS_DELAY;
			}
			else{
				row_buffer = mem/1024;
				clock_end = cycle + 2*ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
			}
			i += 3;
			instructions_count["lw"]++, cycle++;
		}
		else if (instructions[i]=="sw") {
			if(reg_progress != -1){
				printRegisters(clock_end, last_instruction);
			}
			cycle = clock_end;
			instruction = "";
			for (int j=0; j<3; j++) instruction += instructions[i+j] + " ";
			int res = sw(i);
			if (res == -1) {
				cout << "Trying to store in invalid memory location" << endl;
				return -1;
			}
			pair<string,int> temp = offset(instructions[i+2]);
			int reg;
			if (!isNumber(temp.first.substr(1))) reg = reg_name_to_number[temp.first.substr(1)];
			else reg = stoi(temp.first.substr(1));
			int mem = registers[reg] + temp.second;
			cycle++;
			cout << "Cycle Number: " << dec << cycle << endl;
			cout << "DRAM Request Initiated" << endl;
			cout << "----------------------------------------------------------------------------------" << endl;
			clock_start = cycle;
			last_instruction = instruction;
			reg_progress = getRegister(i+1);
			if (row_buffer == -1){
				row_buffer = mem/1024;
				clock_end = cycle + ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
			}
			else if(row_buffer == mem/1024){
				clock_end = cycle + COL_ACCESS_DELAY;
			}
			else{
				row_buffer = mem/1024;
				clock_end = cycle + 2*ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
			}
			i += 3;
			instructions_count["sw"]++, cycle++;
		}
		else if (instructions[i]=="addi") {
			if((getRegister(i+1) == reg_progress || getRegister(i+2) == reg_progress)){
				printRegisters(clock_end, last_instruction);
				cycle = clock_end;
				reg_progress = -1;
			}
			instruction = "";
			for (int j=0; j<4; j++) instruction += instructions[i+j] + " ";
			addi(i);
			i += 4;
			instructions_count["addi"]++, cycle++;
			printRegisters(cycle, instruction);
		}
		else {
			i += 1;
			continue;
		}
		if(cycle == clock_end && reg_progress != -1){
			printRegisters(clock_end, last_instruction);
			reg_progress = -1;
		}
	}
	if(cycle <= clock_end && reg_progress != -1){
		printRegisters(clock_end, last_instruction);
		cycle = clock_end;
	}
	assert(i==n);
	cout << "Number of clock cycles: " << dec << cycle << endl;
	cout << "Frquency of instructions:" << endl;
	for (auto ele : instructions_count) cout << ele.first << ":" << ele.second << "  ";
	cout << "\n" << "\n";
	printRegisters(-1, "");
	cout << "" << endl;
	return 0;
}