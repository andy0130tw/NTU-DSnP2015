/****************************************************************************
  FileName     [ bst.h ]
  PackageName  [ util ]
  Synopsis     [ Define binary search tree package ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2005-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef BST_H
#define BST_H

#include <cassert>
#include <cstddef>   // for size_t
#include <iostream>  // for cout and endl
#include <iomanip>   // for setw

#define DEBUG_PRINT_WIDTH  6
#define DEBUG_INDENT_WIDTH 3

using namespace std;

static const size_t MASK = -2; // 111....10

template <class T> class BSTree;

// BSTreeNode is supposed to be a private class. User don't need to see it.
// Only BSTree and BSTree::iterator can access it.
//
// DO NOT add any public data member or function to this class!!
//
template <class T>
class BSTreeNode
{
   // TODO: design your own class!!
   friend class BSTree<T>;
   friend class BSTree<T>::iterator;

   BSTreeNode(const T& d, BSTreeNode<T>* l = 0, BSTreeNode<T>* r = 0):
      _data(d), _left((size_t)l), _right((size_t)r) {
      setLeftFlag(true);
      setRightFlag(true);
   }

   // these methods can be rewritten, taking advantage of bit slicing.
   bool isLeftThread  ()      { return  _left & 1; }
   bool isRightThread ()      { return _right & 1; }
   inline bool isLeaf()       { return isLeftThread() && isRightThread(); }
   BSTreeNode<T>* leftPtr  () { return (BSTreeNode<T>*)(_left  & MASK); }
   BSTreeNode<T>* rightPtr () { return (BSTreeNode<T>*)(_right & MASK); }
   BSTreeNode<T>* leftPtr  (BSTreeNode<T>* newptr) { _left  = (size_t)newptr | isLeftThread();  return newptr; }
   BSTreeNode<T>* rightPtr (BSTreeNode<T>* newptr) { _right = (size_t)newptr | isRightThread(); return newptr; }
   void setLeftFlag  (bool v) { _left  ^= (-v ^ _left)  & 1; }
   void setRightFlag (bool v) { _right ^= (-v ^ _right) & 1; }

   T               _data;
   // BSTreeNode<T>*  _left;
   // BSTreeNode<T>*  _right;
   size_t          _left;
   size_t          _right;
};


template <class T>
class BSTree
{
   // TODO: design your own class!!

public:
   BSTree() {
      _head = new BSTreeNode<T>(T()); // a dummy node
      // the entire BST is a left child of this node
      // the right thread points to the end of BST
      // to be consistent with other operations
      _head->leftPtr(_head);
      _head->rightPtr(_head);
   }
   ~BSTree() { clear(); delete _head; }

   class iterator {
      friend class BSTree;

   public:
      iterator(BSTreeNode<T>* n = 0): _node(n) {}
      iterator(const iterator& i) : _node(i._node) {}
      ~iterator() {} // Should NOT delete _node
      const T& operator * () const { return _node->_data; }
      T& operator * () { return _node->_data; }
      iterator& operator ++ () { next(); return *(this); }
      iterator operator ++ (int) { iterator ret = *(this); ++ *(this); return ret; }
      iterator& operator -- () { prev(); return *(this); }
      iterator operator -- (int) { iterator ret = *(this); -- *(this); return ret; }

      iterator operator + (int n) { iterator ret = *(this); return ret += n; }
      iterator& operator += (int n) { assert(n >= 0); while (n--) { next(); } return *(this); }
      iterator operator - (int n) { iterator ret = *(this); return ret -= n; }
      iterator& operator -= (int n) { assert(n >= 0); while (n--) { prev(); } return *(this); }

      iterator& operator = (const iterator& i) { _node = i._node; return *(this); }

      bool operator != (const iterator& i) const { return !(*(this) == i); }
      bool operator == (const iterator& i) const { return (_node == i._node); }

   private:
      BSTreeNode<T>* _node;
      void prev() {
         if (_node->isLeftThread()) {
            _node = _node->leftPtr();
         } else {
            BSTreeNode<T>* ptr = _node->leftPtr();
            while (!ptr->isRightThread())
               ptr = ptr->rightPtr();
            _node = ptr;
         }
      }
      void next() {
         if (_node->isRightThread()) {
            _node = _node->rightPtr();
         } else {
            BSTreeNode<T>* ptr = _node->rightPtr();
            while (!ptr->isLeftThread())
               ptr = ptr->leftPtr();
            _node = ptr;
         }
      }
   };

   iterator begin() const {
      BSTreeNode<T>* ptr = _head->leftPtr();
      while (!ptr->isLeftThread())
         ptr = ptr->leftPtr();
      return ptr;
   }
   iterator end() const {
      // return 'pass-the-end' pointer
      return _head;
   }

   bool empty() const { return (_head->leftPtr() == _head); }
   size_t size() const {
      size_t c = 0;
      for(iterator x = this->begin(), end = this->end(); x != end; ++x)
         ++c;
      return c;
   }

   void pop_front() {
      erase(begin());
   }
   void pop_back() {
      erase(end() - 1);
   }

   // return false if nothing to erase
   bool erase(iterator pos) {
      if (empty() || pos == _head) return false;
      BSTreeNode<T>* curr = pos._node;
      BSTreeNode<T>* prev = (pos - 1)._node;
      BSTreeNode<T>* next = (pos + 1)._node;
      if (curr->isLeaf()) {
         if (curr->rightPtr()->leftPtr() == curr) {
            curr->rightPtr()->leftPtr(prev);
            curr->rightPtr()->setLeftFlag(true);
         } else {
            curr->leftPtr()->rightPtr(next);
            curr->leftPtr()->setRightFlag(true);
         };
         if (next->isLeftThread()) next->leftPtr(prev);
         if (prev->isRightThread()) prev->rightPtr(next);
         delete curr;
         return true;
      } else if (!curr->isRightThread()) {
         // updating is too complicated; use replacing instead
         curr->_data = next->_data;
         return erase(next);
      } else {
         curr->_data = prev->_data;
         return erase(prev);
         // !curr->isLeftThread()
      }
   }

   // find and delete
   bool erase(const T& x) {
      iterator it;
      int dir = find(x, it);
      if (dir) return false;
      return erase(it);
   }

   void insert(const T& x){
      iterator it;
      int dir = find(x, it);
      BSTreeNode<T>* ptr = it._node;
      // when deleting, the successor is moved up from the right
      // so maybe this part can help with balancing here? :)
      // cout << "inserting " << x << endl;
      if (dir == 0) {
         BSTreeNode<T>* prev = (it-1)._node;
         BSTreeNode<T>* next = (it+1)._node;
         if (ptr->isLeftThread()) {
            dir = -1;
         } else if (ptr->isRightThread()) {
            dir = 1;
         } else if (prev->isRightThread()) {
            ptr = prev;
            dir = 1;
         } else if (next->isLeftThread()) {
            ptr = next;
            dir = -1;
         }
      }
      assert(dir != 0);
      if (dir == -1) {
         ptr->leftPtr(new BSTreeNode<T>(x, ptr->leftPtr(), ptr));
         ptr->setLeftFlag(false);
         // _nodeList.push_back(ptr->leftPtr());
      } else {
         ptr->rightPtr(new BSTreeNode<T>(x, ptr, ptr->rightPtr()));
         ptr->setRightFlag(false);
         // _nodeList.push_back(ptr->rightPtr());
      }
   }

   // delete all nodes except for the dummy node
   void clear() {
      if (empty()) return;
      purge(_head->leftPtr());
      _head->leftPtr(_head);
      _head->setLeftFlag(true);
      _head->rightPtr(_head);
      _head->setRightFlag(true);
   }

   void sort() const {} // no op; already sorted

   void print() {
      // a debug function
      if (empty()) {
         cout << "--- EMPTY ---" << endl;
         return;
      }
      printNodes(cout, _head->leftPtr());
      cout << endl;
   }

private:
   BSTreeNode<T>* _head; // always point to the dummy node (should it?

   int find(const T& val, iterator& it) {
      // if found `val`, return 0 with `it` set to that node
      // otherwise, set `it` to the node that `val` can be inserted to
      // -1 if left, 1 if right
      int dir = 0;
      BSTreeNode<T>* ptr = _head->leftPtr();
      if (empty()) {
         it = _head;
         return -1;
      }
      while(1) {
         if (val < ptr->_data) {
            dir = -1;
            if (ptr->isLeftThread()) break;
            ptr = ptr->leftPtr();
         } else if (ptr->_data < val) {
            dir = 1;
            if (ptr->isRightThread()) break;
            ptr = ptr->rightPtr();
         } else {
            dir = 0;
            break;
         }
      }
      it = ptr;
      return dir;
   }

   void purge(BSTreeNode<T>* ptr) {
      // recursively delete the subtree `ptr`.
      if (!ptr->isLeftThread()) purge(ptr->leftPtr());
      if (!ptr->isRightThread()) purge(ptr->rightPtr());
      delete ptr;
   }

   // for debugging
   void printNodes(ostream& os, BSTreeNode<T>* node) {
#ifdef BST_VERBOSE
      static int depth = 0;
      static const char* color_leaf  = "\033[1;33m\033[1;01m";  // orange bold
      static const char* color_void  = "\033[1;90m";            // dark gray
      static const char* color_left  = "\033[1;32m";            // green
      static const char* color_right  = "\033[1;34m";           // blue
      static const char* color_reset = "\033[1;0m";

      if (node->isLeaf()) os << color_leaf;
      os << setw(DEBUG_PRINT_WIDTH) << left << node->_data;
      if (node->isLeaf()) os << color_reset;

      if (node->isLeaf()) return;
      depth++;

      os << color_left;
      os << setw(DEBUG_INDENT_WIDTH) << right << "/";
      if (!node->isLeftThread()) printNodes(os, node->leftPtr());
      else os << color_void << "[ 0 ]";
      os << color_reset;

      os << endl;

      os << color_right;
      os << setw(depth * (DEBUG_INDENT_WIDTH + DEBUG_PRINT_WIDTH)) << right << "\\";
      if (node->!isRightThread()) printNodes(os, node->rightPtr());
      else os << color_void << "[ 0 ]";
      os << color_reset;

      depth--;
#endif  // BST_VERBOSE
   }
};

#endif // BST_H
