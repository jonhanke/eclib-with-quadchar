// FILE HOMSPACE.CC: Implemention of class homspace
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

#define USE_SMATS // Warning:  no longer testing without this switched on!

#include "moddata.h"
#include "symb.h"
#include "cusp.h"
#include "homspace.h"
#include "timer.h"

#ifdef USE_SMATS
#include "smatrix_elim.h"
#endif

const string W_opname("W");
const string T_opname("T");

matop::matop(long a, long b, long c, long d)
{
  mats.push_back(mat22(a,b,c,d));
}

matop::matop(long p, const moddata *N)
{
  long n = N->modulus;
  int eps = N->chi(p);
	
 if (p==n)
   {
     mats.push_back(mat22(0,-1,n,0));
     return;
   }
 if ((n%p)==0)   // W involution, 1 term
   {
     long u,v,a,b;
      for (u=1, v=n; v%p==0; v/=p, u*=p) ;
      bezout(u,v,a,b);
      mats.push_back(mat22(u*a,-b,n,u));
      return;
   }
 // else  Hecke operator, p+1 terms
  {
    mats.resize(p+1);
    long j, p2 = p>>1; 
    for (j=0; j<p; j++) {
      mats[j] = mat22(1,j-p2,0,p);
    }
    if ((N->u)==0) // trivial char
      mats[p] = mat22(p,0,0,1);
    else
      mats[p] = mat22(p*(N->gamma_u[0]),N->gamma_u[1],p*(N->gamma_u[2]),N->gamma_u[3]);
  }
}
 
