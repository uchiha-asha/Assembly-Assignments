addi $a1, $zero, 13   
addi $a0, $zero, 2
j jump
addi $t3, $zero, 3
jump:
	addi $t5, $zero, 2
	beq $t5,$a0,12  //instruction 4th
