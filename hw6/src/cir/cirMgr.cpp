/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
   //,UNSPECIFIED_FORMAT_ERROR = -1
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static int tokenCnt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

// intended for parsing
enum TokenType {
   num,
   str,
   str_line,
   chr,
   space,
   newline,
   none
};

enum ParseState { header,  input,   latch,   output,
                  andGate, _symbol,  comment, dummy    };

static bool checkSeparator(istream& f, TokenType exp_sep, CirParseError& err) {
   char c = f.peek();
   switch(exp_sep) {
      case newline:
         if (c != '\n') {
            // cerr << "fail newl [" << c << "]" << endl;
            err = MISSING_NEWLINE;
            return false;
         }
         f.ignore(1); lineNo++; colNo = 0;
         // a dirty workaround to make sure it is cleaned properly after each def
         //tokenCnt = 0;
         if (f.peek() == '\n') {
            errMsg = "...";
            err = MISSING_DEF;
            return false;
         }
         break;

      case space:
         if (c != ' ') {
            // cerr << "fail space [" << c << "]" << endl;
            err = MISSING_SPACE;
            return false;
         }
         f.ignore(1); colNo++;
         if (f.peek() == ' ') {
            err = EXTRA_SPACE;
            return false;
         }
         break;

      default: break;
   }
   return true;
}

