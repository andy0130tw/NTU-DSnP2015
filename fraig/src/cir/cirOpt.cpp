/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <cassert>
#include "cirDef.h"
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
   // Output Example:
   // Sweeping: AIG(XX) removed...
   getDfsList();

   GateMap::iterator it = _gates.begin();
   while (it != _gates.end()) {
      CirGate* g = (*it).second;
      if ((g->_type == UNDEF_GATE || g->isAig()) && !g->isMarked()) {
         cout << "Sweeping: " << g->getTypeStr() << "(" << g->getID() << ") removed..." << endl;

         // remove it from other items' fanouts
         for (size_t i = 0; i < g->_faninCount; i++)
            assert(g->getFanin(i)->eraseFanout(g));

         _gates.erase(it++);
         if (g->isAig())
            _andGateCount--;
      } else ++it;
   }

   #ifdef CHECK_INTEGRITY
   checkIntegrity();
   #endif  // CHECK_INTEGRITY
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
   // Output Example:
   // Simplifying: XX merging (!)YY...

   bool noop, structChanged = false;

   CirGate *ga, *gb;
   CirGate* gnew;
   bool ia, ib;
   bool inew;

   getDfsList();

   for (size_t i = 0, n = _dfsList.size(); i < n; i++) {
      if (!_dfsList[i]->isAig()) continue;

      noop = true;
      ga = _dfsList[i]->getFanin(0);
      gb = _dfsList[i]->getFanin(1);
      ia = _dfsList[i]->getInv(0);
      ib = _dfsList[i]->getInv(1);

      // swap to make sure that if one gate is constant,
      // gb is the non-constant one
      if (gb->_type == CONST_GATE) {
         swap(ga, gb);
         swap(ia, ib);
      }

      if ((ga->_type == CONST_GATE && !ia) || (ga == gb && ia != ib)) {
         // (0 AND X = 0) or (X AND ~X = 0)
         noop = false;
         gnew = getGate(0);
         inew = false;
      } else if ((ga->_type == CONST_GATE && ia) || (ga == gb && ia == ib)) {
         // (1 AND X = X) or (X AND X = X)
         noop = false;
         gnew = gb;
         inew = ib;
      }

      if (noop) continue;

      structChanged = true;

      // not checking for duplicating
      ga->eraseFanout(_dfsList[i]);
      gb->eraseFanout(_dfsList[i]);

      // print out the reason if available
      cout << "Simplifying: " << gnew->getID() << " merging ";
      if (inew) cout << "!";
      cout << _dfsList[i]->getID() << "...";

      #ifdef VERBOSE
      cout << " ["
           << (ia ? "!" : "") << ga->getID() << ","
           << (ib ? "!" : "") << gb->getID() << "]";
      #endif  // VERBOSE

      cout << endl;

      // (ga, gb)--(Y)-->(Zi) to (Y*)-->(Zi)
      for (size_t j = 0, n = _dfsList[i]->_fanoutList.size(); j < n; j++) {
         bool new_inv = (inew != _dfsList[i]->getFanoutInv(j));
         // add Zi to (Y*)s' fanout...
         gnew->addFanout((CirGateV) _dfsList[i]->getFanout(j) | new_inv);
         // replace (Zi)s' fanin to (Y*)... w/ phase applied
         _dfsList[i]->getFanout(j)->replaceFanin(_dfsList[i], (CirGateV)gnew | new_inv);
         // cout << "  replacing fanin of " << _dfsList[i]->getFanout(j)->getID()
         //      << " with inv=" << new_inv << endl;
      }

      eraseGate(_dfsList[i]);
   }

   #ifdef CHECK_INTEGRITY
   checkIntegrity();
   #endif  // CHECK_INTEGRITY

   if (structChanged)
      _dfsList_clean = false;
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/

#ifdef CHECK_INTEGRITY
enum IntegrityState {
   INTEGRITY_OK,
   INTEGRITY_NULLPTR,
   INTEGRITY_NOT_FOUND,
   INTEGRITY_WRONG_INV,
   INTEGRITY_BROKEN
};

static IntegrityState checkFaninIntegrity(const CirMgr* mgr, const CirGate* g, unsigned i) {
   CirGate* tmp = g->getFanin(i);
   if (!tmp) return INTEGRITY_NULLPTR;
   if (!mgr->getGate(tmp->getID())) return INTEGRITY_NOT_FOUND;
   bool found = false, found_inv = false;
   // also match the inverted one to help investigating
   for (size_t j = 0; j < tmp->_fanoutList.size(); j++) {
      if (tmp->getFanout(j) == g) {
         if (tmp->getFanoutInv(j) == g->getInv(i)) found = true;
         else found_inv = true;
         if (found) break;
      }
   }
   if (found) return INTEGRITY_OK;
   return found_inv ? INTEGRITY_WRONG_INV : INTEGRITY_BROKEN;
}

