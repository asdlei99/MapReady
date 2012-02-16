/*makemake.c:

a program to automagically generate the autotar entries and the top-level Makefile.

For more documentation, check out README, in this directory, or
master_program_list, which this program parses to determine
what libraries, programs, and tools to make makefiles and autotar entries for.

06/12/97 O. Lawlor 	Initial Development
06/30/97 D. Corbett updated printMake to create entries for categories

*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <unistd.h>


/******************************** File-Parsing Machinery ***************************/
#define debugfParse(str) /* str */

char codeLoc[255];/* The relative path to the code (i.e. "src_lib") */
FILE *master;
int masterLine=0;
/*Return the next token from master_program_list, ignoring comments and white space.*/
char *getMLine(void)
{
	static char inLine[1024];
	int lineIsComment=1;
	while (lineIsComment)
	{
		debugfParse("Reading a line...\n");
		if (NULL==fgets(inLine,1024,master))
			return NULL;
		masterLine++;
		if (inLine[0]=='#')
			debugfParse("Argh! It's a comment.\n");
		else if (inLine[0]=='@')
		{
			sscanf(inLine,"@%s\n",codeLoc);
			printf("Code in '%s'\n",codeLoc);
		} else {
			lineIsComment=0;
			inLine[strlen(inLine)-1]=0;
		}
	}
	return inLine;
}
char * inM(void)
{
	static char tok[200];
	static char *inLine;
	static int lindex=-1;
	static int endex=0;
	int foundToken=0;
	tok[0]=0;
	while (!foundToken)
	{ /* Read from master_program_list until we find a token.*/
		if (lindex==-1)
		{
			inLine=getMLine();
			if (inLine==NULL)
				return NULL;
			lindex=0;
		}
		debugfParse("Finding a token...\n");
		while ((lindex<1024)&&(inLine[lindex])
		   &&((inLine[lindex]=='\t')||(inLine[lindex]==' ')))
			lindex++; /*skip over whitespace.*/
		if (!((lindex<1024)&&(inLine[lindex])))
		{
			debugfParse("Blast! End of line...\n");
			lindex=-1;
		}
		else
		{
			debugfParse("Found a token!\n");
			foundToken=1;
			endex=lindex;
			while ((endex<1024)&&((endex-lindex)<200)&&(inLine[endex])
			   &&(inLine[endex]!='\t')&&(inLine[endex]!=' '))
			{
				tok[endex-lindex]=inLine[endex];
				endex++; /*find end of token.*/
			}
			tok[endex-lindex]=0;
			lindex=endex;
		}
	}
	return tok;
}

int progExists(char *path,char *name)
{
	char relPath[255];
	sprintf(relPath,"../%s%s",path,name);
	return 0==access(relPath,F_OK);
}
/********************** ParseMasterList, which parses master_program_list ******************/

/*Libraries*/
typedef struct {
	char path[255];
	char name[255];
} lib;
int numLibs;
lib *libs[1000];

/*Programs*/
#define PROG int
typedef struct prog{
	char path[255];
	char name[255];
	int numLibs;
	lib *libs[100];
	int numProgs;
	PROG *progs[100];
	char isOnlyBinary,isCat,isDoc;
} prog;
int numProgs;
prog *progs[10000];

