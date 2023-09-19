/* Copyright (c) 2014, Christopher Just
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above 
 *    copyright notice, this list of conditions and the following 
 *    disclaimer in the documentation and/or other materials 
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
 /* This program was written by Christopher Just for the summer 2014
  * Retrochallenge.  See 
  * http://coronax.wordpress.com/projects/retrochallenge-summer-2014/
  * for more information.
  */
  
#include <stdio.h>
#include <string.h>
//#include <dirent.h>
#include <c64.h>
#include <peekpoke.h>
#include <conio.h>
/*#include <file.h>*/

char vfile[5*1024];
int vlen;
char pfile[100];
int plen;
char tmp[100];

char* VARTAB;	// start of variables
char* ARYTAB;	// start of arrays
char* STREND;	// end of arrays (+1)

int current_char = -1;
	
char character_names[4][32] = {"","","",""};
char vfile_names[4][32] = {"","","",""};
char pfile_names[4][32] = {"","","",""};

extern char _filetype;

// struct for mapping a BASIC variable
struct basicv
{
	char* name;
	char* longname;
	int vartype; // 0 = float, 1 = int
	int val;
	char* addr;
};

struct basicv basic_vars[] = {
	{"hp","hp",0,0,NULL},
	{"gd","gold",0,0,NULL},
	{"fo","food",0,0,NULL},
	{"st","strength",1,0,NULL},
	{"zt","stamina",1,0,NULL},
	{"ag","dexterity",1,0,NULL},
	{"sm","intelligence",1,0,NULL},
	{"ka","charisma",1,0,NULL},
	};
int basic_vars_count = 8;

void InitializeBasicVars ()
{
	int i;
	for (i = 0; i < basic_vars_count; ++i)
	{
		basic_vars[i].val = 0;
		basic_vars[i].addr = NULL;
	}
}


struct basicv* GetBasicVar (const char* name)
{
	int i;
	for (i = 0; i < basic_vars_count; ++i)
		if (!strcmp (name, basic_vars[i].name))
			return &basic_vars[i];
	return NULL;
}


int mapfile (char* filename, char* buffer)
{
	FILE* f1 = fopen(filename, "r");
	int bytes_read = 0;
	if (f1)
	{
		bytes_read = fread (buffer, 1, 5*1024, f1);
		fclose(f1);
	}
	else
	{
		printf ("Failed to open %s", filename);
	}
	
	return bytes_read;
}

int writefile (char* filename, char* buffer, int len)
{
	// it seems like right now the file is being saved as a
	// USR file, and I need it to be saved as a PRG. hm.
	FILE* f1 = fopen(filename, "w");
	int bytes_written = 0;
	if (f1)
	{
		bytes_written = fwrite (buffer, 1, len, f1);
		fclose(f1);
	}
	else
	{
		printf ("failed to open %s", filename);
	}
	
	return bytes_written;
}



int C64ToInt (char* p)
{
	return (p[0]<<8) | p[1];
}

void IntToC64 (int i, char* flt)
{
	flt[0] = i>>8;
	flt[1] = i & 0xff;
}

void IntToFloat (int i, char* flt)
{
	if (i == 0)
	{
		for (i = 0; i < 5; ++i)
			flt[i] = 0;
	}
	else
	{
		int sign = (i < 0)?1:0;
		int exp = 0;
		if (i < 0)
			i = -i;
		while (!(i & 0x8000))
		{
			++exp;
			i = i << 1;
		}
		//printf ("exp is %d\n", exp);
		flt[1] = ((char*)&i)[1]; // store mantissa as big-endian
		flt[2] = ((char*)&i)[0];
		flt[3] = flt[4] = 0;
		if (!sign)
			flt[1] = flt[1] & 0x7f;
		flt[0] = 0x80 + (16-exp);
	
	}
}

