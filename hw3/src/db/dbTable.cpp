/****************************************************************************
  FileName     [ dbTable.cpp ]
  PackageName  [ db ]
  Synopsis     [ Define database Table member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2015-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iomanip>
#include <string>
#include <cctype>
#include <cassert>
#include <set>
#include <algorithm>
#include "dbTable.h"

// custom import
#include "util.h"

using namespace std;

/*****************************************/
/*          Global Functions             */
/*****************************************/
ostream& operator << (ostream& os, const DBRow& r)
{
   // TODO: to print out a row.
   // - Data are seperated by a space. No trailing space at the end.
   // - Null cells are printed as '.'
   for (size_t i = 0; i < r.size(); i++) {
      if (i != 0) os << ' ';
      DBTable::printData(os, r[i]);
   }
   return os;
}

ostream& operator << (ostream& os, const DBTable& t)
{
   // TODO: to print out a table
   // - Data are seperated by setw(6) and aligned right.
   // - Null cells should be left blank (printed as ' ').
   for (size_t i = 0; i < t.nRows(); i++) {
      for (size_t j = 0; j < t.nCols(); j++) {
         os << setw(6) << right;
         if (t[i][j] == INT_MAX)
            os << ' ';
         else
            DBTable::printData(os, t[i][j]);
      }
      os << endl;
   }
   return os;
}

ifstream& operator >> (ifstream& ifs, DBTable& t)
{
   // TODO: to read in data from csv file and store them in a table
   // - You can assume all the data of the table are in a single line.
   string input, bufLine;
   getline(ifs, input);

   size_t last_eol = 0;
   while (1) {
      if (last_eol >= input.length())
         break;
      size_t pos_eol = input.find_first_of('\r', last_eol);
      bufLine = input.substr(last_eol, pos_eol - last_eol);
      last_eol = pos_eol + 1;

      // also break if an empty line is encountered
      if (pos_eol == string::npos || bufLine.empty())
         break;

      DBRow row;
      // force a comma at the end
      bufLine += ',';
      size_t n = bufLine.length();
      string buf;
      for (size_t i = 0; i < n; i++) {
         if (bufLine[i] == ',') {
            int num;
            if (myStr2Int(buf, num))
               row.addData(num);
            else
               row.addData(INT_MAX);
            buf.clear();
         } else {
            buf += bufLine[i];
         }
      }
      t.addRow(row);
   }

   return ifs;
}

/*****************************************/
/*   Member Functions for class DBRow    */
/*****************************************/
void
DBRow::removeCell(size_t c)
{
   // TODO
   _data.erase(_data.begin() + c);
}

/*****************************************/
/*   Member Functions for struct DBSort  */
/*****************************************/
bool
DBSort::operator() (const DBRow& r1, const DBRow& r2) const
{
   // TODO: called as a functional object that compares the data in r1 and r2
   //       based on the order defined in _sortOrder
   const vector<size_t>& _so = _sortOrder;
   const size_t n = _so.size();
   for (size_t i = 0; i < n; i++) {
      const int& a = r1[_so[i]];
      const int& b = r2[_so[i]];
      if (a != b) return a < b;
   }
   return false;
}

/*****************************************/
/*   Member Functions for class DBTable  */
/*****************************************/
void
DBTable::reset()
{
   // TODO
   _table.clear();
}

void
DBTable::addCol(const vector<int>& d)
{
   // TODO: add a column to the right of the table. Data are in 'd'.
   const size_t n = _table.size();
   for (size_t i = 0; i < n; i++) {
      _table[i].addData(d[i]);
   }
}

void
DBTable::delRow(int c)
{
   // TODO: delete row #c. Note #0 is the first row.
   _table.erase(_table.begin() + c);
}

void
DBTable::delCol(int c)
{
   // delete col #c. Note #0 is the first row.
   const size_t n = _table.size();
   for (size_t i = 0; i < n; ++i)
      _table[i].removeCell(c);
}

// For the following getXXX() functions...  (except for getCount())
// - Ignore null cells
// - If all the cells in column #c are null, return NAN
// - Return "float" because NAN is a float.
float
DBTable::getMax(size_t c) const
{
   // TODO: get the max data in column #c
   float rtn = NAN;
   const size_t n = nRows();
   for (size_t i = 0; i < n; i++) {
      if (_table[i][c] == INT_MAX) continue;
      if (isnan(rtn) || rtn < _table[i][c])
         rtn = _table[i][c];
   }
   return rtn;
}

float
DBTable::getMin(size_t c) const
{
   // TODO: get the min data in column #c
   float rtn = NAN;
   const size_t n = nRows();
   for (size_t i = 0; i < n; i++) {
      if (_table[i][c] == INT_MAX) continue;
      if (isnan(rtn) || rtn > _table[i][c])
         rtn = _table[i][c];
   }
   return rtn;
}

float
DBTable::getSum(size_t c) const
{
   // TODO: compute the sum of data in column #c
   float rtn = NAN;
   const size_t n = nRows();
   for (size_t i = 0; i < n; i++) {
      if (_table[i][c] == INT_MAX)
         continue;
      if (isnan(rtn)) rtn = 0;
      rtn += _table[i][c];
   }
   return rtn;
}

int
DBTable::getCount(size_t c) const
{
   // TODO: compute the number of distinct data in column #c
   // - Ignore null cells
   set<int> pool;
   const size_t n = nRows();
   for (size_t i = 0; i < n; i++) {
      if (_table[i][c] != INT_MAX)
         pool.insert(_table[i][c]);
   }
   return pool.size();
}

float
DBTable::getAve(size_t c) const
{
   // TODO: compute the average of data in column #c
   // we can know whether the cells are all null,
   // so no need to initialize it to NAN
   float rtn = 0;
   int cnt = 0;
   for (size_t i = 0, n = nRows(); i < n; i++) {
      if (_table[i][c] != INT_MAX){
         rtn += _table[i][c];
         cnt++;
      }
   }

   if (!cnt)
      return NAN;
   return rtn / cnt;
}

void
DBTable::sort(const struct DBSort& s)
{
   // TODO: sort the data according to the order of columns in 's'
   std::sort(_table.begin(), _table.end(), s);
}

void
DBTable::printCol(size_t c) const
{
   // TODO: to print out a column.
   // - Data are seperated by a space. No trailing space at the end.
   // - Null cells are printed as '.'
   for (size_t i = 0, n = _table.size(); i < n; i++) {
      if (i != 0) cout << ' ';
      printData(cout, _table[i][c]);
   }
}

void
DBTable::printSummary() const
{
   size_t nr = nRows(), nc = nCols(), nv = 0;
   for (size_t i = 0; i < nr; ++i)
      for (size_t j = 0; j < nc; ++j)
         if (_table[i][j] != INT_MAX) ++nv;
   cout << "(#rows, #cols, #data) = (" << nr << ", " << nc << ", "
        << nv << ")" << endl;
}

