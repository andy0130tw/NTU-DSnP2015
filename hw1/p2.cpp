#include<cstdio>
#include<iostream>
#include<iomanip>
#include<algorithm>
#include<vector>
#define UNDEF 9999
using namespace std;

class Data {
public:
  Data(size_t s) {
    _cols = new int[s];
  }
  const int  operator[] (size_t i) const { return _cols[i]; }
        int& operator[] (size_t i)       { return _cols[i]; }
private:
  int *_cols;
};

struct SortData {
  bool operator() (const Data& d1, const Data& d2) {
    int a, b;
    for (int i = 0; i < _sortOrder.size(); i++) {
      a = d1[_sortOrder[i]];
      b = d2[_sortOrder[i]];
      if (a != b) return a < b;
    }
    return false;
  }
  void pushOrder(size_t i) {
    _sortOrder.push_back(i);
  };
  vector<size_t> _sortOrder;
};

int calculateSum(vector<Data> table, int col) {
  int sum = 0;
  int num;
  for (int i = 0; i < table.size(); i++) {
    num = table[i][col];
    sum += (num == UNDEF ? 0 : num);
  }
  return sum;
}

double calculateAvg(vector<Data> table, int col) {
  int sum = 0, cnt = 0;
  int num;
  for (int i = 0; i < table.size(); i++) {
    num = table[i][col];
    if (num != UNDEF) {
      sum += num;
      cnt++;
    }
  }
  return 1.0 * sum / cnt;
}

int calculateMinMax(vector<Data> table, int col, int isMax) {
  int ans = isMax ? -UNDEF : UNDEF;
  int num;
  for (int i = 0; i < table.size(); i++) {
    num = table[i][col];
    if (num != UNDEF && ((isMax && ans < num) || (!isMax && ans > num)))
      ans = num;
  }
  return ans;
}

int calculateCnt(vector<Data> table, int col) {
  int sorted[table.size()], p = 0;
  for (int i = 0; i < table.size(); i++) {
    if (table[i][col] != UNDEF)
      sorted[p++] = table[i][col];
  }
  sort(sorted, sorted + p);

  int cnt = 1;
  for (int i = 0; i < p - 1; i++) {
    if (sorted[i] != sorted[i + 1])
      cnt++;
  }
  return cnt;
}

int main() {
  vector<Data> table;
  int n, m;
  string filename;
  cout << "Please enter the file name: ";
  cin >> filename;
  cout << "Please enter the number of rows and columns: ";
  cin >> n >> m;

  int num = UNDEF;
  int sign = 1;
  int j;
  char tmp;
  int flg;
  
  FILE* f = fopen(filename.c_str(), "r");
  
  for (int i = 0; i < n; i++) {
    Data row(m);
    j = 0;
    flg = 0;
    table.push_back(row);
    while (1) {
      flg = fscanf(f, "%c", &tmp);
      
      // a workaround to force a line break at EOF
      if (flg == EOF)
        tmp = '\n';
      
      if (tmp >= '0' && tmp <= '9') {
        if (num == UNDEF)
          num = 0;
        num = num * 10 + (tmp - '0');
      } else if (tmp == '-') {
        sign = -1;
      } else if (j > 0 || tmp == ',') {
        row[j++] = num * sign;
        num = UNDEF;
        sign = 1;
        if (tmp == '\n' || tmp == '\r') {
          break;
        }
      }

      if (flg == EOF)
        break;
    }
  }

  fclose(f);
  cout << "File \"" << filename << "\" was read in successfully.\n";

  string cmd;
  while (cin >> cmd) {
    if (cmd == "PRINT") {
      for (int i = 0; i < table.size(); i++) {
        for (int j = 0; j < m; j++) {
          num = table[i][j];
          if (num == UNDEF)
            cout << "    ";
          else
             cout << setw(4) << right << num;
        }
        cout << endl;
      }
    } else if (cmd == "ADD") {
      Data row(m);
      table.push_back(row);
      for (int i = 0; i < m; i++) {
        string str;
        num = 0;
        sign = 1;
        cin >> str;
        if (str == "-")
          num = UNDEF;
        else {
          for (int c = 0; c < str.size(); c++) {
            if (str[c] == '-')
              sign = -1;
            else
              num = num * 10 + (str[c] - '0');
          }
        }
        row[i] = num * sign;
      }
    } else if (cmd == "SUM") {
      cin >> num;
      cout << "The summation of data in column #"
           << num << " is " << calculateSum(table, num) << "." << endl;
    } else if (cmd == "AVE") {
      cin >> num;
      cout << "The average of data in column #"
           << num << " is " << calculateAvg(table, num) << "." << endl;
    } else if (cmd == "MAX") {
      cin >> num;
      cout << "The maximum of data in column #"
           << num << " is " << calculateMinMax(table, num, 1) << "." << endl;
    } else if (cmd == "MIN") {
      cin >> num;
      cout << "The minimum of data in column #"
           << num << " is " << calculateMinMax(table, num, 0) << "." << endl;
    } else if (cmd == "COUNT") {
      cin >> num;
      cout << "The distinct count of data in column #"
           << num << " is " << calculateCnt(table, num) << "." << endl;
    } else if (cmd == "SORT") {
      string fields;
      getline(cin, fields);
      SortData cmpfn;
      num = -1;
      for(int i = 0; i < fields.size(); i++) {
        if (fields[i] >= '0' && fields[i] <= '9') {
          if (num < 0)
            num = 0;
          num = num * 10 + fields[i] - '0';
        } else if (num >= 0) {
          cmpfn.pushOrder(num);
          num = -1;
        }
      }
      if (num >= 0)
        cmpfn.pushOrder(num);
      sort(table.begin(), table.end(), cmpfn);
    } else {
      cout << "UNRECOGNIZED: " << cmd << endl;
    }
  }
}

