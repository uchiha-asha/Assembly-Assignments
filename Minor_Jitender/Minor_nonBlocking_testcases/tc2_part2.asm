addi $t0, $zero,1
addi $t1, $zero,2
sw $t1, 2044($zero)
add $t0, $t0, $t1
lw $t0, 2044($zero)
add $t0, $t0, $t1