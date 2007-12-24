asm
        print "Hello, world."
debut:  push 3
        push 4.4
        dup -3
milieu:
        push "bla bla hop"
        nop
        jmp @milieu
        pop 3
        pop
end

