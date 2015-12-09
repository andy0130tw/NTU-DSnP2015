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
enum TokenType { start, num, str, chr, space, newline, until_end, none };
enum ParseState { initial, header, pipo, symbol, comment, dummy };

static bool isDigit(char x) {
   return (x >= '0' && x <= '9');
}

static unsigned toUint(string& x) {
   for (size_t i = 0, n = x.size(); i < n; i++) {
      if (!(x[i] >= '0' && x[i] <= '9')) {
         errMsg = "number";
         throw ILLEGAL_NUM;
      }
   }
   return atoi(x.c_str());
}

static void checkLiteralID(CirMgr* mgr, unsigned gid, bool checkEven, bool checkUnique = true) {
   if (gid / 2 > mgr->_maxNum) {
      throw MAX_LIT_ID;
   } else if (checkEven && gid % 2 != 0) {
      throw CANNOT_INVERTED;
   } else if (checkUnique) {
      errGate = mgr->getGate(gid / 2);
      if (gid == 0) {
         throw REDEF_CONST;
      } else if (errGate != 0 && errGate->_type != UNDEF_GATE) {
         throw REDEF_GATE;
      }
   }
}

static string flushToken(size_t& n) {
   buf[n] = '\0';
   string ret(buf);
   n = 0;
   return ret;
}

static void readChar(istream& f, size_t& n) {
   buf[n++] = f.get();
   colNo++;
}