// Constructed from the level n,
//  hp = plus space flag
//  hcusp = 1  => computes the cuspidal subspace
homspace::homspace(long n, long char_top, int hp, int hcusp, int verbose) :symbdata(n, char_top)
{ 
  init_time();
   plusflag=hp;
   cuspidal=hcusp;
   long i,j,k,k2,mk;
   coordindex = new int[nsymb];  
   if (!coordindex) abort(string("coordindex").c_str());
   int* check = new int[nsymb];  
   if (!check) abort(string("check").c_str());
   i=nsymb; while (i--) check[i]=0;
   long ngens=0;
   int* gens = new int[nsymb+1];    // N.B. Start of gens array is at 1 not 0
   if (!gens) abort(string("gens").c_str());
   long* rel = new long[4]; if (!rel) abort("rel");

// 2-term relations:

// if (plusflag==1)
//   for (j=0; j<nsymb; j++)
//   {if (check[j]==0)
//    { rel[0]=j;
//      rel[1]=rsof(j);
//      rel[2]=sof(j);
//      rel[3]=sof(rel[1]);
//      if (verbose>1)
//        cout << "Relation: " << rel[0]<<" "<<rel[1]<<" "<<rel[2]<<" "<<rel[3]<<endl;
//      for (k=0; k<4; k++) check[rel[k]]=1;
//      if ( (j==rel[2]) || (j==rel[3]) )
//          for (k=0; k<4; k++) coordindex[rel[k]]=0;
//      else
//      {   ngens++;
//          gens[ngens] = j;
//          if (verbose>1)  cout << "gens["<<ngens<<"]="<<j<<endl;
//          coordindex[rel[0]] =  ngens;
//          coordindex[rel[1]] =  ngens;
//          coordindex[rel[2]] = -ngens;
//          coordindex[rel[3]] = -ngens;
//      }
//      }
//    }

// Some new declarations	
int eps1, eps2, eps3;
long J0, J1, J2, J3;
vector<long> J1vec, J2vec, J3vec;
	
if (plusflag!=0)
  for (j=0; j<nsymb; j++)
  {if (check[j]==0)  // haven't dealt with this symbol yet
   { J0=j;           // rel givs the indices of the original four symbols to be identified
     if (plusflag==-1) 
       J1vec=rof_with_pm(j);
     else
       J1vec=rsof_with_pm(j);

	 J2vec=sof_with_pm(j);
	 J3vec=sof_with_pm(J1vec[0]);

	   J1 = J1vec[0];
	   J2 = J2vec[0];
	   J3 = J3vec[0];	   
	   eps1 = J1vec[1];
	   eps2 = J2vec[1];
	   eps3 = eps1 * J3vec[1];
	   
	   if (verbose>1)
       cout << "Relation: " << J0<<" "<< J1vec <<" "<< J2vec <<" "<< J3vec <<endl;
     //for (k=0; k<4; k++) check[rel[k]]=1;  // check that all four of these are handled

 	 // These are now handled!   
	   check[J0] = check[J1] = check[J2] = check[J3] = 1;
	   
	 // This is the case of torsion!
	   if (((J1 == J0) && (eps1 == -1)) || ((J2 == J0) && (eps2 == 1)) || ((J3 == J0) && (eps3 == 1)))
		   coordindex[J0] = coordindex[J1] = coordindex[J2] = coordindex[J3] = 0;
     else
     {   ngens++;
         gens[ngens] = j;
         if (verbose>1)  cout << "gens["<<ngens<<"]="<<j<<endl;
         coordindex[J0] =  ngens;
         coordindex[J1] =  eps1 * ngens;
         coordindex[J2] = -eps2 * ngens;
         coordindex[J3] = -eps3 * ngens;
     }
     }
   }

	
// Deal with the full (plus and minus) space
if (plusflag==0)
  {for (j=0; j<nsymb; j++)
   {if (check[j]==0)
    {J0=j;
     J1vec=sof_with_pm(j);
		J1 = J1vec[0];
		eps1 = J1vec[1];
		check[J0] = check[J1] = 1;
     // Test for symbol is zero
	 if ((J0 == J1) && (eps1 == 1))
		 coordindex[J0] = coordindex[J1] = 0;
     else
     {   ngens++;
         gens[ngens] = j;  // This is J0, but we write it as j here.
         coordindex[J0] =  ngens;
         coordindex[J1] = -eps1 * ngens;
     }
    }
   }
 }


// end of 2-term relations
if (verbose)
{cout << "After 2-term relations, ngens = "<<ngens<<"\n";
// Compare with predicted value:
/*
 int nu2=(::div((long)4,modulus)?0:1);
 static int nu2table[4] = {0,2,1,0};
 for(i=0; nu2&&(i<npdivs); i++)  nu2 *= nu2table[plist[i]%4];
 int ngens0=(nsymb-nu2)/2;
 cout<<"predicted value of ngens = "<<ngens0;
 if(!plusflag) if(ngens!=ngens0) cout<<" --WRONG!";
 cout<<endl;
*/
if (verbose>1)
  {
 cout << "gens = ";
 for (i=1; i<=ngens; i++) cout << gens[i] << " ";
 cout << "\n";
 cout << "coordindex = ";
 for (i=0; i<nsymb; i++) cout << coordindex[i] << " ";
 cout << "\n";
}}
//
// 3-term relations
 
//   long maxnumrel = 20+(2*ngens)/3;
//   long maxnumrel = ngens + 10;
	long maxnumrel = nsymb;  // This is an absolute upper bound for the number of 3-term relations!

   if (verbose)
     {
       cout << "ngens     = "<<ngens<<endl;
       cout << "maxnumrel = "<<maxnumrel<<endl;
       cout << "relation matrix has = "<<(maxnumrel*ngens)<<" entries..."<<endl;
     }
   {
#ifdef USE_SMATS    // This is what we do -- use sparse matrices!
   smat relmat((int)maxnumrel,(int)ngens);  // full relation matrix
   svec newrel(ngens);              // each new row of this matrix
#else
   mat relmat(maxnumrel,ngens);
   vec newrel(ngens);
   if (verbose) cout << "successfully allocated "<<endl;
#endif
   int numrel = 0;    // this is the number of rows we have filled in so far.
   long ij; int fix;

   for (i=0; i<nsymb; i++) check[i]=0;   //zero out the check array. (checked nothing so far)
   for (k=0; k<nsymb; k++) 
     {
     if (check[k]==0)
   {
#ifdef USE_SMATS
     newrel.clear();   // clear the current row.
#else
     for (j=1; j<=ngens; j++) newrel[j]=0;
#endif
	   J0 = k;

	   J1vec = tof_with_pm(J0);
	   J1 = J1vec[0];
	   eps1 = J1vec[1];

	   J2vec = tof_with_pm(J1);
	   J2 = J2vec[0];
	   eps2 = J2vec[1];	   
	   
     if (verbose>1)   cout << "Relation: " << J0 <<" "<< J1vec <<" "<< J2vec <<" --> ";

#ifdef USE_SMATS
	   // Add the contribution from the three signed symbols to the 3-term relation row
	   check[J0] = 1;
	   fix = coordindex[J0];
	   if (fix > 0)
		   newrel.add(fix, 1);
	   else if (fix < 0)
		   newrel.add(-fix, -1);

	   check[J1] = 1;
	   fix = coordindex[J1];
	   if (fix > 0)
		   newrel.add(fix, eps1);
		else if (fix < 0)
			newrel.add(-fix, -eps1);

	   check[J2] = 1;
	   fix = coordindex[J2];
	   if (fix > 0)
		   newrel.add(fix, eps1 * eps2);
	   else if (fix < 0)
		   newrel.add(-fix, -eps1 * eps2);
			   

	   
#else  // BIG WARNING: This has not been changed to work with characters yet!
	   if (verbose>1)  cout << fix << " ";
	   if (fix!=0) newrel[abs(fix)] += sign(fix);
#endif
		 
	   
	   
// ====================

#ifdef USE_SMATS
     if(verbose>1) cout << newrel <<"\n";
#else
     if(verbose>1) cout << "\n";
#endif

#ifdef USE_SMATS
     if(newrel.size()!=0) // Update the matrix for non-zero rows!
       {
	 numrel++;
	 make_primitive(newrel);  // divides out by the gcd (if there is any, but at most 3 here!).
	 if(numrel<=maxnumrel)
	   relmat.setrow(numrel,newrel);  // set this row of the matrix
	 else 
	   cout<<"Too many 3-term relations (numrel="<<numrel
	       <<", maxnumrel="<<maxnumrel<<")"<<endl;
       }
#else
     if (verbose>1)  cout << newrel << "\n";
     long h = vecgcd(newrel);
     if (h!=0)
     {  if (h>1) newrel/=h;
	if(numrel<maxnumrel)
	  relmat.setrow(++numrel,newrel);
	else cout<<"Too many 3-term relations!"<<endl;
     }
#endif
    }
     }
   if (verbose) 
     {
       cout << "Finished 3-term relations: numrel = "<<numrel<<" ( maxnumrel = "<<maxnumrel<<")"<<endl;
       // Compare with predicted value (for full H_1 only):
       /*
       if(!plusflag)
	 {
	   int nu3=(::div((long)9,modulus)?0:1);
	   static int nu3table[3] = {1,2,0};
	   for(i=0; nu3&&(i<npdivs); i++)  nu3 *= nu3table[plist[i]%3];
	   int ntriangles=(nsymb+2*nu3)/3;
	   cout<<"predicted value of ntriangles = "<<ntriangles;
	   if(ntriangles!=numrel) cout<<" --WRONG!";
	   cout<<endl;
	 }
       */
     }


// end of 3-term relations

   if(verbose) 
     {
#ifdef USE_SMATS
       cout << "relmat has "<< get_population(relmat)<<" nonzero entries (density = "<<density(relmat)<<")"<<endl;
#endif

       if(verbose>1) 
         cout << "relmat = " << relmat.as_mat().slice(numrel,ngens) << endl;
       cout << "Computing kernel..."<<endl;
     }
   vec pivs, npivs;
#ifdef USE_SMATS
   smat_elim sme(relmat);
   //relmat=smat(0,0); // clear space
   int d1;
   start_time();
   smat sp = liftmat(sme.kernel(npivs,pivs),MODULUS,d1);  // lift from mod p to characteristic 0
   stop_time();
   if (verbose) {cout<<"time to compute kernel = "; show_time(); cout<<endl;}
   denom1=d1;
   if(verbose>1) 
     {
       cout << "kernel of relmat = " << sp.as_mat() << endl;
       cout << "pivots = "<<pivs << endl;
       cout << "denom = "<<d1 << endl;
     }
   rk = ncols(sp);                   // This is the dimension of symbols, modulo all 2 and 3-term relations!
   coord_vecs.resize(ngens+1); // 0'th is unused
   for(i=1; i<=ngens; i++) 
     coord_vecs[i]=sp.row(i);
   //sp=smat(0,0); // clear space
#else
   subspace sp = kernel(relmat);
   rk = dim(sp);                   // This is the dimension of symbols, modulo all 2 and 3-term relations!
   coord = basis(sp);
   pivs = pivots(sp);
   denom1 = denom(sp);
   for(i=1; i<=ngens; i++) 
     coord_vecs[i]=svec(coord.row(i));
   sp.clear(); 
   relmat.init(); newrel.init();
#endif

   //   cout<<"ngens = "<<ngens<<endl;
   if (verbose) 
     {
       cout << "rk = " << rk << endl;
       if (verbose>1) 
	 {
	   //       cout << "coord:\n" ; coord.output_pretty();
	   cout << "coord_vecs:\n";
           for(i=1; i<=ngens; i++) 
             cout<<i<<": "<<coord_vecs[i].as_vec()<<"\n";
	   cout << "pivots = " << pivs <<endl;
	 }
     }
   freegens = new int[rk]; 
   if (!freegens) abort(string("freegens").c_str());
   if (rk>0)
   {
   for (i=0; i<rk; i++) freegens[i] = gens[pivs[i+1]];
   if (verbose>1)
    { cout << "freegens: ";
      for (i=0; i<rk; i++) cout << freegens[i] << " ";
      cout << "\n";
    }
   }
  delete[] check; delete[] gens; delete[] rel; 
  pivs.init();  npivs.init();
   }

   // Compute the number of cusps
   long maxncusps =0, dd, pp, nc;
   vector<long>::const_iterator d,p;
   for(d=(dlist).begin();d!=(dlist).end();d++)
     {
       dd = *d;
       nc = ::gcd(dd,modulus/dd);
       for(p=plist.begin();p!=plist.end();p++) // computing phi(dd)
         {
           pp = *p;
           if ((nc%pp)==0) 
             nc = nc*(pp-1)/pp;
         }
       maxncusps += nc;
     }  
   if (verbose) cout << "Number of cusps is "<<maxncusps<<endl;

   cusplist cusps(maxncusps, this);

   for (i=0; i<rk; i++)
     {
       modsym m(symbol(freegens[i]));
       for (j=1; j>-3; j-=2)
	 {
	   rational c = (j==1 ? m.beta() : m.alpha());
           if (plusflag==-1)
	     k = cusps.index_1(c);   //adds automatically if new, ignores if [c]=[-c]
           else 
             k = cusps.index(c);   //adds automatically if new
	 }
     }
   ncusps=cusps.count();

   if(verbose) 
     {
       cout << "ncusps = " << ncusps << endl;
       if(verbose>1) {cusps.display(); cout<<endl;}
     }

   if (verbose) cout << "About to compute matrix of delta"<<endl;
   mat deltamat=mat(ncusps,rk);  // should make this sparse

// A matrix in Gamma_H not in Gamma_0(N) (for non-trivial character):
   mat22 g_u(gamma_u[0],gamma_u[1],gamma_u[2],gamma_u[3]);
   int jlim=2; 
   if(u) jlim=4; // nontrivial char
   rational c; long s;
   for (i=0; i<rk; i++)
     {
       modsym m(symbol(freegens[i]));
       for (j=0; j<jlim; j++)
	 {
           switch(j) {
           case 0: c = m.alpha(); s=-1; break;
           case 1: c = m.beta(); s=1; break;
           case 2: c = g_u(m.alpha()); s=-1; break;
           case 3: c = g_u(m.beta()); s=1; break;
           }
           if (plusflag==-1)
             k = cusps.index_1(c);
           else 
             k = cusps.index(c);
           if (k>0)
             deltamat(k,i+1) += s;
           if (k<0)
             deltamat(-k,i+1) -= s;
	 }
     }
   if (verbose)
     {
       cout << "delta matrix done: size "<<nrows(deltamat)<<"x"<<ncols(deltamat)<<". "<<endl;
       if(verbose>1)
	 cout<<"deltamat = "<<deltamat<<endl;
       cout << "About to compute kernel of delta"<<endl;
     }
   
   kern=kernel(smat(deltamat));
   vec pivs, npivs;
   int d2;
   smat sk = liftmat(smat_elim(deltamat).kernel(npivs,pivs),MODULUS,d2);
   denom2=d2;
   tkernbas = transpose(kern.bas());         // dim(kern) x rank
   deltamat.init(0); // clear space.
   if(verbose>1)
     {
       cout<<"tkernbas = "<<tkernbas.as_mat()<<endl;
     }

   if (verbose) cout << "done "<<endl;

   if(cuspidal)
     dimension = dim(kern);
   else
     dimension = rk;
   //   denom2 = 1;
   denom3 = denom1*denom2;

   freemods = new modsym[rk]; 
   if (!freemods) abort(string("freemods").c_str());
   needed   = new int[rk];    
   if (!needed) abort(string("needed").c_str());

   if (dimension>0)
   {
        if (verbose>1)  cout << "Freemods:\n";
        for (i=0; i<rk; i++)
	  {  
	    freemods[i] = modsym(symbol(freegens[i])) ;
	    needed[i]   =  (cuspidal? ! trivial(kern.bas().row(i+1).as_vec()) 
			              : 1);
	    if (verbose>1) 
	      {
		cout << i << ": " << freemods[i];
		if (!needed[i]) cout << " (not needed)";
		cout << "\n";
	      }
	  }
        if ((verbose>1)&&cuspidal)
        {
	  cout << "Basis of ker(delta):\n";
	  cout << kern.bas().as_mat()<<endl;
	  cout << "pivots: " << pivots(kern) << endl;
        }
   }
   if (verbose) cout << "Finished constructing homspace." << endl;
}

