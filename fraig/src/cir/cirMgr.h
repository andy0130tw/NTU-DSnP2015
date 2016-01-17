/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"
#include "cirGate.h"

extern CirMgr *cirMgr;

class CirMgr
{
public:
   CirMgr(): _dfsList_clean(false), _fecGroupList(0), _fecHashMap(0) {}
   ~CirMgr() {
      for (GateMap::const_iterator it = _gates.begin(); it != _gates.end(); ++it) {
         delete it->second;
      }
   }

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const {
      GateMap::const_iterator it = _gates.find(gid);
      if (it == _gates.end()) return 0;
      return it->second;
   }

   void eraseGate(CirGate* g) {
      assert(g->_type == AIG_GATE || g->_type == UNDEF_GATE);
      _gates.erase(g->getID());
      delete g;
      _andGateCount--;
   }

   // DFS!!!
   GateList& getDfsList() const;
   void dfs(GateList* l = 0) const;

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

   CirGate* addPI(int, unsigned);
   CirGate* addPO(int, unsigned);
   CirGate* addAIG(int, unsigned, unsigned, unsigned);
   CirGate* addUndef(unsigned);

   void initialize();

   unsigned int _maxNum;
   unsigned int _inputCount;
   unsigned int _latchCount;
   unsigned int _outputCount;
   unsigned int _andGateCount;

private:
   ofstream           *_simLog;
   GateList           _piList;
   GateList           _poList;
   // GateList           _totalList;

   GateMap            _gates;
   mutable GateList   _dfsList;
   // mutable GateList   _unusedList;
   mutable bool       _dfsList_clean;

   FECGroupList*      _fecGroupList;
   HashMap<CirPatternKey, GateList*>* _fecHashMap;

   // for simulation
   void initFECGroup();
   void simulateOnFECs();
   void outputSimResult(CirSimData[], unsigned = 0);

   #ifdef CHECK_INTEGRITY
   bool checkIntegrity(bool verbose = false) const;
   #endif  // CHECK_INTEGRITY
};

#endif // CIR_MGR_H
