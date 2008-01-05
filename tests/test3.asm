script
global a,b,c,tab,ret,d,e,f,g,h,z

tab="\t"
ret="\n"

a=1	b=1	c=a+b d=a*b e=a-b f=a/b print c,tab,d,tab,e,tab,f,ret
a=1	b=1.0	c=a+b d=a*b e=a-b f=a/b print c,tab,d,tab,e,tab,f,ret
a=1.0	b=1	c=a+b d=a*b e=a-b f=a/b print c,tab,d,tab,e,tab,f,ret
a=1.0	b=1.0	c=a+b d=a*b e=a-b f=a/b print c,tab,d,tab,e,tab,f,ret

d=15 e=4.0 f=4
asm nop end
print 2,tab,d,tab,e,tab,f,ret
asm nop nop end
g=b/e
asm nop nop nop end
print 3,tab,g,ret
asm nop nop nop nop end
h=f-g
asm nop nop nop nop nop end



end