homspace::~homspace()
{ 
  if (coordindex) delete[] coordindex; 
  if (needed) delete[] needed; 
  if (freegens) delete[] freegens; 
  if (freemods) delete[] freemods;  
}

  // Extend a dual vector of length rk to one of length nsymb:
vec homspace::extend_coords(const vec& v)
{
  //  cout<<"Extending vector "<<v<<endl;
  vec ans(nsymb);
  int i,j;
  for(i=1; i<=nsymb; i++)
    {      
      j = coordindex[i-1];
      if (j==0) ans[i] = 0;
      else if (j>0) ans[i] =  v*coord_vecs[j];
      else if (j<0) ans[i] = -v*coord_vecs[-j];
    }
  //  cout<<"returning "<<ans<<endl;
  return ans;
}

  // Contract a dual vector of length nsymb to one of length rk: 
vec homspace::contract_coords(const vec& v)
{
  //  cout<<"Contracting vector "<<v<<endl;
  vec ans(rk);
  int i;
  for(i=1; i<=rk; i++)
    ans[i] = v[1+freegens[i-1]];
  //  cout<<"returning "<<ans<<endl;
  return ans;
}


// JON -- CHANGED THIS ONE!
svec homspace::chain(const symb& s) const
{
 vector<long> V = index_with_pm(s);
 int eps = V[1];
 long i = coordindex[V[0]];
 if (i>0) return  eps * coord_vecs[i];
 if (i<0) return -eps * coord_vecs[-i];
 return svec(rk);
}

void homspace::add_chain(svec& v, const symb& s) const
{
  vector<long> V = index_with_pm(s);
  int eps = V[1];
  long i = coordindex[V[0]];
  if (i>0) {v += (eps) * coord_vecs[i]; return;}
  if (i<0) {v -= (eps) * coord_vecs[-i]; return;}
}

vec homspace::projchaincd(long c, long d, const mat& m) const 
{
  vector<long> V = index2_with_pm(c,d);
  int eps = V[1];
  long i = coordindex[V[0]];
  if (i>0) return  eps * m.row(i);
  if (i<0) return -eps * m.row(-i);
  return vec(ncols(m));
}

long homspace::nfprojchaincd(long c, long d, const vec& bas) const 
{
  vector<long> V = index2_with_pm(c,d);
  int eps = V[1];
  long i = coordindex[V[0]];
  if (i>0) return  eps * bas[i];
  if (i<0) return -eps * bas[-i];
  return 0;
}

svec homspace::chaincd(long c, long d) const 
{
  vector<long> V = index2_with_pm(c,d);
  int eps = V[1];
  long i = coordindex[V[0]];
  if (i>0) return  eps * coord_vecs[i];
  if (i<0) return -eps * coord_vecs[-i];
  return svec(rk);
}

void homspace::add_projchaincd(vec& v, long c, long d, const mat& m) const 
{
  vector<long> V = index2_with_pm(c,d);
  int eps = V[1];
  long i = coordindex[V[0]];
  if (eps > 0){
    if (i>0) {add_row_to_vec(v,m,i); return;}
    if (i<0) {sub_row_to_vec(v,m,-i); return;}
  }
  else {
    if (i>0) {sub_row_to_vec(v,m,i); return;}
    if (i<0) {add_row_to_vec(v,m,-i); return;}
  }		
}

void homspace::add_nfprojchaincd(long& a, long c, long d, const vec& bas) const 
{
  vector<long> V = index2_with_pm(c,d);
  int eps = V[1];
  long i = coordindex[V[0]];
  if (i>0) {a += eps * bas[i]; return;}
  if (i<0) {a -= eps * bas[-i]; return;}
}

void homspace::add_chaincd(svec& v, long c, long d) const 
{
  vector<long> V = index2_with_pm(c,d);
  int eps = V[1];
  long i = coordindex[V[0]];
  if (i>0) {v += (eps) * coord_vecs[i]; return;}
  if (i<0) {v -= (eps) * coord_vecs[-i]; return;}
}

	
	
	
//Jon --  This is unchanged!	
svec homspace::chain(long nn, long dd) const
{
   svec ans = chaincd(0,1);
   long c=0, d=1, e, a=nn, b=dd, q, f;
   while (b)
   { q=a/b; 
     f=a; a=-b; b= f-q*b; 
     e=d; d= c; c=(-q*c-e)%modulus;
     add_chaincd(ans,c,d);
   }
   return ans;
}

void homspace::add_chain(svec& v, long nn, long dd) const
{
   add_chaincd(v,0,1);
   long c=0, d=1, e, a=nn, b=dd, q, f;
   while (b)
   { q=a/b; 
     f=a; a=-b; b= f-q*b; 
     e=d; d= c; c=(-q*c-e)%modulus;
     add_chaincd(v,c,d);
   }
}

vec homspace::projchain(long nn, long dd, const mat& m) const
{
   vec ans = projchaincd(0,1,m);
   long c=0, d=1, e, a=nn, b=dd, q, f;
   while (b)
   { q=a/b; 
     f=a; a=-b; b= f-q*b; 
     e=d; d= c; c=(-q*c-e)%modulus;
     add_projchaincd(ans,c,d,m);
   }
   return ans;
}

