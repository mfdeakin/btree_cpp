
#include "btree.h"

btree::btree(const char *fname, size_t order)
{
  if(!fname)
    throw 1;
  this->fname = new char[strlen(fname) + 1];
  strcpy(this->fname, fname);
  wrtcnt = 0;
  rdcnt = 0;
  btroot.elems = new cd[order - 1];
  btroot.indices = new unsigned int[order];
  dbase.open(fname, ios::in | ios::out | ios::binary);
  if(!dbase)
    {
      dbase.open(fname, ios::in | ios::out | ios::binary | ios::trunc);
      dbase << BTREE_FILESIG;
      dbase.write((char *)&order, sizeof(size_t));
      this->order = order;
      /* Write the location of the root node out */
      rootpos = BTREE_SIGLEN + 2 * sizeof(size_t);
      dbase.write((char *)&rootpos, sizeof(size_t));
      // cout << dbase.tellp() << '\n' << rootpos << '\n';
      /* Write out an empty root node */
      btroot.size = 0;
      memset(btroot.indices, 0, sizeof(int) * order);
      memset(btroot.elems, 0, sizeof(cd) * (order - 1));
      dbase.write((char *)&btroot.size, sizeof(size_t));
      dbase.write((char *)btroot.elems, sizeof(cd) * (order - 1));
      dbase.write((char *)btroot.indices, sizeof(int) * order);
    }
  else
    {
      char sigchk[BTREE_SIGLEN];
      dbase.read(sigchk, BTREE_SIGLEN);
      if(memcmp(sigchk, BTREE_FILESIG, BTREE_SIGLEN))
	throw 2;
      dbase.read((char *)&this->order, sizeof(size_t));
      dbase.read((char *)&rootpos, sizeof(size_t));
      dbase.seekg(rootpos, ios::beg);
      dbase.read((char *)&btroot.size, sizeof(size_t));
      dbase.read((char *)btroot.elems, sizeof(cd) * (order - 1));
      dbase.read((char *)btroot.indices, sizeof(int) * order);
    }
}

btree::~btree()
{
  delete []btroot.elems;
  delete []btroot.indices;
  delete []fname;
}

bool btree::remove(const char *uid)
{
  remove_helper(uid, btroot);
  writenode(btroot, rootpos);
  if(btroot.size == 0 && btroot.indices[0])
    {
      /* Only happens with a merge */
      rootpos = btroot.indices[0];
      readnode(btroot, rootpos);
      dbase.seekp(BTREE_SIGLEN + sizeof(size_t), ios::beg);
      dbase.write((char *)&rootpos, sizeof(int));
    }
  return false;
}

