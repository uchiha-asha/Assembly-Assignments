.globl main

# Data segment goes here
.data
		msg1:	.asciiz "Enter the number of points (n):	"
		msg2_:	.asciiz "Enter the x- co-ordinate of the first point:  "
		msg3_:	.asciiz "Enter the y- co-ordinate of the first point:  " 
		msg2:	.asciiz "Enter the x- co-ordinate of the next point:  "
		msg3:	.asciiz "Enter the y- co-ordinate of the next point:  "
		msg4:	.asciiz "The area of the curve formed by the given points is "

# main code goes here
.text
main:
		# extracting the value of n from the user
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg1
		syscall

		li $v0, 5			# read_int syscall code = 5
		syscall

		move $s0, $v0		# move value of n from v0 to s0
		
		# initialise area
		li.s $f1, 0.0		# store initial area in f1
		ble $s0, 0, printArea		# if n<=0 then printArea

		# reading the co-ordinate of the first point
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg2_		# asking for x-coordinate
		syscall
		
		li $v0, 5			# read_int syscall code = 5
		syscall
		move $t1, $v0		# move value of x-coordinate from v0 to t1
		
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg3_		# asking for y-coordinate
		syscall

		li $v0, 5			# read_int syscall code = 5
		syscall
		
		move $t2, $v0		# move value of y-coordinate from v0 to t2
		
		# registers t1 and t2 shall store the previous co-ordinates
		
		li $t3, 1			# variable to iterate over points i set to 1

loop:
		beq $t3, $s0, printArea		# if i == n, all points have been read, so move to printArea
		
		addi $t3, $t3, 1	# i = i+1
		
		# taken the co-ordinates of the next point as input
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg2		# asking for x-coordinate
		syscall

		li $v0, 5			# read_int syscall code = 5
		syscall
		move $t6,$v0		# move value of x-coordinate from v0 to t6
		
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg3		# asking for y-coordinate
		syscall

		li $v0, 5			# read_int syscall code = 5
		syscall
		move $t7,$v0		# move value of y-coordinate from v0 to t7
		
		# registers t6 and t7 shall store the current co-ordinates
		
		#area calculation
		mul $t8, $t7, $t2	# multiply y1 and y2 and assign to t8
		move $t9, $t7       # will need y2 later so save it
		abs $t2, $t2        # y1 = abs(y1)
		abs $t7, $t7        # y2 = abs(y2)
		
		# if y1*y2 < 0, then, find area using formula for alternating points
		blt $t8, $0, pos_neg
		
		# Otherwise y1*y2 >= 0 and hence y1 and y2 are on same side of x-axis
		# Hence area can be found easily as a trapezium is formed
		add $t4, $t2, $t7   # numerator = abs(y1) + abs(y2)
		sub $t8, $t6, $t1   # width = x2 - x1
		abs $t8, $t8		# store absolute value of width in register t8
		mul $t4,$t4,$t8		# numerator = numerator * height
		li $t5, 2			# denominator = 2

		j findArea			# find area using the current numerator or denominator

pos_neg:
		# Now y1*y2 < 0 and hence y1 and y2 are on opposite side of x, thus, 2 triangles are formed
		add $t5, $t2, $t7   # denominator = abs(y1) + abs(y2)
		mul $t5, $t5, 2		# denominator *= 2
		sub $t8, $t6, $t1   # width = x2 - x1
		abs $t8, $t8		# absolute value of width is used
		mul $t2, $t2, $t2   # y1 = y1*y1
		mul $t7, $t7, $t7	# y2 = y2*y2
		add $t4, $t2, $t7   # numerator = y1 + y2
		mul $t4, $t4, $t8     # numerator = width*numerator

		j findArea			# find area using current numerator and denominator

findArea:
		mtc1 $t4, $f2		# move integer numerator to float register
		mtc1 $t5, $f3     	# load integer denominator to float register
		cvt.s.w $f2, $f2    # convert the numerator to float
		cvt.s.w $f3, $f3    # convert the denominator to float
		div.s $f4, $f2, $f3 # cur_area = numerator/denominator
		add.s $f1, $f1, $f4 # total_area +=  cur_area
		
		# store the current coordinates as previous co-ordinates
		move $t1, $t6       # x1 = x2
		move $t2, $t9       # y1 = y2
		j loop              # go back to loop and finish your work you good for nothing moron
		
printArea:
		# printing the area on the console
		li $v0, 4			# print_string syscall code = 4
		la $a0, msg4
		syscall

		li $v0, 2
	    mov.s $f12, $f1		# Move contents of register $f3 to register $f12 to print the area
		syscall

		li	$v0, 10			# exit the program
		syscall
