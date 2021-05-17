#include <bits/stdc++.h>
#include <iostream>
#include <string>

using namespace std;

int MAX_MEMORY = (1<<20), 										// Maximum memory
	MAX_VALUE_IN_MEMORY = (1<<8)-1,								// Each memory address can store 1 byte
	MAX_VALUE_IN_REGISTER = (1ll<<32)-1, 						// Max value in memory
	cycle = 0,													// Number of cycle cycle
	SZ_DRAM = (1<<10),
	BUFFER_UPDATE = 0,
	QUEUE_SIZE = 0,												// Stores size of the MRQ
	MAX_QUEUE = 6,												// Maximum size of MRM Queue
	ROW_ACCESS_DELAY,
	COL_ACCESS_DELAY,
	N, M;
short int dram[(1<<20)+1] = {0};								// DRAM, memory array
vector<vector<int>> MRM;										// Memory Request Queue

struct core{
	int index,													// Index of the current core
		registers[32] = {0};									// Values of registers
	
	int PC = 0,													// PC stores the address of current instruction
		memory_changed = -1,									// 0 if register changed, 1 if memory changed, -1 if none changed
		ind_changing = -1,										// Index of memory or register changed
		changed_val = -1;										// Changed value of memory or Register
	
	unordered_map<int,int> reg_change;							// Stores the instruction affecting registers
	unordered_map<string, int> reg_name_to_number, 				// Map of (conventional register name, register number)
							instructions_count,					// Map to hold count of instructions
							branches;							// Map of (branch label, pointer to the branch in instructions)
	
	string input_file;											// input file
	string output_s;											// output text
	fstream output;												// new file stream to write output
	vector<string> instructions, regs, instructions_name;		// stores the input instructions
	short int memory[1000] = {0};								// Array to hold memory
	
	//------- Status Variables -------
	int curr_instruction = 0,									// current instruction being processed
		status = 1;												// 1 if last instruction is finished, 0 if waiting
																// 5 if file is finished
	
	//--------------------------------------- Helper and Checker Functions ---------------------------------------
	
	bool isNumber(string str) {
		// check if a string is valid integer
		int i = (str[0]=='-');
		while (i<str.length()) {
			if (str[i]<'0' || str[i]>'9') return false;
			i++;
		}
		return true;
	}
	
	bool isValidRegister(string str, bool modifiable = false) {
		if (str[0] != '$') return false;
		str = str.substr(1);
		if ((str == "zero" || str == "0") && modifiable) return false;
		return reg_name_to_number.find(str) != reg_name_to_number.end() || 
			(isNumber(str) && stoi(str) >= 0 && stoi(str) < 32);// Check if a string represents valid register
	}
	
	bool isValidInstruction(string str) {
		// check if a string represents valid instruction
		return instructions_count.find(str) != instructions_count.end();
	}
	
	int getRegister(int ind) {
		// returns the register number
		// Condition :- instructions[ind] is a valid register
		string s = instructions[ind].substr(1);
		if (isNumber(s)) return stoi(s);
		else return reg_name_to_number[s];
	}
	
	int getInstructionMemory(int ind) {
		// returns the index of the instruction in memory
		int mem = isNumber(instructions[ind]) ? stoi(instructions[ind]) : branches[instructions[ind]];
		if (mem >= PC || mem%4) return -1;
		return mem;
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
	
	bool not_safe(int ind, bool modifiable = false) {
		// check if a given register is being modified or not
		return modifiable?(getRegister(ind)==0):(QUEUE_SIZE>0 && reg_change.find(getRegister(ind)) != reg_change.end());
	}
	
	//--------------------------------------- Initialisation and Filer Reading ---------------------------------------
	
	void initialize() {
		// initialise all registers and hashmaps
		regs = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3","t4",
				"t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9",
				"k0", "k1", "gp", "sp", "fp", "ra"};			// All the valid registers
		instructions_name = {"add", "sub", "mul", "beq", "bne", 
							"slt", "j", "lw", "sw", "addi"};	// All the valid instructions
		for (int i=0; i<regs.size(); i++) {
			reg_name_to_number.insert({regs[i], i});
		}
		for (int i=0; i<instructions_name.size(); i++) {
			instructions_count.insert({instructions_name[i], 0});
		}
	}
	