int FloatToInt (char* p)
{
	// and let's remember we have to do this nondestructively.
	// We're going to do this as an int because cc65 doesn't
	// support floats.  it'll be fun.  
	// it also means we only support values that fit in 16 bit
	// ints.  Which should be good enough for modifying the
	// values in Questron.
	int sign = (p[1]&0x80)?-1:1;
	int exponent = p[0] - 0x80;
	// it's important that the high bit of p[1] is 0 when we do this shift,
	// so that we're not filling in 1s on the left when we do the right shift
	// next.
	int val = ((p[1] & 0x7f)<<8) + p[2];
	val = val >> 16-exponent;
	val = val | (1<<(exponent-1));
	
	return val * sign;
}

#if 0
void FloatTest ()
{
	char flt[5];
	int i;
	i = 15;
	//printf ("byte 0 is %x\n", ((char*)&i)[0]);
	//printf ("byte 1 is %x\n", ((char*)&i)[1]);
	IntToFloat (i, flt);
	i = FloatToInt (flt);
	printf ("value should be 15: %d\n", i);
	
	i = 247;
	IntToFloat (i, flt);
	i = FloatToInt (flt);
	printf ("value should be 247: %d\n", i);

	i = 20395;
	IntToFloat (i, flt);
	i = FloatToInt (flt);
	printf ("value should be 20395: %d\n", i);

	i = -33;
	IntToFloat (i, flt);
	i = FloatToInt (flt);
	printf ("value should be -33: %d\n", i);

	i = 32767;
	IntToFloat (i, flt);
	i = FloatToInt (flt);
	printf ("value should be 32767: %d\n", i);

//	i = 32768;
//	IntToFloat (i, flt);
//	i = FloatToInt (flt);
//	printf ("value should be 32768 (but probably won't): %d\n", i);

}
#endif

void InterpretScalar (char* p)
{
	int i;
	char name[3];
	name[0] = p[0] & 0x7f;
	name[1] = p[1] & 0x7f;
	name[2] = 0;
	
	for (i = 0; i < basic_vars_count; ++i)
	{
		if (basic_vars[i].name[0] == name[0] && basic_vars[i].name[1] == name[1])
		{
			basic_vars[i].addr = p;
			if (basic_vars[i].vartype == 0)
				basic_vars[i].val = FloatToInt (p+2);
			else if (basic_vars[i].vartype == 1)
				basic_vars[i].val = C64ToInt (p+2);
		}
	}
}


/** Get list of names of characters on this Questron disk.
 *  Returns 0 on success and an error code on failure.
 */
int GetNames ()
{
	// the disk has a file called 'names' that has the character
	// names in canonical order.
	FILE* f1 = fopen("names", "r");
	if (f1)
	{
		int i = 0;
		while ((i < 4) && fgets (character_names[i], 32, f1))
		{
			char* s = strstr (character_names[i], "\n");
			if (s)
				*s = 0;
			if (strcmp (character_names[i], "+"))
			{
				sprintf (vfile_names[i], "%s-v", character_names[i]);
				sprintf (pfile_names[i], "%s-p", character_names[i]);
				//printf ("vfile: %s\n", vfile_names[i]);
				//printf ("pfile: %s\n", pfile_names[i]);
			}
			else
			{
				strcpy (character_names[i], "[empty]");
			}
			printf ("read name %s (%s)\n", character_names[i], vfile_names[i]);
			++i;
		}
		fclose (f1);
		//cgetc();
		return 0;
	}
	return 1;
}

/*
void GetDirectory ()
{
	unsigned char status;
	struct cbm_dirent dirent;
	//char* s;
	status = cbm_opendir (8, 8);
	while ((status = cbm_readdir (8, &dirent)) == 0)
	{
		printf ("read dir entry %s\n", dirent.name);
		//if (s = strstr (dirent.name, "-v"
	}
	cbm_closedir (8);
}
*/
char* banner = "\n        \xa5\xb4\xb5 questron ed 1.0 \xb6\xaa\xa7\n\n";


