
#ifndef _CD_H_
#define _CD_H_

#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

#define UPCLEN 13
#define ARTISTLEN 24
#define TITLELEN 36

class cd
{
 public:
  cd();
  cd(const cd &src);
  cd(const char upc[12], const char artist[23], const char title[35]);
  friend ostream & operator << (ostream &, const cd &);
  friend istream & operator >> (istream &, cd &);
  unsigned recordsize() const;
  string getupc() const;
  string gettitle() const;
  string getauthor() const;
  cd operator=(const cd &rhs);
  bool operator==(const char *cmp) const;
  bool operator==(const cd &cmp) const;
  bool operator!=(const char *cmp) const;
  bool operator!=(const cd &cmp) const;
  bool operator>(const cd &cmp) const;
  bool operator>(const char *cmp) const;
  bool operator<(const cd &cmp) const;
  bool operator<(const char *cmp) const;
  bool operator>=(const cd &cmp) const;
  bool operator>=(const char *cmp) const;
  bool operator<=(const cd &cmp) const;
  bool operator<=(const char *cmp) const;
  operator bool() const;
 private:
  char upc[UPCLEN];
  char artist[ARTISTLEN];
  char title[TITLELEN];
  unsigned upclen;
  unsigned artistlen;
  unsigned titlelen;
};

#endif
