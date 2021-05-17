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
	row_buffer = -1,											// Row copied in row buffer
	col_accessed = -1,											// Column last accessed stored in cache
	N, M;

short int dram[(1<<20)+1] = {0};								// DRAM, memory array
vector<vector<int>> MRM;										// Memory Request Queue

int stats = 0,
	// 0 if finished, 1 if col access left, 2 if row copying left, 3 if writeback left
	cycles_left = 0,											// cycles remaining in completing current instruction
	queue_status = 1,											// 0 if reordering, 1 otherwise
	stop_current = 0,											// stop execution of current request
	reorder_left = 0;											// cycles remaining in reordering

vector<int> curr_request;										// current request being processed

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
	
	int recieve = 0,											// 1 if recieved value from MRM
		index_op, lwCount = 0;
	
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
		fstream in;
		in.open(input_file, ios::in);
		string instruction;
		while (in >> instruction) {
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
		output.open("t" + to_string(index + 1) + "_out.txt", ios::out);
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
		for (int i=0; i<4; i++) {
			if ((mem+i)%SZ_DRAM == 0 && i) return -1;
			registers[reg1] += (dram[mem+i] << mask);
			mask += 8;
		}
		lwCount ++;
		popRequest(ind+1);
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
		for (int i=0; i<4; i++) {
			if ((mem+i)%SZ_DRAM == 0 && i) return -1;
			dram[mem+i] = (val >> mask) & MAX_VALUE_IN_MEMORY;
			mask += 8;
		}
		memRequest(ind);
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
		QUEUE_SIZE ++;
		vector<int> temp;
		temp.push_back(index);
		temp.push_back(ind);
		temp.push_back(cycle);
		temp.push_back(0);
		MRM.push_back(temp);
		
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
		int temp = -1000;
		if(status == 5) return;
		if(lwCount == 0 && curr_instruction >= instructions.size()){
			status = 5;
			printContents();
			return;
		}
		if(recieve == 1 && instructions[index_op] == "lw"){
			lwCount--;
			memory_changed = 0;
			ind_changing = getRegister(index_op+1);
			changed_val = registers[ind_changing];
			output_s = "Executed ";
			for (int j=0; j<3; j++) output_s = output_s + instructions[index_op+j] + " ";
			recieve = 0;
		}
		else{
			temp = processInstruction(curr_instruction);
		}
		printCycle();
		if(status == 1 && temp != -1000) curr_instruction = temp;
		status = 1;
		memory_changed = -1;
	}
	
	int processInstruction(int i){
		// processes the instruction at index i
		if (instructions[i]=="add") {
			add(i);
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["add"]++;
			i += 4;
		}
		else if (instructions[i]=="sub") {
			sub(i);
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["sub"]++;
			i += 4;
		}
		else if (instructions[i]=="mul") {
			mul(i);
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["mul"]++;
			i += 4;
		}
		else if (instructions[i]=="beq") {
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			i = beq(i);
			if (i==-1) {
				cout << "Trying to access invalid instruction in memory." << endl;
				return -1;
			}
			memory_changed = -1;
			if(status == 1) instructions_count["beq"]++;
		}
		else if (instructions[i]=="bne") {
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			i = bne(i);
			if (i==-1) {
				return -1;
			}
			memory_changed = -1;
			if(status == 1) instructions_count["bne"]++;
		}
		else if (instructions[i]=="slt") {
			slt(i);
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["slt"]++;
			i += 4;
		}
		else if (instructions[i]=="j") {
			output_s = "Executed ";
			for (int j=0; j<2; j++) output_s = output_s + instructions[i+j] + " ";
			i = j(i);
			if(status == 1) instructions_count["j"]++;
			memory_changed = -1;
			if (i==-1) {
				cout << "Trying to access invalid instruction in memory" << endl;
				return -1;
			}
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
			memory_changed = -1;
			output_s = "Executed ";
			for (int j=0; j<4; j++) output_s = output_s + instructions[i+j] + " ";
			if(status == 1) instructions_count["addi"]++;
			i += 4;
		}
		else i += 1;
		return i;
	}
	
	void printCycle(){
		if(status == 1){
			output << "Cycle " << cycle << ":" << endl << endl;
			output << output_s << endl;
			if(recieve == 1 && instructions[index_op] == "sw"){
				output_s = "Finished ";
				for (int j=0; j<3; j++) output_s = output_s + instructions[index_op+j] + " ";
				recieve = 0;
			}
			if(memory_changed == 0){
				output << "Register $" << regs[ind_changing] << " contents changed." << endl;
				output << "New value in $" << regs[ind_changing] << " : " << changed_val << endl;
			}
			else if(memory_changed == 1){
				output << "Memory Location " << ind_changing << " changed." << endl;
				output << "New value at " << ind_changing << "-" << ind_changing + 4 << " : " << changed_val << endl;
			}
			output << "--------------------------------------------------------------" << endl;
			output_s = "";
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
			int i;
			for(i = 0; i < QUEUE_SIZE; i++){
				if(MRM[i][0] == index && MRM[i][1] == inst){
					MRM.erase(MRM.begin()+i);
					QUEUE_SIZE --;
					break;
				}
			}
			if(i == QUEUE_SIZE) stop_current = 1;
			reg_change.erase(reg_change.find(getRegister(ind)));
		}
	}
};

core CPU[16];

void prioritize(int i){
	// shift ith element to front of the queue
	vector<int> some;
	some.push_back(MRM[i][0]);
	some.push_back(MRM[i][1]);
	some.push_back(MRM[i][2]);
	some.push_back(MRM[i][3]);
	MRM.erase(MRM.begin()+i);
	MRM.insert(MRM.begin()+0,some);
}

