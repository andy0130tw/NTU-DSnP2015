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

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
public:
   CirGate(GateType t = UNDEF_GATE, unsigned gid = 0, int ln = 0):
      _type(t), _faninCount(0), _id(gid), _ref(_global_ref), _lineno(ln) {}
   virtual ~CirGate() {}

   GateType _type;
   string _name;

   GateVList _fanoutList;

   CirGateV _fanin[2];
   size_t _faninCount;
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

private:
   unsigned _id;
   mutable unsigned _ref;
   int _lineno;

   static unsigned _global_ref;

   static CirGateV const PTR_MASK = ~((CirGateV)1);
protected:
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
   ConstGate(): CirGate(CONST_GATE, 0, 0) {};

   bool addFanin(CirGate* g, bool inv) { return false; }

   string getTypeStr() const { return "CONST"; }
   void printGate() const { cout << "0"; }


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

#endif // CIR_GATE_H
