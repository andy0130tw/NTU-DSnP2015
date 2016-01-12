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

#define VERBOSE

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

   HashMap<CirStrashKey, CirGate*> hashStrash(dfsSize * 5 / 3);

   #ifdef VERBOSE
   unsigned foundCount = 0;
   #endif  // VERBOSE

   for (size_t i = 0; i < dfsSize; i++) {
      if (!_dfsList[i]->isAig()) continue;
      CirStrashKey k(_dfsList[i]);
      CirGate* prev = 0;

      if (hashStrash.check(k, prev)) {
         #ifdef VERBOSE
         cout << "strashing " << _dfsList[i]->getID() << " [";
         printFaninPair(_dfsList[i]);
         cout << "] (" << k() << ") == ";
         // match inversely
         size_t which = (_dfsList[i]->getFanin(0) == prev->getFanin(0) ? 0 : 1);
         if (_dfsList[i]->getInv(0) != prev->getInv(which))
            cout << "!";
         cout << prev->getID() << " [";
         printFaninPair(prev);
         cout << "]" << endl;
         foundCount++;
         #endif  // VERBOSE

         // TODO: merge
      } else {
         hashStrash.insert(k, _dfsList[i]);

      }
   }

   #ifdef VERBOSE
   #include<iomanip>
   cout << right;
   cout << "  hash coll count = " << setw(6) << hashStrash.getCollCount() << endl;
   cout << "- hash hit  count = " << setw(6) << foundCount << endl;
   cout << "= hash miss count = " << setw(6) << (hashStrash.getCollCount() - foundCount) << endl;
   #endif  // VERBOSE
}

void
CirMgr::fraig()
{
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
