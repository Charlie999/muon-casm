#!/usr/bin/env python3

#BFCODE = ">++++++++[<+++++++++>-]<.>++++[<+++++++>-]<+.+++++++..+++.>>++++++[<+++++++>-]<++.------------.>++++++[<+++++++++>-]<+.<.+++.------.--------.>>>++++[<++++++++>-]<+."

BFCODE = """
--[----->+<]>---.++++++++++++.+.+++++++++.+[-->+<]>+++.++[-->+++<]>.++++++++++++.+.+++++++++.-[-->+++++<]>++.[--->++<]>-.-----------.
"""

BFCODE = BFCODE.replace("\n","")
print(";; BFCODE=",BFCODE)

out = []

out.append(";; START")
out.append("dw 0")
out.append("jmp {_start}")
out.append(":_bfmemory")
out.append("dw 0x10000")
out.append(":_start")
out.append("ldai {_bfmemory}")
out.append("ota 0xFFFC")

loopidx = 0
curloops = []
for i in range(BFCODE.count("[")):
    curloops.append(999999)

idx = 0
for c in BFCODE:
    out.append(";; BFI="+c)
    if c == '>':
        out.append("ldai 0xFFFC")
        out.append("ldb 1")
        out.append("add 0xFFFC")
    elif c == '<':
        out.append("ldai 0xFFFC")
        out.append("dec 0xFFFF")
        out.append("ota 0xFFFC")
    elif c == '+':
        out.append("ldaptr 0xFFFC")
        out.append("ldb 1")
        out.append("add 0xFFFF")
        out.append("ldai 0xFFFF")
        out.append("otaptr 0xFFFC")
    elif c == '-':
        out.append("ldaptr 0xFFFC")
        out.append("dec 0xFFFF")
        out.append("otaptr 0xFFFC")
    elif c == '.':
        out.append("ldaptr 0xFFFC")
        out.append("ota 0xF00000")
    elif c == '[':
        loopidx += 1
        curloops[loopidx] = idx
        out.append(":loop"+str(loopidx)+"_"+str(curloops[loopidx]))
        out.append("ldaptr 0xFFFC")
        out.append("ldb 0")
        out.append("add 0xFFFF")
        out.append("brch 2 {loopendpc"+str(loopidx)+"_"+str(curloops[loopidx])+"}")
        out.append(":looppc"+str(loopidx)+"_"+str(curloops[loopidx]))
    elif c == ']':
        out.append(":loopend"+str(loopidx)+"_"+str(curloops[loopidx]))
        out.append("ldaptr 0xFFFC")
        out.append("ldb 0 ")
        out.append("add 0xFFFF")
        out.append("brch 2 {loopendpc"+str(loopidx)+"_"+str(curloops[loopidx])+"}")
        out.append("jmp {looppc"+str(loopidx)+"_"+str(curloops[loopidx])+"}")
        out.append(":loopendpc"+str(loopidx)+"_"+str(curloops[loopidx]))
        loopidx -= 1
    else:
        print("ERROR unknown insn",c)
        exit(1)
    idx+=1

if loopidx > 0:
    print("ERROR unterminated loop!")
    exit(1)

out.append("dw 0xFF0000")
out.append("dw 0xFF0000")
out.append("dw 0xFF0000")
out.append("dw 0xFF0000")
out.append("dw 0xFF0000")
out.append("dw 0xFF0000")

for l in out:
    print(l)