long homspace::nfprojchain(long nn, long dd, const vec& bas) const
{
   long ans = nfprojchaincd(0,1,bas);
   long c=0, d=1, e, a=nn, b=dd, q, f;
   while (b)
   { q=a/b; 
     f=a; a=-b; b= f-q*b; 
     e=d; d= c; c=(-q*c-e)%modulus;
     add_nfprojchaincd(ans,c,d,bas);
   }
   return ans;
}

void homspace::add_projchain(vec& v, long nn, long dd, const mat& m) const
{
   add_projchaincd(v,0,1,m);
   long c=0, d=1, e, a=nn, b=dd, q, f;
   while (b)
   { q=a/b; 
     f=a; a=-b; b= f-q*b; 
     e=d; d= c; c=(-q*c-e)%modulus;
     add_projchaincd(v,c,d,m);
   }
}

void homspace::add_nfprojchain(long& aa, long nn, long dd, const vec& bas) const
{
   add_nfprojchaincd(aa,0,1,bas);
   long c=0, d=1, e, a=nn, b=dd, q, f;
   while (b)
   { q=a/b; 
     f=a; a=-b; b= f-q*b; 
     e=d; d= c; c=(-q*c-e)%modulus;
     add_nfprojchaincd(aa,c,d,bas);
   }
}

svec homspace::applyop(const matop& mlist, const rational& q) const
{ svec ans(rk);  long i=mlist.size();
  while (i--) add_chain(ans,mlist[i](q));
  return ans;
}
 
// copy of routine from ../procs/xsplit.cc:
mat sparse_restrict(const mat& m, const subspace& s);
smat restrict(const smat& m, const subspace& s);

mat homspace::calcop_restricted(string opname, long p, const matop& mlist, 
				const subspace& s, int dual, int display) const
{
  long d=dim(s);
  mat m(d,rk);
  for (long j=0; j<d; j++) 
     { 
       long jj = pivots(s)[j+1]-1;
       svec colj = applyop(mlist,freemods[jj]);
       m.setrow(j+1,colj.as_vec());
     }
  m = (smat(m)*smat(basis(s))).as_mat();
  if(!dual) m=transpose(m); // dual is default for restricted ops
  if (display) 
    {
      cout << "Matrix of " << opname << "(" << p << ") = ";
      if (dimension>1) cout << "\n";
      m.output_pretty();
    }
  return m;
}

smat homspace::s_calcop_restricted(string opname, long p, const matop& mlist, 
				   const ssubspace& s, int dual, int display) const
{
  long d=dim(s);
  smat m(d,rk);
  for (long j=1; j<=d; j++) 
     { 
       long jj = pivots(s)[j];
       svec colj = applyop(mlist,freemods[jj-1]);
       m.setrow(j,colj);
     }
  //  m = m*basis(s);
  m = mult_mod_p(m,basis(s),MODULUS);
  if(!dual) m=transpose(m); // dual is default for restricted ops
  if (display) 
    {
      cout << "Matrix of " << opname << "(" << p << ") = ";
      if (dimension>1) cout << "\n";
      cout<<m<<endl;
    }
  return m;
}

mat homspace::calcop(string opname, long p, const matop& mlist, 
		     int dual, int display) const
{
  mat m(rk,rk);
  for (long j=0; j<rk; j++) if (needed[j])
     { svec colj = applyop(mlist,freemods[j]);
       m.setcol(j+1,colj.as_vec());
     }
  if(cuspidal) m = restrict(smat(m),kern).as_mat();
  if(dual) {  m=transpose(m);}
  if (display) 
    {
      cout << "Matrix of " << opname << "(" << p << ") = ";
      if (dimension>1) cout << "\n";
      m.output_pretty();
    }
  return m;
}
 
smat homspace::s_calcop(string opname, long p, const matop& mlist, 
			int dual, int display) const
{
  smat m(rk,rk);
  for (long j=0; j<rk; j++) if (needed[j])
     { svec colj = applyop(mlist,freemods[j]);
       m.setrow(j+1,colj);
     }
  if(cuspidal) 
    {  
      m = restrict(transpose(m),kern);
      if(dual) m = transpose(m);
    }
  else if(!dual) {m=transpose(m);}
  if (display) 
    {
      cout << "Matrix of " << opname << "(" << p << ") = ";
      if (dimension>1) cout << "\n";
      cout<<m.as_mat();
    }
  return m;
}

/* NOTE: heckeop actually only returns the Hecke operator for p not dividing
   the level.  For p that divide the level it actually computes the Atkin-Lehner
   operator. 
*/

mat homspace::heckeop(long p, int dual, int display) const
{
 matop matlist(p,this);
 string name = ((modulus%p) ? T_opname : W_opname);
 return calcop(name,p,matlist,dual,display);
}
 
smat homspace::s_heckeop(long p, int dual, int display) const
{
 matop matlist(p,this);
 string name = ((modulus%p) ? T_opname : W_opname);
 return s_calcop(name,p,matlist,dual,display);
}
 
mat homspace::heckeop_restricted(long p,const subspace& s, 
				 int dual, int display) const
{
 matop matlist(p,this);
 string name = ((modulus%p) ? T_opname : W_opname);
 return calcop_restricted(name,p,matlist,s,dual,display);
}
 
smat homspace::s_heckeop_restricted(long p,const ssubspace& s, 
				    int dual, int display) const
{
 matop matlist(p,this);
 string name = ((modulus%p) ? T_opname : W_opname);
 return s_calcop_restricted(name,p,matlist,s,dual,display);
}
 
mat homspace::newheckeop(long p, int dual, int display) const
{
  if(::div(p,modulus)) return wop(p,display);
  matop hmats(p); // constructs H-matrices
  long j, nmats=hmats.size();
  svec colj(rk);    mat m(rk,rk); 
  for (j=0; j<rk; j++) if (needed[j])
    {  symb s = symbol(freegens[j]);
       colj = hmats[0](s,this);
       colj+= hmats[1](s,this);
       for(long i=2; i<nmats; i++) colj+= (hmats[i](s,this));
       m.setcol(j+1,colj.as_vec());
     }
  if(cuspidal) m = restrict(smat(m),kern).as_mat();
  if(dual) m=transpose(m);
  if (display) 
    {
      cout << "Matrix of T(" << p << ") = ";
      if (dimension>1) cout << "\n";
      m.output_pretty();
    }
  return m;
}

mat homspace::wop(long q, int dual, int display) const
{
 matop matlist(q,this);
 return calcop(W_opname,q,matlist,dual,display);
}
 
smat homspace::s_wop(long q, int dual, int display) const
{
 matop matlist(q,this);
 return s_calcop(W_opname,q,matlist,dual,display);
}
 
mat homspace::conj(int dual, int display) const
{
 mat m(rk,rk);
 for (long j=1; j<=rk; j++) if (needed[j-1])
 {  symb s = symbol(freegens[j-1]);
    svec colj   =  chaincd(-s.cee(),s.dee());
    m.setcol(j,colj.as_vec());
 }
 if(cuspidal) m = restrict(smat(m),kern).as_mat();
 if(dual) m=transpose(m);
 if (display) cout << "Matrix of conjugation = " << m;
 return m;
}

smat homspace::s_conj(int dual, int display) const
{
 smat m(rk,rk);
 for (long j=1; j<=rk; j++) if (needed[j-1])
 {  symb s = symbol(freegens[j-1]);
    svec colj   =  chaincd(-s.cee(),s.dee());
    m.setrow(j,colj);
 }
 if(cuspidal) 
   {  
     m = restrict(transpose(m),kern);
     if(dual) m = transpose(m);
   }
 else if(!dual) {m=transpose(m);}
 if (display) cout << "Matrix of conjugation = " << m;
 return m;
}

