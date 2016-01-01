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
#include <map>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"
#include "cirGate.h"

extern CirMgr *cirMgr;

typedef map<unsigned, CirGate*> GateMap;

class CirMgr
{
public:
   CirMgr() {}
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

   // DFS!!!
   void dfs(GateList*) const;

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

   unsigned int _maxNum;
   unsigned int _inputCount;
   unsigned int _latchCount;
   unsigned int _outputCount;
   unsigned int _andGateCount;

   void initialize() {
      CirGateV pi;
      bool inv;
      CirGate* self;
      CirGate* target;

      GateMap::const_iterator it = _gates.begin();
      for (; it != _gates.end(); ++it) {
         self = (*it).second;
         // cout << "Gate #" << self->getID() << ": " << self->getTypeStr() << endl;

         for (size_t i = 0, n = self->_faninCount; i < n; i++) {
            pi = self->_fanin[i] / 2;
            inv = self->_fanin[i] % 2;
            target = getGate(pi);
            if (!target) target = addUndef((unsigned)pi);
            // cout << "  * PI: " << pi << " " << inv << " " << target->getTypeStr() << endl;
            target->addFanout((CirGateV)self | inv);
            // need not set inv because it was set along (and thus shared with) literal ID
            self->setFanin(i, target);
         }

      }
   }

   CirGate* addPI(int lineno, unsigned lid) {
      unsigned gid = lid / 2;
      CirGate* pi = new InputGate(gid, lineno);
      _piList.push_back(pi);
      _gates[gid] = pi;
      return pi;
   }

   CirGate* addPO(int lineno, unsigned lid) {
      unsigned gid = _maxNum + _poList.size() + 1;
      OutputGate* po = new OutputGate(gid, lineno);
      _poList.push_back(po);
      po->_fanin[0] = lid;
      po->_faninCount = 1;
      return (_gates[gid] = po);
   }

   CirGate* addAIG(int lineno, unsigned lid, unsigned fin1, unsigned fin2) {
      unsigned gid = lid / 2;
      AigGate* aig = new AigGate(gid, lineno);
      aig->_fanin[0] = fin1;
      aig->_fanin[1] = fin2;
      aig->_faninCount = 2;
      return (_gates[gid] = aig);
   }

   CirGate* addUndef(unsigned gid) {
      UndefGate* undef = new UndefGate(gid);
      return (_gates[gid] = undef);
   }

   bool connect(CirGate* po, CirGate* pi, bool inv) {
      // if (!pi || !po) return false;
      // if (!pi->addFanin(po, inv)) return false;
      // po->addFanout(pi);
      return true;
   }

private:
   ofstream           *_simLog;
   GateList           _piList;
   GateList           _poList;
   GateList           _totalList;

   GateMap            _gates;
};

#endif // CIR_MGR_H
