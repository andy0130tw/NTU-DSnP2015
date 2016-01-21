/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <cmath>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
// we often need the size ... and nothing else -w-
static size_t _tmpDfsListSize = 0;


static inline void fancyIO(bool x) {
   cout << (x ? "\033[01m\033[01m1\033[0m" : "\033[90m\033[02m0\033[0m");
}

static void printSimData(const CirSimData s) {
   for (CirSimData i = SIM_HIGHEST_BIT; i > 0; i >>= 1) {
      cout << (s & i ? 1 : 0); //fancyIO(s & i);
   }
}

// clear FEC pointer if list is stuck at original one
// because the list is to destoryed
static void clearCurrentFECPtr(GateList* ls) {
   for (size_t i = 0, n = ls->size(); i < n; i++) {
      CirGate* g = ls->at(i);
      if (g->_fecGroup == ls)
         g->_fecGroup = 0;
   }
}
static CirSimData randomSimData() {
   // rnGen can only produce 31 bits random number
   // so we try to fill it by generating 16 bits repeatly
   CirSimData ret(0), b = SIM_HIGHEST_BIT >> 15;
   while (b) {
      ret |= ((CirSimData)rnGen(1 << 16)) * b;
      b >>= 16;
   }
   return ret;
}

// check if their simulation is all the same
// because creating lists is expensive
static bool isFECGroupChanged(GateList* ls) {
   CirSimData ref = ls->at(0)->getSimData();
   for (size_t i = 1, n = ls->size(); i < n; i++) {
      CirSimData comp = ls->at(i)->getSimData();
      if (ref != comp && ref != ~comp)
         return true;
   }
   return false;
}

// tmpPtr is a gate in disguise; create a new GateList for it
static GateList* getRealFec(size_t tmp, CirGate* el) {
   GateList* realFec = new GateList(2);
   realFec->at(0) = (CirGate*)(tmp ^ 1);
   realFec->at(1) = el;

   // updating FEC pointer of the original gate
   realFec->at(0)->_fecGroup = realFec;
   return realFec;
};

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
   GateList& l = getDfsList();
   _tmpDfsListSize = l.size();

   size_t piSize = _piList.size();
   CirSimData simi[piSize];

   unsigned simulatedCount = 0;
   unsigned previousFEC = 0;
   unsigned failedCount = 0;
   unsigned const maxFail = (unsigned)(3 + log(_tmpDfsListSize) * 20);

   cout << "MAX_FAILS = " << maxFail << endl;

   while (failedCount < maxFail) {

      // assigning input
      for (size_t i = 0; i < piSize; i++) {
         simi[i] = randomSimData();
         _piList[i]->setSimData(simi[i]);
      }

      simulateCircuit();
      manipulateFECs();

      size_t fecCnt = _fecGroupList->size();
      if (previousFEC == fecCnt)
         failedCount++;
      previousFEC = fecCnt;

      // generate log
      outputSimResult(simi);
      simulatedCount += SIM_BITS;

      cout << "\rTotal #FEC Group: " << fecCnt
           << " | Simulated = " << simulatedCount
           << " | CURR_FAILS = " << failedCount  << flush;
   }

   cout << endl << simulatedCount << " patterns simulated." << endl;
}