// Computes matrix of conjugation restricted to a DUAL subspace; here
// the ambient space of s must be H_1(-;cusps) of dimension rk
mat homspace::conj_restricted(const subspace& s, 
			      int dual, int display) const
{
  long d = dim(s);
  mat m(d,rk);
  for (long j=1; j<=d; j++) 
    {  
      long jj=pivots(s)[j];
      symb s = symbol(freegens[jj-1]);
      svec colj   =  chaincd(-s.cee(),s.dee());
      m.setrow(j,colj.as_vec());
    }
  m = matmulmodp(m,basis(s),MODULUS);
  if(!dual) m=transpose(m); // dual is default for restricted ops
  if (display) cout << "Matrix of conjugation = " << m;
  return m;
}

smat homspace::s_conj_restricted(const ssubspace& s, 
				 int dual, int display) const
{
  long d = dim(s);
  smat m(d,rk);
  for (long j=1; j<=d; j++) 
    {  
      long jj=pivots(s)[j];
      symb s = symbol(freegens[jj-1]);
      svec colj   =  chaincd(-s.cee(),s.dee());
      m.setrow(j,colj);
    }
  //  cout<<"m = "<<m<<" = "<<m.as_mat()<<endl;
  m = mult_mod_p(m,basis(s),MODULUS);
  if(!dual) m=transpose(m); // dual is default for restricted ops
  if (display) cout << "Matrix of conjugation = " << m.as_mat();
  return m;
}

mat homspace::fricke(int dual, int display) const
{
 matop frickelist(modulus, this);
 return calcop(W_opname,modulus,frickelist,dual,display);
}

long homspace::op_prime(int i)  // the i'th operator prime for Tp or Wq
{
#ifdef NEW_OP_ORDER
  long p = prime_number(i+1);
  //  cout<<"opmat("<<i<<") using p="<<p<<endl;
#else
  long p = primelist[i];
#endif
  return p;
}

mat homspace::opmat(int i, int dual, int v)
{
  if(i==-1) return conj(dual,v);
  if((i<0)||(i>=nap)) 
    {
      cout<<"Error in homspace::opmat(): called with i = " << i << endl;
      ::abort();
      return mat(dimension);  // shouldn't happen
    }
  long p = op_prime(i);
  if(v) 
    {
      cout<<"Computing " << ((::div(p,modulus)) ? W_opname : T_opname) <<"("<<p<<")..."<<flush;
      mat ans = heckeop(p,dual,0); // Automatically chooses W or T
      cout<<"done."<<endl;
      return ans;
    }
  else return heckeop(p,dual,0); // Automatically chooses W or T
}

smat homspace::s_opmat(int i, int dual, int v)
{
  if(i==-1) return s_conj(dual,v);
  if((i<0)||(i>=nap)) 
    {
      cout<<"Error in homspace::s_opmat(): called with i = " << i << endl;
      ::abort();
      return smat(dimension);  // shouldn't happen
    }
  long p = op_prime(i);
  if(v) 
    {
      cout<<"Computing " << ((::div(p,modulus)) ? W_opname : T_opname) <<"("<<p<<")..."<<flush;
      smat ans = s_heckeop(p,dual,0); // Automatically chooses W or T
      cout<<"done."<<endl;
      return ans;
    }
  else return s_heckeop(p,dual,0); // Automatically chooses W or T
}

mat homspace::opmat_restricted(int i, const subspace& s, int dual, int v)
{
  if(i==-1) return conj_restricted(s,dual,v);
  if((i<0)||(i>=nap)) 
    {
      cout<<"Error in homspace::opmat_restricted(): called with i = " 
	  << i << endl;
      ::abort();
      return mat(dim(s));  // shouldn't happen
    }
  long p = op_prime(i);
  if(v) 
    {
      cout<<"Computing " << ((::div(p,modulus)) ? W_opname : T_opname) <<"("<<p
	  <<") restricted to subspace of dimension "<<dim(s)<<" ..."<<flush;
      mat ans = heckeop_restricted(p,s,dual,0); // Automatically chooses W or T
      cout<<"done."<<endl;
      return ans;
    }
  else return heckeop_restricted(p,s,dual,0); // Automatically chooses W or T
}

smat homspace::s_opmat_restricted(int i, const ssubspace& s, int dual, int v)
{
  if(i==-1) return s_conj_restricted(s,dual,v);
  if((i<0)||(i>=nap)) 
    {
      cout<<"Error in homspace::s_opmat_restricted(): called with i = " 
	  << i << endl;
      ::abort();
      return smat(dim(s));  // shouldn't happen
    }
  long p = op_prime(i);
  if(v) 
    {
      cout<<"Computing " << ((::div(p,modulus)) ? W_opname : T_opname) <<"("<<p
	  <<") restricted to subspace of dimension "<<dim(s)<<" ..."<<flush;
      smat ans = s_heckeop_restricted(p,s,dual,(v>2)); // Automatically chooses W or T
      cout<<"done."<<endl;
      return ans;
    }
  else return s_heckeop_restricted(p,s,dual,0); // Automatically chooses W or T
}

#ifdef OLD_EIG_ORDER

static long*pm1={1,-1};

vector<long> T_eigrange(long p)
{
  vector<long> ans;
  ans.push_back(0);
  long aplim=1;
  while (aplim*aplim<=4*p) aplim++; aplim--;
  for(long ap=1; ap<=aplim; ap++)
    {
      ans.push_back(ap);
      ans.push_back(-ap);
    }
  return ans;
}

#else  // new eig order, in strict numerical order

static long pm1[]={-1,1};

vector<long> T_eigrange(long p)
{
  long aplim=3, four_p=p<<2;
  while (aplim*aplim<=four_p) aplim++; 
  aplim--;
  vector<long> ans(1+2*aplim);
  iota(ans.begin(),ans.end(),-aplim);
  return ans;
}
#endif

vector<long> homspace::eigrange(long i)
{
  if((i<0)||(i>=nap)) return vector<long>(0);  // shouldn't happen
  long p = op_prime(i);
  if(::div(p,modulus))  return vector<long>(pm1,pm1+2);
  return T_eigrange(p);
}

vec homspace::maninvector(long p) const
{
  long i,p2;
  svec tvec = chain(0,p);             // =0, but sets the right length.
  if (plusflag!=-1) 
    {
      if (p==2) 
	add_chain(tvec,1,2); 
      else
	{ 
	  p2=(p-1)>>1;
	  for (i=1; i<=p2; i++) { add_chain(tvec,i,p); }
	  if(plusflag)   
	    tvec *=2;
	  else
	    for (i=1; i<=p2; i++) { add_chain(tvec,-i,p); }
	}
    }
  if(cuspidal) 
    return cuspidalpart(tvec.as_vec()); 
  else 
    return tvec.as_vec();
}

vec homspace::manintwist(long p) const
{
 svec sum = chain(0,p);                   // =0, but sets the right length.
 for (long i=1; i<p; i++) sum += legendre(i,p)*chain(i,p);
 if(cuspidal) return cuspidalpart(sum.as_vec()); 
 else return sum.as_vec();
}

#if(0) // no longer used
vec homspace::projmaninvector(long p) const  // Will only work after "proj"
{
  long i,p2;
  vec tvec = projchain(0,p);             // =0, but sets the right length.
  if (plusflag==-1) return tvec;
  if (p==2) 
    add_projchain(tvec,1,2);
  else
    { 
      p2=(p-1)>>1;
      for (i=1; i<=p2; i++) { add_projchain(tvec,i,p); }
      if(plusflag)   
	tvec *=2;
      else
	for (i=1; i<=p2; i++) { add_projchain(tvec,-i,p); }
    }
  return tvec;
}

