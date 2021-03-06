/*
 * parse.c - Parse source file.
 */

#include <stdio.h>
#include <stdlib.h>


#include "absyn.h"
#include "errormsg.h"
#include "parse.h"
#include "prabsyn.h"
#include "types.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "translate.h"
#include "semant.h"
#include "printtree.h"
#include "canon.h"
#include "assem.h"
#include "codegen.h"

extern anyErrors;
extern int yyparse(void);
extern A_exp absyn_root;
extern Temp_map F_tempMap;

/* parse source file fname; 
   return abstract syntax data structure */

A_exp parse(string fname) 
{
	EM_reset(fname);
    if (yyparse() == 0) 
		return absyn_root;
	else 
		printf("Syntax Error.\n");
		return NULL;
}

static void doProc(FILE *out, F_frame frame, T_stm body)
{
 AS_proc proc;
 T_stmList stmList;
 AS_instrList iList;

 stmList = C_linearize(body);
 stmList = C_traceSchedule(C_basicBlocks(stmList));
 iList  = codegen(frame, stmList); 
 fprintf(out, "BEGIN %s\n", Temp_labelstring(F_name(frame)));
 AS_printInstrList (out, iList, Temp_layerMap(F_tempMap,Temp_name()));
 fprintf(out, "END %s\n\n", Temp_labelstring(F_name(frame)));
}

int main(int argc, string *argv)
{
	A_exp absyn_root;
	S_table base_env, base_tenv;
	F_fragList fragList;
	char outfile[100];
	FILE *out = stdout;

	if (argc==2) 
	{
		absyn_root = parse(argv[1]);
		if (!absyn_root) 
			return 1;
		#if 0 
		pr_exp(out, absyn_root, 0); /* print absyn data structure */
		fprintf(out, "\n");
		#endif

		fragList = SEM_transProg(absyn_root);
		if (anyErrors) 
			return 1; /* don't continue */

		/* convert the filename */
		sprintf(outfile, "%s.s", argv[1]);
		out = fopen(outfile, "w");
		FILE * f1 = fopen("Output/IR Tree.txt", "w");
		FILE * f2 = fopen("Output/Absyn Tree.txt", "w");
		if (absyn_root) 
		{
			pr_exp(f2, absyn_root, 0);
			fclose(f2);
			fragOutput(fragList, f1);
			fclose(f1);
		}
		for (;fragList;fragList=fragList->tail)
			if (fragList->head->kind == F_procFrag)
				doProc(out, fragList->head->u.proc.frame, fragList->head->u.proc.body);
			else if (fragList->head->kind == F_stringFrag)
				fprintf(out, "%s: %s\n",Temp_labelstring(fragList->head->u.stringg.label), fragList->head->u.stringg.str);
			else assert(0);
		fclose(out);
		return 0;
	}
	EM_error(0,"usage: tiger file.tig");
	return 1;
}