btree::removestate btree::remove_helper(const char *uid, btree::btnode &current)
{
  unsigned pos;
  for(pos = 0; pos < current.size; pos++)
    {
      if(current.elems[pos] > uid)
	break;
      if(current.elems[pos] == uid)
	{
	  return remove_key(uid, current, pos);
	}
    }
  if(!current.indices[0])
    return DONE;
  btree::btnode next;
  next.elems = new cd[order - 1];
  next.indices = new unsigned int[order];
  readnode(next, current.indices[pos]);
  if(remove_helper(uid, next) == INVALID)
    {
      writenode(next, current.indices[pos]);
      bool goleft = false, found = false;
      unsigned right = pos + 1;
      unsigned left = pos;
      btnode buf;
      buf.elems = new cd[order - 1];
      buf.indices = new unsigned[order];
      for(;;)
	{
	  if(goleft && left)
	    {
	      readnode(buf, current.indices[left - 1]);
	      if(buf.size > order / 2)
		{
		  found = true;
		  break;
		}
	      left--;
	    }
	  else if(right <= current.size)
	    {
	      readnode(buf, current.indices[right]);
	      if(buf.size > order / 2)
		{
		  found = true;
		  break;
		}
	      right++;
	    }
	  else
	    break;
	  goleft = !goleft;
	}
      if(found)
	{
	  if(!goleft)
	    {
	      while(left != pos)
		{
		  roll_right(current, left);
		  left++;
		}
	    }
	  else
	    {
	      while(right != pos)
		{
		  right--;
		  roll_left(current, right);
		}
	    }
	}
      else
	{
	  /* Need to merge two nodes */
	  /* Merge the two nodes regardless of the parent, and then worry about it */
	  readnode(buf, current.indices[pos]);
	  btnode combined;
	  combined.elems = new cd[order - 1];
	  combined.indices = new unsigned[order];
	  if(pos > 0)
	    {
	      readnode(combined, current.indices[pos - 1]);
	      combined.elems[combined.size] = current.elems[pos - 1];
	      combined.size++;
	      for(int i = 0; i < buf.size; i++)
		{
		  combined.elems[combined.size] = buf.elems[i];
		  combined.indices[combined.size] = buf.indices[i];
		  combined.size++;
		}
	      combined.indices[combined.size] = buf.indices[buf.size];
	      writenode(combined, current.indices[pos - 1]);
	      for(int i = pos - 1; i < current.size - 1; i++)
		{
		  current.elems[i] = current.elems[i + 1];
		  current.indices[i] = current.indices[i + 1];
		}
	      current.indices[current.size - 1] = current.indices[current.size];
	    }
	  else
	    {
	      readnode(combined, current.indices[1]);
	      buf.elems[buf.size] = current.elems[pos];
	      buf.size++;
	      for(int i = 0; i < combined.size; i++)
		{
		  buf.elems[buf.size] = combined.elems[i];
		  buf.indices[buf.size] = combined.indices[i];
		  buf.size++;
		}
	      buf.indices[buf.size] = combined.indices[combined.size];
	      writenode(buf, current.indices[0]);
	      for(int i = pos; i < current.size - 1; i++)
		{
		  current.elems[i] = current.elems[i + 1];
		  current.indices[i + 1] = current.indices[i + 2];
		}
	    }
	  current.size--;
	  delete []combined.elems;
	  delete []combined.indices;
	}
    }
  else
    writenode(next, current.indices[pos]);
  delete []next.elems;
  delete []next.indices;
  return current.size >= order/ 2 ? DONE : INVALID;
}

btree::removestate btree::remove_key(const char *uid, btree::btnode &node, unsigned int pos)
{
  if(!node.indices[0])
    {
      node.size--;
      for(int i = pos; i < node.size; i++)
	{
	  node.elems[i] = node.elems[i + 1];
	}
      node.elems[node.size] = cd();
      return node.size >= order / 2 ? DONE : INVALID;
    }
  else
    {
      /* We need to get to the bottom of one of the subtrees,
       * and swap the key we're deleting with one of the outer nodes */
      btree::btnode down;
      down.elems = new cd[order - 1];
      down.indices = new unsigned[order];
      /* Going down the right side of the left subtree */
      readnode(down, node.indices[pos]);
      while(down.indices[0])
	readnode(down, down.indices[down.size]);
      cd buf = down.elems[down.size - 1];
      down.elems[down.size - 1] = node.elems[pos];
      node.elems[pos] = buf;
      readnode(down, node.indices[pos]);
      remove_helper(uid, down);
      delete []down.elems;
      delete []down.indices;
      return DONE;
    }
}

void btree::roll_left(btree::btnode &node, unsigned pos)
{
  btnode left, right;
  left.elems = new cd[order - 1];
  left.indices = new unsigned[order];
  right.elems = new cd[order - 1];
  right.indices = new unsigned[order];
  readnode(left, node.indices[pos]);
  readnode(right, node.indices[pos + 1]);
  if(left.size < order - 1 && right.size > order / 2)
    {
      left.elems[left.size] = node.elems[pos];
      left.size++;
      left.indices[left.size] = right.indices[0];
      node.elems[pos] = right.elems[0];
      for(int i = 0; i < right.size - 1; i++)
	{
	  right.elems[i] = right.elems[i + 1];
	  right.indices[i] = right.indices[i + 1];
	}
      right.indices[right.size - 1] = right.indices[right.size];
      right.size--;
      writenode(left, node.indices[pos]);
      writenode(right, node.indices[pos + 1]);
    }
  delete []left.elems;
  delete []left.indices;
  delete []right.elems;
  delete []right.indices;
}