int InsertDiskScreen ()
{
	char ch;
	int result;
	clrscr();
	
	printf (banner);
	do
	{
		printf ("   insert questron side 1 in drive 8 \n"
		"  and press any key (press q to quit).\n\n");
		ch = cgetc();
		if (ch == 'q')
			return -1;
	
		result = GetNames();
		if (result == 0)
			return 1;
		else
			printf ("   error reading character names\n\n");
	} while (1);
}



int CharacterSelectScreen ()
{
	int i;
	int select;
	
	clrscr();
	puts (banner);
	
	printf ("   select the character you wish to\n   edit:\n\n");
	for (i = 0; i < 4; ++i)
	{
		printf ("             %d. %s\n\n", i+1, character_names[i]);
	}
	printf ("             5. go back");
	
	do
	{
		select = cgetc();
		if (select == '5')
			return -1;
		if (select >= '1' && select < '5')
			select = select - '1';
		if (strcmp (character_names[select], "[empty]"))
			return select;
	} while (1);
}



int LoadSaveFiles (int current)
{
	unsigned int vartab_raw, arytab_raw;
	char* p;

	clrscr();
	
	InitializeBasicVars();
	
	printf ("loading character %d: %s\n", current, character_names[current]);
	printf ("reading pointers data %s\n", pfile_names[current]);
	plen = mapfile (pfile_names[current], pfile);
	if (plen == 0)
	{
		printf ("   Error reading %s - press any key to continue.", pfile_names[current]);
		cgetc();
		return -1;
	}
	printf ("pfile size is %d\n", plen);
	
	vartab_raw = pfile[2] + (pfile[3]<<8);
	arytab_raw = pfile[4] + (pfile[5]<<8);
	printf ("vartab_raw is %x\narytab_raw is %x\n", vartab_raw, arytab_raw);
	
	/* now we've got to figure out where the sections of the values file
	 * are.  The tricky thing is keeping track of the offsets.  From
	 * pfile, we get the addresses in the C64's memory.  Subtract the
	 * vfile base address, but add 2 (for the 2-byte header containing
	 * the base address).
	 */

	VARTAB = vfile + 2;
	ARYTAB = vfile + 2 + ((pfile[4] + (pfile[5]<<8)) - (pfile[2] + (pfile[3]<<8)));
	STREND = vfile + 2 + ((pfile[6] + (pfile[7]<<8)) - (pfile[2] + (pfile[3]<<8)));
	
	
	printf ("\nreading save file %s\n", vfile_names[current]);	
	vlen = mapfile (vfile_names[current], vfile);
	if (vlen == 0)
	{
		printf ("   error reading %s - press any key to continue.", vfile_names[current]);
		cgetc();
		return -1;
	}

	printf ("vfile size is %d\n", vlen);
	
	printf ("file loaded at %u\n", (int)vfile);
	printf ("vartab = %u\n", (int)VARTAB);
	printf ("arytab = %u\n", (int)ARYTAB);
	printf ("strend = %u\n", (int)STREND);
	
	for (p = VARTAB; p < ARYTAB; p += 7)
	{
		InterpretScalar (p);
	}
	
	printf ("\nhp   = %d\n",basic_vars[0].val);
	printf ("gold = %d\n",basic_vars[1].val);
	printf ("food = %d\n",basic_vars[2].val);

	return 1;
}

/** Utility for EditScreen. */
void PrintValues ()
{
	//struct basicv* var;
	int i,x,y;
	
	x = 10; y = 8;
	for (i = 0; i < 3; ++i)
	{
		gotoxy (x, y++);
		cprintf ("%5d", basic_vars[i].val);
	}

	x = 32; y = 8;
	for (i = 3; i < 8; ++i)
	{
		gotoxy (x, y++);
		cprintf ("%5d", basic_vars[i].val);
	}
	/*
	var = GetBasicVar ("hp");
	if (var)
	{
		gotoxy (10,8);
		cprintf ("%d", var->val);
	}
	*/
}

