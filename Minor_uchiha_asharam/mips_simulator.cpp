#include <bits/stdc++.h>

using namespace std;

struct MIPS {
	int MAX_MEMORY = (1<<20), 				// Maximum memory
		MAX_INSTRUCTIONS_MEMORY = 1000,	// Maximum memory instructions can occupy
		MAX_VALUE_IN_MEMORY = (1<<8)-1,		// Each memory address can store 1 byte
		MAX_VALUE_IN_REGISTER = (1ll<<32)-1, // Max value in memory
	    PC = 0,								// PC stores the address of current instruction
	    cycle = 0,							// Number of cycle cycle
	    registers[32] = {0},			// Values of registers
	    SZ_DRAM = (1<<10),
	    COL_ACCESS_DELAY = 0,
	    ROW_ACCESS_DELAY = 0,
	    NON_BLOCKING_MODE = 0,
	    changed_reg = -1,
	    BUFFER_UPDATE = 0,
	    last_access_sw = 0;

	short int memory[(1<<20)+1] = {0};			// Array to hold memory

	int ROW_BUFFER = -1,
		QUEUE_SIZE = 0,
		memory_changed = 0,
		ind_changing = -1,
		changed_val = -1,
		cycle_end = -1,
		cycle_start = -1,
		index_op = -1;

	unordered_map<string, int> reg_name_to_number, 	// Map of (conventional register name, register number)
							   instructions_count,		// Map to hold count of instructions
							   branches;			// Map of (branch label, pointer to the branch in instructions)

	string input_file;									// input file
	vector<string> instructions, regs, instructions_name;			// stores the input instructions

