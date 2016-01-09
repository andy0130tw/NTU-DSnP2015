/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
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
enum ParseState {
   STATE_INITIAL,
   STATE_HEADER,
   STATE_PI,
   STATE_PO,
   STATE_AIG,
   STATE_SYMBOL,
   STATE_FINISHED,

   STATE_DUMMY
};
static ParseState state = STATE_DUMMY;

static inline bool isTerminatingChar(char c) {
   return (c == ' ' || c == '\n');
}

static inline bool isDigit(char c) {
   return (c >= '0' && c <= '9');
}

// due to some errors are universal but state-dependent,
// when reading or consuming fails, this function is called
// to make an approperiate error message based on internal state
static void emitStatefulError(CirParseError pe) {
   static const string headerErrMsg[5] = { "vars", "PIs", "Latches", "POs", "AIGs" };
   bool doSuffix = false;
   switch (state) {
      case STATE_HEADER: errMsg = "number of " + headerErrMsg[errInt]; break;
      case STATE_PI: errMsg = "PI"; doSuffix = true; break;
      case STATE_PO: errMsg = "PO"; doSuffix = true; break;
      case STATE_AIG: errMsg = "AIG"; doSuffix = true; break;
      case STATE_SYMBOL:
         if (pe == ILLEGAL_NUM)
            errMsg = "symbol index";
         else if (pe == MISSING_IDENTIFIER)
            errMsg = "symbolic name";
         break;
      default: break;
   }
   if (doSuffix) {
      if (pe == MISSING_NUM)
         errMsg += " literal ID";
   }
   throw pe;
}

// maintains colNo
// ### exceptions: ILLEGAL_WSPACE ###
static char readChar(istream& f) {
   char ch = f.get();
   if (ch < 0) return ch; // EOF; do not raise error
   if (ch < 32 && ch != '\n') {
      errInt = (int)ch;
      throw ILLEGAL_WSPACE;
   }
   colNo++;
   return ch;
}

// unget a character from the stream
// update colNo accordingly (sorry, only within a line)
static bool retreatChar(istream& f) {
   if (colNo == 0) return false;
   colNo--;
   f.unget();
   return true;
}

// count colNo backward BASED ON THE INTERNAL BUFFER
// dangerous!! retreating more than once causes troubles
static bool rejectBuffer() {
   size_t bp = 0;
   while (buf[bp] != '\0') {
      bp++;
      if (bp >= 1023) return false;
   }
   if (colNo < bp) {
      // should not happen
      colNo = 0;
      return false;
   }
   colNo -= bp;
   return true;
}

// read an unsigned integer until a space
// ### exceptions: EXTRA_SPACE, ILLEGAL_NUM ###
static unsigned readUint(istream& f) {
   size_t bp = 0;
   char ch = '\0';
   while (bp < 1023) {
      ch = readChar(f);
      if (isTerminatingChar(ch)) {
         retreatChar(f);
         if (bp == 0 && ch == ' ')
            throw EXTRA_SPACE;
         break;
      } else if (ch < 0) {
         emitStatefulError(MISSING_DEF);
         break;
      }
      if (!isDigit(ch))
         emitStatefulError(ILLEGAL_NUM);

      buf[bp++] = ch;
   }
   if (bp == 0) {
      // read number failed
      emitStatefulError(MISSING_NUM);
   }
   buf[bp] = '\0';
   // cout << "read num " << buf << endl;
   return atoi(buf);
}

// read a string (can contain spaces) until line end
// ### exceptions: MISSING_IDENTIFIER ###
static string readStr(istream& f, unsigned maxlen = 0) {
   size_t bp = 0;
   char ch = '\0';
   if (maxlen > 1023) maxlen = 1023;
   while (maxlen == 0 || bp < maxlen) {
      ch = readChar(f);
      if (ch == '\n') {
         retreatChar(f);
         break;
      }
      buf[bp++] = ch;
   }
   if (bp == 0) {
      // read string failed
      emitStatefulError(MISSING_IDENTIFIER);
   }
   buf[bp] = '\0';
   string rtn(buf);
   // cout << "read str [" << buf << "]" << endl;
   return rtn;
}

