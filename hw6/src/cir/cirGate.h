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
#include <cassert>
#include <set>
#include <iostream>
#include "cirDef.h"

using namespace std;

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: Define your own data members and member functions, or classes
class CirGate
{
public:
   CirGate(GateType t = UNDEF_GATE, unsigned gid = 0, int ln = 0):
      _type(t), _id(gid), _lineno(ln) {}
   virtual ~CirGate() {}

   GateType _type;
   string _name;

   GateList _faninList;
   GateList _fanoutList;

   // Basic access methods
   string getTypeStr() const {
      switch (_type) {
         case PI_GATE:    return "PI";
         case PO_GATE:    return "PO";
         case AIG_GATE:   return "AIG";
         case CONST_GATE: return "CONST0";
         case UNDEF_GATE: return "UNDEF";
         default:         return "";
      }
   }
   unsigned getLineNo() const { return _lineno; }
   unsigned getID() const { return _id; }

   virtual bool addFanin(CirGate* g, bool inv) { _faninList.push_back(g); return true; }
   virtual bool addFanout(CirGate* g) { _fanoutList.push_back(g); return true; }

   virtual bool getInv(size_t i) const { return false; }

   static void clearMark() { _global_ref++; }
   void mark() { _ref = _global_ref; }
   bool isMarked() const { return (_ref == _global_ref); }

   // for making DFS list
   void traversal(GateList* l);

   // Printing functions
   virtual void printGate() const = 0;
   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanout(int level) const;

   void reportRecursive(set<unsigned>& visited, bool reverse, int limit, const CirGate* parent) const;

private:
   unsigned _id;
   unsigned _ref;
   int _lineno;

   static unsigned _global_ref;

protected:
};

class AigGate: public CirGate {
public:
   AigGate(unsigned gid, int ln): CirGate(AIG_GATE, gid, ln) {}

   void printGate() const {}

   bool addFanin(CirGate* fin, bool inv) {
      size_t n = _faninList.size();
      if (n == 2) return false;
      _inv[n] = inv;
      _faninList.push_back(fin);
      return true;
   }

   bool isUsed() {
      return (_faninList.size() == 2 && _fanoutList.size() > 0);
   }

   bool getInv(size_t i) const { assert(i < 2); return _inv[i]; }

private:
   bool _inv[2];
};

class InputGate: public CirGate {
public:
   InputGate(unsigned gid, int ln): CirGate(PI_GATE, gid, ln) {};

   bool addFanin(CirGate* fin, bool inv) { return false; }

   void printGate() const {}
};

class ConstGate: public CirGate {
public:
   // constant is always from gid 0
   ConstGate(): CirGate(CONST_GATE, 0, 0) {};

   bool addFanin(CirGate* fin, bool inv) { return false; }

   void printGate() const {}

   bool getInv(size_t i) const { return _inv; }

private:
   bool _inv;

};

class OutputGate: public CirGate {
public:
   OutputGate(unsigned gid, int ln): CirGate(PO_GATE, gid, ln) {};

   bool addFanin(CirGate* fin, bool inv) {
      if (!_faninList.empty())
         return false;
      _faninList.push_back(fin);
      _inv = inv;
      return true;
   }
   bool addFanout(CirGate* fin) { return false; }
   bool getInv(size_t i) const { return _inv; }

   void printGate() const {}
private:
   bool _inv;
};

class UndefGate: public CirGate {
public:
   UndefGate(unsigned gid): CirGate(UNDEF_GATE, gid, 0) {};
   bool addFanin(CirGate* fin, bool inv) {
      size_t n = _faninList.size();
      if (n == 2) return false;
      _inv[n] = inv;
      _faninList.push_back(fin);
      return true;
   }
   bool getInv(size_t i) const { assert(i < 2); return _inv[i]; }
   void printGate() const {}

private:
   bool _inv[2];
};

#endif // CIR_GATE_H
