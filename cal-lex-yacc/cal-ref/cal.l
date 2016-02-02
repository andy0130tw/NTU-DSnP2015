%{
#include <iostream>
#include <cstdlib>
#include "calYacc.hpp"

using namespace std;

extern "C" {
   int calwrap() { return 1; }
}

%}

WS       [ \t]
ENDL     [\n]
DIGIT    [0-9]
NUMBER   [-]?{DIGIT}+

%%

{WS}     ;
{ENDL}   { return ENDL; }
{NUMBER} { callval.nv = atoi(yytext); return NUMBER; }
"+"      { return ADD; }
"-"      { return SUB; }
"*"      { return MUL; }
"/"      { return DIV; }
"("      { return BRLEFT; }
")"      { return BRRIGHT; }

%%

