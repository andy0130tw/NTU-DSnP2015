/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <algorithm>
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
#if defined(VERBOSE) && defined(HASHMAP_DEBUG)
static void printFaninPair(CirGate* g) {
   cout << (g->getInv(0) ? "!" : "") << g->getFanin(0)->getID() << ","
        << (g->getInv(1) ? "!" : "") << g->getFanin(1)->getID();
}
#endif  // VERBOSE && HASHMAP_DEBUG

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

   #if defined(VERBOSE) && defined(HASHMAP_DEBUG)
   unsigned foundCount = 0;
   #endif  // VERBOSE && HASHMAP_DEBUG

   for (size_t i = 0; i < dfsSize; i++) {
      if (!_dfsList[i]->isAig()) continue;
      CirStrashKey k(_dfsList[i]);
      CirGate* t = 0;

      if (hashStrash.check(k, t)) {

         // remove old gate from fanin's fanout
         t->getFanin(0)->eraseFanout(_dfsList[i]);
         t->getFanin(1)->eraseFanout(_dfsList[i]);

         mergeGate(t, _dfsList[i]);

         cout << "Strashing: " << t->getID() << " merging " << _dfsList[i]->getID() << "...";

         #if defined(VERBOSE) && defined(HASHMAP_DEBUG)
         cout << " hash " << _dfsList[i]->getID() << " [";
         printFaninPair(_dfsList[i]);
         cout << "] (" << k() << ") == " << t->getID() << " [";
         printFaninPair(t);
         cout << "]";
         foundCount++;
         #endif  // VERBOSE && HASHMAP_DEBUG

         cout << endl;

         eraseGate(_dfsList[i]);
      } else {
         hashStrash.forceInsert(k, _dfsList[i]);
      }
   }

   #ifdef CHECK_INTEGRITY
   checkIntegrity();
   #endif  // CHECK_INTEGRITY

   #if defined(VERBOSE) && defined(HASHMAP_DEBUG)
   #include<iomanip>
   cout << endl << right
        << "hash coll count = " << setw(6) << hashStrash.getCollCount() << endl
        << "hash hit  count = " << setw(6) << foundCount << endl
        << "hash miss count = " << setw(6) << (hashStrash.getCollCount() - foundCount) << endl;
   #endif  // VERBOSE && HASHMAP_DEBUG
}

void CirMgr::genProofModel(SatSolver& s) {
   GateList& l = getDfsList();

   for (size_t i = 0, n = l.size(); i < n; i++) {
      l[i]->dfsListIdx = i;
      l[i]->_satVar = s.newVar();
      if (l[i]->isAig()) {
         s.addAigCNF(
            l[i]->_satVar,
            l[i]->getFanin(0)->_satVar, l[i]->getInv(0),
            l[i]->getFanin(1)->_satVar, l[i]->getInv(1)
         );
      }
   }

   // constraint for the constant gate
   s.assertProperty(_gates[0]->_satVar, false);
}

static bool fecGroupListCompFN(GateList* a, GateList* b) {
   GateList& la = *a;
   GateList& lb = *b;
   // so naive
   size_t ia = (la[0]->getID() == 0 ? 1 : 0);
   size_t ib = (lb[0]->getID() == 0 ? 1 : 0);
   return (la[ia]->dfsListIdx < lb[ib]->dfsListIdx);
}

// use given SAT engine to prove (phase ? x == !y : x == y)
// returns if the assumption is satisifiable
// if y != 0, prove (x, y) pair
// if y == 0, prove x against const
static bool satProve(SatSolver& s, CirGate* x, CirGate* y, bool phase) {
   assert(x != 0);

   s.assumeRelease();
   if (y) {
      Var out = s.newVar();
      s.addXorCNF(out, x->_satVar, false, y->_satVar, phase);
      s.assumeProperty(out, true);
   } else {
      s.assumeProperty(x->_satVar, phase);
   }
   return s.assumpSolve();
}