void btree::roll_right(btree::btnode &node, unsigned pos)
{
  btnode left, right;
  left.elems = new cd[order - 1];
  left.indices = new unsigned[order];
  right.elems = new cd[order - 1];
  right.indices = new unsigned[order];
  readnode(left, node.indices[pos]);
  readnode(right, node.indices[pos + 1]);
  if(right.size < order - 1 && left.size > order / 2)
    {
      for(int i = right.size; i; i--)
	{
	  right.elems[i] = right.elems[i - 1];
	  right.indices[i + 1] = right.indices[i];
	}
      right.indices[1] = right.indices[0];
      right.elems[0] = node.elems[pos];
      right.indices[0] = left.indices[left.size];
      right.size++;
      left.size--;
      node.elems[pos] = left.elems[left.size];
      writenode(left, node.indices[pos]);
      writenode(right, node.indices[pos + 1]);
    }
  delete []left.elems;
  delete []left.indices;
  delete []right.elems;
  delete []right.indices;
}

cd btree::search(const char *uid)
{
  btree::btnode current = btroot;
  for(;;)
    {
      int pos;
      for(pos = 0; pos < current.size; pos++)
	{
	  if(current.elems[pos] == uid)
	    return current.elems[pos];
	  if(current.elems[pos] > uid)
	    {
	      readnode(current, current.indices[pos]);
	      break;
	    }
	}
      if(pos == current.size)
	readnode(current, current.indices[pos]);
    }
  return cd();
}

bool btree::add(const cd &rec)
{
  btree::split uhoh = add_helper(rec, btroot);
  writenode(btroot, rootpos);
  if(uhoh.issplit)
    {
      writenode(uhoh.rnode, uhoh.index);
      /* Uh oh.
       * We need a new root */
      memset(btroot.indices, 0, sizeof(int[order]));
      memset(btroot.elems, 0, sizeof(cd[order - 1]));
      btroot.size = 1;
      btroot.elems[0] = uhoh.up;
      btroot.indices[0] = rootpos;
      btroot.indices[1] = uhoh.index;
      dbase.seekp(0, ios::end);
      rootpos = dbase.tellp();
      writenode(btroot, rootpos);
      dbase.seekp(BTREE_SIGLEN + sizeof(size_t), ios::beg);
      dbase.write((char *)&rootpos, sizeof(int));
    }
  return false;
}

btree::split btree::add_helper(const cd &rec, btree::btnode &current)
{
  if(current.indices[0])
    {
      /* Not a leaf, so we need to go deeper */
      struct btnode next;
      int currentpos = 0;
      for(int idx = 0; idx < current.size; idx++)
	{
	  if(rec < current.elems[idx])
	    {
	      currentpos = current.indices[idx];
	      break;
	    }
	}
      if(!currentpos)
	currentpos = current.indices[current.size];
      next.elems = new cd[order - 1];
      next.indices = new unsigned int[order];
      readnode(next, currentpos);
      btree::split help = add_helper(rec, next);
      writenode(next, currentpos);
      btree::split ret;
      ret.issplit = false;
      if(help.issplit)
	{
	  writenode(help.rnode, help.index);
	  ret = add_split(help, current);
	  delete []help.rnode.elems;
	  delete []help.rnode.indices;
	}
      delete []next.elems;
      delete []next.indices;
      return ret;
    }
  /* We are a leaf!!! */
  return add_key(rec, current);
}