void
CirMgr::fileSim(ifstream& patternFile)
{
   size_t piSize = _piList.size();
   unsigned readCount = 0, simulatedCount = 0, usedBits = 0;
   bool stopRead = false;
   string strBuf;
   CirSimData bit = SIM_HIGHEST_BIT;
   CirSimData inputBuf[piSize];

   while (true) {
      patternFile >> strBuf;
      size_t len = strBuf.length();
      if (len > 0 && len != piSize) {
         cerr << "Error: Pattern(" << strBuf << ") length(" << len
            << ") does not match the number of inputs(" << piSize
            << ") in a circuit!!" << endl;
         break;
      }

      if (!patternFile.fail()) {
         for (size_t i = 0; i < piSize; i++) {
            if (strBuf[i] == '0') {
               inputBuf[i] &= ~bit;
            } else if (strBuf[i] == '1') {
               inputBuf[i] |= bit;
            } else {
               cerr << "Error: Pattern(" << strBuf << ") contains a non-0/1 character('"
                    << strBuf[i] << "')." << endl;
               stopRead = true;
               break;
            }
         }
         if (stopRead) break;
         bit >>= 1;
         readCount++;
      } else {
         // clear remaining bits to 0
         for (size_t i = 0; i < piSize; i++)
            inputBuf[i] &= ~((bit << 1) - 1);
         // count remaining bits; clear it eventually
         while (bit <<= 1)
            usedBits++;
         // but if no bits are used, break directly
         if (!usedBits) break;
      }

      if (!bit) {
         for (size_t i = 0; i < piSize; i++)
            _piList[i]->setSimData(inputBuf[i]);

         simulateCircuit();
         manipulateFECs();

         // `usedBits == 0` means unlimited
         outputSimResult(inputBuf, usedBits);

         simulatedCount += readCount;

         size_t n = _fecGroupList->size();
         cout << "\rTotal #FEC Group: " << n
              << " | simulated = " << simulatedCount << flush;

         readCount = 0;

         if (patternFile.fail())
            break;

         bit = SIM_HIGHEST_BIT;
      }
   }

   cout << endl << simulatedCount << " patterns simulated." << endl;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void CirMgr::simulateCircuit() {
   GateList& l = getDfsList();
   _tmpDfsListSize = l.size();

   for (size_t i = 0; i < _tmpDfsListSize; i++)
      l[i]->simulate();
}

void CirMgr::initFECGroup() {
   // init all gates to be one group
   delete _fecGroupList;
   _fecGroupList = new FECGroupList();

   GateList* ls = new GateList();
   _fecGroupList->push_back(ls);
   ls->reserve(_tmpDfsListSize);

   // must in ascending order
   for (GateMap::const_iterator it = _gates.begin(); it != _gates.end(); ++it) {
      CirGate* gate = (*it).second;
      if (!gate->isMarked() || !gate->isAig()) continue;
      ls->push_back(gate);
      gate->_fecGroup = ls;
   }
}

// FIXME
// The verdict of each FEC group can be collected:
//   1) U - unchanged: the patterns are all the same
//   2) D - diverge:   some new FEC groups formed
//   3) S - shrink:    no new FEC groups formed
//
// return if this operation is considered as "successful"
// used to control the effort in random-simulations
void CirMgr::manipulateFECs() {
   if (!_fecGroupList) initFECGroup();

   HashMap<CirPatternKey, GateList*> patHash(getHashSize(10 + _tmpDfsListSize / 100));

   // pre-process "stuck-at-const" gates; constant gate always have a 0 key
   CirGate* constGate = _gates[0];
   if (!constGate->_fecGroup) {
      // must use revision 0 to only process before the first group
      CirPatternKey kConst(0, 0);
      patHash.insert(kConst, (GateList*)((size_t)constGate | 1));
   }

   unsigned revNum = 0;

   for (size_t i = 0, n = _fecGroupList->size(); i < n; i++) {
      #ifdef VERBOSE
      cout << "====== FEC pairs #" << i << " ======" << endl;
      #endif  // VERBOSE

      GateList* currList = _fecGroupList->at(i);
      GateList* replacingList = 0;

      if (constGate->_fecGroup && !isFECGroupChanged(currList)) continue;

      for (size_t j = 0, m = currList->size(); j < m; j++) {
         CirGate* g = currList->at(j);
         CirSimData result = g->getSimData();
         // reusing hash map... different revisions are not confused
         CirPatternKey k(result, revNum);

         #ifdef VERBOSE
         cout << "  Gate #" << setw(4) << j << "(" << setw(4) << g->getID() << "): ";
         printSimData(result);
         cout << "... ";
         #endif  // VERBOSE

         GateList* tmpPtr;
         if (patHash.check(k, tmpPtr)) {
            if ((size_t)tmpPtr & 1) {
               tmpPtr = getRealFec((size_t) tmpPtr, g);
               patHash.replaceInsert(k, tmpPtr);

               #ifdef VERBOSE
               cout << "matched \033[1;34m" << tmpPtr->at(0)->getID() << "\033[1;0m to pair";
               if (!replacingList)
                  cout << " \033[1;01m(replacing)\033[0m";
               cout << endl;
               #endif  // VERBOSE

               if (replacingList)
                  _fecGroupList->push_back(tmpPtr);
               else
                  replacingList = tmpPtr;
            } else {
               // tmpPtr is really a GateList
               tmpPtr->push_back(g);

               #ifdef VERBOSE
               cout << "matched \033[1;32m" << tmpPtr->at(0)->getID() << "\033[1;0m again" << endl;
               #endif  // VERBOSE
            }

            // update FEC group
            g->_fecGroup = tmpPtr;
         } else {
            #ifdef VERBOSE
            cout << "\033[1;02mno\033[1;0m" << endl;
            #endif  // VERBOSE
            patHash.forceInsert(k, (GateList*)((size_t)g | 1));
         }
      }

      // update rev num; note `i` may be changed, so it cannot be the revision!!!
      // (debugged for > 1e6 sec and realized)
      revNum++;

      // recycle singletons
      clearCurrentFECPtr(currList);

      // discard old FEC list
      if (replacingList) {
         #ifdef VERBOSE
         cout << "Result: use FEC " << replacingList->at(0)->getID() << " to replace" << endl;
         #endif  // VERBOSE

         _fecGroupList->at(i) = replacingList;
      } else {
         #ifdef VERBOSE
         cout << "Result: \033[1;33mshrink...\033[1;0m move #" << (n-1) << "("
              << _fecGroupList->back()->at(0)->getID() << ") here" << endl;
         #endif  // VERBOSE

         // if no FEC is discovered,
         // move the last one item here to prevent O(n) erase
         _fecGroupList->at(i) = _fecGroupList->at(n - 1);
         _fecGroupList->at(n - 1) = _fecGroupList->back();
         _fecGroupList->pop_back();

         // update total; keep the index intact
         n--; i--;
      }
      delete currList;
   }
}

// output the result to file if specified in `-f` option.
// note that in verbose mode, if no file is specified,
// the result will still be printed on screen.
void CirMgr::outputSimResult(CirSimData input[], unsigned len) {
   #ifndef VERBOSE
   if (!_simLog) return;
   #endif  // VERBOSE

   ostream& simout = _simLog ? *_simLog : cout;
   unsigned cnt = 0;
   size_t poSize = _poList.size();
   CirSimData output[poSize];

   // fetching simulating result
   for (size_t i = 0; i < poSize; i++) {
      _poList[i]->simulate();
      output[i] = _poList[i]->getSimData();
   }

   for (CirSimData b = SIM_HIGHEST_BIT; b > 0; b >>= 1) {
      for (size_t i = 0, n = _piList.size(); i < n; i++)
         simout << (bool)(input[i] & b);
      simout << " ";

      for (size_t i = 0; i < poSize; i++)
         simout << (bool)(output[i] & b);
      simout << endl;

      cnt++;
      if (len && cnt == len) break;
   }
}