bool find(unordered_set<int> set, int i){
	// check if an element is in the set
	return !(set.find(i) == set.end());
}

void reorder(){
	// finds the most prior DRAM request
	int top = -1, topcol = -1, toprow = -1, topold = -1;
	int stat = 0; // 0 means no condition, 1 means got a value 
	unordered_set<int> reg_aff[N];
	for(int i = 0; i < MRM.size(); i++){
		int j = MRM[i][0], ind = MRM[i][1], reg = 0, wait = MRM[i][3];
		
		pair<string, int> temp = CPU[j].offset(CPU[j].instructions[ind+2]);
		if (!CPU[j].isNumber(temp.first.substr(1))) reg = CPU[j].reg_name_to_number[temp.first.substr(1)];
		else reg = stoi(temp.first.substr(1));
		int mem = temp.second + CPU[j].registers[reg];
		
		int registerVal = CPU[j].getRegister(ind+1);
		if(find(reg_aff[j], registerVal) || find(reg_aff[j], reg)) continue;
		reg_aff[j].insert(registerVal);
		if(topold == -1 && wait > 26){
			topold = i;
		}
		else if(toprow == -1 && mem/SZ_DRAM == row_buffer){
			toprow = i;
		}
		else if(topcol == -1 && mem == col_accessed){
			topcol = i;
		}
	}
	if(topcol != -1) top = topcol;
	else if(topold != -1) top = topold;
	else if(toprow != -1) top = toprow;
	if(top != -1) prioritize(top);
	reorder_left = 5;
	queue_status = 1;
}

void writeBack(fstream &out){
	// writes current row back to memory
	stats = 3;
	cycles_left = 10;
	out << "DRAM Writeback Initiated. "<< endl;
}

void copyRow(){
	// copies required row into row buffer
	stats = 2;
	cycles_left = 2;
}

void getCol(){
	// extracts required data from row buffer
	stats = 1;
	cycles_left = 0;
}

void sendValue(){
	// sends output value to the core
	int i = curr_request[0], ind = curr_request[1];
	CPU[i].recieve = 1;
	CPU[i].index_op = ind;
	stats = 0;
}

void nextStep(fstream &out){
	// proceeds to the next step in memory extraction
	
	int i = curr_request[0], ind = curr_request[1], reg;
	pair<string, int> temp = CPU[i].offset(CPU[i].instructions[ind+2]);
	if (!CPU[i].isNumber(temp.first.substr(1))) reg = CPU[i].reg_name_to_number[temp.first.substr(1)];
	else reg = stoi(temp.first.substr(1));
	int mem = CPU[i].registers[reg] + temp.second;
	
	switch(stats){
		case 1 : {
			sendValue();
			out << "Memory Location " << mem << " accessed." << endl;
			col_accessed = mem;
			break;
		}
		case 2 : {
			getCol();
			out << "Row " << mem/SZ_DRAM  << " copied to buffer"<< endl;
			row_buffer = mem/SZ_DRAM;
			break;
		}
		case 3 : {
			copyRow();
			out << "DRAM Writeback finished, written row " << row_buffer << endl;
			break;
		}
	}
}

void clearQueue(fstream &out){
	// retrieve and process the topmost request
	QUEUE_SIZE--;
	curr_request = MRM[0];
	MRM.erase(MRM.begin());
	
	int i = curr_request[0], ind = curr_request[1], reg;
	pair<string, int> temp = CPU[i].offset(CPU[i].instructions[ind+2]);
	if (!CPU[i].isNumber(temp.first.substr(1))) reg = CPU[i].reg_name_to_number[temp.first.substr(1)];
	else reg = stoi(temp.first.substr(1));
	int mem = CPU[i].registers[reg] + temp.second;
	
	if(col_accessed == mem){
		sendValue();
	}
	else if(row_buffer == mem/SZ_DRAM){
		getCol();
	}
	else if(row_buffer == -1){
		copyRow();
	}
	else{
		writeBack(out);
	}
}

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
		files[i-1] = file_name;
	}
	cout << "ROW_ACCESS_DELAY, in number of cycles: ";
	cin >> ROW_ACCESS_DELAY;
	cout << "COL_ACCESS_DELAY in number of cycles: ";
	cin >> COL_ACCESS_DELAY;
	
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
	
	fstream out_MRM;
	out_MRM.open("MRM_out.txt", ios::out);
	
	for(cycle = 1; cycle <= M; cycle++){
		out_MRM << "Cycle " << cycle << ":" << endl;
		reorder_left = max(0, reorder_left-1);
		for(int i = 0; i < QUEUE_SIZE; i++) MRM[i][3]++;
		// if(stop_current){
			
		// }
		
		if(reorder_left == 0){
			if(queue_status == 0) out_MRM << "Memory Requests Reordering finished." << endl;
			queue_status = 1;
			else if(QUEUE_SIZE > 0 && stats == 0){
				clearQueue(out_MRM);
				out_MRM << "Starting to execute ";
				for (int j=0; j<3; j++) out_MRM << CPU[curr_request[0]].instructions[curr_request[1]+j] + " ";
				out_MRM << endl << "Reordering Started." << endl;
				reorder();
			}
			else if(stats != 0 && cycles_left == 0){
				nextStep(out_MRM);
			}
			else out_MRM << "No Memory Requests." << endl;
		}
		out_MRM << "--------------------------------------------------------------" << endl;
		
		for(int i = 0; i < N; i++){
			CPU[i].nextCycle();
		}
	}
}


















