import sys
import os
fd=open('libx264.def','r')
fd2=open('libx264.def2','w')
fd2.write('LIBRARY "libx264"\n')
fd2.write('DESCRIPTION "x264DeCoder library"\n')
fd2.write("EXPORTS\n")
fd2.flush()
for i in range(19):
    buf=fd.readline()
buf=fd.read()
buf=buf.split('\n')
for line in buf:
    line=line.split()
    fd2.write(line[3]+' '+'@'+line[0]+'\n')
    fd2.flush()




