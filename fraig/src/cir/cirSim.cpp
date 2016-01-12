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
// static inline void fancyIO(bool x) {
//    cout << (x ? "\033[01m\033[01m1\033[0m" : "\033[90m\033[02m0\033[0m");
// }

// static void printSimData(CirSimData& s) {
//    for (CirSimData i = SIM_HIGHEST_BIT; i > 0; i >>= 1) {
//       fancyIO(s & i);
//    }
// }

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
   GateList& l = getDfsList();
   size_t ni = _piList.size(), no = _poList.size();
   CirSimData simi[ni], simo[no];

   // assigning input
   for (size_t i = 0; i < ni; i++) {
      simi[i] = ((CirSimData)rnGen(1 << 16) << 16 | rnGen(1 << 16));
      _piList[i]->setSimData(simi[i]);
      // cout << "assign PI  " << setw(3) << _piList[i]->getID() << " = ";
      // printSimData(simi[i]);
      // cout << endl;
   }

   // simulate along the list
   for (size_t i = 0; i < l.size(); i++) {
      if (!l[i]->isAig()) continue;
      l[i]->simulate();
      // CirSimData val = l[i]->getSimData();
      // cout << "sim    AIG " << setw(3) << l[i]->getID() << " = ";
      // printSimData(val);
      // cout << endl;
   }

   // fetching simulating result
   for (size_t i = 0; i < no; i++) {
      _poList[i]->simulate();
      simo[i] = _poList[i]->getSimData();
      // cout << "result PO  " << setw(3) << _poList[i]->getID() << " = ";
      // printSimData(simo[i]);
      // cout << endl;
   }

   // generate log
   #ifndef VERBOSE
   if (!_simLog) return;
   #endif  // VERBOSE

   ostream& simout = _simLog ? *_simLog : cout;
   for (CirSimData b = SIM_HIGHEST_BIT; b > 0; b >>= 1) {
      for (size_t i = 0; i < ni; i++)
         simout << (bool)(simi[i] & b);
      simout << " ";
      for (size_t i = 0; i < no; i++)
         simout << (bool)(simo[i] & b);
      simout << endl;
   }
}

void
CirMgr::fileSim(ifstream& patternFile)
{
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
