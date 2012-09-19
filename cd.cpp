
#include "cd.h"

istream & operator >> (istream & in, cd & rec)
{
  in.getline(rec.upc, 12, '\n');
  in.getline(rec.artist, 23, '\n');
  in.getline(rec.title, 35, '\n');
  rec.upc[12] = 0;
  rec.artist[23] = 0;
  rec.title[35] = 0;
  rec.upclen = strlen(rec.upc);
  rec.artistlen = strlen(rec.artist);
  rec.titlelen = strlen(rec.title);
  return in;
}

ostream & operator << (ostream & out, const cd & rec)
{
  out << rec.upc << '|' << rec.artist << '|' << rec.title << '|';
  return out;
}

cd::cd()
{
  upclen = 0xFFFFFFFF;
  artistlen = 0xFFFFFFFF;
  titlelen = 0xFFFFFFFF;
}

cd::cd(const cd &src)
{
  if(src.upclen == 0xFFFFFFFF || src.artistlen == 0xFFFFFFFF ||
     src.titlelen == 0xFFFFFFFF)
    {
      upclen = 0xFFFFFFFF;
      artistlen = 0xFFFFFFFF;
      titlelen = 0xFFFFFFFF;
      return;
    }
  memcpy(upc, src.upc, src.upclen);
  memcpy(artist, src.artist, src.artistlen);
  memcpy(title,  src.title, src.titlelen);
  upclen = src.upclen;
  artistlen = src.artistlen;
  titlelen = src.titlelen;
  upc[upclen] = 0;
  artist[artistlen] = 0;
  title[titlelen] = 0;
  upc[12] = 0;
  artist[23] = 0;
  title[35] = 0;
}

cd::cd(const char u[13], const char a[23], const char t[35])
{
  strcpy(upc, u);
  strcpy(artist, a);
  strcpy(title, t);
  upclen = strlen(upc);
  artistlen = strlen(artist);
  titlelen = strlen(title);
}

unsigned cd::recordsize() const
{
  return upclen + artistlen + titlelen;
}

string cd::getupc() const
{
  return string(upc);
}

string cd::getauthor() const
{
  return string(artist);
}

string cd::gettitle() const
{
  return string(title);
}

cd cd::operator=(const cd &rhs)
{
  upclen = rhs.upclen;
  artistlen = rhs.artistlen;
  titlelen = rhs.titlelen;
  strcpy(upc, rhs.upc);
  strcpy(title, rhs.title);
  strcpy(artist, rhs.artist);
  return *this;
}

bool cd::operator==(const char *cmp) const
{
  return !memcmp(upc, cmp, upclen);
}

bool cd::operator==(const cd &cmp) const
{
  return !memcmp(upc, cmp.upc, UPCLEN);
}

bool cd::operator!=(const cd &cmp) const
{
  return memcmp(upc, cmp.upc, UPCLEN);
}

bool cd::operator!=(const char *cmp) const
{
  return memcmp(upc, cmp, UPCLEN);
}

bool cd::operator>(const cd &cmp) const
{
  return (memcmp(upc, cmp.upc, UPCLEN) > 0);
}

bool cd::operator>(const char *cmp) const
{
  return (memcmp(upc, cmp, UPCLEN) > 0);
}

bool cd::operator<(const cd &cmp) const
{
  return (memcmp(upc, cmp.upc, UPCLEN) < 0);
}

bool cd::operator<(const char *cmp) const
{
  return (memcmp(upc, cmp, UPCLEN) < 0);
}

bool cd::operator>=(const cd &cmp) const
{
  return (memcmp(upc, cmp.upc, UPCLEN) >= 0);
}

bool cd::operator>=(const char *cmp) const
{
  return (memcmp(upc, cmp, UPCLEN) >= 0);
}

bool cd::operator<=(const cd &cmp) const
{
  return (memcmp(upc, cmp.upc, UPCLEN) <= 0);
}

bool cd::operator<=(const char *cmp) const
{
  return (memcmp(upc, cmp, UPCLEN) <= 0);
}

cd::operator bool() const
{
  return upclen == 0xFFFFFFFF;
}
