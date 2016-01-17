/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

unsigned CirGate::_global_ref = 0;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void CirGate::replaceFanin(CirGate* a, CirGateV b) {
   for (size_t i = 0; i < _faninCount; i++) {
      if (getFanin(i) == a) {
         setFanin(i, b);
      }
   }
}

bool CirGate::eraseFanout(CirGate* f) {
   for (GateVList::iterator it = _fanoutList.begin(); it != _fanoutList.end(); ++it) {
      if (!(((*it) ^ (CirGateV)f) >> 1)) {
         _fanoutList.erase(it);
         return true;
      }
   }
   return false;
}

void CirGate::traversal(GateList* l = 0) const {
   mark();
   CirGate* fin;

   for (size_t i = 0; i < _faninCount; i++) {
      fin = getFanin(i);
      if (!fin->isMarked())
         fin->traversal(l);
   }

   // FIXME
   if (l) l->push_back((CirGate*)this);
}

void
CirGate::reportGate() const
{
   static const string H_DIV = "==================================================";
   stringstream ss;

   cout << H_DIV << endl;

   ss << getTypeStr() << "(" << _id << ")";
   if (_name.length())
      ss << "\"" << _name << "\"";
   ss << ", line " << getLineNo();
   cout << "= " << setw(46) << left << ss.str() << " =" << endl;

   ss.str("");
   ss << "FECs: ";
   if (_fecGroup) {
      CirSimData ref = getSimData();
      for (size_t i = 0; i < _fecGroup->size(); i++) {
         CirGate* g = _fecGroup->at(i);
         if (g == this) continue;
         if (g->getSimData() != ref)
            ss << "!";
         ss << g->getID() << " ";
      }
   }
   cout << "= " << setw(46) << left << ss.str() << " =" << endl;

   ss.str("");
   ss << "Value: ";
   for (CirSimData i = SIM_HIGHEST_BIT, n = 0; i > 0; i >>= 1, n++) {
      if (n && n % 4 == 0) ss << "_";
      ss << (_sim_data & i ? 1 : 0);
   }
   cout << "= " << setw(46) << left << ss.str() << " =" << endl;

   cout << H_DIV << endl;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   clearMark();
   reportFaninRecursive(level);
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   clearMark();
   reportFanoutRecursive(level);
}

void CirGate::reportFaninRecursive(int limit) const {
   static unsigned depth = 0;

   // print current
   cout << this->getTypeStr() << " " << this->getID();

   if (isMarked() && _faninCount > 0 && limit != 0) {
      cout << " (*)" << endl;
      return;
   }

   mark();
   cout << endl;
   if (limit == 0) return;
   for (size_t i = 0; i < _faninCount; i++) {
      depth++;
      cout << setw(depth * 2) << "";
      if (getInv(i)) cout << "!";
      getFanin(i)->reportFaninRecursive(limit - 1);
      depth--;
   }
}

void CirGate::reportFanoutRecursive(int limit) const {
   static unsigned depth = 0;

   // print current
   cout << this->getTypeStr() << " " << this->getID();

   if (isMarked() && !_fanoutList.empty() && limit != 0) {
      cout << " (*)" << endl;
      return;
   }

   mark();
   cout << endl;

   if (limit == 0) return;
   for (size_t i = 0, n = _fanoutList.size(); i < n; i++) {
      depth++;
      cout << setw(depth * 2) << "";
      if (getFanoutInv(i)) cout << "!";
      getFanout(i)->reportFanoutRecursive(limit - 1);
      depth--;
   }
}
