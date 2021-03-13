.globl main



.text
	
main:
	addi $t1, $t1 , 10000 
add $t2 , $t1, $t3
add $t3, $t1, $t2
sub $t4,$t3  ,$t1
sw $t3, 0($t1)                                                       
add $t4, $t2, $t3      
sub $t5, $t2, $t3
lw $t5, 0($t1)

