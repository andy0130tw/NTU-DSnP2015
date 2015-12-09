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

extern CirMgr *cirMgr;

// TODO: Implement memeber functions for class(es) in cirGate.h
/**************************************/
/*   class CirGate member functions   */
/**************************************/
void CirGate::traversal(GateList* l = 0) {
   mark();
   for (GateList::const_iterator it = _faninList.begin(); it != _faninList.end(); ++it)
      if (!(*it)->isMarked())
         (*it)->traversal(l);

   if (l) l->push_back(this);
}

void CirGate::reportRecursive(set<unsigned>& visited, bool reverse, int limit, const CirGate* parent = 0) const {
   static unsigned level = 0;
   const GateList& next = reverse ? _faninList : _fanoutList;
   const CirGate* pred = reverse ? parent : this;
   const CirGate* succ = reverse ? this : parent;

   // print current
   cout << setw(level * 2) << "";

   // decide inv; when directly called, it is never inverted
   bool inv = false;
   if (parent && reverse) {
      for (size_t i = 0, n = pred->_faninList.size(); i < n; i++) {
         if (pred->_faninList[i] == succ) {
            inv = pred->getInv(i);
            break;
         }
      }
   }

   if (inv) cout << '!';
   cout << this->getTypeStr() << " " << this->getID();

   if (visited.find(this->getID()) != visited.end() && !next.empty() && limit != 0)
      cout << " (*)" << endl;
   else {
      cout << endl;
      if (limit == 0) return;

      visited.insert(this->getID());
      // push to next indention
      level++;
      for (size_t i = 0, n = next.size(); i < n; i++)
         next[i]->reportRecursive(visited, reverse, limit - 1, this);
      level--;
   }
}

void
CirGate::reportGate() const
{
   static const string H_DIV = "==================================================";
   stringstream ss;
   ss << getTypeStr() << "(" << _id << ")";
   if (_name.length())
      ss << "\"" << _name << "\"";
   ss << ", line " << getLineNo();

   cout << H_DIV << endl;
   cout << "= " << setw(46) << left << ss.str() << " =" << endl;
   cout << H_DIV << endl;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   set<unsigned> visited;
   reportRecursive(visited, true, level);
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   set<unsigned> visited;
   reportRecursive(visited, false, level);
}