void wrong(char *what1,char *what2)
{
	printf("Master_parser error: %s%s, line %i of master_program_list.\n",what1,what2,masterLine);
	exit(1);
}
/*ParseMasterList: reads the master file into the above structures.*/
#define tokNot(str) (0!=strcmp(tok,str))
#define tokIs(str) (0==strcmp(tok,str))
#define nextTok() tok=inM();
#define nextTokN() {tok=inM();if (!tok) wrong("Unexpected EOF.","");}
void parseMasterList(void)
{
	char *tok=inM();
	numLibs=numProgs=0;

/*Parse LIBS section*/
	if (tokNot("LIBS")) wrong("Expected LIBS, got ",tok);
	nextTok();
	if (tokNot("{")) wrong("Expected {, got",tok);
	nextTokN();
	while (tokNot("}"))
	{
		lib *l=(lib *)malloc(sizeof(lib));
		strcpy(l->path,codeLoc);
		strcpy(l->name,tok);
		libs[numLibs++]=l;
		nextTokN();
	}
	nextTok();
/*Parse PROGRAMS section*/
	if (tokNot("PROGRAMS")) wrong("Expected PROGRAMS, got ",tok);
	nextTok();
	if (tokNot("{")) wrong("Expected {, got ",tok);
	nextTokN();
	while (tokNot("}"))
	{
		prog *p=(prog *)malloc(sizeof(prog));
		strcpy(p->path,codeLoc);
		strcpy(p->name,tok);
		p->numLibs=p->numProgs=p->isOnlyBinary=p->isCat=p->isDoc=0;
		progs[numProgs++]=p;
		nextTok();
		if (tokNot("{")) wrong("Expected {, got ",tok);
		nextTok();
		if (tokIs("PROGRAMS"))
		{ /*We have a program list-- parse it.*/
			nextTokN();
			while (tokNot("}")&&tokNot("BINS"))
			{
		 		int progNo,oldNumProgs=p->numProgs;
		 		for (progNo=0;progNo<numProgs;progNo++)
		 			if (0==strcmp(tok,progs[progNo]->name))
		 				p->progs[p->numProgs++]=(PROG *)progs[progNo];
				if (oldNumProgs==p->numProgs)
				/*We couldn't find the referenced program.*/
					printf("SEVERE WARNING: couldn't find program '%s',\n"
						" part of program '%s'!  Ignoring...\n\n",tok,p->name);
				nextTokN();
			}
		}
		if (tokIs("BINS"))
		{ /*We have a "directory-less binaries" list-- parse it.*/
			nextTokN();
			while (tokNot("}"))
			{
				prog *q; /*q will be a directory-less binary for program p*/
				q=(prog *)malloc(sizeof(prog));
				p->progs[p->numProgs++]=(PROG *)q;
				strcpy(q->path,codeLoc);/*Same path.*/
				strcpy(q->name,tok);/*New name.*/
				q->numLibs=q->numProgs=0;/*No subprograms.*/
				q->isOnlyBinary=1;
				nextTokN();
			}
		}

		if (tokIs("}"))
			nextTokN()
		else
			wrong("Expected } to close PROGRAM, got ",tok);
	}
	nextTok();
/*Parse DOCUMENTATION section*/
	if (tokNot("DOCUMENTATION")) wrong("Expected DOCUMENTATION, got ",tok);
	nextTok();
	if (tokNot("{")) wrong("Expected {, got",tok);
	nextTokN();
	while (tokNot("}"))
	{
		prog *p=(prog *)malloc(sizeof(prog));
		strcpy(p->path,codeLoc);
		strcpy(p->name,tok);
		p->numLibs=p->numProgs=p->isOnlyBinary=p->isCat=0;
		p->isDoc=1;
		progs[numProgs++]=p;
		nextTokN();
	}
	nextTok();
/*Parse CATEGORIES section*/
	if (tokNot("CATEGORIES")) wrong("Expected CATEGORIES, got ",tok);
	nextTok();
	if (tokNot("{")) wrong("Expected {, got ",tok);
	nextTokN();
	while (tokNot("}"))
	{
		prog *p=(prog *)malloc(sizeof(prog));
		strcpy(p->path,"CATEGORY/");
		strcpy(p->name,tok);
		p->numProgs=p->numLibs=p->isOnlyBinary=p->isDoc=0;
		p->isCat=1;
		progs[numProgs++]=p;
		nextTok();
		if (tokNot("{")) wrong("Expected {, got ",tok);
		nextTok();
		while (tokNot("}"))
		{
		 	int progNo,oldNumProgs=p->numProgs;
		 	for (progNo=0;progNo<numProgs;progNo++)
		 		if (0==strcmp(tok,progs[progNo]->name))
		 			p->progs[p->numProgs++]=(PROG *)progs[progNo];
			if (oldNumProgs==p->numProgs)
			/*We couldn't find the referenced program.*/
				printf("WARNING: couldn't find '%s',\n"
					" a part of the '%s' package! Ignoring...\n\n",tok,p->name);
			nextTokN();
		}
		if (tokIs("}"))
			nextTokN()
		else
			wrong("Expected } to close CATEGORIES, got ",tok);
	}
}
/******************** Find libraries used by each program.**********/
void findLibraries(void)
{
	int progNo;
	for (progNo=0;progNo<numProgs;progNo++)
	{
		prog *p=progs[progNo];
		int libNo;
		if (!progExists(p->path,p->name))
			continue;

		// Skip over documentation, but let the user know its there
		if ( p->isDoc ) {
			printf("\nAdding documentation: %s\n",p->name);
			continue;
		}

		printf("\nProgram '%s' has libraries:\n",p->name);
		for (libNo=0;libNo<numLibs;libNo++)
		{
			char makeFile[255];
			char cmd[255];
			int ret;
			sprintf(makeFile,"../%s%s/Makefile", p->path, p->name);
			sprintf(cmd,"fgrep %s.a %s > /dev/null\n",
				libs[libNo]->name,makeFile);
			ret=system(cmd);
			if (ret==0)
			{
				printf("\t%s.a\n",libs[libNo]->name);
				p->libs[p->numLibs++]=libs[libNo];
			}
		}
	}
}