static bool readToken(istream& f, TokenType exp_type, string& buf_str, unsigned int& buf_int) {
   char c = f.peek();
   size_t i = 0;
   switch (exp_type) {
      case num:
         while (true) {
            c = f.peek();
            if (!(c >= '0' && c <= '9')) break;
            f.ignore(1);
            buf[i++] = c;
         }
         if (!i)
            return false;
         buf[i] = '\0';
         buf_int = atoi(buf);
         // cerr << "read int " << buf_int << endl;
         colNo += buf_str.length();
         break;
      case str:
         f >> buf_str;
         // cerr << "read str " << buf_str << endl;
         colNo += buf_str.length();
         break;
      case str_line:
         getline(f, buf_str);
         // cerr << "read line " << buf_str << endl;
         colNo += buf_str.length();
         break;
      case chr:
         buf_str = f.get();
         // cerr << "read chr " << buf_str << endl;
         colNo++;
         break;
      default: break;
   }
   return true;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
void CirMgr::dfs(GateList* l = 0) const {
   // do dfs and leave the mark for tracing
   GateList::const_iterator it = _poList.begin();
   CirGate::clearMark();
   for (; it != _poList.end(); ++it)
      (*it)->traversal(l);
}

bool
CirMgr::readCircuit(const string& fileName)
{
   ifstream f(fileName.c_str());
   if (!f.is_open()) {
      f.close();
      cerr << "Cannot open design \"" << fileName << "\"!!" << endl;
      return false;
   }

   CirParseError err = DUMMY_END;

   lineNo = 0;
   colNo = 0;
   tokenCnt = 0;

   TokenType exp_type = str;
   TokenType exp_sep = none;
   ParseState state = header;

   // storing parsing data
   unsigned int buf_int = 0;
   string buf_str;

   // temporary objects for creating instances from definition
   unsigned defCount;
   unsigned defTokens[3];
   struct {
      char type;
      int id;
   } sym;

   // for reversely searching
   // GateList gates;

   try{

      while (!f.eof()) {

         if (!checkSeparator(f, exp_sep, err))
            break;

         if (!readToken(f, exp_type, buf_str, buf_int)) {
            errMsg = "...";
            err = ILLEGAL_NUM;
            break;
         }

         /********** INTEPRET  **********/
         switch (state) {
            case header:   // 6 input;  "aag"  _uint_*5
               switch (tokenCnt) {
                  case 0: // header id
                     if (buf_str != "aag") {
                        errMsg = buf_str;
                        err = ILLEGAL_IDENTIFIER;
                     }
                     break;
                  case 1: /* M */ _maxNum = buf_int; break;
                  case 2: /* I */ _inputCount = buf_int; break;
                  case 3: /* L */ _latchCount = buf_int; break;
                  case 4: /* O */ _outputCount = buf_int; break;
                  case 5: /* A */ _andGateCount = buf_int; break;
               }
               exp_type = num;
               exp_sep = space;
               if (tokenCnt == 5) {
                  // M >= I + L + A
                  if (_maxNum < _inputCount + _latchCount + _andGateCount) {
                     errMsg = "Num of variables";
                     errInt = _maxNum;
                     err = NUM_TOO_SMALL;
                     break;
                  }

                  // _gates.resize(_maxNum, 0);
                  _gates[0] = new ConstGate(); // constant false

                  exp_sep = newline;
               }
               break;
            case input:    // 1 input;  _even_
               exp_sep = none;
               // if meet >= 2 inputs, interpret it as a latch
               if (!checkSeparator(f, newline, err)) {
                  errMsg = "PI";
                  err = MISSING_DEF;
                  break;
               }
               if (buf_int % 2 != 0) {
                  errMsg = "PI";
                  err = ILLEGAL_NUM;
                  break;
               } else if (buf_int / 2 == 0) {
                  err = REDEF_CONST;
                  break;
               } else if (buf_int / 2 > _maxNum) {
                  errMsg = "PI index";
                  err = NUM_TOO_BIG;
                  break;
               } else if (getGate(buf_int / 2) != 0) {
                  errInt = buf_int / 2;
                  errGate = _gates[buf_int / 2];
                  err = REDEF_GATE;
                  break;
               }
               cerr << "=== PI === " << buf_int/2 << endl;
               // gates[buf_int / 2] =
               cirMgr->addPI(lineNo, buf_int/2);
               // _piList.push_back(new PiGate());
               defCount++;
               break;
            case latch:    // 2 inputs; _even_ _uint_; omitting here
               break;
            case output:   // 1 input;  _uint_
               exp_sep = none;
               // if meet >= 2 inputs, interpret it as an andGate
               if (!checkSeparator(f, newline, err)) {
                  errMsg = "PO";
                  err = MISSING_DEF;
                  break;
               }
               if (buf_int / 2 > _maxNum) {
                  errMsg = "PO index";
                  err = NUM_TOO_BIG;
                  break;
               }
               cerr << "=== PO === " << buf_int/2 << " " << buf_int%2 << endl;
               //buf_int
               //gates[buf_int / 2] =
               if (!cirMgr->getGate(buf_int/2)) {
                  CirGate* src = cirMgr->addUndef(buf_int/2);
                  cirMgr->addPO(lineNo, src, buf_int%2);
               }

               // _poList.push_back(new AigGate());
               defCount++;
               break;
            case andGate:  // 3 inputs; _even_ _uint_ _uint_
               exp_sep = none;
               // if meet more inputs, interpret it as extra input (i.e. spaces)
               if (tokenCnt == 2) {
                  defTokens[2] = buf_int;
                  defCount++;
                  tokenCnt = -1;
                  cerr << "=== AIG === " << defTokens[0] << " " << defTokens[1] << " " << defTokens[2] << endl;
                  CirGate* fan1 = cirMgr->getGate(defTokens[1] / 2);
                  CirGate* fan2 = cirMgr->getGate(defTokens[2] / 2);
                  if (!fan1) fan1 = cirMgr->addUndef(defTokens[1] / 2);
                  if (!fan2) fan2 = cirMgr->addUndef(defTokens[2] / 2);
                  cirMgr->addAIG(
                     lineNo, defTokens[0]/2,
                     fan1, fan2,
                     defTokens[1] % 2, defTokens[2] % 2
                  );

                  exp_sep = newline;
               } else {
                  if (tokenCnt == 0) {
                     defTokens[0] = buf_int;
                     // todo: check even; redef; size
                  } else if (tokenCnt == 1) {
                     defTokens[1] = buf_int;
                     // todo: check even; size
                  }
                  exp_sep = space;
               }
               break;
            case _symbol:   // 2 inputs; _symb_ _toend_
               if (tokenCnt == 0) {
                  char c = buf_str[0];
                  if (c == 'c') {
                     // go to comment area
                     exp_sep = newline;
                     exp_type = none;
                  } else if (c == 'i' || c == 'o') {
                     sym.type = c;
                     exp_sep = none;
                     exp_type = num;
                  } else {
                     errMsg = c;
                     err = ILLEGAL_SYMBOL_TYPE;
                     break;
                  }
               } else if (tokenCnt == 1) {
                  sym.id = buf_int;
                  exp_sep = space;
                  exp_type = str_line;
               } else {
                  //cerr << "=== SYMBOL === " << symbol.id << " " << symbol.type << endl;
                  GateList& gg = (sym.type == 'i' ? _piList : _poList);
                  gg[sym.id]->_name = buf_str;
                  defCount++;
                  tokenCnt = -1;
                  // process symbol line
                  exp_sep = none;
                  exp_type = chr;
               }
               break;
            case comment:  // 1 input;  "c\n"  _toend_
               break;
            default:
               err = ILLEGAL_IDENTIFIER; //UNSPECIFIED_FORMAT_ERROR;
               break;
         }

         if (err != DUMMY_END)
            break;

         // ascend states
         ParseState ps = state;
         if (state == header && tokenCnt == 5) { state = input; defCount = 0; }
         if (state == input && defCount == _inputCount) {state = latch; defCount = 0; }
         if (state == latch && defCount == _latchCount) {state = output; defCount = 0; }
         if (state == output && defCount == _outputCount) {state = andGate; defCount = 0; lineNo++; }
         if (state == andGate && defCount == _andGateCount) {state = _symbol; defCount = 0; exp_type = chr; }

         if (ps != state) {
            // cerr << "===== change state: " << ps << " -> " << state << " =====" << endl;
            tokenCnt = 0;
            defCount = 0;
         } else tokenCnt++;

         if (exp_type == none)
            break;

         // cerr << "read over [" << lineNo << ":" << colNo << "]; tokenCnt=" << tokenCnt << endl;
      }
   } catch (CirParseError) {
      f.close();
      return false;
   }

   f.close();

   // for backward comp.
   if (err != DUMMY_END) {
      parseError(err);
      return false;
   }
   return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   unsigned int sum = _inputCount + _outputCount + _andGateCount;

   cout << "Circuit Statistics" << endl;
   cout << "==================" << endl;
   cout << "  PI    " << setw(8) << right << _inputCount << endl;
   cout << "  PO    " << setw(8) << right << _outputCount << endl;
   cout << "  AIG   " << setw(8) << right << _andGateCount << endl;
   cout << "------------------" << endl;
   cout << "  Total " << setw(8) << right << sum << endl;
}

void
CirMgr::printNetlist() const
{
   GateList dfs_list;
   dfs(&dfs_list);

   CirGate* curr;
   for (size_t i = 0, n = dfs_list.size(); i < n; i++) {
      curr = dfs_list[i];
      if (!curr) continue;
      cout << "[" << i << "] "
           << setw(3) << left << curr->getTypeStr()
           << " ";

      if (curr->_type == UNDEF_GATE)
         cout << "*";
      cout << curr->getID();

      if (curr->_type == AIG_GATE) {
         cout << " ";
         if (curr->getInv(0)) cout << "!";
         cout << curr->_faninList[0]->getID() << " ";
         if (curr->getInv(1)) cout << "!";
         cout << curr->_faninList[1]->getID();
      } else if (curr->_type == PO_GATE) {
         cout << " ";
         if (curr->getInv(0)) cout << "!";
         cout << curr->_faninList[0]->getID();
      }

      if (!curr->_name.empty())
         cout << " (" << curr->_name << ")";

      cout << endl;
   }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for (size_t i = 0, n = _piList.size(); i < n; i++) {
      cout << " " << _piList[i]->getID();
   }
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for (size_t i = 0, n = _poList.size(); i < n; i++) {
      cout << " " << _poList[i]->getID();
   }
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   vector<unsigned> fl, unu;
   for (GateMap::const_iterator it = _gates.begin(); it != _gates.end(); ++it) {
      unsigned gid = (*it).first;
      CirGate* gate = (*it).second;

      // skip const gate
      if (gid == 0) continue;

      // floating fanin
      for (size_t i = 0, n = gate->_faninList.size(); i < n; i++)
         if (gate->_faninList[i]->_type == UNDEF_GATE) {
            fl.push_back(gid);
            break;
         }

      // unused; having no DIRECT (not "effective") fanout
      if (gate->_type != PO_GATE && gate->_fanoutList.empty())
         unu.push_back(gid);
   }

   if (!fl.empty()) {
      cout << "Gates with floating fanin(s):";
      for(size_t i = 0, n = fl.size(); i < n; i++)
         cout << " " << fl[i];
      cout << endl;
   }

   if (!unu.empty()) {
      cout << "Gates defined but not used  :";
      for(size_t i = 0, n = unu.size(); i < n; i++)
         cout << " " << unu[i];
      cout << endl;
   }
}

void
CirMgr::writeAag(ostream& outfile) const
{
   // preprocessing
   GateList l, piGen, poGen;
   unsigned newA = 0;
   dfs(&l);

   for (GateMap::const_iterator it = _gates.begin(); it != _gates.end(); ++it)
      switch (it->second->_type) {
         case PI_GATE: piGen.push_back(it->second); break;
         case PO_GATE: poGen.push_back(it->second); break;
         case AIG_GATE: newA++; break;
         default: break;
      }

   // header: aag M I O L "A", only andGate count is recalculated.
   outfile << "aag "
           << _maxNum << " "
           << _inputCount << " "
           << _latchCount << " "
           << _outputCount << " "
           << newA << endl;

   // input
   for (size_t i = 0, n = _piList.size(); i < n; i++)
      outfile << _piList[i]->getID() * 2 << endl;

   // output
   for (size_t i = 0, n = _poList.size(); i < n; i++)
      outfile << (_poList[i]->_faninList[0]->getID() * 2 + (_poList[i]->getInv(0) ? 1 : 0)) << endl;

   // aig
   for (size_t i = 0, n = l.size(); i < n; i++)
      if (l[i]->_type == AIG_GATE) {
         outfile << (l[i]->getID() * 2) << " "
                 << (l[i]->_faninList[0]->getID() * 2 + (l[i]->getInv(0) ? 1 : 0)) << " "
                 << (l[i]->_faninList[1]->getID() * 2 + (l[i]->getInv(1) ? 1 : 0)) << endl;
      }

   // symbol
   for (size_t i = 0, n = piGen.size(); i < n; i++)
      outfile << 'i' << i << " " << piGen[i]->_name << endl;
   for (size_t i = 0, n = poGen.size(); i < n; i++)
      outfile << 'o' << i << " " << poGen[i]->_name << endl;

   // comment
   cout << "c\n" << "generated by cirWrite function" << endl;
}
