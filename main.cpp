
#include "btree.h"
#include "cd.h"
#include <unistd.h>
#include <stdio.h>
#include <fstream>

using namespace std;

#define DEFORDER 5

enum dbase_state
  {
    CREATE,
    TRANSACT,
    LIST,
    NA
  };

void version();
void help();
int getargs(int argc, char **argv, char **dbase,
	    char **file, dbase_state *state);
int makedbase(char *dbase, char *master);
void listdbase(char *dbase);
void moddbase(char *dbase, char *trans);

int main(int argc, char **argv)
{
  char *dbase, *file;
  dbase_state state;
  getargs(argc, argv, &dbase, &file, &state);
  switch(state)
    {
    case CREATE:
	unlink(dbase);
	makedbase(dbase, file);
      break;
    case TRANSACT:
      moddbase(dbase, file);
      break;
    case LIST:
      listdbase(dbase);
      break;
    }
  return 0;
}

void moddbase(char *dbfile, char *transfile)
{
  btree db(dbfile, DEFORDER);
  ifstream trans(transfile);
  while(trans)
    {
      char cmd;
      string uid;
      cmd = 0;
      getline(trans, uid);
      cmd = uid[0];
      if(!trans)
	break;
      uid = "";
      getline(trans, uid);
      if(uid.length() > 12)
	uid[12] = 0;
      cout << "Processing " << cmd << ' ' << uid << '\n';
      switch(cmd | 32)
	{
	case 'i':
	  {
	    string ttl, aut;
	    getline(trans, aut);
	    getline(trans, ttl);
	    if(aut.length() > 24)
	      aut[24] = 0;
	    if(ttl.length() > 35)
	      ttl[35] = 0;
	    cd dat(uid.c_str(), aut.c_str(), ttl.c_str());
	    cout << "  Size: " << dat.recordsize() << " Artist: "
		 << aut.c_str() << " Title: " << ttl.c_str() << '\n';
	    db.add(dat);
	  }
	  break;
	case 's':
	  {
	    cd rec;
	    try
	      {
		rec = db.search(uid.c_str());
	      }
	    catch(int err)
	      {
		cout << '"' << uid << "\" not found\n";
		break;
	      }
	    cout << rec.getupc() << '|' << rec.getauthor() << '|'
		 << rec.gettitle() << "|\n";
	  }
	  break;
	case 'd':
	  {
	    try
	      {
		db.print();
		cout << "Deleting " << uid << '\n';
		db.remove(uid.c_str());
	      }
	    catch(int err)
	      {
		cout << '"' << uid << "\" not found; not deleted\n";
	      }
	  }
	  break;
	}
    }
  db.print();
}

void listdbase(char *dbfile)
{
  btree db(dbfile, DEFORDER);
  db.print();
}

int makedbase(char *dbfile, char *master)
{
  btree db(dbfile, DEFORDER);
  fstream mf(master, ios::in);
  cd buf;
  while(mf)
    {
      mf >> buf;
      cout << buf << '\n';
      if(buf.getupc().length())
	db.add(buf);
    }
  return 0;
}

int getargs(int argc, char **argv, char **dbase,
	    char **file, dbase_state *state)
{
  int i;
  bool prevset = false;
  *state = TRANSACT;
  *dbase = argv[1];
  *file = NULL;
  for(i = 2; i < argc; i++)
    {
      if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
	{
	  version();
	  exit(0);
	}
      if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
	{
	  help();
	  exit(0);
	}
      if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--master"))
	{
	  if(*state == TRANSACT)
	    {
	      prevset = true;
	      *state = CREATE;
	    }
	  else
	    {
	      printf("Unable to execute muliple commands!\n");
	      exit(1);
	    }
	  continue;
	}
      if(!strcmp(argv[i], "-l") || !strcmp(argv[i], "--list"))
	{
	  if(*state == TRANSACT)
	    {
	      *state = LIST;
	    }
	  else
	    {
	      printf("Unable to execute muliple commands!\n");
	      exit(1);
	    }
	  continue;
	}
      if(argv[i][0] == '-')
	{
	  printf("Unrecognized option: %s. Ignoring\n", argv[i]);
	}
      else
	{
	  *file = argv[i];
	  prevset = false;
	}
    }
  return 0;
}

void version()
{
  printf("CD Database with BTree version 0.0.0.0\n");
}

void help()
{
  printf("dbase [-(m, l)] DBASE FILE\n  -m FILE Create a new database with the "
	 "given master file.\n  -l Lists the current contents of the database.\n"
	 "  Defaults to evaluating a transaction file.\n");
}

