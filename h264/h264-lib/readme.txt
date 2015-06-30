1, use vc71 compiling build\vc71\t264.sln _or_
2, compiling win32\dshow\dshow.sln
3, if u use 1, then copy src\enconfig.txt to build\vc71\bin\, 
   perform the cmd.exe, type t264 enconfig.txt
4, if u use 2, run graphedit.exe, setup a graph, add 'T264 Encoder' 
   into graph, just like this
   ...(some decoder) --> T264 Encoder --> AVI Mux --> File writer
   the avi file can play with vssh(http://www.videosoftinc.com/pub/vssh3dec.exe)

contact us at: www.sourceforege.net/projects/t264