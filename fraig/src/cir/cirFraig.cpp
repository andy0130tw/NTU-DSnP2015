/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "cirFraig.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
#ifdef VERBOSE
static void printFaninPair(CirGate* g) {
   cout << (g->getInv(0) ? "!" : "") << g->getFanin(0)->getID() << ","
        << (g->getInv(1) ? "!" : "") << g->getFanin(1)->getID();
}
#endif  // VERBOSE

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
   getDfsList();

   size_t dfsSize = _dfsList.size();

   HashMap<CirStrashKey, CirGate*> hashStrash(getHashSize(dfsSize * 5 / 3));

   #ifdef VERBOSE
   unsigned foundCount = 0;
   #endif  // VERBOSE

   for (size_t i = 0; i < dfsSize; i++) {
      if (!_dfsList[i]->isAig()) continue;
      CirStrashKey k(_dfsList[i]);
      CirGate* t = 0;

      if (hashStrash.check(k, t)) {

         _dfsList_clean = false;

         // remove old gate from fanin's fanout
         t->getFanin(0)->eraseFanout(_dfsList[i]);
         t->getFanin(1)->eraseFanout(_dfsList[i]);

         // merge
         for (size_t j = 0, n = _dfsList[i]->_fanoutList.size(); j < n; j++) {
            _dfsList[i]->getFanout(j)->replaceFanin(_dfsList[i], (size_t)t | _dfsList[i]->getFanoutInv(j));
            t->addFanout(_dfsList[i]->_fanoutList[j]);
         }

         cout << "Strashing: " << t->getID() << " merging " << _dfsList[i]->getID() << "...";
         #ifdef VERBOSE
         cout << " hash " << _dfsList[i]->getID() << " [";
         printFaninPair(_dfsList[i]);
         cout << "] (" << k() << ") == " << t->getID() << " [";
         printFaninPair(t);
         cout << "]";
         foundCount++;
         #endif  // VERBOSE
         cout << endl;

         eraseGate(_dfsList[i]);
      } else {
         hashStrash.forceInsert(k, _dfsList[i]);
      }
   }

   #ifdef CHECK_INTEGRITY
   checkIntegrity();
   #endif  // CHECK_INTEGRITY

   #ifdef VERBOSE
   #include<iomanip>
   cout << endl << right
        << "hash coll count = " << setw(6) << hashStrash.getCollCount() << endl
        << "hash hit  count = " << setw(6) << foundCount << endl
        << "hash miss count = " << setw(6) << (hashStrash.getCollCount() - foundCount) << endl;
   #endif  // VERBOSE
}

void
CirMgr::fraig()
{
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
