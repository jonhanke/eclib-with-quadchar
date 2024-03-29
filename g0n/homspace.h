// FILE HOMSPACE.H: Declaration of class homspace
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

#include "svector.h"
class mat22;  // fully defined below
class matop;  // fully defined below

class homspace :public symbdata {
  //private:
public:
  int *coordindex,*needed,*freegens;
  long rk,denom1,denom2;
  ssubspace kern; // kernel(delta) basis
  smat tkernbas; // transpose of kernel(delta) basis
  modsym *freemods;
public:
  vector<svec> coord_vecs;
  mat projcoord; // # cols = # newforms after they are found
  long dimension, denom3, ncusps, ncusps2;
  int cuspidal;  // if 1 then compute cuspidal homology
public:
  // Constructor (does all the work):
  homspace(long n, // the level
	   long char_top, // the character
	   int hp, // plus-space flag (0 or 1 or -1)
	   int hcusp, // cuspidal flag (0 or 1)
	   int verbose // verbosity (0 : no output
	               //	     1 : basic short output
	               //            2 : lots of detail)
	   );
  ~homspace();
  long h1cuspdim() const {return dim(kern);}
  long h1dim() const {return dimension;}  // No confusion with subspace::dim
  long h1denom() const {return denom1;}
  long h1cdenom() const {return denom3;}
  long h1ncusps() const {return ncusps;}
  vector<long> eigrange(long i);
  long op_prime(int i);  // the i'th operator prime for Tp or Wq
  mat opmat(int i, int dual, int verb=0);
  mat opmat_restricted(int i,const subspace& s, int dual, int verb=0);
  // versions returning an smat:
  smat s_opmat(int i,int dual,int verb=0);
  smat s_opmat_restricted(int i,const ssubspace& s, int dual,int verb=0);

  // Extend a dual vector of length rk to one of length nsymb:
  vec extend_coords(const vec& v);
  // Contract a dual vector of length nsymb to one of length rk: 
  vec contract_coords(const vec& v);

public:
  // The next functions express M- & modular symbols in terms of the
  // basis for H_1(X_0(N);cusps;Z) of dimension rk
  svec chain(const symb& s) const;
  void add_chain(svec& v, const symb& s) const;
  svec chaincd(long c, long d) const;
  void add_chaincd(svec& v, long c, long d) const;

  svec chain(long nn, long dd) const;
  svec chain(const rational& r) const 
  {return chain(num(r),den(r));}
  void add_chain(svec& v, long nn, long dd) const;
  void add_chain(svec& v, const rational& r) const 
  {add_chain(v,num(r),den(r));}

  // The next functions express M- & modular symbols in terms of the
  // basis of a (dual) subspace of the whole space
  // The "projcoord" default (which requires the initialization of the
  // projcoord matrix as done in  newforms::createfromscratch()) which
  // has one column for each newform
  // NB these operate on vecs and not svecs
  // The "nf" versions give scalars and instead of a matrix take a
  // vector = unique column of a matrix, such as a newform's coordplus
  // vector.
  vec projchaincd(long c, long d, const mat& m) const;
  vec projchaincd(long c, long d) const {return projchaincd(c,d,projcoord);}
  long nfprojchaincd(long c, long d, const vec& bas) const;
  void add_projchaincd(vec& v, long c, long d, const mat& m) const;
  void add_projchaincd(vec& v, long c, long d) const {add_projchaincd(v,c,d,projcoord);}
  void add_nfprojchaincd(long& a, long c, long d, const vec& bas) const;

  vec projchain(long n, long d, const mat& m) const;
  vec projchain(long n, long d) const  {return projchain(n,d,projcoord);}
  long nfprojchain(long n, long d, const vec& bas) const;

  void add_projchain(vec& v, long n, long d, const mat& m) const;
  void add_projchain(vec& v, long n, long d) const {add_projchain(v,n,d,projcoord);}
  void add_nfprojchain(long& aa, long n, long d, const vec& bas) const;