void
CirMgr::fraig()
{
   assert(_fecGroupList);

   if (!_satSolver) {
      _satSolver = new SatSolver();
      _satSolver->initialize();
      genProofModel(*_satSolver);
   }

   SatSolver& s = *_satSolver;

   // size_t piSize = _piList.size();
   // CirSimData cexPool[piSize];
   // unsigned cexCount = 0;
   // CirSimData bit = SIM_HIGHEST_BIT;

   CirGate* constGate = _gates[0];
   GateList* constGroup = constGate->_fecGroup;

   sort(_fecGroupList->begin(), _fecGroupList->end(), fecGroupListCompFN);

   for (size_t i = 0, n = _fecGroupList->size(); i < n; i++) {
      GateList* gl = _fecGroupList->at(i);
      cout << "FEC Group #" << i << " / " << (n - i) << ", len = " << gl->size() << " ------" << endl;

      bool halt = false;

      for (size_t x = 0, m = gl->size(); x < m; x++) {
         CirGate* gx = gl->at(x);
         if (!gx) continue;
         if (gl == constGroup) {
            // constant SAT
            if (gx == constGate) continue;
            bool cond = (gx->getSimData() == 0);
            cout << "  Proving " << (gx->getID()) << " = " << (cond ? 1 : 0) << "... ";

            bool result = satProve(s, gx, 0, cond);

            cout << (result ? "SAT" : "UNSAT") << "!!" << endl;
            if (result) {
               // SAT
               if (m > 2) {
                  // use CEX to separate this list!
                  // because it only diverges to 0 and 1
                  // two vectors are just enough
                  // vector<CirGate*> *vKeep = new vector<CirGate*>, *vSeparate = new vector<CirGate*>;

                  // CirSimData refSimData = gl->at(0)->getSimData();
                  // bool refVal = s.getValue(gl->at(0)->_satVar);

                  // for (size_t i = 1; i < m; i++) {
                  //    if (!gl->at(i)) continue;
                  //    bool val = s.getValue(gl->at(i)->_satVar);
                  //    bool valExpect = (gl->at(i)->getSimData() == refSimData);
                  //    bool valSame = (val == refVal);
                  //    // cout << "  " << gl->at(i)->getID() << ": " << val;
                  //    // cout << ", " << (valExpect == valSame ? "same" : "different") << endl;

                  //    if (valExpect == valSame)
                  //       vKeep->push_back(gl->at(i));
                  //    else
                  //       vSeparate->push_back(gl->at(i));
                  // }

                  // if (vKeep->size() >= 2) {
                  //    _fecGroupList->push_back(vKeep);
                  //    n++;
                  // }
                  // if (vSeparate->size() >= 2) {
                  //    _fecGroupList->push_back(vSeparate);
                  //    n++;
                  // }
               }
            } else {
               // UNSAT
               gx->getFanin(0)->eraseFanout(gx);
               gx->getFanin(1)->eraseFanout(gx);

               cout << "Fraig: " << (cond ? "" : "!") << "0 merging " << gx->getID() << endl;

               // CHECK: is the phase correct?
               for (size_t i = 0, n = gx->_fanoutList.size(); i < n; i++) {
                  gx->getFanout(i)->replaceFanin(gx, (CirGateV)constGate | cond);
                  constGate->addFanout((CirGateV)gx->getFanout(i) | cond);
               }
               eraseGate(gx);
            }
            gl->at(x) = 0;
         } else {
            for (size_t y = x + 1; y < m; y++) {
               CirGate* gy = gl->at(y);
               if (!gy) continue;

               bool inv = (gx->getSimData() != gy->getSimData());
               cout << "  Proving (" << (gx->getID()) << ", " << (inv ? "!" : "") << (gy->getID()) << ")... ";
               bool result = satProve(s, gx, gy, inv);

               cout << (result ? "SAT" : "UNSAT") << "!!" << endl;
               if (result) {
                  // SAT
                  // bool failed = false;
                  // for (size_t i = 0, n = piSize; i < n; ++i) {
                  //    int val = s.getValue(_piList[i]->_satVar);
                  //    if (val == 0) {
                  //       cexPool[i] &= ~bit;
                  //    } else if (val == 1) {
                  //       cexPool[i] |= bit;
                  //    } else {
                  //       failed = true;
                  //       break;
                  //    }
                  // }
                  // if (!failed) {
                  //    bit >>= 1;
                  //    cexCount++;
                  //    cout << "!! CEX count = " << cexCount << endl;
                  // }
                  // if (!bit) {
                  //    halt = true;
                  //    break;
                  // }
                  if (m > 2) {
                     // use CEX to separate this list!
                     // because it only diverges to 0 and 1
                     // two vectors are just enough
                     vector<CirGate*> *vKeep = new vector<CirGate*>, *vSeparate = new vector<CirGate*>;

                     CirSimData refSimData = gl->at(0)->getSimData();
                     bool refVal = s.getValue(gl->at(0)->_satVar);

                     for (size_t i = 1; i < m; i++) {
                        if (!gl->at(i)) continue;
                        bool val = s.getValue(gl->at(i)->_satVar);
                        bool valExpect = (gl->at(i)->getSimData() == refSimData);
                        bool valSame = (val == refVal);
                        // cout << "  " << gl->at(i)->getID() << ": " << val;
                        // cout << ", " << (valExpect == valSame ? "same" : "different") << endl;

                        if (valExpect == valSame)
                           vKeep->push_back(gl->at(i));
                        else
                           vSeparate->push_back(gl->at(i));
                     }

                     if (vKeep->size() >= 2) {
                        _fecGroupList->push_back(vKeep);
                        n++;
                     }
                     if (vSeparate->size() >= 2) {
                        _fecGroupList->push_back(vSeparate);
                        n++;
                     }
                  }
                  halt = true;
                  break;
               } else {
                  // UNSAT
                  // discard this gate; may produce floating gates
                  gy->getFanin(0)->eraseFanout(gy);
                  gy->getFanin(1)->eraseFanout(gy);

                  mergeGate(gx, gy);
                  cout << "Fraig: " << gx->getID() << " merging " << (inv ? "!" : "") << gy->getID() << endl;
                  eraseGate(gy);
                  gl->at(y) = 0;
               }
            }
            if (halt) break;
         }
      }

      // if (!bit) {
      //    // vacuum
      //    for (size_t u = 0; u < _fecGroupList->size(); u++) {
      //       size_t cnt = 0;
      //       GateList* sweepList = _fecGroupList->at(u);
      //       for (size_t r = 0, z = sweepList->size(); r < z; r++) {
      //          if (sweepList->at(r))
      //             sweepList->at(cnt++) = sweepList->at(r);
      //       }
      //       cout << "len = " << cnt << endl;
      //       if (_fecGroupList->at(u)->size() < 2) {
      //          for (size_t r = 0, z = sweepList->size(); r < z; r++) {
      //             sweepList->at(r)->_fecGroup = 0;
      //          }
      //          _fecGroupList->at(u) = _fecGroupList->back();
      //          _fecGroupList->pop_back();
      //          n--;
      //       } else {
      //          sweepList->resize(cnt);
      //       }
      //    }

      //    for (size_t u = 0; u < piSize; u++)
      //       _piList[u]->setSimData(cexPool[u]);
      //    simulateCircuit();
      //    manipulateFECs();
      //    printFECPairs();

      //    sort(_fecGroupList->begin(), _fecGroupList->end(), fecGroupListCompFN);
      //    n = _fecGroupList->size();
      //    constGroup = constGate->_fecGroup;
      //    i = 0;
      //    bit = SIM_HIGHEST_BIT;
      // }
   }



   // clean up floating gates
   sweep();

   // invalidate all FEC group lists
   for (GateMap::iterator it = _gates.begin(); it != _gates.end(); ++it)
      (*it).second->_fecGroup = 0;
   for (size_t i = 0, n = _fecGroupList->size(); i < n; i++)
      delete _fecGroupList->at(i);
   delete _fecGroupList;
   _fecGroupList = 0;
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
// merge `mergeFrom` to `mergeTo`
// after this operation, src become floating
// but simply calling this method is not enough
// fanins of `mergeTo` need to be purged(?)
void CirMgr::mergeGate(CirGate* mergeTo, CirGate* mergeFrom) {
   _dfsList_clean = false;

   for (size_t i = 0, n = mergeFrom->_fanoutList.size(); i < n; i++) {
      mergeFrom->getFanout(i)->replaceFanin(mergeFrom, (CirGateV)mergeTo | mergeFrom->getFanoutInv(i));
      mergeTo->addFanout(mergeFrom->_fanoutList[i]);
   }
}
