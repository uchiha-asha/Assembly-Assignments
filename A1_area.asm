	.globl main
	.text

main:
		# Print msg1
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg1
		syscall

		# Get N 
		li $v0, 5			# read_int syscall code = 5
		syscall
		move $s0,$v0		# move value of N from v0 to s0

		li.s $f1,0.0		# store initial area in f1
		#li $t9,0			# Reference for 0

		ble $s0,0,exit		# if n<=0 then exit

		# print msg2
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg2
		syscall

		# Get x co-ordinate of first point
		li $v0, 5			# read_int syscall code = 5
		syscall
		move $t1,$v0		# move value of N from v0 to t1

		# Get y co-ordinate of first point
		li $v0, 5			# read_int syscall code = 5
		syscall
		move $t2,$v0		# move value of N from v0 to t2

#		li $t3,0			# x co-ordinate of 2nd point
#		li $t4,0			# y co-ordinate of 2nd point

		li $t3,1			# variable to iterate over points (int i = 1)
#		li $t4,0			# variable to store numerator of area (int numerator = 0)
#		li $t5,0			# variable to store denominator of area (int denominator = 0)

loop:
		beq $t3, $s0, exit  # if i==n then exit

		addi $t3, $t3, 1	# i = i+1
		# print msg2
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg2
		syscall

		# Get x co-ordinate of second point
		li $v0, 5			# read_int syscall code = 5
		syscall
		move $t6,$v0		# move value of N from v0 to t6

		# Get y co-ordinate of second point
		li $v0, 5			# read_int syscall code = 5
		syscall
		move $t7,$v0		# move value of N from v0 to t7

		mul $t8, $t7, $t2	# multiply y1 and y2 and assign to t8
		#mfhi $t8 			# move y1*y2 last 32 bits from hi to t8

		move $t9, $t7       # will need y2 later so save it
		abs $t2, $t2        # y1 = abs(y1)
		abs $t7, $t7        # y2 = abs(y2)

		blt $t8,$0,pos_neg  # if y1*y2 < 0, then, find area using formula for alternating points

		# Now y1*y2 >= 0 and hence y1 and y2 are on same side of x, thus, trapezium is formed
		add $t4, $t2, $t7  # numerator = abs(y1) + abs(y2)
		sub $t8, $t6, $t1  # height = x2 - x1
		mul $t4,$t4,$t8		# numerator = numerator*height
		li $t5,2			# denominator = 2

		j find_area			# find area using current numerator and denominator

pos_neg:
		# Now y1*y2 < 0 and hence y1 and y2 are on opposite side of x, thus, 2 triangles are formed
		add $t5, $t2, $t7  # denominator = abs(y1) + abs(y2)
		mul $t5,$t5,2		# denominator *= 2
		sub $t8, $t6, $t1  # b = x2 - x1
		mul $t2,$t2,$t2     # y1 = y1*y1

		mul $t7,$t7,$t7		# y2 = y2*y2
		add $t4, $t2, $t7  # numerator = y1 + y2

		mul $t4,$t4,$t8     # numerator = b*numerator

		j find_area			# find area using current numerator and denominator

find_area:

		mtc1 $t4, $f2		# move integer numerator to float register
		mtc1 $t5, $f3     # load integer denominator to float register

		cvt.s.w $f2, $f2    # convert to float
		cvt.s.w $f3, $f3    # convert to float

		div.s $f4,$f2,$f3   # cur_area = numerator/denominator
		add.s $f1,$f1,$f4   # total_area +=  cur_area

		move $t1, $t6       # x1 = x2
		move $t2, $t9       # y1 = y2

		j loop              # go back to loop and finish your work you good for nothing moron
		
exit:
		# Print msg3
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg3
		syscall

		li $v0, 2
	    mov.s $f12, $f1   # Move contents of register $f3 to register $f12
		syscall

		# Print newline
		li $v0, 4			# print_string syscall code = 4
		la $a0, newline
		syscall

		li	$v0,10		# exit
		syscall


	# Data segment
	.data
msg1:	.asciiz "Enter number of points (N):	"
msg2:	.asciiz "Enter point:\n"
msg3:	.asciiz "Area is: 	"
newline:	.asciiz "\n"
