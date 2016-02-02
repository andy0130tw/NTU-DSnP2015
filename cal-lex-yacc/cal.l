%{
#include <iostream>
#include <cstdlib>
#include "calYacc.hpp"

using namespace std;

extern "C" {
   int calwrap() { return 1; }
}

%}

WS        [ \t]
DIGIT     [0-9]
OP_ADD    "+"
OP_SUB    "-"
OP_MUL    "*"
OP_DIV    "/"
OP        {OP_ADD}|{OP_SUB}|{OP_MUL}|{OP_DIV}
LBRACE    "("
RBRACE    ")"
INTEGER   ([1-9]{DIGIT}*|0)
NUMBER    {INTEGER}
ENDL      "\n"

%%

{WS} ;

{LBRACE}  { return LBRACE; }
{RBRACE}  { return RBRACE; }
{OP_ADD}  { return TOK_ADD; }
{OP_SUB}  { return TOK_SUB; }
{OP_MUL}  { return TOK_MUL; }
{OP_DIV}  { return TOK_DIV; }

{NUMBER}  { callval = atoi(yytext); return NUMBER; }
{ENDL}    { return ENDL; }
.         { cerr << "unrecognized token \"" << yytext << "\"" << endl; }

%%

