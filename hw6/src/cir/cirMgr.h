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
#include <map>
#include <cassert>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

#include "cirDef.h"
#include "cirGate.h"

extern CirMgr *cirMgr;

typedef map<unsigned, CirGate*> GateMap;

// TODO: Define your own data members and member functions
class CirMgr
{
public:
   CirMgr(){}
   ~CirMgr() {
      for (GateMap::const_iterator it = _gates.begin(); it != _gates.end(); ++it) {
         delete it->second;
      }
   }

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const {
      //_maxNum + _outputCount
      // if (gid > gates.size()) return 0;
      GateMap::const_iterator it = _gates.find(gid);
      if (it == _gates.end()) return 0;
      return it->second;
   }

   // DFS!!!
   void dfs(GateList*) const;

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void writeAag(ostream&) const;

   unsigned int _maxNum;
   unsigned int _inputCount;
   unsigned int _latchCount;
   unsigned int _outputCount;
   unsigned int _andGateCount;

   bool addToGateList(unsigned gid, CirGate* x) {
      GateMap::const_iterator it = _gates.find(gid);
      if (it != _gates.end()) {
         CirGate* oldGate = it->second;
         if (oldGate->_type != UNDEF_GATE) return false;
         // cerr << " - disgard " << oldGate->getTypeStr() << " #" << oldGate->getID() << endl;
         // replace fanouts
         GateList& fout = oldGate->_fanoutList;
         for (size_t i = 0, n1 = fout.size(); i < n1; i++) {
            GateList& src = fout[i]->_faninList;
            for (size_t j = 0, n2 = src.size(); j < n2; j++) {
               if (src[j] == oldGate) {
                  // cerr << "   - replace to " << src[j]->getID() << endl;
                  src[j] = x;
                  x->addFanout(fout[i]);
               }
            }
         }
         // replace fanins
         GateList& fin = oldGate->_faninList;
         for (size_t i = 0, n1 = fin.size(); i < n1; i++) {
            GateList& src = fin[i]->_fanoutList;
            for (size_t j = 0, n2 = src.size(); j < n2; j++) {
               if (src[j] == oldGate) {
                  // cerr << "   - replace from " << src[j]->getID() << endl;
                  src[j] = x;
                  x->addFanin(fin[i], oldGate->getInv(i));
               }
            }
         }
         delete oldGate;
      }
      cerr << " - add " << x->getTypeStr() << " #" << gid << endl;
      _gates[gid] = x;
      if (x->_type != UNDEF_GATE)
         _totalList.push_back(x);
      return true;
   }

   CirGate* addPI(int lineno, unsigned gid) {
      CirGate* pi = new InputGate(gid, lineno);
      _piList.push_back(pi);
      addToGateList(gid, pi);
      return pi;
   }

   CirGate* addPO(int lineno, CirGate* src, bool inv) {
      unsigned gid = _maxNum + _poList.size() + 1;
      OutputGate* po = new OutputGate(gid, lineno);
      connect(src, po, inv);
      _poList.push_back(po);
      addToGateList(gid, po);
      return po;
   }

   CirGate* addAIG(int lineno, unsigned gid, CirGate* fin1, CirGate* fin2, bool inv1, bool inv2) {
      AigGate* aig = new AigGate(gid, lineno);
      // aig->addFanin(fin1, inv1);
      // aig->addFanin(fin2, inv2);
      connect(fin1, aig, inv1);
      connect(fin2, aig, inv2);
      addToGateList(gid, aig);
      return aig;
   }

   CirGate* addUndef(unsigned gid) {
      UndefGate* undef = new UndefGate(gid);
      addToGateList(gid, undef);
      return undef;
   }

   bool connect(CirGate* po, CirGate* pi, bool inv) {
      if (!pi || !po) return false;
      if (!pi->addFanin(po, inv)) return false;
      assert(po->addFanout(pi));
      return true;
   }

private:
   GateList _piList;
   GateList _poList;
   GateList _totalList;

   GateMap _gates;
};

#endif // CIR_MGR_H