static void parseTokens(istream& f, vector< vector<string>* >& tokenList) {
   char c;
   TokenType tok = start;
   ParseState p = initial;
   size_t bp = 0;

   lineNo = 0;
   colNo = -1;

   vector<string>* currLine = new vector<string>;
   tokenList.push_back(currLine);

   while (!f.eof()) {
      c = f.peek();
      // cerr << "state=" << tok << " peek=" << c << endl;
      switch (tok) {
         case start:
            if (isDigit(c)) {
               tok = num;
            } else if (currLine->empty() && p != initial && p != header) {
               // specialize the very first character; it may be a symbol
               tok = chr;
               p = symbol;
            } else if (p == initial) {
               if (c == ' ')
                  throw EXTRA_SPACE;
               else if (c < 32) {
                  errInt = (int)c;
                  throw ILLEGAL_WSPACE;
               }
               p = header;
            } else if (p == comment) {
               if (c != '\n') {
                  throw MISSING_NEWLINE;
               }
               tok = until_end;
            }
            else if (c == ' ') { f.ignore(1); tok = space; }
            else if (c == '\n') { f.ignore(1); tok = newline; }
            else if (c > 32) { tok = str; }
            else {
               errInt = (int)c;
               throw ILLEGAL_WSPACE;
            }
            break;
         case num:
            if (isDigit(c)) {
               readChar(f, bp);
               // cerr << "=== read int " << c << endl;
            } else if (c != ' ' && c != '\n') {
               errMsg = "space character";
               throw MISSING_NUM;
            } else {
               currLine->push_back(flushToken(bp));
               // cerr << "===== flush int [" << buf << "]" << endl;
               tok = start;
            }
            break;
         case str:
            if (c > 32 || (p == symbol && c == ' ')) {
               readChar(f, bp);
               // cerr << "=== read char [" << c << "]" << endl;
            } else {
               currLine->push_back(flushToken(bp));
               // cerr << "===== flush str [" << buf << "]" << endl;
               tok = start;
            }
            break;
         case space:
            if (c == ' ' && p != symbol)
               throw EXTRA_SPACE;
            colNo++;
            tok = start;
            break;
         case newline:
            // if (c == '\n') {
            //    errMsg = "/FIXME/";
            //    throw MISSING_DEF;
            // }
            lineNo++;
            colNo = 0;
            currLine = new vector<string>;
            tokenList.push_back(currLine);
            tok = start;
            if (p == header) p = pipo;
            break;
         case chr:
            buf[bp++] = c;
            f.ignore(1); colNo++;
            if (p == symbol) {
               if (c == 'c') {
                  p = comment;
               } else if (c > 32 && c != 'i' && c != 'o' && c != 'l') {
                  errMsg = c;
                  throw ILLEGAL_SYMBOL_TYPE;
               } else if (f.peek() <= 32) {
                  // symbols MUST have an ID after it
                  throw EXTRA_SPACE;
               }
            }
            currLine->push_back(flushToken(bp));
            // cerr << "===== flush one char [" << c << "]" << endl;
            tok = start;
            break;
         case until_end:
            while (!f.eof() && bp < 1024)
               buf[bp++] = f.get();

            currLine->push_back(flushToken(bp));
            tok = none;
            break;
         default: break;
      }
      if (tok == none) {
         // disgard buffer and ignore anything left
         break;
      }
   }
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

   bool ok = true;

   vector< vector<string>* > tokens;
   vector<string>* line;
   try{
      parseTokens(f, tokens);

      lineNo = 0;
      colNo = 0;

      // ========== DEBUG  ==========
      // for (size_t i = 0; i < tokens.size(); i++) {
      //    line = tokens[i];
      //    for (size_t j = 0; j < line->size(); j++) {
      //       cout << "[" << (*line)[j] << "] ";
      //    }
      //    cout << endl;
      // }

      // ========== HEADER ==========
      line = tokens[0];
      string headerErrMsg[5] = {"variables", "PIs", "Latches", "POs", "AIGs"};
      for (size_t i = 0; i < 6; i++) {
         if (i >= line->size()) {
            if (i == 0) {
               errMsg = "identifier";
            } else {
               errMsg = "number of " + headerErrMsg[i-1];
            }
            throw MISSING_NUM;
         } else if (i == 0 && (*line)[0] != "aag") {
            errMsg = (*line)[0];
            throw ILLEGAL_IDENTIFIER;
         } else {
                 if (i == 1) { /* M */ _maxNum       = toUint((*line)[1]); }
            else if (i == 2) { /* I */ _inputCount   = toUint((*line)[2]); }
            else if (i == 3) { /* L */ _latchCount   = toUint((*line)[3]); }
            else if (i == 4) { /* O */ _outputCount  = toUint((*line)[4]); }
            else if (i == 5) { /* A */ _andGateCount = toUint((*line)[5]); }
            colNo += (*line)[i].length() + 1;
         }
      }
      if (line->size() > 6)
         throw MISSING_NEWLINE;

      // check for numbers
      if (_maxNum < _inputCount + _latchCount + _andGateCount)
         throw NUM_TOO_SMALL;  // no need to handle colNo here

      // const gate is safe to be created
      _gates[0] = new ConstGate();

      // ========== INPUT  ==========
      errMsg = "PI";
      for (size_t i = 0; i < _inputCount; i++) {
         lineNo++; colNo = 0;
         line = tokens[lineNo];
         if (line->empty()) {
            throw MISSING_DEF;
         } else if (line->size() != 1) {
            colNo = line[i].size();
            throw MISSING_NEWLINE;
         }
         unsigned gid = toUint((*line)[0]);
         errInt = gid;
         checkLiteralID(this, gid, true);

         addPI(lineNo+1, gid / 2);
         cerr << "=== PI === " << gid/2 << endl;
      }

      // ========== LATCH  ========== (omitted)

      // ========== OUTPUT ==========
      errMsg = "PO";
      for (size_t i = 0; i < _outputCount; i++) {
         lineNo++; colNo = 0;
         line = tokens[lineNo];
         if (line->empty()) {
            throw MISSING_DEF;
         } else if (line->size() != 1) {
            colNo = (*line)[i].length();
            throw MISSING_NEWLINE;
         }
         unsigned gid = toUint((*line)[0]);

         errInt = gid;
         checkLiteralID(this, gid, false, false);

         CirGate* src = addUndef(gid / 2);
         addPO(lineNo+1, src, gid%2);
         // cerr << "=== PO === " << gid/2 << " " << gid%2 << endl;
      }

      // ========== AIGATE ==========
      errMsg = "AIG";
      for (size_t i = 0; i < _andGateCount; i++) {
         lineNo++; colNo = 0;
         line = tokens[lineNo];

         unsigned lhs1, lhs2, rhs;

         for (size_t j = 0; j < 3; j++) {
            if (j >= line->size()) {
               if (j == 0)
                  throw MISSING_DEF;
               else
                  throw MISSING_NUM;
            } else if (j == 0) rhs  = toUint((*line)[0]);
              else if (j == 1) lhs1 = toUint((*line)[1]);
              else if (j == 2) lhs2 = toUint((*line)[2]);

            colNo += (*line)[j].length() + 1;
         }

         checkLiteralID(this, rhs, true);
         checkLiteralID(this, lhs1, false, false);
         checkLiteralID(this, lhs2, false, false);

         CirGate* fan1 = cirMgr->getGate(lhs1 / 2);
         if (!fan1) fan1 = cirMgr->addUndef(lhs1 / 2);

         CirGate* fan2 = cirMgr->getGate(lhs2 / 2);
         if (!fan2) fan2 = cirMgr->addUndef(lhs2 / 2);

         cirMgr->addAIG(lineNo+1, rhs/2, fan1, fan2, lhs1 % 2, lhs2 % 2);
         // cerr << "=== AIG === " << rhs << " " << lhs1 << " " << lhs2 << endl;
      }
      lineNo++;

      // ========== SYMBOL ==========
      for (bool done = false; lineNo < tokens.size(); lineNo++) {
         line = tokens[lineNo];
         char c;
         unsigned cnt;
         GateList* ls;
         for (size_t i = 0; i < 3; i++) {
            if (i >= line->size() && i != 0) {
               // if the first character is not found, it is not an error
               errMsg = "symbolic name";
               throw MISSING_DEF;
            } else if (i == 0) {
               if (line->empty() || (*line)[0][0] == 'c') {
                  done = true;
                  break;
               }
               c = (*line)[0][0];
            } else if (i == 1) {
               cnt = toUint((*line)[1]);
            }
            else if (i == 2) {
               if ((*line)[2].empty()) {
                  errMsg = "symbolic name";
                  throw MISSING_DEF;
               }
               else if (c == 'i') { ls = &_piList; errMsg = "PI index"; }
               else if (c == 'o') { ls = &_poList; errMsg = "PO index"; }
               else {
                  errMsg = c;
                  throw ILLEGAL_SYMBOL_TYPE;
               }
               if (cnt >= ls->size()) {
                  throw NUM_TOO_BIG;
               }
            };
            colNo += (*line)[i].length() + 1;
         }

         if (done) break;

         errGate = (*ls)[cnt];
         if (!(*ls)[cnt]->_name.empty()) {
            errMsg = (*ls)[cnt]->getTypeStr();
            errInt = (*ls)[cnt]->getID();
            cout << (*ls)[cnt]->_name;
            throw REDEF_SYMBOLIC_NAME;
         }

         (*ls)[cnt]->_name = (*line)[2];
         // cout << "=== SYM === " << (*line)[0] << " " << (*line)[1] << " " << (*line)[2] << endl;
      }

      // ========== COMMENT ========= (no need to record this)

   } catch (CirParseError err) {
      ok = false;
      parseError(err);
   }

   for (size_t i = 0; i < tokens.size(); i++)
      delete tokens[i];

   f.close();
   return ok;
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
      if (!piGen[i]->_name.empty())
         outfile << 'i' << i << " " << piGen[i]->_name << endl;
   for (size_t i = 0, n = poGen.size(); i < n; i++)
      if (!poGen[i]->_name.empty())
         outfile << 'o' << i << " " << poGen[i]->_name << endl;

   // comment
   cout << "c\n" << "generated by cirWrite function" << endl;
}
