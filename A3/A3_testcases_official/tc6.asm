addi $r2, $zero, 2
addi $r1, $zero, 256
mul $r2, $r2, $r2     //3
bne $r2, $r1, 8   //third instruction from the top
addi $r4, $r2, 3 