/********************************************************************/
/* MakeMake, which prints the now-parsed program list to a makefile */
/********************************************************************/
void printMake(FILE *f,char *platform);
#include "printMake.c"
  /*
  Note that I've hidden the unspeakable hideousness of printMake
  in a separate file.  Basically, it generates the tail end
  (the library, program, and tool targets) of the makefile.
  */
void makeMakefile(char *platform)
{
	char command[255];
	char makefileName[255];
	FILE *make;

	strcpy(makefileName,"../Makefile.old");

	sprintf(command,"rm -f %s\n",makefileName);
	system(command);
	sprintf(command,"cat makestart > %s\n",makefileName);
	system(command);

	make=fopen(makefileName,"a");

	if (make==NULL)
		{printf("Can't open output '%s'.\n",makefileName);exit(1);}

	printMake(make,platform);

	fclose(make);
}
/***************************************************************/
/* UpdateAutotar, which creates the stuff in asf_tools/autotar */
/***************************************************************/
int strcmpr(const char **a, const char **b)
{
	return strcmp(*a,*b);
}
char fnames[10000][256];
int numFnames=0;
void cleanAndWrite(FILE *out)
{
	int i;
	char *lastName="";
	for (i=0;i<numFnames;i++)
	{
		if (0!=strcmp(lastName,fnames[i]))
			fprintf(out,"%s\n",fnames[i]);
		lastName=fnames[i];
	}
	numFnames=0;
}
void addPathName(char *path,char *name)
{
	char *outName = fnames[numFnames];
	numFnames++;
	strcpy(outName,path);
	strcat(outName,name);
}
void addProgram(prog *p,int writeSource)
{
	int i;
	if (p->isOnlyBinary&&writeSource)
		return;
	if (!p->isCat&&!p->isDoc)
		addPathName(writeSource?p->path:"",p->name);
	if (p->isDoc)
		addPathName(p->path,p->name);
	/*Also write out library names.*/
	if (writeSource)
		for (i=0;i<p->numLibs;i++)
			addPathName(p->libs[i]->path,p->libs[i]->name);
	/*Recursively add children's names.*/
	for (i=0;i<p->numProgs;i++)
		addProgram((prog *)p->progs[i],writeSource);
}
void updateAutotar(void)
{
	char name[255];
	int i;
	FILE *out;
  /* Clean out the crap in the old directory.*/
	system("/bin/rm -f ../../release/autotar/source.* ../../release/autotar/binary.*\n");
  /* Now fill it up with new crap.*/
	for (i=0;i<numProgs;i++)
	{
		prog *p=progs[i];
		/* if (!p->isCat) continue; *//*Skip things that aren't categories*/
		strcpy(name,"../../release/autotar/source.");
		strcat(name,p->name);
		out=fopen(name,"w");
		if (!p->isDoc)
			fprintf(out,"include\n");
		addProgram(p,1);
		cleanAndWrite(out);
		fclose(out);

		strcpy(name,"../../release/autotar/binary.");
		strcat(name,p->name);
		out=fopen(name,"w");
		addProgram(p,0);
		cleanAndWrite(out);
		fclose(out);
	}
}
/**************************MAIN*******************************/
int main(int argc, char **argv)
{
	char *sys="solaris";
	if (argc>1)
		sys=argv[1];
	printf( "Makemake reads master_program_list, and the make \n"
		"fragment in this directory (makestart) to generate\n"
		"the top-level Makefile. It's working its magic now...\n\n");
	if (NULL==(master=fopen("master_program_list","r")))
		{printf("Can't open './master_program_list'.\n");exit(1);}

	parseMasterList();

	findLibraries();

	printf("\nA total of %i libraries and %i programs\n",numLibs,numProgs);

	makeMakefile(sys);

	if (0==system("test -x ../../release/autotar"))
	{/*Autotar directory exists-- so fill it.*/
		printf("\n\nCreating autotar directory (slowly...)\n");
		updateAutotar();
		printf("Autotar directory updated!\n\n");
	}

	printf("Makemake Complete\n");

	return 0;
}