static IntegrityState checkFanoutIntegrity(const CirMgr* mgr, const CirGate* g, unsigned i) {
   CirGate* tmp = g->getFanout(i);
   if (!tmp) return INTEGRITY_NULLPTR;
   if (!mgr->getGate(tmp->getID())) return INTEGRITY_NOT_FOUND;
   bool found = false, found_inv = false;
   for (size_t j = 0; j < tmp->_faninCount; j++) {
      if (tmp->getFanin(j) == g) {
         if (tmp->getInv(j) == g->getFanoutInv(i)) found = true;
         else found_inv = true;
         if (found) break;
      }
   }
   if (found) return INTEGRITY_OK;
   return found_inv ? INTEGRITY_WRONG_INV : INTEGRITY_BROKEN;
}

// iterate over all gates and check for connection and invertion states
bool CirMgr::checkIntegrity(bool verbose) const {
   cerr << endl;
   cerr << "------ Integrity check start ------" << endl;
   bool ok = true;

   for (GateMap::const_iterator it = _gates.begin(); it != _gates.end(); ++it) {
      CirGate* g = (*it).second;

      bool gate_fail = false, gate_fail_all = true;

      cerr << "checking \033[1;01m" << g->getTypeStr() << " " << g->getID() << "\033[1;0m... ";

      // fanin
      for (size_t i = 0; i < g->_faninCount; i++) {
         if (i) cerr << " / ";
         IntegrityState result = checkFaninIntegrity(this, g, i);
         if (result != INTEGRITY_OK)
            ok = false;

         if (result == INTEGRITY_NULLPTR)
            cerr << ": \033[1;31mNULL!!";
         else {
            cerr << "\033[1;34m"
                 << (g->getInv(i) ? "!" : "")
                 << g->getFanin(i)->getID() << "\033[1;0m: ";
            if (result == INTEGRITY_OK) {
               cerr << "ok";
               gate_fail_all = false;
            } else {
               gate_fail = true;
               cerr << "\033[1;31m";
               if (result == INTEGRITY_NOT_FOUND)
                  cerr << "DNE!!";
               else if (result == INTEGRITY_BROKEN)
                  cerr << "NOFOUT!!";
               else if (result == INTEGRITY_WRONG_INV)
                  cerr << "WINV!!";
            }
         }
         cerr << "\033[1;0m";
      }

      // fanin count
      unsigned expectFaninCount = 0;
      switch (g->_type) {
         case PO_GATE:  expectFaninCount = 1; break;
         case AIG_GATE: expectFaninCount = 2; break;
         default: break;
      }
      if (g->_faninCount != expectFaninCount) {
         cerr << " -- \033[1;31mhaving "
              << g->_faninCount << " fanin, "
              << expectFaninCount << "expected\033[1;0m";
         ok = false;
      }
      cerr << endl;

      // fanout
      for (size_t i = 0; i < g->_fanoutList.size(); i++) {
         IntegrityState result = checkFanoutIntegrity(this, g, i);
         if (result != INTEGRITY_OK)
            ok = false;

         if (i && i % 4 == 0) cerr << endl;
         cerr << "    ";

         cerr << "#" << setw(2) << i;
         if (result == INTEGRITY_NULLPTR)
            cerr << ": \033[1;31mNULL!!";
         else {
            cerr << "(\033[1;36m"
                 << (g->getFanoutInv(i) ? "!" : "")
                 << g->getFanout(i)->getID() << "\033[1;0m): ";
            if (result == INTEGRITY_OK) {
               cerr << "ok";
               gate_fail_all = false;
            } else {
               gate_fail = true;
               cerr << "\033[1;31m";
               if (result == INTEGRITY_NOT_FOUND)
                  cerr << "DNE!!";
               else if (result == INTEGRITY_BROKEN)
                  cerr << "NOFIN!!";
               else if (result == INTEGRITY_WRONG_INV)
                  cerr << "WINV!!";
            }
         }
         cerr << "\033[1;0m";
      }
      if (!g->_fanoutList.empty()) cerr << endl;

      // floating check; it can be directly removed so just give it a warning
      if (g->_type == AIG_GATE && gate_fail && gate_fail_all) {
         cerr << "\033[1;93mnot ok, but recognized as floating...\033[1;0m" << endl;
      }
   }
   if (ok)
      cerr << "\033[1;32m------ Integrity check SUCCESS ------\033[1;0m" << endl;
   else
      cerr << "\033[1;31m------ Integrity check FAILED!! ------\033[1;0m" << endl;
   cerr << endl;

   return ok;
}
#endif  // CHECK_INTEGRITY
