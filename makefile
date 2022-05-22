
all: tst00.exe
run: tst00.exe
	tst00.exe

tst00.exe: makefile tst00.cc
	cl tst00.cc /EHsc /MT /Ob2 /O1 /W4 /std:c++latest
