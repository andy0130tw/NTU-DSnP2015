/****************************************************************************
  FileName     [ myMinHeap.h ]
  PackageName  [ util ]
  Synopsis     [ Define MinHeap ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2014-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_MIN_HEAP_H
#define MY_MIN_HEAP_H

#include <algorithm>
#include <vector>

#include<iostream>
using namespace std;

#define NUM_P(x) (((x) - 1) / 2)
#define NUM_L(x) ((x) * 2 + 1)
#define NUM_R(x) ((x) * 2 + 2)

template <class Data>
class MinHeap
{
public:
   MinHeap(size_t s = 0) { if (s != 0) _data.reserve(s); }
   ~MinHeap() {}

   #define CURRENT(x)     _data[x]
   #define PARENT(x)      _data[NUM_P(x)]
   #define LEFT_CHILD(x)  _data[NUM_L(x)]
   #define RIGHT_CHILD(x) _data[NUM_R(x)]

   void clear() { _data.clear(); }

   // For the following member functions,
   // We don't respond for the case vector "_data" is empty!
   const Data& operator [] (size_t i) const { return _data[i]; }
   Data& operator [] (size_t i) { return _data[i]; }

   size_t size() const { return _data.size(); }

   // TODO
   const Data& min() const {
      if(size())
         return _data[0];
      // there is a design flaw in the reference code,
      // that for an empty _data, an UB is occurred.
      // though we don't respond, crashes should be avoided anyway,
      // as a workaround, here we create a temporary node.
      // (indeed, this causes memory leak but we don't care)
      return *(new Data("__NON_EXISTENT__", -1));
   }
   void insert(const Data& d) {
      size_t i = size();
      _data.push_back(d);
      // promote _data[i] until value is larger than its parent
      while (i > 0 && CURRENT(i) < PARENT(i)) {
         swap(CURRENT(i), PARENT(i));
         i = NUM_P(i);
      }
   }
   void delMin() { delData(0); }

   void delData(size_t i) {
      // swap the current item and the last one
      swap(CURRENT(i), CURRENT(size() - 1));

      // remove it immediately to avoid being swapped off
      _data.erase(_data.end() - 1);

      // maintain heap structure
      // sink it until value is smaller than both its children
      size_t s = size();
      while (NUM_R(i) < s) {
         if (LEFT_CHILD(i) < CURRENT(i) && LEFT_CHILD(i) < RIGHT_CHILD(i)) {
            swap(CURRENT(i), LEFT_CHILD(i));
            i = NUM_L(i);
         } else if (RIGHT_CHILD(i) < CURRENT(i)) {
            swap(CURRENT(i), RIGHT_CHILD(i));
            i = NUM_R(i);
         } else break;
      }

      // special case: only left child is available
      // that is, the left child is the last item
      if (NUM_R(i) == s && LEFT_CHILD(i) < CURRENT(i))
         swap(CURRENT(i), LEFT_CHILD(i));
   }

private:
   // DO NOT add or change data members
   vector<Data>   _data;
};

#endif // MY_MIN_HEAP_H
