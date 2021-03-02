.globl main

# Data Segment goes here
.data 
		msg1:	.asciiz	"Enter a valid postfix expression:\n"
		msg2:	.asciiz "The input expression is invalid.\n"
		msg3:	.asciiz "The output of the expression is "

		input:	.space	100		# reserved space for input string
		output: .word	0		# memory location for final output

# main code goes here
.text
main:
		# Get the postfix expression from user
		li $v0, 4				# print_string syscall code = 4
		la $a0, msg1
		syscall

		li $v0, 8				# read_string syscall code = 8
		la $a0, input			# address to put the string
		li $a1, 100				# max chars for string
		syscall

		li $s0, 0				# number of digits in expression is 0 (initialising)
		la $t0, input			# load input address to $t0 for traversing the string
		
		j build_stack			# move to label build_stack to start reading the string

build_stack:
		# reading the operands (digits) one by one
		lb $t1, 0($t0)			# load the byte pointed by t0 to t1
		andi $t1, $t1, 0xff		# isolate the first character using the 0x000000FF mask
		beqz $t1, error			# if the end of string is encountered, raise error as no operators (or objects) were found
		
		li $t9, '0'				# store '0' in t9
		bgt $t9, $t1, evaluate 	# digit occurrences ended, going to evaluate the expression
		
		li $t9, '9'				# store '9' in t9
		bgt $t1, $t9, evaluate 	# digit occurrences ended, going to evaluate the expression
		
		addi $t1, $t1, -48		# convert ascii value to decimal
		
		# push to stack
		subu $sp, $sp, 4
		sw $t1, ($sp)			# store current digit into top of the stack
		
		addi $s0, $s0, 1		# number of digits are increased by 1
		addi $t0, $t0, 1		# move to next character in the string
		
		j build_stack			# get back to work dimwit (read next character)

evaluate:
		# reading the operators and evaluating the output
		lb $t1, 0($t0)			# load the byte pointed by t0 to t1
		andi $t1, $t1, 0xff		# isolate the first character using the 0x000000FF mask
		beqz $t1, Output		# if the end of string is encountered, move to Output
		
		# store '+', '*' and '-' and compare the contents of the current character with these operators
		li $t9, '+'
		beq $t1, $t9, addNum	# if the current character is +, move to addNum
		li $t9, '*'
		beq $t1, $t9, mulNum	# if the current character is *, move to mulNum
		li $t9, '-'
		beq $t1, $t9, subNum	# if the current character is +, move to subNum
		
		j error					# if the digit is none of +, - or *, raise invalid expression error

addNum:
		# addition operator action
		li $t6, 2
		blt $s0, $t6, error		# if the depth of stack is less than 2, raise insufficient operands error
		
		lw $t2, ($sp)			# store first operand in register $t2
		addu $sp, $sp, 4		# pop from the stack
		addi $s0, $s0, -1		# reduce the depth of stack by 1
		
		lw $t3, ($sp)			# store second operand in register $t3
		addu $sp, $sp, 4		# pop from the stack
		addi $s0, $s0, -1		# reduce the depth of stack by 1
		
		add $t2, $t2, $t3		# store the sum of operands in $t2
		subu $sp, $sp, 4		# make space for output in the stack
		sw $t2, ($sp)			# push the output onto the stack
		addi $s0, $s0, 1		# increase the depth of stack by 1
		
		addi $t0, $t0, 1		# move to next character in the string
		j evaluate				# read the next character

mulNum:
		# multiplication operator action
		li $t6, 2
		blt $s0, $t6, error		# if the depth of stack is less than 2, raise insufficient operands error
		
		lw $t2, ($sp)			# store first operand in register $t2
		addu $sp, $sp, 4		# pop from the stack
		addi $s0, $s0, -1		# reduce the depth of stack by 1
		
		lw $t3, ($sp)			# store second operand in register $t3
		addu $sp, $sp, 4		# pop from the stack
		addi $s0, $s0, -1		# reduce the depth of stack by 1
		
		mul $t2, $t2, $t3		# store the product of operands in $t2
		subu $sp, $sp, 4		# make space for output in the stack
		sw $t2, ($sp)			# push the output onto the stack
		addi $s0, $s0, 1		# increase the depth of stack by 1
		
		addi $t0, $t0, 1		# move to next character in the string
		j evaluate				# read the next character

subNum:
		# subtraction operator action
		li $t6, 2
		blt $s0, $t6, error		# if the depth of stack is less than 2, raise insufficient operands error
		
		lw $t2, ($sp)			# store first operand in register $t2
		addu $sp, $sp, 4		# pop from the stack
		addi $s0, $s0, -1		# reduce the depth of stack by 1
		
		lw $t3, ($sp)			# store second operand in register $t3
		addu $sp, $sp, 4		# pop from the stack
		addi $s0, $s0, -1		# reduce the depth of stack by 1
		
		sub $t2, $t3, $t2		# store the difference of operands in $t2
		subu $sp, $sp, 4		# make space for output in the stack
		sw $t2, ($sp)			# push the output onto the stack
		addi $s0, $s0, 1		# increase the depth of stack by 1
		
		addi $t0, $t0, 1		# move to next character in the string
		j evaluate				# read the next character

Output:
		# validating and storing the output
		li $t6, 1
		bne $s0, $t6, error		# if the depth of stack is not equal to 1, raise insufficient operators error
		
		lw $t2, ($sp)			# store the result in register $t2
		addu $sp, $sp, 4		# pop from the stack
		sw $t2, output		# store the output in suitable memory location
		
		j print					# move to print to print the output

error:
		# printing the error message
		li $v0, 4				# print_string syscall code = 4
		la $a0, msg2			# print the error message
		syscall
		j end 					# jump to end

print:
		# printing the output
		li $v0, 4				# print_string syscall code = 4
		la $a0, msg3
		syscall
		
		li $v0, 1				# print_integer syscall code = 1
		la $a0, output			# print the contents of 'output' variable
		syscall
		
		j end 					# jump to end the program

end:
		li	$v0, 10				# exit the program
		syscall

.end main





