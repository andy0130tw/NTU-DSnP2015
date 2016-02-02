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
static void calerror(const char *str)
{
   cerr << "Error: " << str << endl;
}

%}

%union {
   int    nv;
};

%left ADD SUB
%left MUL DIV
%left BRLEFT BRRIGHT

%token ENDL

%token <nv> NUMBER

%type <nv> SUM_TERM PRODUCT_TERM TERM

%%

CAL : EXPRESSION
    | CAL EXPRESSION

EXPRESSION : SUM_TERM ENDL { cout << "= " << $1 << endl; }

SUM_TERM : SUM_TERM ADD SUM_TERM { $$ = $1 + $3; }
         | SUM_TERM SUB SUM_TERM { $$ = $1 - $3; }
         | PRODUCT_TERM { $$ = $1; }

PRODUCT_TERM : PRODUCT_TERM MUL PRODUCT_TERM { $$ = $1 * $3; }
             | PRODUCT_TERM DIV PRODUCT_TERM { $$ = $1 / $3; }
             | TERM { $$ = $1; }

TERM : NUMBER { $$ = $1; }
     | BRLEFT SUM_TERM BRRIGHT { $$ = $2; }
     | SUB TERM { $$ = -$2; }

%%

int main()
{
   calparse();
}

