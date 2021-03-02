.globl main

# Data Segment goes here
.data 
		msg1: 	.asciiz	"Enter the expression: "
		msg2:	.asciiz "Invalid expression\n"
		msg3:	.asciiz "Output is: "

		input:	.space	69	# maximum length of input expression

# main code goes here
.text
main:
		# Get the expression from user
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg1
		syscall

		li $v0, 8			# read_string syscall code = 8
		la $a0, input		# addr, where to put the char
		li $a1, 69			# max chars for string
		syscall

		li $s0, 0			# Number of digits in expression is 0
		la $t0, input		# Load input addr to $t0 for traversing the string

build_stack:
		lb $t1, ($t0)		# Load the byte pointed by t0 to t1
		li $t9, 10			# store '\0' in t9
		beq $t1, $t9, error	# if at the end of string, then, go to end (No operators found)

		li $t9, '0'			# store '0' in t9
		bgt $t9, $t1, solve # Digit occurrences ended, so goto solve

		li $t9, '9'			# store '9' in t9
		bgt $t1, $t9, solve # Digit occurrences ended, so goto solve

		addi $t1, $t1, -48	# Convert ascii value to decimal

		# push to stack
		subu $sp, $sp, 4
		sw $t1, ($sp)		# store current digit into top of the stack

		addi $s0, $s0, 1	# Number of digits are increased by 1
		addi $t0, $t0, 1	# Move to next character in the string

		j build_stack		# Get back to work dimwit

solve:
		# Do work
		j print_output
		
error:
		# Print the error message
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg2
		syscall

		j end 				# Jump to end

print_output:
		# Print the output message
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg3
		syscall

		j end 				# Jump to end

end:
		li	$v0, 10			# exit the program
		syscall


.end main





