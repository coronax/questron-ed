# About QuestronEd

QuestronEd is a character editor for the Commodore 64 game Questron, a CRPG released by SSI in 1984. The character editor allows the user to change the character stats, HP, Gold, and Food amounts for any of the four character slots used by the game. It doesn't support inventory editing, but I'd like to try that in the future. Its current features are enough to give the player an easier beginning in the game, and avoid most of the grind along the way.

QuestronEd began as a project for the 2014 Summer [Retrochallenge]. I spent most of the month-long event investigating the inner workings of the C64 version of Questron, exploring its copy protection mechanisms, and eventually deciphering the way the game's save files overwrite the internal state of the C64's BASIC interpreter. Actually writing the editor was crammed into the last few days of July. You can read all about my findings at the [Project Blog].

## Building QuestronEd

QuestronEd is a C64 native program. It is built using the [CC65] cross-compiler. To build, either use the included Makefile, or run this command line (with the CC65 bin directory in your path):

```sh
cl65 QuestronEd.c -o QuestronEd.prg 
```

[//]: # 

   [Project Blog]: <https://coronax.wordpress.com/projects/retrochallenge-summer-2014/>
   [Retrochallenge]: <https://www.retrochallenge.org/>
   [CC65]: <https://github.com/cc65/cc65>
   
