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
   ss << "FECs: " << "[Not Implemented...]";
   cout << "= " << setw(46) << left << ss.str() << " =" << endl;

   ss.str("");
   ss << "Values: " << "[Not Implemented...]";
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
