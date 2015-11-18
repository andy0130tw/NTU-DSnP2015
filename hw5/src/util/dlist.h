/****************************************************************************
  FileName     [ dlist.h ]
  PackageName  [ util ]
  Synopsis     [ Define doubly linked list package ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2005-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef DLIST_H
#define DLIST_H

#include <cassert>
#include <cstddef>  // for size_t

template <class T> class DList;

// DListNode is supposed to be a private class. User don't need to see it.
// Only DList and DList::iterator can access it.
//
// DO NOT add any public data member or function to this class!!
//
template <class T>
class DListNode
{
   friend class DList<T>;
   friend class DList<T>::iterator;

   DListNode(const T& d, DListNode<T>* p = 0, DListNode<T>* n = 0):
      _data(d), _prev(p), _next(n) {}

   T              _data;
   DListNode<T>*  _prev;
   DListNode<T>*  _next;
};


template <class T>
class DList
{
public:
   DList() {
      _head = new DListNode<T>(T());
      _head->_prev = _head->_next = _head; // _head is a dummy node
   }
   ~DList() { clear(); delete _head; }

   // DO NOT add any more data member or function for class iterator
   class iterator
   {
      friend class DList;

   public:
      iterator(DListNode<T>* n= 0): _node(n) {}
      iterator(const iterator& i) : _node(i._node) {}
      ~iterator() {} // Should NOT delete _node

      // TODO: implement these overloaded operators
      const T& operator * () const { return _node->_data; }
      T& operator * () { return _node->_data; }
      iterator& operator ++ () { _node = _node->_next; return *(this); }
      iterator operator ++ (int) { iterator ret = *(this); ++ *(this); return ret; }
      iterator& operator -- () { _node = _node->_prev; return *(this); }
      iterator operator -- (int) { iterator ret = *(this); -- *(this); return ret; }

      iterator& operator = (const iterator& i) { _node = i._node; return *(this); }

      bool operator != (const iterator& i) const { return !(*(this) == i); }
      bool operator == (const iterator& i) const { return (_node == i._node); }

   private:
      DListNode<T>* _node;
   };

   // TODO: implement these functions
   iterator begin() const { return _head->_next; }
   iterator end() const { return _head; }
   bool empty() const { return (begin() == end()); }
   size_t size() const {
      size_t c = 0;
      for(iterator x = this->begin(), end = this->end(); x != end; ++x)
         ++c;
      return c;
   }

   void push_back(const T& x) {
      DListNode<T>* oldNode = _head->_prev;
      DListNode<T>* newNode = new DListNode<T>(x, oldNode, _head);
      _head->_prev = newNode;
      oldNode->_next = newNode;
   }
   void pop_front() {
      erase(begin());

      // alternative
      // if (empty()) return;
      // DListNode<T>* popping = _head->_next;
      // _head->_next = popping->_next;
      // popping->_next->_prev = _head;
      // delete popping;
   }
   void pop_back() {
      erase(end()._node->_prev);

      // alternative-1
      // DListNode<T>* popping = _head->_prev;
      // popping->_prev->_next = _head;
      // _head->_prev = popping->_prev;
      // delete popping;

      // alternative-2, using pop_front
      // if (empty()) return;
      // DListNode<T>* tmp = _head;
      // _head = _head->_prev->_prev;
      // pop_front();
      // _head = tmp;
   }

   // return false if nothing to erase
   bool erase(iterator pos) {
      if (empty()) return false;
      assert(pos._node != _head);
      DListNode<T>* cur = pos._node;
      cur->_prev->_next = cur->_next;
      cur->_next->_prev = cur->_prev;
      delete cur;
      return true;
   }
   // find and delete
   bool erase(const T& x) {
      for (iterator it = begin(); it != end(); ++it) {
         if (it._node->_data == x) {
            erase(it);
            return true;
         }
      }
      return false;
   }

   // delete all nodes except for the dummy node
   void clear() {
      if (empty()) return;
      DListNode<T>* temp = _head->_next;
      do {
         temp = temp->_next;
         delete temp->_prev;
      } while (temp != _head);
      _head->_next = _head->_prev = _head;
   }

   void sort() const {
      bool ok = false;
      iterator last = end()._node->_prev;
      iterator last_check = last;
      for (iterator i = begin(); i != last; ++i) {
         if (ok) break;
         ok = true;
         for (iterator j = begin(); j != last_check; ++j) {
            if (!(j._node->_data < j._node->_next->_data)) {
               swap(j);
               ok = false;
            }
         }
         --last_check;
      }
   }

private:
   DListNode<T>*  _head;  // = dummy node if list is empty

   // [OPTIONAL TODO] helper functions; called by public member functions
   // swap the element and the next one
   void swap(iterator pos) const {
      assert(pos != end()._node->_prev);
      // swap the internal data; somehow dirty; super slow
      T tmp = pos._node->_next->_data;
      pos._node->_next->_data = pos._node->_data;
      pos._node->_data = tmp;
   }
};

#endif // DLIST_H
