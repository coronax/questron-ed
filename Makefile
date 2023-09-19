
QuestronEd.prg: QuestronEd.c
	cl65 QuestronEd.c -o QuestronEd.prg 
	
clean:
	del questroned.o questroned.lst questroned.prg