// a space after the header is then excepted
// return whether this check is passed
// if not, more string should be read before crashing
// ### exceptions: MISSING_IDENTIFIER, EXTRA_SPACE, ILLEGAL_IDENTIFIER ###
static bool consumeAagHeader(istream& f) {
   char ch = f.peek();
   if (ch < 0) {
      errMsg = "aag";
      throw MISSING_IDENTIFIER;
   }
   if (f.peek() == ' ') throw EXTRA_SPACE;

   string str = readStr(f, 3);
   if (str != "aag")
      throw ILLEGAL_IDENTIFIER;

   return isTerminatingChar(f.peek());
}

// ### exceptions: MISSING_SPACE ###
static bool consumeSpace(istream& f) {
   char ch = f.get();
   if (ch != ' ') throw MISSING_SPACE;
   colNo++;
   return true;
}

// ### exceptions: MISSING_NEWLINE ###
static bool consumeNewline(istream& f) {
   char ch = f.get();
   if (ch != '\n') throw MISSING_NEWLINE;
   colNo = 0;
   lineNo++;
   return true;
}

// ### exceptions: MAX_LIT_ID, REDEF_CONST, REDEF_GATE, CANNOT_INVERTED ###
static void checkLiteralID(CirMgr* mgr, unsigned gid, bool checkEven, bool checkUnique = true) {
   if (gid / 2 > mgr->_maxNum) {
      errInt = gid; rejectBuffer();
      throw MAX_LIT_ID;
   }
   if (checkUnique) {
      errGate = mgr->getGate(gid / 2);
      if (gid / 2 == 0) {
         errInt = gid; rejectBuffer();
         throw REDEF_CONST;
      } else if (errGate != 0 && errGate->_type != UNDEF_GATE) {
         errInt = gid;
         throw REDEF_GATE;
      }
   }
   if (checkEven && gid % 2 != 0) {
      errInt = gid; rejectBuffer();
      throw CANNOT_INVERTED;
   }
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
GateList& CirMgr::getDfsList() const {
   if (!_dfsList_clean) {
      _dfsList.clear();
      dfs(&_dfsList);
      _dfsList_clean = true;
   }
   return _dfsList;
}

void CirMgr::dfs(GateList* l) const {
   // do dfs and leave the mark for tracing
   GateList::const_iterator it = _poList.begin();
   CirGate::clearMark();
   for (; it != _poList.end(); ++it)
      (*it)->traversal(l);
}

CirGate* CirMgr::addPI(int lineno, unsigned lid) {
      unsigned gid = lid / 2;
      CirGate* pi = new InputGate(gid, lineno);
      _piList.push_back(pi);
      _gates[gid] = pi;
      return pi;
   }

CirGate* CirMgr::addPO(int lineno, unsigned lid) {
   unsigned gid = _maxNum + _poList.size() + 1;
   OutputGate* po = new OutputGate(gid, lineno);
   _poList.push_back(po);
   po->_fanin[0] = lid;
   po->_faninCount = 1;
   return (_gates[gid] = po);
}

CirGate* CirMgr::addAIG(int lineno, unsigned lid, unsigned fin1, unsigned fin2) {
   unsigned gid = lid / 2;
   AigGate* aig = new AigGate(gid, lineno);
   aig->_fanin[0] = fin1;
   aig->_fanin[1] = fin2;
   aig->_faninCount = 2;
   return (_gates[gid] = aig);
}

CirGate* CirMgr::addUndef(unsigned gid) {
   UndefGate* undef = new UndefGate(gid);
   return (_gates[gid] = undef);
}

void CirMgr::initialize() {
   CirGateV pi;
   bool inv;
   CirGate* self;
   CirGate* target;

   GateMap::const_iterator it = _gates.begin();
   for (; it != _gates.end(); ++it) {
      self = (*it).second;
      // cout << "Gate #" << self->getID() << ": " << self->getTypeStr() << endl;

      for (size_t i = 0, n = self->_faninCount; i < n; i++) {
         pi = self->_fanin[i] / 2;
         inv = self->_fanin[i] % 2;
         target = getGate(pi);
         if (!target) target = addUndef((unsigned)pi);
         // cout << "  * PI: " << pi << " " << inv << " " << target->getTypeStr() << endl;
         target->addFanout((CirGateV)self | inv);
         // need not set inv because it was set along (and thus shared with) literal ID
         self->setFanin(i, target);
      }

   }
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

   try {
      lineNo = 0;
      colNo = 0;

      // ========== HEADER ==========
      state = STATE_HEADER;
      if (!consumeAagHeader(f)) {
         if (isDigit(f.peek()))
            throw MISSING_SPACE;

         // continue reading and report the full string
         getline(f, errMsg, ' ');
         errMsg = "aag" + errMsg;
         throw ILLEGAL_IDENTIFIER;
      }

      // errInt is relied here to record sub-state for error handling
      errInt = 0;
      consumeSpace(f); /* M */ _maxNum       = readUint(f); errInt++;
      consumeSpace(f); /* I */ _inputCount   = readUint(f); errInt++;
      consumeSpace(f); /* L */ _latchCount   = readUint(f); errInt++;
      consumeSpace(f); /* O */ _outputCount  = readUint(f); errInt++;
      consumeSpace(f); /* A */ _andGateCount = readUint(f);
      consumeNewline(f);

      // some stupid actions on lineNo here is just for following the ref program
      lineNo--;

      // check max num
      if (_maxNum < _inputCount + _latchCount + _andGateCount) {
         errInt = _maxNum;
         errMsg = "Num of variables";
         throw NUM_TOO_SMALL;  // no need to handle colNo here
      }

      // we CANNOT handle latches now; throw an error if containing latches
      if (_latchCount) {
         errMsg = "latches";
         throw ILLEGAL_NUM;
      }

      lineNo++;

      // const gate is safe to be created
      _gates[0] = new ConstGate();

      // ========== INPUT  ==========
      state = STATE_PI;
      for (size_t i = 0; i < _inputCount; i++) {
         unsigned lid = readUint(f);
         checkLiteralID(this, lid, true);
         addPI(lineNo+1, lid);
         // cerr << "=== PI === " << lid/2 << endl;
         consumeNewline(f);
      }

      // ========== LATCH  ========== (omitted)

      // ========== OUTPUT ==========
      state = STATE_PO;
      for (size_t i = 0; i < _outputCount; i++) {
         unsigned lid = readUint(f);
         checkLiteralID(this, lid, false, false);
         addPO(lineNo+1, lid);
         // cerr << "=== PO === " << lid/2 << " " << lid%2 << endl;
         consumeNewline(f);
      }

      // ========== AIGATE ==========
      state = STATE_AIG;
      for (size_t i = 0; i < _andGateCount; i++) {
         unsigned lhs1, lhs2, rhs;
         rhs = readUint(f);
         checkLiteralID(this, rhs, true);
         consumeSpace(f);

         lhs1 = readUint(f);
         checkLiteralID(this, lhs1, false, false);
         consumeSpace(f);

         lhs2 = readUint(f);
         checkLiteralID(this, lhs2, false, false);

         addAIG(lineNo+1, rhs, lhs1, lhs2);
         // cerr << "=== AIG === " << rhs << " " << lhs1 << " " << lhs2 << endl;
         consumeNewline(f);
      }

      // ========== SYMBOL ==========
      state = STATE_SYMBOL;

      GateList* ls;

      while (true) {
         ls = 0;

         char symbolType = readChar(f);
         switch (symbolType) {
            case 'i': ls = &_piList; break;
            case 'o': ls = &_poList; break;
            case 'c': consumeNewline(f); break;
            case -1: break; // EOF; it is just fine
            case ' ':  retreatChar(f); throw EXTRA_SPACE; break;
            case '\n': retreatChar(f); throw MISSING_IDENTIFIER; break;
            default:   retreatChar(f); errMsg = symbolType; throw ILLEGAL_SYMBOL_TYPE; break;
         }

         if (!ls) break;

         unsigned cnt;
         try {
            cnt = readUint(f);
         } catch (CirParseError err) {
            if (err == ILLEGAL_NUM) {
               // circulated with pain...
               retreatChar(f);
               string illIdx;
               getline(f, illIdx, ' ');
               errMsg = errMsg + "(" + illIdx + ")";
            }
            throw err;
         }
         consumeSpace(f);

         if (cnt >= ls->size()) {
            if (ls == &_piList) errMsg = "PI index";
            else if (ls == &_poList) errMsg = "PO index";
            errInt = cnt;
            throw NUM_TOO_BIG;
         }

         try {
            errMsg = readStr(f);
         } catch (CirParseError err) {
            if (err == ILLEGAL_WSPACE)
               throw ILLEGAL_SYMBOL_NAME;
            throw err;
         }

         errGate = (*ls)[cnt];
         if (!errGate->_name.empty()) {
            errMsg = symbolType;
            errInt = cnt;
            throw REDEF_SYMBOLIC_NAME;
         }

         errGate->_name = errMsg;
         // cout << "=== SYM === " << symbolType << " " << cnt << " " << errMsg << endl;
         consumeNewline(f);
      }

      // ========== COMMENT ========= (no need to record this)
      state = STATE_FINISHED;
   } catch (CirParseError err) {
      ok = false;
      parseError(err);
   }

   initialize();

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

   cout << endl;
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
   cout << endl;

   GateList& _dfsList = getDfsList();

   for (unsigned i = 0, n = _dfsList.size(); i < n; ++i) {
      if (!_dfsList[i]) continue;
      cout << "[" << i << "] "
           << setw(4) << left << _dfsList[i]->getTypeStr();

      _dfsList[i]->printGate();
      if (!_dfsList[i]->_name.empty())
         cout << " (" << _dfsList[i]->_name << ")";
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
      for (size_t i = 0, n = gate->_faninCount; i < n; i++)
         if (gate->getFanin(i)->_type == UNDEF_GATE) {
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
CirMgr::printFECPairs() const
{
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

   // header: aag M I L O "A", only andGate count is recalculated.
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
      outfile << (_poList[i]->getFanin(0)->getID() * 2 + (_poList[i]->getInv(0) ? 1 : 0)) << endl;

   // aig
   for (size_t i = 0, n = l.size(); i < n; i++)
      if (l[i]->isAig()) {
         outfile << (l[i]->getID() * 2) << " "
                 << (l[i]->getFanin(0)->getID() * 2 + (l[i]->getInv(0) ? 1 : 0)) << " "
                 << (l[i]->getFanin(1)->getID() * 2 + (l[i]->getInv(1) ? 1 : 0)) << endl;
      }

   // symbol
   for (size_t i = 0, n = piGen.size(); i < n; i++)
      if (!piGen[i]->_name.empty())
         outfile << 'i' << i << " " << piGen[i]->_name << endl;
   for (size_t i = 0, n = poGen.size(); i < n; i++)
      if (!poGen[i]->_name.empty())
         outfile << 'o' << i << " " << poGen[i]->_name << endl;

   // comment
   outfile << "c\n" << "generated by cirWrite command" << endl;
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
   // preprocessing
   GateList l, piGen, aigGen;
   unsigned newA = 0, gid = g->getID();
   CirGate::clearMark();
   g->traversal(&l);

   for (GateMap::const_iterator it = _gates.begin(); it != _gates.end(); ++it)
      if (it->second->isMarked())
         switch (it->second->_type) {
            case PI_GATE: piGen.push_back(it->second); break;
            case AIG_GATE: newA++; break;
            default: break;
         }

   // header: aag M "I" L "1" "A", input and andGate counts are recalculated.
   // also note that there will be only one output
   outfile << "aag "
           << _maxNum << " "
           << piGen.size() << " "
           << _latchCount << " "
           << 1 << " "
           << newA << endl;

   // input
   for (size_t i = 0, n = _piList.size(); i < n; i++)
      if (_piList[i]->isMarked())
         outfile << _piList[i]->getID() * 2 << endl;

   // output
   outfile << gid * 2 << endl;

   // aig
   for (size_t i = 0, n = l.size(); i < n; i++)
      if (l[i]->isAig()) {
         outfile << (l[i]->getID() * 2) << " "
                 << (l[i]->getFanin(0)->getID() * 2 + (l[i]->getInv(0) ? 1 : 0)) << " "
                 << (l[i]->getFanin(1)->getID() * 2 + (l[i]->getInv(1) ? 1 : 0)) << endl;
      }

   // symbol
   for (size_t i = 0, n = piGen.size(); i < n; i++)
   if (!piGen[i]->_name.empty())
      outfile << 'i' << i << " " << piGen[i]->_name << endl;
   outfile << "o0 " << gid << endl;

   // comment
   outfile << "c\n" << "generated by cirWrite command of gate " << gid << endl;
}