	void initialize() {
		regs = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
							   "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
							   "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
							   "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};		// All the valid registers

		instructions_name = {"add", "sub", "mul", "beq", "bne", 
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
		return true;// Check if a string is valid integer
	}

	bool isValidRegister(string str) {
		if (str[0] != '$') return false;
		str = str.substr(1);
		// cout << str << endl;
		return reg_name_to_number.find(str) != reg_name_to_number.end() || 
			   (isNumber(str) && stoi(str) >= 0 && stoi(str) < 32);// Check if a string represents valid register
	}

	bool isValidInstruction(string str) { 	// check if a string represents valid instruction
		return instructions_count.find(str) != instructions_count.end();
	}


	pair<string,int> offset(string s) {		// parse the offset instructions
		string off = "", reg = "";
		int i=0, n=s.length();
		while (i<n && s[i] != '(') off += s[i], i++;
		i++;
		while (i<n && s[i] != ')') reg += s[i], i++;
		if (i!=n-1 || !isNumber(off)) return {"",0};
		return {reg, stoi(off)};
	}

	int lexParse() {	// Function to check the syntax of the input file and load instructions to memory
		int i = 0, n = instructions.size();

		while (i<n) {
			// cout << "ha" << endl;
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
				PC -= 4;
				// memory[PC-4] = i;
				
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

	void reduce_queue() {
		assert(QUEUE_SIZE > 0);
		QUEUE_SIZE--;
		if (cycle_start==cycle_end) cycle_start--;
		cout << "Cycle " << cycle_start+1 << "-" << cycle_end << ": Executed ";
		for (int j=0; j<3; j++) cout << instructions[index_op+j] << " ";
		cout << ":::::: ";
		if (memory_changed) {
			cout << "memory address (" << ind_changing/SZ_DRAM << "," << ind_changing%SZ_DRAM << "-" << 
				    ind_changing%SZ_DRAM + 3 << ") = " << changed_val << endl;
		}
		else {
			cout << "$" << ind_changing << " = " << changed_val << endl;
		}
		cycle = cycle_end;
	}

	bool not_safe(int ind) {
		return (QUEUE_SIZE==1 && !memory_changed && ind_changing == getRegister(ind));
	}

	bool empty_queue() {
		return QUEUE_SIZE==0;
	}

	void add(int ind) {
		while (not_safe(ind+2) || not_safe(ind+3)) reduce_queue();
		long long int sum = registers[getRegister(ind+2)] + 0ll + registers[getRegister(ind+3)];
		changed_reg = getRegister(ind+1);
		registers[getRegister(ind+1)] = sum & MAX_VALUE_IN_REGISTER;
	}

	void sub(int ind) {
		while (not_safe(ind+2) || not_safe(ind+3)) reduce_queue();
		long long int dif = registers[getRegister(ind+2)] + 0ll - registers[getRegister(ind+3)];
		changed_reg = getRegister(ind+1);
		registers[getRegister(ind+1)] = dif & MAX_VALUE_IN_REGISTER;
	}

	void mul(int ind) {
		while (not_safe(ind+2) || not_safe(ind+3)) reduce_queue();
		long long int prod = registers[getRegister(ind+2)] * 1ll * registers[getRegister(ind+3)];
		changed_reg = getRegister(ind+1);
		registers[getRegister(ind+1)] = prod & MAX_VALUE_IN_REGISTER;
	}

	int beq(int ind) {
		while (not_safe(ind+2) || not_safe(ind+3) || not_safe(ind+1)) reduce_queue();
		if (getInstructionMemory(ind+3) == -1) return -1; 
		return (registers[getRegister(ind+1)] == registers[getRegister(ind+2)]) ? memory[getInstructionMemory(ind+3)] : ind+4;
	}

	int bne(int ind) {
		while (not_safe(ind+2) || not_safe(ind+3) || not_safe(ind+1)) reduce_queue();
		if (getInstructionMemory(ind+3) == -1) return -1; 
		return (registers[getRegister(ind+1)] != registers[getRegister(ind+2)]) ? memory[getInstructionMemory(ind+3)] : ind+4;
	}

	void slt(int ind) {
		while (not_safe(ind+2) || not_safe(ind+3)) reduce_queue();
		changed_reg = getRegister(ind+1);
		registers[getRegister(ind+1)] = registers[getRegister(ind+2)] < registers[getRegister(ind+3)];
	}

	int j(int ind) {
		int mem = getInstructionMemory(ind+1);
		return (mem==-1) ? -1 : memory[mem];
	}

	int lw(int ind) {
		last_access_sw = 1;
		pair<string,int> temp = offset(instructions[ind+2]);
		int reg, mask=0, delay=0;
		if (!isNumber(temp.first.substr(1))) reg = reg_name_to_number[temp.first.substr(1)];
		else reg = stoi(temp.first.substr(1));
		int mem = registers[reg] + SZ_DRAM*temp.second;
		if (mem < MAX_INSTRUCTIONS_MEMORY || mem >= MAX_MEMORY || registers[reg] >= SZ_DRAM) return -1;
		int reg1 = getRegister(ind+1);
		registers[reg1] = 0;
		for (int i=0; i<4; i++) {
			if ((mem+i)%SZ_DRAM == 0 && i) return -1;
			// cout << memory[mem+i] << endl;
			registers[reg1] += (memory[mem+i] << mask);
			mask += 8;
		}

		if (ROW_BUFFER == -1) delay += ROW_ACCESS_DELAY, BUFFER_UPDATE += 1;
		else if (ROW_BUFFER != temp.second) delay += 2*ROW_ACCESS_DELAY, BUFFER_UPDATE += 1;
		ROW_BUFFER = temp.second;
		delay += COL_ACCESS_DELAY;
		if (true) {
			index_op = ind;
			cycle_start = cycle, cycle_end = cycle + delay ;
			memory_changed = false;
			ind_changing = reg1;	
			changed_val = registers[reg1];
			QUEUE_SIZE += 1;
		}
		
		// registers[getRegister(ind+1)] = memory[mem] + (memory[mem+1] << 8) + (memory[mem+2] << 16) + (memory[mem+3] << 24);
		return 0;
	}

	int sw(int ind) {
		last_access_sw = 1;
		pair<string,int> temp = offset(instructions[ind+2]);
		int reg, mask=0, delay = 0;
		if (!isNumber(temp.first.substr(1))) reg = reg_name_to_number[temp.first.substr(1)];
		else reg = stoi(temp.first.substr(1));
		int mem = registers[reg] + SZ_DRAM*temp.second;
		if (mem < MAX_INSTRUCTIONS_MEMORY || mem >= MAX_MEMORY || registers[reg] >= SZ_DRAM) return -1;
		int val = registers[getRegister(ind+1)];
		for (int i=0; i<4; i++) {
			if ((mem+i)%SZ_DRAM == 0 && i) return -1;
			memory[mem+i] = (val >> mask) & MAX_VALUE_IN_MEMORY;
			// cout << memory[mem+i] << endl;
			mask += 8;
		}

		if (ROW_BUFFER == -1) delay += ROW_ACCESS_DELAY, BUFFER_UPDATE++;
		else if (ROW_BUFFER != temp.second) delay += 2*ROW_ACCESS_DELAY, BUFFER_UPDATE++;
		ROW_BUFFER = temp.second;
		delay += COL_ACCESS_DELAY;
		if (true) {
			index_op = ind;
			BUFFER_UPDATE++;
			cycle_start = cycle, cycle_end = cycle + delay ;
			memory_changed = true;
			ind_changing = mem;	
			changed_val = val;
			QUEUE_SIZE += 1;
		}
		/*memory[mem] = val & MAX_VALUE_IN_MEMORY, memory[mem+1] = (val >> 8) & MAX_VALUE_IN_MEMORY, 
		memory[mem+2] = (val >> 16) & MAX_VALUE_IN_MEMORY, memory[mem+3] = (val >> 24) & MAX_VALUE_IN_MEMORY;*/
		// for (int i=0; i<4; i++) cout << memory[mem+i] << " \n"[i==3];
		return 0;
	}

	void addi(int ind) {
		while (not_safe(ind+2)) reduce_queue();
		long long int sum = registers[getRegister(ind+2)] + 0ll + stoi(instructions[ind+3]);
		changed_reg = getRegister(ind+1);
		registers[getRegister(ind+1)] = sum & MAX_VALUE_IN_REGISTER;
	}

	void printRegistersAndMemory() {
		cout << endl << "-------------------------Register Contents----------------------------" << endl;
		for (int i=0; i<32; i++) {
			if (registers[i]) cout << "R" << dec << i << ":" << registers[i] << " ";
		}
		cout << endl << "--------------------------Memory Contents---------------------------" << endl;
		for (int i=1000; i<MAX_MEMORY; i+=4) {
			int res = 0, mask = 0;
			for (int j=0; j<4; j++) res +=  (memory[j+i] << mask), mask += 8;
			if (res) cout << "M(" << i/SZ_DRAM << "," << i%SZ_DRAM << "-" << i%SZ_DRAM+3 << "):" << res << " ";
		}
		cout << endl;
	}

	void read_file() {

		cout << "Enter file name: ";
		cin >> input_file;

		cout << "ROW_ACCESS_DELAY: ";
		cin >> ROW_ACCESS_DELAY;

		cout << "COL_ACCESS_DELAY: ";
		cin >> COL_ACCESS_DELAY;

		cout << "Only Dram Mode (0): ";
		cin >> NON_BLOCKING_MODE;

		freopen(input_file.c_str(), "r", stdin);

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
	}

	int simulate() {
		initialize();
		read_file();

		// lex and parse the input instructions
		int res = lexParse();

		if (res == -1) {
			cout << "Invalid syntax" << endl;
			return -1;
		}
		else if (res == -2) {
			cout << "Memory is full! Don't write too much instructions you noob ;)" << endl;
			return -1;
		}
		cout << "-------------------------------------------------------------" << endl;
		int i=0, n=instructions.size(), temp = 0;
		while (i<n) {
			changed_reg = -1, temp  = i;
			if (instructions[i]=="add") {
				add(i);
				i += 4;
				instructions_count["add"]++, cycle++;
			}
			else if (instructions[i]=="sub") {
				sub(i);
				i += 4;
				instructions_count["sub"]++, cycle++;
			}
			else if (instructions[i]=="mul") {
				mul(i);
				i += 4;
				instructions_count["mul"]++, cycle++;
			}
			else if (instructions[i]=="beq") {
				i = beq(i);
				if (i==-1) {
					cout << "Trying to access invalid instruction in memory" << endl;
					return -1;
				}
				instructions_count["beq"]++, cycle++;
			}
			else if (instructions[i]=="bne") {
				for (int j=0; j<4; j++) cout << instructions[i+j] << " \n"[j==3];
				i = bne(i);
				if (i==-1) {
					return -1;
				}
				instructions_count["bne"]++, cycle++;
			}
			else if (instructions[i]=="slt") {
				slt(i);
				i += 4;
				instructions_count["slt"]++, cycle++;
			}
			else if (instructions[i]=="j") {
				i = j(i);
				instructions_count["j"]++, cycle++;
				if (i==-1) {
					cout << "Trying to access invalid instruction in memory" << endl;
					return -1;
				}
				
			}
			else if (instructions[i]=="lw") {
				while (!empty_queue()) reduce_queue();
				instructions_count["lw"]++, cycle++;
				int res = lw(i);
				if (res == -1) {
					cout << "Trying to load from invalid memory location" << endl;
					return -1;
				}
				cout << "Cycle " << cycle << ": DRAM request issued by executing ";
				for (int j=0; j<3; j++) cout << instructions[i+j] << " ";
				cout << endl;
				if (!NON_BLOCKING_MODE) reduce_queue();
				i += 3;
				
			}
			else if (instructions[i]=="sw") {
				while (!empty_queue()) reduce_queue();
				instructions_count["sw"]++, cycle++;
				int res = sw(i);
				if (res == -1) {
					cout << "Trying to store in invalid memory location" << endl;
					return -1;
				}
				cout << "Cycle " << cycle << ": DRAM request issued by executing ";
				for (int j=0; j<3; j++) cout << instructions[i+j] << " ";
				cout << endl;
				if (!NON_BLOCKING_MODE) reduce_queue();
				i += 3;
				
			}
			else if (instructions[i]=="addi") {
				addi(i);
				i += 4;
				instructions_count["addi"]++, cycle++;
			}
			else i += 1;
			
			if (QUEUE_SIZE==1 && cycle==cycle_end) reduce_queue();
			// printRegisters();
			if (changed_reg != -1) {

				cout << "Cycle " << cycle << ": Executed ";
				for (int j=0; j<4; j++) cout << instructions[temp+j] << " ";
				cout << "::::: $" << changed_reg << " = " << registers[changed_reg] << endl;  
			}
		}
		while (!empty_queue()) reduce_queue();
		
		assert(i==n);
		cout << "--------------------------------------------------------------" << endl;
		cout << "Number of clock cycles: " << cycle << endl;
		cout << "Buffer Updates: " << BUFFER_UPDATE << endl;
		cout << "Frquency of instructions:" << endl;
		for (auto ele : instructions_count) cout << ele.first << ":" << ele.second << "  ";

		printRegistersAndMemory();
		cout << endl << "Done" << endl;

		return 0;
	}


};



int main() 
{
	

	MIPS mips;

	return mips.simulate();

}

