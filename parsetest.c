#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "errormsg.h"
#include "absyn.h"  // Add this for abstract syntax tree
#include "semant.h" // Add this for semantic analysis

extern int yyparse(void);
extern A_exp absyn_root; // Add this to access the AST root

void parse(string fname) 
{
  EM_reset(fname);
  if (yyparse() == 0) { /* parsing worked */
    fprintf(stderr, "Parsing successful!\n");
    
    // Now perform semantic analysis on the parsed AST
    fprintf(stderr, "Starting semantic analysis...\n");
    SEM_transProg(absyn_root);
    fprintf(stderr, "Semantic analysis completed!\n");
  }
  else 
    fprintf(stderr, "Parsing failed\n");
}

int main(int argc, char **argv) {
  if (argc!=2) {fprintf(stderr,"usage: a.out filename\n"); exit(1);}
  parse(argv[1]); 
  return 0;

  // for(int i = 1; i <= 9; i++){
  //   char filename_buffer[20]; 
  //   snprintf(filename_buffer, sizeof(filename_buffer), "tests/test%d", i);
  //   parse(filename_buffer);
  //   printf("\n");
  // }
  //  return 0;
}