%{
#include <iostream>

using namespace std;

//-----------------------
//    Extern variables
//-----------------------
int callex (void);
int calparse();
extern "C" int calwrap();

//-----------------------
//    static functoins
//-----------------------
static void calerror(const char *str) {
   cerr << "Error: " << str << endl;
}

%}

%left TOK_ADD TOK_SUB
%left TOK_MUL TOK_DIV
%right LBRACE RBRACE

%token NUMBER
%token ENDL

%%

CAL : EXPR
    | CAL EXPR

EXPR : SUM_TERM ENDL  { cout << "= " << $1 << endl; }

SUM_TERM : SUM_TERM TOK_ADD SUM_TERM  { $$ = $1 + $3; }
         | SUM_TERM TOK_SUB SUM_TERM  { $$ = $1 - $3; }
         | PROD_TERM  { $$ = $1; }

PROD_TERM : PROD_TERM TOK_MUL PROD_TERM  { $$ = $1 * $3; }
          | PROD_TERM TOK_DIV PROD_TERM  { if ($3 == 0) { calerror("divided by zero error"); throw; } $$ = $1 / $3; }
          | TERM  { $$ = $1; }

TERM : NUMBER  { $$ = $1; }
     | LBRACE SUM_TERM RBRACE { $$ = $2; }
     | TOK_ADD TERM { $$ = $2; }
     | TOK_SUB TERM { $$ = -$2; }

%%

int main() {
   calparse();
}

