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

   BSTreeNode(const T& d, BSTreeNode<T>* p = 0, BSTreeNode<T>* n = 0):
      _data(d), _left(p), _right(n), _lthr(true), _rthr(true) {}

   // these methods can be rewritten, taking adventage of bit slicing.
   bool hasLeftChild  ()       { return !this->_lthr; }
   bool hasRightChild ()       { return !this->_rthr; }
   void setLeftFlag   (bool v) { this->_lthr = v; }
   void setRightFlag  (bool v) { this->_rthr = v; }
   bool isLeaf() { return !hasLeftChild() && !hasRightChild(); }

   T               _data;
   BSTreeNode<T>*  _left;
   BSTreeNode<T>*  _right;
   // true if the pointer is a `thread`, false if it is a child.
   bool            _lthr;
   bool            _rthr;
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
      _head->_left = _head->_right = _head;
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
         if (!_node->hasLeftChild()) {
            _node = _node->_left;
         } else {
            BSTreeNode<T>* ptr = _node->_left;
            while (ptr->hasRightChild())
               ptr = ptr->_right;
            _node = ptr;
         }
      }
      void next() {
         if (!_node->hasRightChild()) {
            _node = _node->_right;
         } else {
            BSTreeNode<T>* ptr = _node->_right;
            while (ptr->hasLeftChild())
               ptr = ptr->_left;
            _node = ptr;
         }
      }
   };

   iterator begin() const {
      BSTreeNode<T>* ptr = _head->_left;
      while (ptr->hasLeftChild())
         ptr = ptr->_left;
      return ptr;
   }
   iterator end() const {
      // return 'pass-the-end' pointer
      return _head;
   }

   bool empty() const { return (_head->_left == _head); }
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
      if (empty()) return false;
      BSTreeNode<T>* curr = pos._node;
      BSTreeNode<T>* prev = (pos - 1)._node;
      BSTreeNode<T>* next = (pos + 1)._node;
      if (curr->isLeaf()) {
         if (curr->_right->_left == curr) {
            curr->_right->_left = prev;
            curr->_right->setLeftFlag(true);
         } else {
            curr->_left->_right = next;
            curr->_left->setRightFlag(true);
         };
         if (!next->hasLeftChild()) next->_left = prev;
         if (!prev->hasRightChild()) prev->_right = next;
         delete curr;
         return true;
      } else if (curr->hasRightChild()) {
         // updating is too complicated; use replacing instead
         curr->_data = next->_data;
         return erase(next);
      } else {
         curr->_data = prev->_data;
         return erase(prev);
         // curr->hasLeftChild()
      }
      return false;
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
         if (!ptr->hasLeftChild()) {
            dir = -1;
         } else if (!ptr->hasRightChild()) {
            dir = 1;
         } else if (!prev->hasRightChild()) {
            ptr = prev;
            dir = 1;
         } else if (!next->hasLeftChild()) {
            ptr = next;
            dir = -1;
         }
      }
      assert(dir != 0);
      if (dir == -1) {
         ptr->_left = new BSTreeNode<T>(x, ptr->_left, ptr);
         ptr->setLeftFlag(false);
         // _nodeList.push_back(ptr->_left);
      } else {
         ptr->_right = new BSTreeNode<T>(x, ptr, ptr->_right);
         ptr->setRightFlag(false);
         // _nodeList.push_back(ptr->_right);
      }
   }

   // delete all nodes except for the dummy node
   void clear() {
      if (empty()) return;
      purge(_head->_left);
      _head->_left = _head->_right = _head;
      _head->setLeftFlag(true);
      _head->setRightFlag(true);
   }

   void sort() const {} // no op; already sorted

   void print() {
      // a debug function
      if (empty()) {
         cout << "--- EMPTY ---" << endl;
         return;
      }
      printNodes(cout, _head->_left);
      cout << endl;
   }

private:
   BSTreeNode<T>* _head; // always point to the dummy node (should it?

   int find(const T& val, iterator& it) {
      // if found `val`, return 0 with `it` set to that node
      // otherwise, set `it` to the node that `val` can be inserted to
      // -1 if left, 1 if right
      int dir = 0;
      BSTreeNode<T>* ptr = _head->_left;
      if (empty()) {
         it = _head;
         return -1;
      }
      while(1) {
         if (val < ptr->_data) {
            dir = -1;
            if (!ptr->hasLeftChild()) break;
            ptr = ptr->_left;
         } else if (ptr->_data < val) {
            dir = 1;
            if (!ptr->hasRightChild()) break;
            ptr = ptr->_right;
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
      if (ptr->hasLeftChild()) purge(ptr->_left);
      if (ptr->hasRightChild()) purge(ptr->_right);
      delete ptr;
   }

   // for debugging
   // vector< BSTreeNode<T>* > _nodeList;
   void printNodes(ostream& os, BSTreeNode<T>* node) {
      static int depth = 0;
      static string color_leaf  = "\033[1;33m\033[1;01m";  // orange bold
      static string color_left  = "\033[1;32m";            // green
      static string color_right  = "\033[1;34m";           // blue
      static string color_reset = "\033[1;0m";

      if (node->isLeaf()) os << color_leaf;
      os << setw(DEBUG_PRINT_WIDTH) << left << node->_data;
      if (node->isLeaf()) os << color_reset;

      if (node->isLeaf()) return;
      depth++;

      os << color_left;
      os << setw(DEBUG_INDENT_WIDTH) << right << "/";
      if (node->hasLeftChild()) printNodes(os, node->_left);
      else os << "[ 0 ]";
      os << color_reset;

      os << endl;

      os << color_right;
      os << setw(depth * (DEBUG_INDENT_WIDTH + DEBUG_PRINT_WIDTH)) << right << "\\";
      if (node->hasRightChild()) printNodes(os, node->_right);
      else os << "[ 0 ]";
      os << color_reset;

      depth--;
   }
};

/*
// backup for BSTree::print
// cout << "Node\tLeft\tTL Right\tTR" << endl;
// for (iterator it = begin(); it != end(); ++it) {
// for (size_t i = 0, n = _nodeList.size(); i < n; i++) {
   // cout         << it._node->_data
   //      << "\t" << it._node->_left->_data
   //      << "\t" << it._node->_right->_data
   //      << "  " << it._node->_lthr
   //      << "\t" << it._node->_rthr << endl;
   // cout         << _nodeList[i]->_data
   //      << "\t" << _nodeList[i]->_left->_data
   //      << "\t" << _nodeList[i]->_lthr
   //      << "  " << _nodeList[i]->_right->_data
   //      << "\t" << _nodeList[i]->_rthr << endl;
// }
*/

#endif // BST_H