/** var_index is an index into basic_vars */
void EditValue (int var_index)
{
	int val, i;
	struct basicv* var = &basic_vars[var_index];
	gotoxy (3,17);
	cprintf ("enter new value for %s:", var->longname);
	cursor(1);
	for (;;)
	{
		gotoxy (16,19);
		cscanf ("%d",&val);
		if (val == 0)
			break;
		else if (val < 0)
		{
			gotoxy (3,16);
			cprintf ("enter a number between 1 and 9999");
		}
		else
		{
			var->val = val;
			if (var->vartype == 0)
				IntToFloat (val,var->addr+2);
			else if (var->vartype == 1)
				IntToC64 (val,var->addr+2);
			break;
		}
	}
	// clean up the section of screen we used.
	cursor(0);
	for (i = 16; i < 24; ++i)
		cclearxy (0,i,40);
}

void SaveChanges (int current)
{
	int bytes_written, i;
	
	gotoxy (1,17);
	cprintf ("saving character values for %s", character_names[current]);
	bytes_written = writefile (vfile_names[current], vfile, vlen);
	gotoxy (3,19);
	if (bytes_written == vlen)
	{
		gotoxy (5,19);
		cprintf ("save successful. press any key");
		gotoxy (14,20);
		cprintf ("to continue.");
	}
	else
	{
		gotoxy (7,19);
		cprintf ("error writing file. press any key");
		gotoxy (14,20);
		cprintf ("to continue.");
	}
	cgetc();
	for (i = 16; i < 24; ++i)
		cclearxy (0,i,40);
	
}


int EditScreen (int current)
{
	int i,x,y;
	char ch;
	clrscr();
	puts (banner);
	
	gotoxy (3,3);
	cprintf ("   editing character %d.%s\n\n", current, character_names[current]);
	gotoxy (3,4);
	cprintf ("  enter the number of an option");
	gotoxy (3,5);
	cprintf ("          to change it.\n");
	
	x = 3; y = 8;
	for (i = 0; i < 3; ++i)
	{
		gotoxy (x, y++);
		cprintf ("%d.%s",i+1,basic_vars[i].longname);
	}

	x = 17; y = 8;
	for (i = 3; i < 8; ++i)
	{
		gotoxy (x, y++);
		cprintf ("%d.%s",i+1,basic_vars[i].longname);
	}

	gotoxy (3, 14);
	cprintf ("0.go back");
	gotoxy (17, 14);
	cprintf ("9.save changes");
	
	PrintValues();
	
	for (;;)
	{
		ch = cgetc();
		i = ch - '0';
		if (i == 0)
		{
			return -1;
		}
		else if (i == 9)
		{
			SaveChanges(current);
		}
		else if (i >= 1 && i < 9)
		{
			EditValue (i-1);
			PrintValues();
		}
	}
	
	return -1;
}



int main (int argc, char** argv)
{
	//char ch;
	//DIR *d;
	//char* p;
	int state = 0;
	int result = 0;
	int finished = 0;

	// set file creation type to PRG. There has to be a better way to do this.
	_filetype = 'p';
	
	VIC.addr = 21;					// Set to upper-case/graphics mode.
	VIC.bgcolor0 = COLOR_BLACK;
	VIC.bordercolor = COLOR_GRAY1;
	POKE(646, COLOR_GRAY3);
	
	while (!finished)
	{
		switch (state)
		{
			case 0:
				result = InsertDiskScreen();
				if (result == -1)
					finished = 1;
				else
					state = 1;
				break;
			case 1:
				result = CharacterSelectScreen();
				if (result == -1)
					state = 0;
				else
				{
					current_char = result;
					state = 2;
				}
				break;
			case 2:
				result = LoadSaveFiles (current_char);
				if (result == -1)
					state = 1;
				else
					state = 3;
				break;
			case 3:
				result = EditScreen (current_char);
				if (result == -1)
					state = 1;
				break;
		}
	}
				
	return 0;
}
