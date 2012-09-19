
#ifndef _BTREE_H_
#define _BTREE_H_

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <set>
#include "cd.h"

using namespace std;

#define BTREE_FILESIG "BTREEFILE"
#define BTREE_SIGLEN 9

/* Possible direction: Add Huffmann coding to the read and write functions;
 * with a Huffmann table at the top of the file. */

class btree
{
 public:
  btree(const char *file, size_t order);
  ~btree();
  bool add(const cd &);
  bool remove(const char *uid);
  cd search(const char *uid);
  void print();
  unsigned height();
  unsigned writecnt();
  unsigned readcnt();
 private:
  struct btnode
  {
    size_t size;
    cd *elems;
    unsigned int *indices;
  } btroot;
  struct split
  {
    bool issplit;
    cd up;
    unsigned int index;
    btnode rnode;
  };
  enum removestate
  {
	  DONE,
	  INVALID
  };
  int findindex(const struct btnode &, const cd &) const;
  struct split add_helper(const cd &, struct btnode &);
  /*add_key should only be used for leaves */
  struct split add_key(const cd &, struct btnode &);
  /*add_split can be used for keys, but is slower */
  struct split add_split(const struct split &, struct btnode &);
  removestate remove_helper(const char *, struct btnode &);
  removestate remove_key(const char *, struct btnode &, unsigned int pos);
  void roll_left(struct btnode &, unsigned int elnum);
  void roll_right(struct btnode &, unsigned int elnum);
  void print_helper(const struct btnode &);
  void readnode(struct btnode &node, int pos);
  void writenode(const struct btnode &node, int pos);
  size_t order;
  fstream dbase;
  unsigned wrtcnt, rdcnt;
  char *fname;
  size_t rootpos;
  void printsplit(const struct split &data);
  void printnode(const struct btnode &node) const;
};

#endif
