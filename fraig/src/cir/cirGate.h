/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;
class CirStrashKey;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
   friend CirStrashKey;

public:
   CirGate(GateType t = UNDEF_GATE, unsigned gid = 0, int ln = 0):
      _type(t), _faninCount(0), _id(gid), _ref(_global_ref), _lineno(ln) {}
   virtual ~CirGate() {}

   GateType _type;
   string _name;

   CirGateV _fanin[2];
   size_t _faninCount;

   GateVList _fanoutList;

   CirGate* getFanin(size_t i) const { return (CirGate*) (_fanin[i] & PTR_MASK); }
   void setFanin(size_t i, CirGate* const g) { _fanin[i] = (CirGateV)g | (_fanin[i] & 1); }
   void setFanin(size_t i, CirGateV const g) { _fanin[i] = g; }
   bool getInv(size_t i) const { return _fanin[i] & 1; }
   void setInv(size_t i, bool v) { _fanin[i] ^= -v ^ _fanin[i]; }

   CirGate* getFanout(size_t i) const {
      assert(i < _fanoutList.size());
      return (CirGate*) (_fanoutList[i] & PTR_MASK);
   }

   bool getFanoutInv(size_t i) const {
      assert(i < _fanoutList.size());
      return _fanoutList[i] & 1;
   }

   // Basic access methods
   virtual string getTypeStr() const { return ""; }
   unsigned getLineNo() const { return _lineno; }
   unsigned getID() const { return _id; }

   virtual bool addFanin(CirGateV fi) {
      if (_faninCount > 2) return false;
      _fanin[_faninCount++] = fi;
      return true;
   }
   virtual bool addFanout(CirGateV fo) {
      _fanoutList.push_back(fo);
      return true;
   }
   virtual bool addFanin(CirGate* gate, bool inv) { return addFanin((CirGateV)gate | inv); }
   virtual bool addFanout(CirGate* gate, bool inv) { return addFanout((CirGateV)gate | inv); }

   void replaceFanin(CirGate* a, CirGateV b) {
      for (size_t i = 0; i < _faninCount; i++) {
         if (getFanin(i) == a) {
            setFanin(i, b);
         }
      }
   }

   bool eraseFanout(CirGate* f) {
      for (GateVList::iterator it = _fanoutList.begin(); it != _fanoutList.end(); ++it) {
         if (!(((*it) ^ (CirGateV)f) >> 1)) {
            _fanoutList.erase(it);
            return true;
         }
      }
      return false;
   }

   static void const clearMark() { _global_ref++; }
   void mark() const { _ref = _global_ref; }
   bool isMarked() const { return (_ref == _global_ref); }

   // for making DFS list
   void traversal(GateList* l) const;

   // Printing functions
   virtual void printGate() const {}
   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanout(int level) const;

   void reportFaninRecursive(int) const;
   void reportFanoutRecursive(int) const;

   virtual bool isAig() const { return false; }

   virtual void simulate() {}
   CirSimData getSimData(bool inv = false) { return (inv ? ~_sim_data : _sim_data); }
   void setSimData(CirSimData s) { _sim_data = s; }

private:
   unsigned _id;
   mutable unsigned _ref;
   int _lineno;

   static unsigned _global_ref;
   static CirGateV const PTR_MASK = ~((CirGateV)1);

protected:
   CirSimData _sim_data;
};

class AigGate: public CirGate {
public:
   AigGate(unsigned gid, int ln): CirGate(AIG_GATE, gid, ln) {}

   string getTypeStr() const { return "AIG"; }

   void printGate() const {
      cout << getID() << " ";
      if (getFanin(0)->_type == UNDEF_GATE) cout << "*";
      if (getInv(0)) cout << "!";
      cout << getFanin(0)->getID() << " ";

      if (getFanin(1)->_type == UNDEF_GATE) cout << "*";
      if (getInv(1)) cout << "!";
      cout << getFanin(1)->getID();
   }

   bool isAig() const { return true; }
   void simulate() {
      assert(_faninCount == 2);
      CirSimData sim1 = getFanin(0)->getSimData(getInv(0));
      CirSimData sim2 = getFanin(1)->getSimData(getInv(1));
      _sim_data = sim1 & sim2;
   }

private:
};

class InputGate: public CirGate {
public:
   InputGate(unsigned gid, int ln): CirGate(PI_GATE, gid, ln) {};

   bool addFanin(CirGate* g, bool inv) { return false; }

   string getTypeStr() const { return "PI"; }
   void printGate() const {
      cout << getID();
   }

private:
};

class ConstGate: public CirGate {
public:
   // constant is always from gid 0
   ConstGate(): CirGate(CONST_GATE, 0, 0) { _sim_data = 0; };

   bool addFanin(CirGate* g, bool inv) { return false; }

   string getTypeStr() const { return "CONST"; }
   void printGate() const { cout << "0"; }

   // cannot write simData to it
   void setSimData(CirSimData s) {}
private:
};

class OutputGate: public CirGate {
public:
   OutputGate(unsigned gid, int ln): CirGate(PO_GATE, gid, ln) {};

   string getTypeStr() const { return "PO"; }
   bool addFanout(CirGate* fin) { return false; }

   void printGate() const {
      cout << getID() << " ";
      if (getFanin(0)->_type == UNDEF_GATE) cout << "*";
      if (getInv(0)) cout << "!";
      cout << getFanin(0)->getID();
   }

   void simulate() {
      _sim_data = getFanin(0)->getSimData(getInv(0));
   }
private:
};

class UndefGate: public CirGate {
public:
   UndefGate(unsigned gid): CirGate(UNDEF_GATE, gid, 0) {};
   string getTypeStr() const { return "UNDEF"; }
   void printGate() const {
      cout << "*" << getID();
   }

private:
};

//--------------------------------
// hash keys used in HashMap class
//--------------------------------
class CirStrashKey {
public:
   CirStrashKey(CirGate* g) {
      // dicision: hash pointer (more random?) or hash id?
      // _in0 = g->_fanin[0] >> 1;
      _v0 = g->getFanin(0)->getID();
      // _v1 = g->_fanin[1] >> 1;
      _v1 = g->getFanin(1)->getID();
      if (_v0 > _v1) swap(_v0, _v1);

      // |    fin0    |    fin1    |  inv  |
      // |<--- 15 --->|<--- 16 --->|<- 1 ->|
      // use + to cause _v1 to overflow, prevent collision even more
      _hash = ((_v0 << 17) + (_v1 << 1)) | (g->getInv(0) ^ g->getInv(1));
   };
   ~CirStrashKey() {};
   size_t operator() () const { return _hash; }
   bool operator == (const CirStrashKey& k) const {
      return _v0 == k._v0 && _v1 == k._v1 && (_hash & 1) == (k._hash & 1);
   }
private:
   size_t _v0, _v1, _hash;
   // CirGate* _in0, _in1;
};

#endif // CIR_GATE_H
