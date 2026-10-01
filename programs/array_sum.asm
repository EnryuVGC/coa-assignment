        .text
main:   ADDI   R1, R0, array
        ADDI   R2, R0, 8
        ADDI   R3, R0, 0
loop:   LW   R4, 0(R1)
        ADD  R3, R3, R4
        ADDI R1, R1, 4
        ADDI R2, R2, -1
        BNE  R2, R0, loop
        SW   R3, sum
        HALT

        .data
array:  .word 5, 12, -3, 40, 7, 9, 21, 1
sum:    .word 0