btree::split btree::add_split(const btree::split &datas, btree::btnode &dest)
{
  struct btree::split ret;
  /* Do two things: First make sure the node isn't already in the array;
   * Second, find the location the next item will be added to. */
  int pos;
  for(pos = 0; pos < dest.size; pos++)
    {
      if(datas.up < dest.elems[pos])
	break;
      else if(dest.elems[pos] == datas.up)
	{
	  ret.issplit = false;
	  return ret;
	}
    }
  if(dest.size == order - 1)
    {
      ret.issplit = true;
      dbase.seekp(0, ios::end);
      ret.index = dbase.tellp();
      ret.rnode.elems = new cd[order - 1];
      ret.rnode.indices = new unsigned int[order];
      ret.rnode.size = 0;
      if(pos == order / 2)
	{
	  ret.up = datas.up;
	  ret.rnode.indices[0] = datas.index;
	  for(int idx = pos; idx < order - 1; idx++)
	    {
	      ret.rnode.elems[ret.rnode.size] = dest.elems[idx];
	      ret.rnode.indices[ret.rnode.size + 1] = dest.indices[idx + 1];
	      dest.elems[idx] = cd();
	      dest.indices[idx + 1] = 0;
	      dest.size--;
	      ret.rnode.size++;
	    }
	}
      else if(pos < order / 2)
	{
	  ret.up = dest.elems[order / 2 - 1];
	  ret.rnode.indices[0] = dest.indices[order / 2];
	  dest.indices[order / 2] = 0;
	  for(int idx = order / 2 - 1; idx > pos; idx--)
	    dest.elems[idx] = dest.elems[idx - 1];
	  dest.elems[pos] = datas.up;
	  for(int idx = order / 2; idx < order - 1; idx++)
	    {
	      ret.rnode.elems[ret.rnode.size] = dest.elems[idx];
	      ret.rnode.size++;
	      ret.rnode.indices[ret.rnode.size] = dest.indices[idx + 1];
	      dest.elems[idx] = cd();
	      dest.indices[idx + 1] = 0;
	      dest.size--;
	    }
	  dest.size--;
	}
      else
	{
	  ret.up = dest.elems[order / 2];
	  dest.elems[order / 2] = cd();
	  ret.rnode.indices[0] = dest.indices[order / 2 + 1];
	  dest.indices[order / 2 + 1] = 0;
	  bool added = false;
	  for(int idx = order / 2 + 1; ret.rnode.size < order / 2; ret.rnode.size++)
	    {
	      if(added || idx != pos)
		{
		  ret.rnode.elems[ret.rnode.size] = dest.elems[idx];
		  ret.rnode.indices[ret.rnode.size + 1] = dest.indices[idx + 1];
		  dest.elems[idx] = cd();
		  dest.indices[idx + 1] = 0;
		  dest.size--;
		  idx++;
		}
	      else
		{
		  ret.rnode.elems[ret.rnode.size] = datas.up;
		  ret.rnode.indices[ret.rnode.size + 1] = datas.index;
		  added = true;
		}
	    }
	  dest.size--;
	}
    }
  else
    {
      ret.issplit = false;
      for(int idx = dest.size; idx > pos; idx--)
	{
	  dest.elems[idx] = dest.elems[idx - 1];
	  dest.indices[idx + 1] = dest.indices[idx];
	}
      dest.indices[pos + 1] = dest.indices[pos];
      dest.elems[pos] = datas.up;
      dest.indices[pos + 1] = datas.index;
      dest.size++;
    }
  return ret;
}

