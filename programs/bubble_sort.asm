        .text
main:   ADDI   R2, R0, 6
outer:  ADDI R2, R2, -1
        BEQ  R2, R0, done
        ADDI   R3, R0, arr
        ADDI   R4, R0, 0
inner:  LW   R5, 0(R3)
        LW   R6, 4(R3)
        BGE  R6, R5, noswap
        SW   R6, 0(R3)
        SW   R5, 4(R3)
noswap: ADDI R3, R3, 4
        ADDI R4, R4, 1
        BLT  R4, R2, inner
        JMP  outer
done:   HALT

        .data
arr:    .word 42, -7, 19, 3, 88, 0
