// FILE MODDATA.H: Declaration of class moddata
//////////////////////////////////////////////////////////////////////////
//
// Copyright 1990-2007 John Cremona
// 
// This file is part of the mwrank/g0n package.
// 
// mwrank is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
// 
// mwrank is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
// 
// You should have received a copy of the GNU General Public License
// along with mwrank; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
// 
//////////////////////////////////////////////////////////////////////////

#include <fstream>
#include "rat.h"
#include "xsplit.h"

class level {
  friend class symb;
  friend class cusplist;
  friend class oldforms;
  friend class eigdata;  
  friend class newform;  
  friend class h1newform;  
  friend class summer;  
//protected:
public:     // got tired of making everything friends...
 long modulus;
 int plusflag; int squarelevel;
 vector<long> plist,dlist,primelist;
 long p0;  // first good prime
 long npdivs,ndivs,sqfac,nap;
 long reduce(long res)const {res%=modulus; return (res<0)?modulus+res:res;}
public:
 level(long n, long neigs=20);
};

class moddata :public level {
protected:
 long phi,psi,nsymb1,nsymb2;
 vector<long> invlist,noninvlist,noninvdlist,dstarts,gcdtable,unitdivlist;
 long code(long res) const {return invlist[reduce(res)];}
public:
  long chi_top, chi_modulus;      // This is the top number and conductor of the quadratic character.
  vector<long> chi_table;         // This stores the values of the quadratic character!
  long chi(long res) const;       // return the value chi(r)
  long u;                         // A residue with chi(u)=-1, when chi is nontrivial
  vector<long> gamma_u;           // Entries of a matrix [a,b;N,u] in Gamma_0(N)
  long nsymb;
 moddata(long n, long chi_d=1);                                //constructor
 void init_quadchar(long chi_d);   // Initialize the quadratic character
 void display_chitable() const;
 void display() const;
 void abort(const char* mess) const 
  {
    cout<<"Out of memory ("<<mess<<")\n";  
    ::abort();
  }
 long gcd(long res) const {return gcdtable[reduce(res)];}
 long unitdiv(long res) const {return unitdivlist[reduce(res)];}
};