vec homspace::projmaninvector(long p, const mat& m) const
{
  long i,p2;
  vec tvec = projchain(0,p,m);       // =0, but sets the right length.
  if (plusflag==-1) return tvec;
  if (p==2) 
    add_projchain(tvec,1,2,m);
  else
    { 
      p2=(p-1)>>1;
      for (i=1; i<=p2; i++) { add_projchain(tvec,i,p,m); }
      if(plusflag)   
	tvec *=2;
      else
	for (i=1; i<=p2; i++) { add_projchain(tvec,-i,p,m); }
    }
  return tvec;
}

vec homspace::newhecke(long p, long n, long d) const
                                     // Will only work after "proj"
{ vec tvec = projchain(p*n,d);
// cout<<"newhecke starts: tvec = "<<tvec<<endl; 
 long p2 = p>>1, dp = d*p, k;
  for (k=1+p2-p; k<=p2; k++) 
    {
      add_projchain(tvec,n+d*k,dp);
      // cout<<"tvec = "<<tvec<<endl; 
    }
  // cout<<"newhecke returns: tvec = "<<tvec<<endl; 
  return tvec;
}
#endif // no longer used

matop::matop(long p)
{

  //    case 31: nmats = 106; break;
  //    case 37: nmats = 128; break;
  //    case 41: nmats = 146; break;
  //    case 43: nmats = 154; break;
  //    case 47: nmats = 170; break;

  switch (p) {
  case 2: 
    mats.resize(4);
    mats[0]=mat22(2,0,0,1); mats[1]=mat22(2,1,0,1);
    mats[2]=mat22(1,0,1,2); mats[3]=mat22(1,0,0,2);
    return;
  case 3:
    mats.resize(6);
    mats[0]=mat22(1,0,0,3);
    mats[1]=mat22(3,1,0,1);
    mats[2]=mat22(1,0,1,3);
    mats[3]=mat22(3,0,0,1);
    mats[4]=mat22(3,-1,0,1);
    mats[5]=mat22(-1,0,1,-3);
    return;
  case 5:
    mats.resize(12);
    mats[0]=mat22(1,0,0,5);
    mats[1]=mat22(5,2,0,1);
    mats[2]=mat22(2,1,1,3);
    mats[3]=mat22(1,0,3,5);
    mats[4]=mat22(5,1,0,1);
    mats[5]=mat22(1,0,1,5);
    mats[6]=mat22(5,0,0,1);
    mats[7]=mat22(5,-1,0,1);
    mats[8]=mat22(-1,0,1,-5);
    mats[9]=mat22(5,-2,0,1);
    mats[10]=mat22(-2,1,1,-3);
    mats[11]=mat22(1,0,-3,5);
    return;
  case 7:
    mats.resize(18);
    mats[0]=mat22(1,0,0,7);
    mats[1]=mat22(7,3,0,1);
    mats[2]=mat22(3,-1,1,2);
    mats[3]=mat22(-1,0,2,-7);
    mats[4]=mat22(7,2,0,1);
    mats[5]=mat22(2,1,1,4);
    mats[6]=mat22(1,0,4,7);
    mats[7]=mat22(7,1,0,1);
    mats[8]=mat22(1,0,1,7);
    mats[9]=mat22(7,0,0,1);
    mats[10]=mat22(7,-1,0,1);
    mats[11]=mat22(-1,0,1,-7);
    mats[12]=mat22(7,-2,0,1);
    mats[13]=mat22(-2,1,1,-4);
    mats[14]=mat22(1,0,-4,7);
    mats[15]=mat22(7,-3,0,1);
    mats[16]=mat22(-3,-1,1,-2);
    mats[17]=mat22(-1,0,-2,-7);
    return;
  case 11:
    mats.resize(30);
    mats[0]=mat22(1,0,0,11);
    mats[1]=mat22(11,5,0,1);
    mats[2]=mat22(5,-1,1,2);
    mats[3]=mat22(-1,0,2,-11);
    mats[4]=mat22(11,4,0,1);
    mats[5]=mat22(4,1,1,3);
    mats[6]=mat22(1,0,3,11);
    mats[7]=mat22(11,3,0,1);
    mats[8]=mat22(3,1,1,4);
    mats[9]=mat22(1,0,4,11);
    mats[10]=mat22(11,2,0,1);
    mats[11]=mat22(2,1,1,6);
    mats[12]=mat22(1,0,6,11);
    mats[13]=mat22(11,1,0,1);
    mats[14]=mat22(1,0,1,11);
    mats[15]=mat22(11,0,0,1);
    mats[16]=mat22(11,-1,0,1);
    mats[17]=mat22(-1,0,1,-11);
    mats[18]=mat22(11,-2,0,1);
    mats[19]=mat22(-2,1,1,-6);
    mats[20]=mat22(1,0,-6,11);
    mats[21]=mat22(11,-3,0,1);
    mats[22]=mat22(-3,1,1,-4);
    mats[23]=mat22(1,0,-4,11);
    mats[24]=mat22(11,-4,0,1);
    mats[25]=mat22(-4,1,1,-3);
    mats[26]=mat22(1,0,-3,11);
    mats[27]=mat22(11,-5,0,1);
    mats[28]=mat22(-5,-1,1,-2);
    mats[29]=mat22(-1,0,-2,-11);
    return;
  case 13:
    mats.resize(38);
    mats[0]=mat22(1,0,0,13);
    mats[1]=mat22(13,6,0,1);
    mats[2]=mat22(6,-1,1,2);
    mats[3]=mat22(-1,0,2,-13);
    mats[4]=mat22(13,5,0,1);
    mats[5]=mat22(5,2,1,3);
    mats[6]=mat22(2,-1,3,5);
    mats[7]=mat22(-1,0,5,-13);
    mats[8]=mat22(13,4,0,1);
    mats[9]=mat22(4,-1,1,3);
    mats[10]=mat22(-1,0,3,-13);
    mats[11]=mat22(13,3,0,1);
    mats[12]=mat22(3,-1,1,4);
    mats[13]=mat22(-1,0,4,-13);
    mats[14]=mat22(13,2,0,1);
    mats[15]=mat22(2,1,1,7);
    mats[16]=mat22(1,0,7,13);
    mats[17]=mat22(13,1,0,1);
    mats[18]=mat22(1,0,1,13);
    mats[19]=mat22(13,0,0,1);
    mats[20]=mat22(13,-1,0,1);
    mats[21]=mat22(-1,0,1,-13);
    mats[22]=mat22(13,-2,0,1);
    mats[23]=mat22(-2,1,1,-7);
    mats[24]=mat22(1,0,-7,13);
    mats[25]=mat22(13,-3,0,1);
    mats[26]=mat22(-3,-1,1,-4);
    mats[27]=mat22(-1,0,-4,-13);
    mats[28]=mat22(13,-4,0,1);
    mats[29]=mat22(-4,-1,1,-3);
    mats[30]=mat22(-1,0,-3,-13);
    mats[31]=mat22(13,-5,0,1);
    mats[32]=mat22(-5,2,1,-3);
    mats[33]=mat22(2,-1,-3,8);
    mats[34]=mat22(-1,0,8,-13);
    mats[35]=mat22(13,-6,0,1);
    mats[36]=mat22(-6,-1,1,-2);
    mats[37]=mat22(-1,0,-2,-13);
    return;
  case 17:
    mats.resize(52);
    mats[0]=mat22(1,0,0,17);
    mats[1]=mat22(17,8,0,1);
    mats[2]=mat22(8,-1,1,2);
    mats[3]=mat22(-1,0,2,-17);
    mats[4]=mat22(17,7,0,1);
    mats[5]=mat22(7,-3,1,2);
    mats[6]=mat22(-3,-1,2,-5);
    mats[7]=mat22(-1,0,-5,-17);
    mats[8]=mat22(17,6,0,1);
    mats[9]=mat22(6,1,1,3);
    mats[10]=mat22(1,0,3,17);
    mats[11]=mat22(17,5,0,1);
    mats[12]=mat22(5,-2,1,3);
    mats[13]=mat22(-2,-1,3,-7);
    mats[14]=mat22(-1,0,-7,-17);
    mats[15]=mat22(17,4,0,1);
    mats[16]=mat22(4,-1,1,4);
    mats[17]=mat22(-1,0,4,-17);
    mats[18]=mat22(17,3,0,1);
    mats[19]=mat22(3,1,1,6);
    mats[20]=mat22(1,0,6,17);
    mats[21]=mat22(17,2,0,1);
    mats[22]=mat22(2,1,1,9);
    mats[23]=mat22(1,0,9,17);
    mats[24]=mat22(17,1,0,1);
    mats[25]=mat22(1,0,1,17);
    mats[26]=mat22(17,0,0,1);
    mats[27]=mat22(17,-1,0,1);
    mats[28]=mat22(-1,0,1,-17);
    mats[29]=mat22(17,-2,0,1);
    mats[30]=mat22(-2,1,1,-9);
    mats[31]=mat22(1,0,-9,17);
    mats[32]=mat22(17,-3,0,1);
    mats[33]=mat22(-3,1,1,-6);
    mats[34]=mat22(1,0,-6,17);
    mats[35]=mat22(17,-4,0,1);
    mats[36]=mat22(-4,-1,1,-4);
    mats[37]=mat22(-1,0,-4,-17);
    mats[38]=mat22(17,-5,0,1);
    mats[39]=mat22(-5,-2,1,-3);
    mats[40]=mat22(-2,-1,-3,-10);
    mats[41]=mat22(-1,0,-10,-17);
    mats[42]=mat22(17,-6,0,1);
    mats[43]=mat22(-6,1,1,-3);
    mats[44]=mat22(1,0,-3,17);
    mats[45]=mat22(17,-7,0,1);
    mats[46]=mat22(-7,-3,1,-2);
    mats[47]=mat22(-3,1,-2,-5);
    mats[48]=mat22(1,0,-5,17);
    mats[49]=mat22(17,-8,0,1);
    mats[50]=mat22(-8,-1,1,-2);
    mats[51]=mat22(-1,0,-2,-17);
    return;
  case 19:
    mats.resize(58);
    mats[0]=mat22(1,0,0,19);
    mats[1]=mat22(19,9,0,1);
    mats[2]=mat22(9,-1,1,2);
    mats[3]=mat22(-1,0,2,-19);
    mats[4]=mat22(19,8,0,1);
    mats[5]=mat22(8,-3,1,2);
    mats[6]=mat22(-3,1,2,-7);
    mats[7]=mat22(1,0,-7,19);
    mats[8]=mat22(19,7,0,1);
    mats[9]=mat22(7,2,1,3);
    mats[10]=mat22(2,-1,3,8);
    mats[11]=mat22(-1,0,8,-19);
    mats[12]=mat22(19,6,0,1);
    mats[13]=mat22(6,-1,1,3);
    mats[14]=mat22(-1,0,3,-19);
    mats[15]=mat22(19,5,0,1);
    mats[16]=mat22(5,1,1,4);
    mats[17]=mat22(1,0,4,19);
    mats[18]=mat22(19,4,0,1);
    mats[19]=mat22(4,1,1,5);
    mats[20]=mat22(1,0,5,19);
    mats[21]=mat22(19,3,0,1);
    mats[22]=mat22(3,-1,1,6);
    mats[23]=mat22(-1,0,6,-19);
    mats[24]=mat22(19,2,0,1);
    mats[25]=mat22(2,1,1,10);
    mats[26]=mat22(1,0,10,19);
    mats[27]=mat22(19,1,0,1);
    mats[28]=mat22(1,0,1,19);
    mats[29]=mat22(19,0,0,1);
    mats[30]=mat22(19,-1,0,1);
    mats[31]=mat22(-1,0,1,-19);
    mats[32]=mat22(19,-2,0,1);
    mats[33]=mat22(-2,1,1,-10);
    mats[34]=mat22(1,0,-10,19);
    mats[35]=mat22(19,-3,0,1);
    mats[36]=mat22(-3,-1,1,-6);
    mats[37]=mat22(-1,0,-6,-19);
    mats[38]=mat22(19,-4,0,1);
    mats[39]=mat22(-4,1,1,-5);
    mats[40]=mat22(1,0,-5,19);
    mats[41]=mat22(19,-5,0,1);
    mats[42]=mat22(-5,1,1,-4);
    mats[43]=mat22(1,0,-4,19);
    mats[44]=mat22(19,-6,0,1);
    mats[45]=mat22(-6,-1,1,-3);
    mats[46]=mat22(-1,0,-3,-19);
    mats[47]=mat22(19,-7,0,1);
    mats[48]=mat22(-7,2,1,-3);
    mats[49]=mat22(2,-1,-3,11);
    mats[50]=mat22(-1,0,11,-19);
    mats[51]=mat22(19,-8,0,1);
    mats[52]=mat22(-8,-3,1,-2);
    mats[53]=mat22(-3,-1,-2,-7);
    mats[54]=mat22(-1,0,-7,-19);
    mats[55]=mat22(19,-9,0,1);
    mats[56]=mat22(-9,-1,1,-2);
    mats[57]=mat22(-1,0,-2,-19);
    return;
  case 23:
    mats.resize(74);
    mats[0]=mat22(1,0,0,23);
    mats[1]=mat22(23,11,0,1);
    mats[2]=mat22(11,-1,1,2);
    mats[3]=mat22(-1,0,2,-23);
    mats[4]=mat22(23,10,0,1);
    mats[5]=mat22(10,-3,1,2);
    mats[6]=mat22(-3,-1,2,-7);
    mats[7]=mat22(-1,0,-7,-23);
    mats[8]=mat22(23,9,0,1);
    mats[9]=mat22(9,4,1,3);
    mats[10]=mat22(4,-1,3,5);
    mats[11]=mat22(-1,0,5,-23);
    mats[12]=mat22(23,8,0,1);
    mats[13]=mat22(8,1,1,3);
    mats[14]=mat22(1,0,3,23);
    mats[15]=mat22(23,7,0,1);
    mats[16]=mat22(7,-2,1,3);
    mats[17]=mat22(-2,-1,3,-10);
    mats[18]=mat22(-1,0,-10,-23);
    mats[19]=mat22(23,6,0,1);
    mats[20]=mat22(6,1,1,4);
    mats[21]=mat22(1,0,4,23);
    mats[22]=mat22(23,5,0,1);
    mats[23]=mat22(5,2,1,5);
    mats[24]=mat22(2,-1,5,9);
    mats[25]=mat22(-1,0,9,-23);
    mats[26]=mat22(23,4,0,1);
    mats[27]=mat22(4,1,1,6);
    mats[28]=mat22(1,0,6,23);
    mats[29]=mat22(23,3,0,1);
    mats[30]=mat22(3,1,1,8);
    mats[31]=mat22(1,0,8,23);
    mats[32]=mat22(23,2,0,1);
    mats[33]=mat22(2,1,1,12);
    mats[34]=mat22(1,0,12,23);
    mats[35]=mat22(23,1,0,1);
    mats[36]=mat22(1,0,1,23);
    mats[37]=mat22(23,0,0,1);
    mats[38]=mat22(23,-1,0,1);
    mats[39]=mat22(-1,0,1,-23);
    mats[40]=mat22(23,-2,0,1);
    mats[41]=mat22(-2,1,1,-12);
    mats[42]=mat22(1,0,-12,23);
    mats[43]=mat22(23,-3,0,1);
    mats[44]=mat22(-3,1,1,-8);
    mats[45]=mat22(1,0,-8,23);
    mats[46]=mat22(23,-4,0,1);
    mats[47]=mat22(-4,1,1,-6);
    mats[48]=mat22(1,0,-6,23);
    mats[49]=mat22(23,-5,0,1);
    mats[50]=mat22(-5,2,1,-5);
    mats[51]=mat22(2,-1,-5,14);
    mats[52]=mat22(-1,0,14,-23);
    mats[53]=mat22(23,-6,0,1);
    mats[54]=mat22(-6,1,1,-4);
    mats[55]=mat22(1,0,-4,23);
    mats[56]=mat22(23,-7,0,1);
    mats[57]=mat22(-7,-2,1,-3);
    mats[58]=mat22(-2,-1,-3,-13);
    mats[59]=mat22(-1,0,-13,-23);
    mats[60]=mat22(23,-8,0,1);
    mats[61]=mat22(-8,1,1,-3);
    mats[62]=mat22(1,0,-3,23);
    mats[63]=mat22(23,-9,0,1);
    mats[64]=mat22(-9,4,1,-3);
    mats[65]=mat22(4,1,-3,5);
    mats[66]=mat22(1,0,5,23);
    mats[67]=mat22(23,-10,0,1);
    mats[68]=mat22(-10,-3,1,-2);
    mats[69]=mat22(-3,1,-2,-7);
    mats[70]=mat22(1,0,-7,23);
    mats[71]=mat22(23,-11,0,1);
    mats[72]=mat22(-11,-1,1,-2);
    mats[73]=mat22(-1,0,-2,-23);
    return;
  case 29:
    mats.resize(96);
    mats[0]=mat22(1,0,0,29);
    mats[1]=mat22(29,14,0,1);
    mats[2]=mat22(14,-1,1,2);
    mats[3]=mat22(-1,0,2,-29);
    mats[4]=mat22(29,13,0,1);
    mats[5]=mat22(13,-3,1,2);
    mats[6]=mat22(-3,-1,2,-9);
    mats[7]=mat22(-1,0,-9,-29);
    mats[8]=mat22(29,12,0,1);
    mats[9]=mat22(12,-5,1,2);
    mats[10]=mat22(-5,-2,2,-5);
    mats[11]=mat22(-2,1,-5,-12);
    mats[12]=mat22(1,0,-12,29);
    mats[13]=mat22(29,11,0,1);
    mats[14]=mat22(11,4,1,3);
    mats[15]=mat22(4,1,3,8);
    mats[16]=mat22(1,0,8,29);
    mats[17]=mat22(29,10,0,1);
    mats[18]=mat22(10,1,1,3);
    mats[19]=mat22(1,0,3,29);
    mats[20]=mat22(29,9,0,1);
    mats[21]=mat22(9,-2,1,3);
    mats[22]=mat22(-2,-1,3,-13);
    mats[23]=mat22(-1,0,-13,-29);
    mats[24]=mat22(29,8,0,1);
    mats[25]=mat22(8,3,1,4);
    mats[26]=mat22(3,1,4,11);
    mats[27]=mat22(1,0,11,29);
    mats[28]=mat22(29,7,0,1);
    mats[29]=mat22(7,-1,1,4);
    mats[30]=mat22(-1,0,4,-29);
    mats[31]=mat22(29,6,0,1);
    mats[32]=mat22(6,1,1,5);
    mats[33]=mat22(1,0,5,29);
    mats[34]=mat22(29,5,0,1);
    mats[35]=mat22(5,1,1,6);
    mats[36]=mat22(1,0,6,29);
    mats[37]=mat22(29,4,0,1);
    mats[38]=mat22(4,-1,1,7);
    mats[39]=mat22(-1,0,7,-29);
    mats[40]=mat22(29,3,0,1);
    mats[41]=mat22(3,1,1,10);
    mats[42]=mat22(1,0,10,29);
    mats[43]=mat22(29,2,0,1);
    mats[44]=mat22(2,1,1,15);
    mats[45]=mat22(1,0,15,29);
    mats[46]=mat22(29,1,0,1);
    mats[47]=mat22(1,0,1,29);
    mats[48]=mat22(29,0,0,1);
    mats[49]=mat22(29,-1,0,1);
    mats[50]=mat22(-1,0,1,-29);
    mats[51]=mat22(29,-2,0,1);
    mats[52]=mat22(-2,1,1,-15);
    mats[53]=mat22(1,0,-15,29);
    mats[54]=mat22(29,-3,0,1);
    mats[55]=mat22(-3,1,1,-10);
    mats[56]=mat22(1,0,-10,29);
    mats[57]=mat22(29,-4,0,1);
    mats[58]=mat22(-4,-1,1,-7);
    mats[59]=mat22(-1,0,-7,-29);
    mats[60]=mat22(29,-5,0,1);
    mats[61]=mat22(-5,1,1,-6);
    mats[62]=mat22(1,0,-6,29);
    mats[63]=mat22(29,-6,0,1);
    mats[64]=mat22(-6,1,1,-5);
    mats[65]=mat22(1,0,-5,29);
    mats[66]=mat22(29,-7,0,1);
    mats[67]=mat22(-7,-1,1,-4);
    mats[68]=mat22(-1,0,-4,-29);
    mats[69]=mat22(29,-8,0,1);
    mats[70]=mat22(-8,3,1,-4);
    mats[71]=mat22(3,-1,-4,11);
    mats[72]=mat22(-1,0,11,-29);
    mats[73]=mat22(29,-9,0,1);
    mats[74]=mat22(-9,-2,1,-3);
    mats[75]=mat22(-2,-1,-3,-16);
    mats[76]=mat22(-1,0,-16,-29);
    mats[77]=mat22(29,-10,0,1);
    mats[78]=mat22(-10,1,1,-3);
    mats[79]=mat22(1,0,-3,29);
    mats[80]=mat22(29,-11,0,1);
    mats[81]=mat22(-11,4,1,-3);
    mats[82]=mat22(4,-1,-3,8);
    mats[83]=mat22(-1,0,8,-29);
    mats[84]=mat22(29,-12,0,1);
    mats[85]=mat22(-12,-5,1,-2);
    mats[86]=mat22(-5,2,-2,-5);
    mats[87]=mat22(2,1,-5,12);
    mats[88]=mat22(1,0,12,29);
    mats[89]=mat22(29,-13,0,1);
    mats[90]=mat22(-13,-3,1,-2);
    mats[91]=mat22(-3,1,-2,-9);
    mats[92]=mat22(1,0,-9,29);
    mats[93]=mat22(29,-14,0,1);
    mats[94]=mat22(-14,-1,1,-2);
    mats[95]=mat22(-1,0,-2,-29);
    return;
  default:
    long p2=(p-1)>>1;
    long sl, sg, x1, x2, x3, y1, y2, y3, a, b, c, q, r;
    mats.push_back(mat22(1,0,0,p));
    mats.push_back(mat22(p,0,0,1));
    sl = -2;
    for(sg=1; sg>sl; sg-=2) // i.e. sg = +1 then -1
      for(r=1; r<=p2; r++)
	{
	  x1=p; x2=-sg*r; y1=0; y2=1; a=-p; b=sg*r;
	  mats.push_back(mat22(x1,x2,y1,y2));
	  while(b!=0)
	    {
	      c=mod(a,b); q=(a-c)/b;
	      x3=q*x2-x1; y3=q*y2-y1;
	      a=-b; b=c; x1=x2; x2=x3; y1=y2; y2=y3;
	      mats.push_back(mat22(x1,x2,y1,y2));
	    }
	}
  }
}