	void read_file(string input_file) {
		// read the input file
		freopen(input_file.c_str(), "r", stdin);
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
		
		// open a file for output
		output.open("t" + to_string(index) + "_out.txt", ios::out);
	}
	
	int lexParse() {
		// Function to check the syntax of the input file and load instructions to memory
		int i = 0, n = instructions.size();
		while (i<n) {
			PC += 4;
			if (PC > 1000) {
				return -2;
			}
			if (instructions[i] == "add" || instructions[i] == "sub" || instructions[i] == "mul" || instructions[i] == "slt") {
				if (i+3 >= n) return -1;
				if (!isValidRegister(instructions[i+1], true) ||
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
				if (!isValidRegister(instructions[i+1], true) ||
					!isValidRegister(instructions[i+2]) ||
					!isNumber(instructions[i+3])) return -1;

				memory[PC-4] = i;
				i += 4;
			}
			else {
				PC -= 4;				
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
	
	//--------------------------------------- Execution of Instructions ---------------------------------------
	
	void add(int ind) {
		// execute an add instruction
		if (not_safe(ind+1, true) || not_safe(ind+2) || not_safe(ind+3)){
			pauseInstructions(ind);
			return;
		}
		long long int sum = registers[getRegister(ind+2)] + 0ll + registers[getRegister(ind+3)];
		memory_changed = 0;
		ind_changing = getRegister(ind+1);
		registers[getRegister(ind+1)] = sum & MAX_VALUE_IN_REGISTER;
		changed_val = registers[ind_changing];
		popRequest(ind+1);
	}
	
	void sub(int ind) {
		// execute a sub instruction
		if (not_safe(ind+1, true) || not_safe(ind+2) || not_safe(ind+3)){
			pauseInstructions(ind);
			return;
		}
		long long int dif = registers[getRegister(ind+2)] + 0ll - registers[getRegister(ind+3)];
		memory_changed = 0;
		ind_changing = getRegister(ind+1);
		registers[getRegister(ind+1)] = dif & MAX_VALUE_IN_REGISTER;
		changed_val = registers[ind_changing];
		popRequest(ind+1);
	}
	
	void mul(int ind) {
		// execute a mul instruction
		if (not_safe(ind+1, true) || not_safe(ind+2) || not_safe(ind+3)){
			pauseInstructions(ind);
			return;
		}
		long long int prod = registers[getRegister(ind+2)] * 1ll * registers[getRegister(ind+3)];
		memory_changed = 0;
		ind_changing = getRegister(ind+1);
		registers[getRegister(ind+1)] = prod & MAX_VALUE_IN_REGISTER;
		changed_val = registers[ind_changing];
		popRequest(ind+1);
	}
	
	int beq(int ind) {
		// execute a beq instruction
		while (not_safe(ind+2) || not_safe(ind+1)) pauseInstructions(ind);
		if (getInstructionMemory(ind+3) == -1) return -1; 
		return (registers[getRegister(ind+1)] == registers[getRegister(ind+2)]) ? memory[getInstructionMemory(ind+3)] : ind+4;
	}
	
	int bne(int ind) {
		// execute a bne instruction
		while (not_safe(ind+2) || not_safe(ind+1)) pauseInstructions(ind);
		if (getInstructionMemory(ind+3) == -1) return -1; 
		return (registers[getRegister(ind+1)] != registers[getRegister(ind+2)]) ? memory[getInstructionMemory(ind+3)] : ind+4;
	}
	
	void slt(int ind) {
		// execute a slt instruction
		if (not_safe(ind+1, true) || not_safe(ind+2) || not_safe(ind+3)){
			pauseInstructions(ind);
			return;
		}
		memory_changed = 0;
		ind_changing = getRegister(ind+1);
		registers[getRegister(ind+1)] = registers[getRegister(ind+2)] < registers[getRegister(ind+3)];
		changed_val = registers[ind_changing];
		popRequest(ind+1);
	}
	
	int j(int ind) {
		// execute a j instruction
		int mem = getInstructionMemory(ind+1);
		return (mem==-1) ? -1 : memory[mem];
	}
	
	int lw(int ind) {
		// execute an lw instruction
		pair<string,int> temp = offset(instructions[ind+2]);
		int reg, mask=0, delay=0;
		if (!isNumber(temp.first.substr(1))) reg = reg_name_to_number[temp.first.substr(1)];
		else reg = stoi(temp.first.substr(1));
		int mem = registers[reg] + temp.second;
		if (mem >= MAX_MEMORY) return -1;
		int reg1 = getRegister(ind+1);
		registers[reg1] = 0;
		// for (int i=0; i<4; i++) {
			// if ((mem+i)%SZ_DRAM == 0 && i) return -1;
			// registers[reg1] += (memory[mem+i] << mask);
			// mask += 8;
		// }
		memRequest(ind);
		return 0;
	}

	int sw(int ind) {
		// execute an sw instruction
		pair<string,int> temp = offset(instructions[ind+2]);
		int reg, mask=0, delay = 0;
		if (!isNumber(temp.first.substr(1))) reg = reg_name_to_number[temp.first.substr(1)];
		else reg = stoi(temp.first.substr(1));
		int mem = registers[reg] + temp.second;
		if (mem >= MAX_MEMORY) return -1;
		int val = registers[getRegister(ind+1)];
		// for (int i=0; i<4; i++) {
			// if ((mem+i)%SZ_DRAM == 0 && i) return -1;
			// memory[mem+i] = (val >> mask) & MAX_VALUE_IN_MEMORY;
			// mask += 8;
		// }
		memRequest(ind);
		popRequest(ind+1);
		return 0;
	}
	
	void addi(int ind) {
		// execute an addi instruction
		if (not_safe(ind+1, true) || not_safe(ind+2)){
			pauseInstructions(ind);
			return;
		}
		long long int sum = registers[getRegister(ind+2)] + 0ll + stoi(instructions[ind+3]);
		memory_changed = 0;
		ind_changing = getRegister(ind+1);
		registers[getRegister(ind+1)] = sum & MAX_VALUE_IN_REGISTER;
		changed_val = registers[ind_changing];
		popRequest(ind+1);
	}
	
	//--------------------------------------- Simulation Functions ---------------------------------------
	
	void memRequest(int ind){
		// send request to access memory to MRM
		if(QUEUE_SIZE > MAX_QUEUE){
			pauseInstructions(ind);
			return;
		}
		QUEUE_SIZE += 1;
		vector<int> temp;
		temp.push_back(index);
		temp.push_back(ind);
		temp.push_back(cycle);
		
		// Add dependency of registers
		if (instructions[ind] == "lw") {
			int reg = getRegister(ind+1);
			if (reg_change.find(reg) != reg_change.end()) reg_change[reg] = ind;
			else reg_change.insert({reg, ind});
		}
	}
	
	void pauseInstructions(int ind){
		// ask core to pause processing of instructions till value extracted from Memory
		curr_instruction = ind;
		status = 0;
	}
	
	void nextCycle(){
		// operate the nextCycle
		if(status == 5) return;
		if(curr_instruction >= instructions.size()){
			status = 5;
			printContents();
			return;
		}
		int temp = processInstruction(curr_instruction);
		printCycle();
		if(status == 1) curr_instruction = temp;
		status = 1;
		memory_changed = -1;
	}
	
	int processInstruction(int i){
		// processes the instruction at index i
		if (instructions[i]=="add") {
			add(i);
			i += 4;
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["add"]++;
		}
		else if (instructions[i]=="sub") {
			sub(i);
			i += 4;
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["sub"]++;
		}
		else if (instructions[i]=="mul") {
			mul(i);
			i += 4;
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["mul"]++;
		}
		else if (instructions[i]=="beq") {
			i = beq(i);
			if (i==-1) {
				cout << "Trying to access invalid instruction in memory." << endl;
				return -1;
			}
			memory_changed = -1;
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["beq"]++;
		}
		else if (instructions[i]=="bne") {
			i = bne(i);
			if (i==-1) {
				return -1;
			}
			memory_changed = -1;
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["bne"]++;
		}
		else if (instructions[i]=="slt") {
			slt(i);
			i += 4;
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["slt"]++;
		}
		else if (instructions[i]=="j") {
			i = j(i);
			if(status == 1) instructions_count["j"]++;
			memory_changed = -1;
			if (i==-1) {
				cout << "Trying to access invalid instruction in memory" << endl;
				return -1;
			}
			output_s = "Executed ";
			for (int j=0; j<2; j++) output_s = output_s + instructions[i+j] + " ";
		}
		else if (instructions[i]=="lw") {
			int res = lw(i);
			if (res == -1) {
				cout << "Trying to load from invalid memory location" << endl;
				return -1;
			}
			memory_changed = -1;
			output_s = "DRAM request issued by ";
			for (int j=0; j<3; j++) output_s = output_s + instructions[i+j] + " ";
			i += 3;
			if(status == 1) instructions_count["lw"]++;
		}
		else if (instructions[i]=="sw") {
			int res = sw(i);
			if (res == -1) {
				cout << "Trying to store in invalid memory location" << endl;
				return -1;
			}
			memory_changed = -1;
			output_s = "DRAM request issued by ";
			for (int j=0; j<3; j++) output_s = output_s + instructions[i+j] + " ";
			i += 3;
			if(status == 1) instructions_count["sw"]++;
		}
		else if (instructions[i]=="addi") {
			addi(i);
			i += 4;
			memory_changed = -1;
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["addi"]++;
		}
		else i += 1;
		return i;
	}
	
	void printCycle(){
		if(status == 1){
			output << "Cycle " << cycle << ":" << endl << endl;
			output << output_s << endl;
			if(memory_changed == 0){
				output << "Register $" << regs[ind_changing] << " contents changed." << endl;
				output << "New value in $" << regs[ind_changing] << " : " << changed_val << endl;
			}
			else if(memory_changed == 1){
				output << "Memory Location " << ind_changing << " changed." << endl;
				output << "New value at " << ind_changing << "-" << ind_changing + 4 << " : " << changed_val << endl;
			}
			output << "--------------------------------------------------------------" << endl;
		}
	}
	
	void printContents(){
		output << endl << "Number of clock cycles: " << cycle << endl;
		output << "Buffer Updates: " << BUFFER_UPDATE << endl;
		output << "Frquency of instructions: " << endl;
		for (auto ele : instructions_count) output << ele.first << " : " << ele.second << ";  ";
		output << "--------------------------------------------------------------" << endl;
		output << "Register Contents" << endl;
		for (int i=0; i<32; i++) {
			if (registers[i]) output << "$" << regs[i] << " : " << registers[i] << "; ";
		}
		output << "--------------------------------------------------------------" << endl;
	}
	
	void popRequest(int ind){
		// check and remove an existing lw request being overwritten
		if(reg_change.find(getRegister(ind)) != reg_change.end()){
			int inst = reg_change[getRegister(ind)];
			for(int i = 0; i < QUEUE_SIZE; i++){
				if(MRM[i][0] == index && MRM[i][1] == inst){
					MRM.erase(MRM.begin()+i);
					QUEUE_SIZE --;
				}
			}
		}
	}
};

struct MRM{
	
};

int main(){
	cout << "Number of input files , N: ";
	cin >> N;	
	cout << "Number of cycles to be executed, M: ";
	cin >> M;
	string files[N];
	string file_name;
	for(int i = 1; i <= N; i++){
		cout << "Name of the file " << i << " (0 for easy mode) : ";
		cin >> file_name;
		if(file_name == "0") file_name = "t" + to_string(i) + ".txt";
	}
	cout << "ROW_ACCESS_DELAY, in number of cycles: ";
	cin >> ROW_ACCESS_DELAY;
	cout << "COL_ACCESS_DELAY in number of cycles: ";
	cin >> COL_ACCESS_DELAY;
	
	struct core CPU[N];
	for(int i = 0; i < N; i++){
		CPU[i].index = i;
		CPU[i].initialize();
		CPU[i].read_file(files[i]);
		int res = CPU[i].lexParse();
		if (res == -1) {
			cout << "Invalid syntax" << endl;
			return -1;
		}
		else if (res == -2) {
			cout << "Instruction memory is full! Too Many Instructions.)" << endl;
			return -1;
		}
	}
}


















