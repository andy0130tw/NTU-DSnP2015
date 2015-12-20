/****************************************************************************
  FileName     [ myHashSet.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashSet ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2014-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_SET_H
#define MY_HASH_SET_H

#include <vector>

using namespace std;

//---------------------
// Define HashSet class
//---------------------
// To use HashSet ADT,
// the class "Data" should at least overload the "()" and "==" operators.
//
// "operator ()" is to generate the hash key (size_t)
// that will be % by _numBuckets to get the bucket number.
// ==> See "bucketNum()"
//
// "operator ()" is to check whether there has already been
// an equivalent "Data" object in the HashSet.
// Note that HashSet does not allow equivalent nodes to be inserted
//
template <class Data>
class HashSet
{
public:
   HashSet(size_t b = 0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
   ~HashSet() { reset(); }

   typedef vector<Data> HashBucket;

   // TODO: implement the HashSet<Data>::iterator
   // o An iterator should be able to go through all the valid Data
   //   in the Hash
   // o Functions to be implemented:
   //   - constructor(s), destructor
   //   - operator '*': return the HashNode
   //   - ++/--iterator, iterator++/--
   //   - operators '=', '==', !="
   //
   class iterator
   {
      friend class HashSet<Data>;
   public:
      iterator(): _hashSet(0), _key(0), _pos(0) {}
      iterator(const HashSet<Data>* hs, size_t k, size_t p):
         _hashSet((HashSet<Data>*)hs), _key(k), _pos(p) {}
      iterator(const iterator& i):
         _hashSet(i._hashSet), _key(i._key), _pos(i._pos) {}
      ~iterator() {}

      const Data& operator * () const { return (*_hashSet)[_key][_pos]; }
      Data& operator * () { return (*_hashSet)[_key][_pos]; }
      iterator& operator ++ () {
         _pos++;
         while (_key < _hashSet->numBuckets()) {
            HashBucket* bucket = &(*_hashSet)[_key];
            if (_pos < bucket->size())
               break;
            _key++;
            _pos = 0;
         }
         return *this;
      }
      iterator operator ++ (int) { iterator it = *this; ++this; return it; }
      iterator& operator -- () {
         _pos--;
         while (_key > 0) {
            // if it is 0, decrement will result in underflow
            _key--;
            HashBucket* bucket = &(*_hashSet)[_key];
            if (bucket->size()) {
               _pos = bucket->size() - 1;
               return *this;
            }
         }
         // to be consistent with end(), use pass-the-end
         _key = _hashSet->numBuckets();
         _pos = 0;
         return *this;
      }
      iterator operator -- (int) { iterator it = *this; ++this; return it; }
      iterator& operator = (iterator i) {
         _hashSet = i._hashSet;
         _key = i._key;
         _pos = i._pos;
         return this;
      }
      bool operator == (const iterator& i) {
         if (_hashSet != i._hashSet) return false;
         if (_key != i._key) return false;
         if (_pos != i._pos) return false;
         return true;
      }
      bool operator != (const iterator& i) { return !(*this == i); }

   private:
      HashSet<Data>*       _hashSet;
      size_t               _key;
      size_t               _pos;
   };

   void init(size_t b) { _numBuckets = b; _buckets = new HashBucket[b]; }
   void reset() {
      _numBuckets = 0;
      if (_buckets) { delete [] _buckets; _buckets = 0; }
   }
   void clear() {
      for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
   }
   size_t numBuckets() const { return _numBuckets; }

   HashBucket& operator [] (size_t i) { return _buckets[i]; }
   const HashBucket& operator [](size_t i) const { return _buckets[i]; }

   // TODO: implement these functions
   //
   // Point to the first valid data
   iterator begin() const {
      for (size_t i = 0; i < _numBuckets; i++) {
         if (_buckets[i].size())
            return iterator(this, i, 0);
      }
      return end();
   }
   // Pass the end
   iterator end() const {
      return iterator(this, _numBuckets, 0);
   }
   // return true if no valid data
   bool empty() const { return begin() == end(); }
   // number of valid data
   size_t size() const {
       size_t s = 0;
       for (iterator it = begin(); it != end(); ++it) s++;
       return s;
   }

   // check if d is in the hash...
   // if yes, return true;
   // else return false;
   bool check(const Data& d) const {
      return (get(d) != 0);
   }

   // query if d is in the hash...
   // if yes, replace d with the data in the hash and return true;
   // else return false;
   bool query(Data& d) const {
      Data* t = get(d);
      if (!t) return false;
      d = *t;
      return true;
   }

   // update the entry in hash that is equal to d
   // if found, update that entry with d and return true;
   // else insert d into hash as a new entry and return false;
   bool update(const Data& d) {
      Data* t = get(d, true);
      if (!t) return false;
      *t = d;
      return true;
   }

   // return true if inserted successfully (i.e. d is not in the hash)
   // return false is d is already in the hash ==> will not insert
   bool insert(const Data& d) {
      size_t key = bucketNum(d);
      _buckets[key].push_back(d);
      return true;
   }

   // return true if removed successfully (i.e. d is in the hash)
   // return fasle otherwise (i.e. nothing is removed)
   bool remove(const Data& d) {
      HashBucket* bucket = &_buckets[bucketNum(d)];
      typename HashBucket::iterator it = bucket->begin();
      for (; it != bucket->end(); ++it)
         if ((*it) == d) {
            bucket->erase(it);
            return true;
         }
      return false;
   }

private:
   // Do not add any extra data member
   size_t            _numBuckets;
   HashBucket*       _buckets;

   size_t bucketNum(const Data& d) const {
      return (d() % _numBuckets); }

   // [custom]
   // find data in the hash; null if not found
   // if insert is true, data is inserted (but null is still returned)
   Data* get(const Data& d, bool insert = false) const {
      HashBucket* bucket = &_buckets[bucketNum(d)];
      typename HashBucket::iterator it = bucket->begin();
      for (; it != bucket->end(); ++it)
         if ((*it) == d)
            return &(*it);
      if (insert)
         bucket->push_back(d);
      return 0;
   }
};

#endif // MY_HASH_SET_H