  vec cuspidalpart(const vec& v) const 
  {return v[pivots(kern)];}

#if(0) //no longer used
  vec cycle(long n, long d) const 
  {
    vec v = chain(n,d).as_vec(); 
    if (cuspidal) return cuspidalpart(v); else return v;
  }
  vec cycle(const rational& r) const 
  {
    vec v = chain(num(r),den(r)).as_vec(); 
    if (cuspidal) return cuspidalpart(v); else return v;
  }
  vec cycle(const modsym& m) const 
  {
    vec v = (chain(m.beta())-chain(m.alpha())).as_vec();
    if (cuspidal) return cuspidalpart(v); else return v;
  }
#endif // no longer used

  svec applyop(const matop& mlist, const rational& q) const;
  svec applyop(const matop& mlist, const modsym& m) const 
  {return applyop(mlist,m.beta())-applyop(mlist,m.alpha());} 

  mat calcop(string opname, long p, const matop& mlist, int dual, int display=0) const;
  mat calcop_restricted(string opname, long p, const matop& mlist, const subspace& s, int dual, int display=0) const;

  smat s_calcop(string opname, long p, const matop& mlist, int dual, int display=0) const;
  smat s_calcop_restricted(string opname, long p, const matop& mlist, const ssubspace& s, int dual, int display=0) const;

public:

  mat heckeop(long p, int dual, int display=0) const;
  mat heckeop_restricted(long p, const subspace& s, int dual, int display=0) const;

  smat s_heckeop(long p, int dual, int display=0) const;
  smat s_heckeop_restricted(long p, const ssubspace& s, int dual, int display=0) const;

  mat newheckeop(long p, int dual, int display=0) const;
  mat wop(long q, int dual, int display=0) const;
  smat s_wop(long q, int dual, int display=0) const;
  mat fricke(int dual, int display=0) const;
  mat conj(int dual,int display=0) const;
  mat conj_restricted(const subspace& s, int dual,int display=0) const;
  smat s_conj(int dual, int display=0) const;
  smat s_conj_restricted(const ssubspace& s, int dual, int display=0) const;
  vec maninvector(long p) const;
  vec projmaninvector(long p) const;
  vec projmaninvector(long p, const mat& m) const;
  vec manintwist(long p) const;
  vec newhecke(long p, long n, long d) const;
  //
  friend class jumps;
  friend class newforms;
};

class mat22 {  //2x2 matrix for linear fractional transformations
  friend class homspace;
private: 
  long a,b,c,d;
public: 
  mat22(long ia=0, long ib=0, long ic=0, long id=0) 
    :a(ia),b(ib),c(ic),d(id){;}
  mat22(const mat22& m) :a(m.a),b(m.b),c(m.c),d(m.d) 
  {;}
  void operator=(const mat22& m) 
  {a=m.a; b=m.b; c=m.c; d=m.d;}
  void show(ostream& s) const
  {s << "[" << a << "," << b << ";"<< c << "," << d << "]";}
  rational operator()(const rational& q)const 
  {
    long n=num(q),de=den(q); 
    return rational(a*n+b*de,c*n+d*de);
  }
  svec operator()(const symb& s, const homspace* h)const 
  {
    long u=s.ceered(),v=s.deered(); 
    return h->chaincd(a*u+c*v,b*u+d*v);
  }
  vec operator()(const symb& s, const homspace* h, const mat& m)const 
  {
    long u=s.cee(),v=s.dee(); 
    return h->projchaincd(a*u+c*v,b*u+d*v,m);
  }
};

class matop {  // formal sum of 2x2 matrices
private: 
	vector<mat22> mats;
public: 
  matop(long p, const moddata *N);   // constructor for hecke ops
  matop(long p);           // constructor for heilbronn matrices
  matop(long a, long b, long c, long d);  // constructor for a single matrix
  long size() const {return mats.size();}
  mat22 operator[](long i) const {return mats[i];}
  friend matop degen_mat(long d);
};

inline matop degen_mat(long d)
{
  return matop(d,0,0,1);
}

inline ostream& operator<< (ostream& s, const mat22& m)
{
   m.show(s);
   return s;
}  