btree::split btree::add_key(const cd &key, btree::btnode &dest)
{
  split ret;
  int pos;
  /* Do two things: First make sure the node isn't already in the array;
   * Second, find the location the next item will be added to. */
  for(pos = 0; pos < dest.size; pos++)
    {
      if(key < dest.elems[pos])
	{
	  break;
	}
      else if(key == dest.elems[pos])
	{
	  ret.issplit = false;
	  return ret;
	}
    }
  if(dest.size == order - 1)
    {
      ret.issplit = true;
      ret.rnode.elems = new cd[order - 1];
      ret.rnode.indices = new unsigned int[order];
      ret.rnode.indices[0] = 0;
      dbase.seekg(0, ios::end);
      ret.index = dbase.tellg();
      ret.rnode.size = 0;

      if(pos == order / 2)
	{
	  ret.up = key;
	  for(int idx = pos; idx < order - 1; idx++)
	    {
	      ret.rnode.elems[ret.rnode.size] = dest.elems[idx];
	      dest.elems[idx] = cd();
	      dest.size--;
	      ret.rnode.size++;
	    }
	}
      else if(pos < order / 2)
	{
	  ret.up = dest.elems[order / 2 - 1];
	  for(int idx = order / 2 - 1; idx > pos; idx--)
	    dest.elems[idx] = dest.elems[idx - 1];
	  dest.elems[pos] = key;
	  for(int idx = order / 2; idx < order - 1; idx++)
	    {
	      ret.rnode.elems[ret.rnode.size] = dest.elems[idx];
	      ret.rnode.size++;
	      dest.elems[idx] = cd();
	      dest.size--;
	    }
	  dest.size--;
	}
      else
	{
	  ret.up = dest.elems[order / 2];
	  dest.elems[order / 2] = cd();
	  bool added = false;
	  for(int idx = order / 2 + 1; ret.rnode.size < order / 2; ret.rnode.size++)
	    {
	      if(added || idx != pos)
		{
		  ret.rnode.elems[ret.rnode.size] = dest.elems[idx];
		  dest.elems[idx] = cd();
		  dest.size--;
		  idx++;
		}
	      else
		{
		  ret.rnode.elems[ret.rnode.size] = key;
		  added = true;
		}
	    }
	  dest.size--;
	}
    }
  else
    {
      ret.issplit = false;
      for(int idx = dest.size; idx > pos; idx--)
	dest.elems[idx] = dest.elems[idx - 1];
      dest.elems[pos] = key;
      dest.size++;
    }
  return ret;
}

void btree::readnode(btree::btnode &dest, int pos)
{
  dbase.seekg(pos, ios::beg);
  dbase.read((char *)&dest.size, sizeof(size_t));
  dbase.read((char *)dest.elems, sizeof(cd[order - 1]));
  dbase.read((char *)dest.indices, sizeof(int[order]));
  rdcnt++;
}

void btree::writenode(const btree::btnode &src, int pos)
{
  dbase.seekg(pos, ios::beg);
  dbase.write((char *)&src.size, sizeof(size_t));
  dbase.write((char *)src.elems, sizeof(cd[order - 1]));
  dbase.write((char *)src.indices, sizeof(int[order]));
  wrtcnt++;
}

void btree::print()
{
  print_helper(btroot);
}

void btree::print_helper(const btree::btnode &current)
{
  for(int i = 0; i < current.size && i < order; i++)
    {
      cout << current.elems[i] << '\n';
    }
  if(current.indices[0])
    {
      btree::btnode next;
      next.elems = new cd[order - 1];
      next.indices = new unsigned int[order];
      cout << "\n";
      for(int i = 0; i < current.size + 1; i++)
	{
	  readnode(next, current.indices[i]);
	  print_helper(next);
	  cout << '\n';
	}
      delete []next.indices;
      delete []next.elems;
    }
}

void btree::printsplit(const btree::split &data)
{
  if(!data.issplit)
    {
      cout << "Not a split\n";
      return;
    }
  cout << "Split data:\n";
  cout << data.up << '\n' << data.index << '\n';
  print_helper(data.rnode);
}

void btree::printnode(const btree::btnode &node) const
{
  cout << "node size: " << node.size << '\n';
  for(int i = 0; i < node.size && i < order - 1; i++)
    {
      cout << node.elems[i] << "   " << node.indices[i] << '\n';
    }
  cout << node.indices[node.size] << '\n';
}
