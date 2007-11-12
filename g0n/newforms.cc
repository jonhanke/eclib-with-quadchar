// FILE  NEWFORMS.CC: implementation of newforms class
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

#include <iomanip>
#ifdef LiDIA_INTS
#include <LiDIA/bigint_matrix.h>
#include <LiDIA/bigmod_matrix.h>
#endif
#include "interface.h"
#include "moddata.h"
#include "symb.h"
#include "cusp.h"
#include "homspace.h"
#include "oldforms.h"
#include "cperiods.h"
#include "newforms.h"

// Functions for ordering newforms
// (1) Old ordering (first aq, then ap for good p);
// with the order for eigenvalues being
// 1,-1 or 0,1,-1,2,-2,...
// (2) New ordering (ap for all p in natural order)
// with the order for eigenvalues being
// -1,1 or ...,-2,-1,0,1,2,... (plain numerical order)

// less_ap(a,b) returns +1,0,-1 according to whether a is
// before/equal/after b in the above ordering
int less_ap(long a, long b, int old=0) 
{ 
  if(!old) return sign(b-a); // the simple new ordering!
  if(a==b) return 0;
  int s = sign(abs(b)-abs(a));
  if(s) return s;
  else return sign(a-b); // this way round! +1 before -1 etc
}

int less_apvec(const vector<long>& v, const vector<long>& w, int old=0); 
int less_apvec(const vector<long>& v, const vector<long>& w, int old) 
{
  vector<long>::const_iterator vi=v.begin(), wi=w.begin();
  while(vi!=v.end())
    {
      int s = less_ap(*vi++,*wi++,old); 
      if(s) return s;
    }
  return 0;
}

struct less_newform_old : public binary_function<newform, newform, bool> { 
  bool operator()(const newform& f, const newform& g) 
  {
    int s = less_apvec(f.aqlist,g.aqlist,1);
    if(s==0) s = less_apvec(f.aplist,g.aplist,1);
    return (s==1);
  }
};

struct less_apvec_function : public binary_function<const vector<long>&, const vector<long>&, bool> { 
  bool operator()(const vector<long>& f, const vector<long>& g) 
  {
    return 1==less_apvec(f,g);
  }
};

vector<long> eiglist(const newform& f, int oldorder)
{
  /*
  cout<<"Entering eiglist with f.aqlist="<<f.aqlist<<"\nand f.aplist=";
  vec_out(cout,f.aplist,10);
  cout<<endl;
  */
  vector<long> eigs;
  primevar pr;
  long N = (f.nf)->modulus;
  vector<long>::const_iterator aqi=f.aqlist.begin();
  vector<long>::const_iterator api=f.aplist.begin();
  vector<long>::iterator eigsi;
  if(oldorder)
    {
      eigs.resize(f.aplist.size());
      eigsi=eigs.begin();
      while(aqi!=f.aqlist.end()) 
	*eigsi++ = *aqi++;
      while(api!=f.aplist.end())
	{
	  if(!::div(pr,N)) *eigsi++ = *api;     
	  api++; pr++;
	}
    }
  else
    {
      eigs=f.aplist; // copy; now adjust the aq:
      eigsi=eigs.begin();
      while(aqi!=f.aqlist.end())
	{
	  if(::div(pr,N)) *eigsi = (*aqi++);
	  eigsi++; pr++;
	}
    }
  /*
  cout<<"Leaving eiglist with eigs=";
  vec_out(cout,eigs,10);
  cout<<endl;
  */
  return eigs;
}

struct less_newform_new : public binary_function<newform, newform, bool> { 
  bool operator()(const newform& f, const newform& g) 
  {
    //    return less_apvec(eiglist(f),eiglist(g),0)==1;    
    return less_apvec(f.aplist,g.aplist,0)==1;    
  }
};

// Newform constructor given the ap and aq lists and extra data (but
// no homology basis), e.g. after reading from newforms file

newform::newform(const vector<int>& data, const vector<long>& aq, const vector<long>& ap, newforms* nfs) : nf(nfs)
{
  sfe=data[0];
  ap0=data[1];
  np0=data[2];
  dp0=data[3];
  loverp=rational(dp0,np0);
  lplus=data[4];
  mplus=data[5];
  lminus=data[6];
  mminus=data[7];
  a=data[8];
  b=data[9];
  c=data[10];
  d=data[11];
  dotplus=data[12];
  dotminus=data[13];
  type=data[14];
  degphi=data[15];
  aqlist=aq;
  aplist=ap;
  index=-1;
  pdot=qdot=0;
}

// Newform constructor, given the homology basis vector(s) and
// Hecke eigenvalues

newform::newform(const vec& vplus, const vec& vminus, const vector<long>& ap, newforms* nfs,long ind)
   :nf(nfs), plusflag(nfs->plusflag), bplus(vplus),bminus(vminus),aplist(ap),index(ind) 
{
  int verbose=(nf->verbose);
  if(verbose) 
    {
      cout<<"Creating H1"; if(plusflag) cout<<"+"; 
      cout<<" newform from aplist..."<<endl;
    }
  // Some default/unset values:
  a=d=1;b=c=0; dotplus=dotminus=1;
  degphi=type=qdot=0; // flags that these has not yet been set
  lminus = mminus = 0;
  lplus = mplus = 0;
  
  long n = nf->modulus;
  long denom = nf->h1->h1denom();
  
  // Fixing the eigenvalue lists: ap is indexed by primes in natural
  // order we need to extract aq (computing any not yet there).
  
  // At the same time we change the enties in aplist for bad primes q
  // from the Wq-eigenvalue to the newform coefficient.
  
  aqlist.resize(nf->npdivs);
  vector<long>::iterator api=aplist.begin(), pi=nf->plist.begin();
  vector<long>::iterator aqi=aqlist.begin();
  primevar pr;   long q, i, j;
  while((api!=aplist.end())&&(aqi!=aqlist.end()))
    {
      q=pr.value(); pr++;
      if(::div(q,n)) 
	{
	  *aqi++=*api; 
	  *api=(::div(q*q,n)? 0: -*api);
	  pi++;
	}
      api++;
    }
  if(aqi!=aqlist.end()) // compute missing aq
    {
      long piv;
      ssubspace espace=make1d(bplus,piv); 
      piv*=denom;
      while(aqi!=aqlist.end()) // compute missing aq
	{
	  q=*pi++;
	  if(verbose) cout<<"Computing Wq for q="<<q<<"..."<<flush;
	  smat Wq = nf->h1->s_heckeop_restricted(q,espace,1,0);
	  long aq = Wq.elem(1,1) / piv;
	  if(verbose) cout<<"aq ="<<aq<<endl;
	  *aqi++=aq;
	}
    }
  if(verbose) cout<<"aqlist = "<<aqlist<<endl;

  // get ap for p=p0:

  pr.init(); api=aplist.begin();
  while(pr.value()!=nf->p0) {pr++; api++;}
  ap0=*api;
  np0 = 1 + (nf->p0) - ap0;
  if(verbose) cout<<"ap0 = "<<ap0<<"\tnp0 = "<<np0<<endl;

  //Compute sfe:

  sfe=-1;
  for(i=0; i<(nf->npdivs); i++) sfe*=aqlist[i];
  if(verbose) cout<<"sfe = "<<sfe<<endl;

  //Compute pdot, dp0, loverp

  long cuspidalfactorplus = 1;
  long cuspidalfactorminus = 1;
  vec bplusc, bminusc;
  if(!(nf->h1->cuspidal))
    {
      smat tkernbas = transpose(nf->h1->kern.bas());
      bplusc=tkernbas*bplus;
      cuspidalfactorplus = vecgcd(bplusc);
      bplusc /= cuspidalfactorplus;
      if(!plusflag) 
	{
	  bminusc=tkernbas*bminus;
	  cuspidalfactorminus = vecgcd(bminusc);
	  bminusc/= cuspidalfactorminus;
	  type=3-vecgcd(bplusc-bminusc);
	}
      if(verbose&&(cuspidalfactorplus*cuspidalfactorminus>1))
	{
	  cout<<"cuspidalfactorplus  = "<<cuspidalfactorplus<<endl;
	  if(!plusflag) 
	    cout<<"cuspidalfactorminus = "<<cuspidalfactorminus<<endl;
	}
    } 
  int ncoords=nf->h1->coord_vecs.size()-1;
  coords=vec(ncoords);
  vec mcoords; if(!plusflag)  mcoords=vec(ncoords);
  for(i=1; i<=ncoords; i++)
    {
      coords[i]=dotmodp(nf->h1->coord_vecs[i],bplus,92681);
      if(!plusflag) mcoords[i]=dotmodp(nf->h1->coord_vecs[i],bminus,92681);
    }
  long denomplus=vecgcd(coords);
  long denomminus; if(!plusflag) denomminus=vecgcd(mcoords);
  if(verbose)
    { 
      cout<<"denomplus   = "<<denomplus<<endl;
      if(!plusflag) 
	{
	  cout<<"denomminus   = "<<denomminus<<endl;
	  cout<<"type = "<<type<<endl;
	}
    }
  denomplus  *=cuspidalfactorplus;
  if(!plusflag) denomminus *=cuspidalfactorminus;
  
  // DO NOT scale pdot by denom: factor will cancel when used to compute ap
  pdot = abs((nf->mvp)*bplus); 
  dp0=pdot;
  // DO scale dp0 since it is used to compute L/P
  if(dp0!=0)
    {
      if(denomplus>1)
	{
	  if(::div(denomplus,dp0))  dp0/=denomplus;
	  else 
	    cout<<"newform constructor error: dp0 not divisible by denomplus!"
		<<endl; 
	}
    }
  loverp = rational(dp0,np0);   
  if(verbose) 
    {
      cout<<"pdot = "<<pdot<<endl;
      cout<<"dp0 = "<<dp0<<endl;
      cout<<"np0 = "<<np0<<endl;
      cout<<"loverp = "<<loverp<<endl;
    }

  if(verbose) cout<<"type = "<<type<<endl;

if(!plusflag) 
  {
  //
  // Find deg(phi)
  //
#ifdef DEG_PHI
    if(verbose) cout<<"computing deg(phi)..."<<flush;
    degphi=jumpinfo->degphi(bplusc,bminusc,type);
    if(verbose) cout<<"done..."<<flush;
#else
    degphi=0;
#endif
  }
 
  //
  // Find twisting primes if N non-square
  //
  if(verbose) cout<<"computing twisting primes..."<<flush;

  if(dp0!=0)
    {
      lplus=1; 
      mplus=1; // dummy value, not used
    }

  if(!nf->squarelevel)
    for (primevar lvar; lvar.ok() && (mplus==0
				      ||(!plusflag)&&(mminus==0)); lvar++)
      {
	while (n%lvar==0) {lvar++;}
	long l = lvar;
	if (legendre(-n,l)!=sfe) continue;

	if((mplus==0)&&(l%4==1))
	  {
	    lplus = l;  // cout << "Trying lplus = " << l << "\n";
	    map<long,vec>::const_iterator vi = nf->mvlplusvecs.find(l);
	    if(vi==nf->mvlplusvecs.end())
	      mplus = abs((nf->mvlplusvecs[l]=nf->h1->manintwist(l))*bplus);
	    else
	      mplus = abs((vi->second)*bplus);
	    if((denomplus>1)&&(mplus!=0))
	      {
		if(::div(denomplus,mplus))  mplus/=denomplus;
		else 
		  cout<<"Warning in newform constructor: mplus not divisible by denomplus!"
		      <<endl; 
	      }
	  }
	if((!plusflag)&&(mminus==0)&&(l%4==3))
	  {
	    lminus = l;  // cout << "Trying lminus = " << l << "\n";
	    map<long,vec>::const_iterator vi = nf->mvlminusvecs.find(l);
	    if(vi==nf->mvlminusvecs.end())
	      mminus = abs((nf->mvlminusvecs[l]=nf->h1->manintwist(l))*bminus);
	    else
	      mminus = abs((vi->second)*bminus);
	    if((denomminus>1)&&(mminus!=0))
	      {
		if(::div(denomminus,mminus))  mminus/=denomminus;
		else 
		  cout<<"Warning in newform constructor: mminus not divisible by denomminus!"
		      <<endl; 
	      }
	  }
      }
  
  if(verbose) 
    {
      cout<<"done..."<<flush;
      cout<<"lplus = "<<lplus<<endl;
      cout<<"mplus = "<<mplus<<endl;
      cout<<"lminus = "<<lminus<<endl;
      cout<<"mminus = "<<mminus<<endl;
    }

  if(plusflag) return;

  // find a,b,c,d,dotplus,dotminus
  if(verbose) cout<<"computing a,b,c,d..."<<flush;
  int found=0;
  for(d=2; !found; d++)
    {
      if(1==gcd(d,n))
        {
          for(b=1; (b<d) && !found; b++)
            {
              if(1==bezout(d,-n*b,a,c))
                {
                  vec v = nf->h1->cycle(b,d);
//                if(!(nf->h1->cuspidal)) v = nf->h1->cuspidalpart(v);
                  dotplus=abs(v*bplus/denomplus);
                  dotminus=abs(v*bminus/denomminus);
                  found=((dotplus!=0)&&(dotminus!=0));
                }
            }
        }
    }
  b--; d--;  //because they get incremented BEFORE the loop end-test
  if(d<0) {a=-a; b=-b; c=-c; d=-d;} // because we need d>0 for integration
  if(verbose) 
    {
      cout<<"done: ";
      cout << "[(" <<a<<","<<b<<";"<<c
	   <<","<<d<<"),"<<dotplus<<","<<dotminus
	   <<";"<<type<<"]"<<endl;
    }
}

void newform::add_more_ap(int nap)
{
  if(aplist.size()>=nap) return;
  int verbose=(nf->verbose);
  long piv, p, ap;
  // Do not make the espace right away, as it is possible that the
  // only ap we are missing are aq which we already have...
  ssubspace espace;
  int have_espace=0;

  primevar pr(nap,aplist.size()+1);
  while(aplist.size()<nap)
    {
      p=pr;
      if(::div(p,nf->modulus))
	{
	  if(::div(p*p,nf->modulus)) 
	    ap=0; 
	  else
	    ap=-aqlist[find(nf->plist.begin(),nf->plist.end(),p)-nf->plist.begin()];
	}
      else
	{	  
	  if(verbose>1) cout<<"Computing Tp for p="<<p<<endl;
	  if(!have_espace)
	    {
	      espace=make1d(bplus,piv);
	      piv*=nf->h1->h1denom();
	      have_espace=1;
	    }
	  ap = (nf->h1->s_heckeop_restricted(p,espace,1,0)).elem(1,1) / piv;
	}
      aplist.push_back(ap);
      pr++;
    }
  if(verbose>1) cout<<"aplist = "<<aplist<<endl;
}

newforms::~newforms(void)
{
  if(of) delete of;  
  if(h1) delete h1;
}

void newforms::makeh1()
{
  //  if(!h1) cout<<"Constructing h1 with plusflag="<<plusflag<<endl;
  //  else cout<<"h1 already exists"<<endl;
  if(!h1) h1 = new homspace(modulus,plusflag,cuspidal,0);
}

void newforms::createfromscratch(long ntp)
{
  makeh1();
  //  cout<<"Constructing oldforms with plusflag="<<plusflag<<endl; 
  of = new oldforms(ntp,h1,(verbose>1),plusflag); // h1 provides the level*
  if(verbose>1) of->display();
  maxdepth = of->nap;
  long mindepth = npdivs;
  dimsplit = n1ds = 0;
  upperbound = h1->dimension-(of->totalolddim);
//N.B. For later reference, upperbound=n1ds+n2ds;
  if(upperbound>0)  // Else no newforms certainly so do no work!
    {  
       mvp=h1->maninvector(p0); 
       //       cout<<"mvp                 = "<<mvp<<endl;
       if(verbose>1) cout<<"h1 denom = "<<h1->h1denom()<<endl;
       long totalmult=upperbound;
       if (totalmult==0) n1ds=0;      
       else 
	 {
	   form_finder ff(this,plusflag,maxdepth,mindepth,1,cuspidal,verbose);
	   basisflag=0;
	   ff.find();
	 }
    }
  n2ds=0;
  if(verbose)
    {
      cout << "Total dimension " << h1->dimension 
	   << " made up as follows:\n";
      cout << "dim(newforms) = " << n1ds+n2ds 
	   << " of which " << n1ds << " is rational; \n";
      if(h1->dimension==of->totalolddim+n1ds)
	cout<<"The whole space splits over Q.\n";
      cout<<endl;
    }

  if(n1ds==0) return;

  int i,j,nap,maxnap=0;

  if((n1ds>1)&&(modulus<130000)) // reorder into old order
    {
      if(verbose) 
	{
	  cout<<"Reordering newforms into old order as N<130000"<<endl;
	  //	  cout<<"Before sorting:\n"; display();
	}
      sort(1);
      if(verbose) 
	{
	  //	  cout<<"After sorting:\n"; display();
	}
    }
  // At this point the newforms may contain different numbers of ap,
  // so we need to even these up, which we do by computing more ap for
  // those which need it.
   if(n1ds>1)
    {
      for(i=0; i<n1ds; i++) 
	if((nap=nflist[i].aplist.size())>maxnap) maxnap=nap;
      if(verbose) 
	cout<<"Max number of ap in newforms so far is "<<maxnap<<endl;
      for(i=0; i<n1ds; i++) 
	if((nap=nflist[i].aplist.size())<maxnap) 
	  {
	    if(verbose)
	      cout<<"Newform #"<<(i+1)<<" has only "<<nap
		  <<" ap so we need to compute more..."<<endl;
	    nflist[i].add_more_ap(maxnap);
	  }  
    }

  // Compute homspace::projcoord, so projcycle can be used
  // Replaces coord_vecs of homspace with projections onto eigenspaces
  h1->projcoord.init(h1->coord_vecs.size()-1,n1ds);
  for (j=1; j<=n1ds; j++)
    h1->projcoord.setcol(j, nflist[j-1].coords);

  easy = 1;
  for (i=0; i<n1ds; i++)
    if (nflist[i].pdot==0) easy=0;

  // Look for a j0 such that nflist[i].bplus[j0]!=0 for all i
  int ok=0; j0=0;
  /*
  cout<<h1->h1dim()<<endl;
  cout<<dim(nflist[0].bplus)<<endl;
  for (i=0; i<n1ds; i++)
    {
      cout<<i<<": "<<nflist[i].bplus<<endl;
    }
  */
  for(j=1; (!ok)&&(j<=h1->h1dim()); j++)
    {
      ok=1;
      for (i=0; (i<n1ds)&&ok; i++)
	ok=(nflist[i].bplus[j]!=0);
      if(ok) j0=j;
    }
  if(ok)
    {
      if(verbose)  cout<<"j0="<<j0<<endl;
      jlist.insert(j0);
    }
  else
    {
      if(verbose) 
	cout<<"Failed to find j0 such that nflist[i].bplus[j0]!=0 for all i"
	    <<endl;
      // Find out which pivots we'll be using:
      for (i=0; i<n1ds; i++)
	{
	  vec& bas = nflist[i].bplus;
	  j=1; while(bas[j]==0) j++;
	  jlist.insert(j);
	}
      if(verbose)  cout<<"jlist="<<jlist<<endl;
    }
}  


long newforms::dimoldpart(const vector<long> l) 
{
  return of->dimoldpart(l);
}

// if(!cuspidal) we really should check here that the basis vector b1
// is in ker(delta), by cheking that b1*h1->deltamat == 0

void newforms::use(const vec& b1, const vec& b2, const vector<long> aplist)
{
  if(basisflag&&plusflag)
    {
      nflist[j1ds++].bplus=b1;
      if(verbose) 
	cout<<"Finished constructing basis vector for newform #"<<j1ds<<endl;
      return;
    }
  n1ds++;
  if(verbose) 
    {
      cout<<"Constructing newform #"<<n1ds<<" with eigs ";
      vec_out(cout,aplist,10);
      cout<<endl;
    }
  nflist.push_back(newform(b1,b2,aplist,this));
  if(verbose) 
    cout<<"Finished constructing newform #"<<n1ds<<endl;
}

// Sort newforms 
void newforms::sort(int oldorder)
{
  if(oldorder)
    ::sort(nflist.begin(),nflist.end(),less_newform_old());
  else
    ::sort(nflist.begin(),nflist.end(),less_newform_new());
}
  

void newforms::display(void) const
{
 if (n1ds==0) {cout<<"No newforms."<<endl; return;}
 cout << "\n"<<n1ds<<" newform(s) at level " << modulus << ":" << endl;
 cout<<"p0="<<p0<<endl;
 // if(dim(mvp)!=0) cout<<"mvp="<<mvp<<endl;
 if(easy!=-1) cout<<"easy="<<easy<<endl;
 cout<<"#ap=\t"<<nflist[0].aplist.size()<<endl;
 for(long i=0; i<n1ds; i++)
   {cout<<i+1<<":\t";
    nflist[i].display();
  }
}

void newform::display(void) const
{
  cout << "aplist = ";  
  vec_out(cout,aplist,20);  // outputs at most 20 eigs.
  cout<< endl;
  //  cout << "basis = " << bplus << endl;
  cout << "aq = " << aqlist<<endl;
  cout << "ap0 = " << ap0
       <<", dp0 = " << dp0
       <<", np0 = " << np0;
  if(pdot!=0) cout <<", pdot = " << pdot;
  if(qdot!=0) cout << ", qdot = " << qdot;
  cout <<endl;
  cout << "SFE = " << sfe << ",\tL/P = " << loverp << endl;
  if(lplus>0) cout << "lplus = " << lplus << ", mplus = " << mplus << endl;
  if(lminus>0) cout << "lminus = " << lminus << ", mminus = " << mminus << endl;
  if(a!=0) cout << "[(" <<a<<","<<b<<";"<<c
		<<","<<d<<"),"<<dotplus<<","<<dotminus
		<<";"<<type<<"]"<<endl;

  if(index!=-1)cout << "Splitting index = " << index << endl;
}

void putout(ofstream& of, short a, int binflag)
{
  if(binflag)
    of.write((char*)&a,sizeof(short));
  else
    of<<setw(5)<<a;
}
void putout(ofstream& of, int a, int binflag)
{
  if(binflag)
    of.write((char*)&a,sizeof(int));
  else
    of<<setw(10)<<a;
}
void putout(ofstream& of, long a, int binflag)
{
  if(binflag)
    of.write((char*)&a,sizeof(long));
  else
    of<<setw(15)<<a;
}
void nl(ofstream& of, int binflag) 
{if(!binflag) of<<"\n";}

void newforms::output_to_file(int binflag) const
{
  long i,j;

  char* name = new char[20];
  if(binflag) 
    sprintf(name,"%s/x%ld",NF_DIR,modulus);
  else 
    sprintf(name,"%s/e%ld",NF_DIR,modulus);
  ofstream out(name);
  if(!out)
    {
      cout<<"Unable to open file "<<name<<" for newform output"<<endl;
      delete name;
      abort();
    }
  delete name;
  
  if(n1ds==0) 
    {  
      putout(out,(int)0,binflag);
      putout(out,(int)0,binflag);
      putout(out,(int)0,binflag);
      out.close(); 
      return;
    }
    
  // Line 1:  #newforms, #aq, #ap
  putout(out,(int)n1ds,binflag);
  putout(out,(int)nflist[0].aqlist.size(),binflag);
  putout(out,(int)nflist[0].aplist.size(),binflag);
  nl(out,binflag);
  // Line 2:  blank line
  nl(out,binflag);
  // Line 3:  sign of f.e. for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].sfe,binflag); 
  nl(out,binflag);
  // Line 4:  ap0 for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].ap0,binflag); 
  nl(out,binflag);
  // Line 5:  np0 for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].np0,binflag); 
  nl(out,binflag);
  // Line 6:  dp0 for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].dp0,binflag); 
  nl(out,binflag);
  // Line 7:  lplus for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].lplus,binflag);	    
  nl(out,binflag);
  // Line 8:  mplus each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].mplus,binflag);	
  nl(out,binflag);
  // Line 9:  lminus each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].lminus,binflag);	
  nl(out,binflag);
  // Line 10:  mminus for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].mminus,binflag);	
  nl(out,binflag);
  // Line 11:  matrix entry a for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].a,binflag); 
  nl(out,binflag);
  // Line 12:  matrix entry b for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].b,binflag);	
  nl(out,binflag);
  // Line 13:  matrix entry c for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].c,binflag);	
  nl(out,binflag);
  // Line 14:  matrix entry d for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].d,binflag);	
  nl(out,binflag);
  // Line 15:  dotplus for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].dotplus,binflag); 
  nl(out,binflag);
  // Line 16:  dotminus for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].dotminus,binflag); 
  nl(out,binflag);
  // Line 17:  lattice type for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].type,binflag); 
  nl(out,binflag);
  // Line 18:  deg(phi) for each newform
  for(i=0; i<n1ds; i++) putout(out,(int)nflist[i].degphi,binflag);	
  nl(out,binflag);
  // Line 19:  blank line
  nl(out,binflag);
  
  // Lines 20-(20+#aq):  aq for each newform;  then blank line
  for(j=0; j<nflist[0].aqlist.size(); j++)
    {
      for(i=0; i<n1ds; i++) putout(out,(short)nflist[i].aqlist[j],binflag); 
      nl(out,binflag);
    }
  nl(out,binflag);
  
  // Lines (21+#aq)-(20+#aq+#ap):  ap for each newform
  for(j=0; j<nflist[0].aplist.size(); j++)
    {
      for(i=0; i<n1ds; i++) putout(out,(short)nflist[i].aplist[j],binflag); 
      nl(out,binflag);
    }
  out.close();
}

// Read in newform data from file NF_DIR/xN 

void newforms::createfromdata(long ntp, int create_from_scratch_if_absent)
{
  long i, j, n = modulus;
  if(verbose) cout << "Retrieving newform data for N = " << n << endl;

  char* name = new char[20];
  sprintf(name,"%s/x%ld",NF_DIR,modulus);
  ifstream datafile(name);
  if(!datafile.is_open())
    {
      cout<<"Unable to open file "<<name<<" for newform input"<<endl;
      delete name;
      if(create_from_scratch_if_absent)
	{
	  cout<<"Creating from scratch instead"<<endl;
	  createfromscratch(ntp);
	  output_to_file();
	  cout << "Finished creating newform data for N = " << n << endl;
	  if(verbose) display();
	  return;
	}
      else
	{
	  cout<<"Quitting"<<endl;
	  abort();
	}
    }
  delete name;

  short temp_short;
  int temp_int;
  datafile.read((char*)&temp_int,sizeof(int));   // = number of newforms
  n1ds=temp_int;
  datafile.read((char*)&temp_int,sizeof(int));   // = number of bad primes
  datafile.read((char*)&temp_int,sizeof(int));   // = number of eigs
  nap=temp_int;
  if(n1ds==0) 
    {
      if(verbose) cout << "No newforms at level " << n << endl;
      datafile.close();
      return;
    }

  vector<int> * data = new vector<int>[n1ds];
  vector<long> * aq = new vector<long>[n1ds];
  vector<long> * ap = new vector<long>[n1ds];

  // read extra data for each newform
  for(i=0; i<n1ds; i++) data[i].resize(16);
  long ntotal = 16*n1ds;
  int* batch_i = new int[ntotal];
  datafile.read((char*)batch_i,ntotal*sizeof(int));
  int *batch_i_ptr = batch_i;
  for(j=0; j<16; j++) 
    for(i=0; i<n1ds; i++) 
      data[i][j]=*batch_i_ptr++;
  delete batch_i;
  //  cout<<"Raw  data:\n";  for(i=0; i<n1ds; i++) cout<<data[i]<<endl;

  // read aq for each newform
  for(i=0; i<n1ds; i++) aq[i].resize(npdivs);
  ntotal = npdivs*n1ds;
  short* batch = new short[ntotal];
  datafile.read((char*)batch,ntotal*sizeof(short));
  short *batchptr = batch;
  for(j=0; j<npdivs; j++) 
    for(i=0; i<n1ds; i++) 
      aq[i][j]=*batchptr++;
  //  cout<<"Raw  aq:\n";  for(i=0; i<n1ds; i++) cout<<aq[i]<<endl;

  // read ap for each newform
  for(i=0; i<n1ds; i++) ap[i].resize(nap);
  ntotal = nap*n1ds;
  delete batch;
  batch = new short[ntotal];
  datafile.read((char*)batch,ntotal*sizeof(short));
  batchptr = batch;
  for(j=0; j<nap; j++) 
    for(i=0; i<n1ds; i++) 
      ap[i][j]=*batchptr++;
  //  cout<<"Raw  ap:\n";  for(i=0; i<n1ds; i++) cout<<ap[i]<<endl;
  delete batch;
  datafile.close();

  // construct the newforms from this data
  nflist.reserve(n1ds);
  for(i=0; i<n1ds; i++) 
    nflist.push_back(newform(data[i],aq[i],ap[i],this));

  delete[] ap; delete[] aq; delete[] data;

  if(verbose) 
    {
      cout << "Finished reading newform data for N = " << n << endl;
      display();
    }
}

// Read in newform data from old-style data files eigs/xN and intdata/[ep]N

void newforms::createfromolddata()
{
  long i, j, n = modulus;
  if(verbose) 
    cout << "Retrieving old-style newform data for N = " << n << endl;

  char* eigsname = new char[20];
  sprintf(eigsname,"eigs/x%ld",n);
  ifstream eigsfile(eigsname);

  if(!eigsfile.is_open())
    {
      cout<<"Unable to open file "<<eigsname<<" for eigs input"<<endl;
      delete eigsname;
      abort();
      return;
    }
  delete eigsname;

  easy=0;
  short temp;
  eigsfile.read((char*)&temp,sizeof(short));   // # newforms
  n1ds=temp;
  eigsfile.read((char*)&temp,sizeof(short));   // # irrational newforms
  eigsfile.read((char*)&temp,sizeof(short));   // # ap
  nap=temp;
  if(n1ds==0) 
    {
      if(verbose) cout << "No newforms at level " << n << endl;
      eigsfile.close();
      return;
    }

  // read ap for each newform
  vector<long> * ap = new vector<long>[n1ds];
  for(i=0; i<n1ds; i++) ap[i].resize(nap);
  long ntotal = nap*n1ds;
  short* batch = new short[ntotal];
  eigsfile.read((char*)batch,ntotal*sizeof(short));
  eigsfile.close();
  short* batchptr = batch;
  for(j=0; j<nap; j++) 
    for(i=0; i<n1ds; i++) 
      ap[i][j]=*batchptr++;
  //  cout<<"Raw  ap:\n";  for(i=0; i<n1ds; i++) cout<<ap[i]<<endl;
  delete batch;

  // extract aq for each newform
  vector<long> * aq = new vector<long>[n1ds];
  for(i=0; i<n1ds; i++) aq[i].resize(npdivs);
  primevar pr; long q, k, a;
  for(k=0, j=0; j<plist.size(); j++)
    {
      q = plist[j];
      int q2divN = ::div(q*q,modulus);
      while((long)pr!=q) {pr++; k++;}
      for(i=0; i<n1ds; i++)
	{
	  a = ap[i][k];
	  aq[i][j] = a;
	  ap[i][k] = (q2divN? 0: -a);
	}
    }
  //  cout<<"Raw  aq:\n";  for(i=0; i<n1ds; i++) cout<<aq[i]<<endl;

  // read extra data for each newform
  vector<int> * data = new vector<int>[n1ds];
  for(i=0; i<n1ds; i++) data[i].resize(16);
  char* intdataname = new char[20];
  sprintf(intdataname,"intdata/e%ld",n);
  ifstream intdatafile(intdataname);
  if(!intdatafile.is_open())
    {
     intdataname[8] = 'p';
     intdatafile.clear();
     intdatafile.open(intdataname);
     if(!intdatafile.is_open())
       {
	 cout<<"Unable to open data file "<<intdataname<<" for data input"<<endl;
	 delete intdataname;
	 abort();
	 return;
       }
    }
  delete intdataname;
  
  long nloverp, dloverp, dp0, np0;
  for(i=0; i<n1ds; i++)
    {
      //      cout<<"Reading intdata for form #"<<(i+1)<<endl;
      intdatafile >> data[i][15]; // degphi
      //      cout<<"degphi = "<<data[i][15]<<endl;
      intdatafile >> data[i][0];  // sfe
      //      cout<<"sfe = "<<data[i][0]<<endl;
      intdatafile >> nloverp;     // num(L/P)
      intdatafile >> dloverp;     // den(L/P)
      intdatafile >> data[i][14]; // type
      //      cout<<"type = "<<data[i][14]<<endl;
      intdatafile >> dp0; data[i][3]=dp0;  // dp0
      //      cout<<"dp0 = "<<data[i]=dp0[3]<<endl;
      intdatafile >> np0; data[i][2]=np0;  // np0
      //      cout<<"np0 = "<<data[i][2]<<endl;
      if(dp0==0) data[i][3]=(2*nloverp*np0)/dloverp;
      data[i][1]=1+p0-data[i][2]; // ap0 (not in intdata file)
      //      cout<<"ap0 = "<<data[i][1]<<endl;
      intdatafile >> data[i][4];  // lplus
      intdatafile >> data[i][5];  // mplus
      intdatafile >> data[i][6];  // lminus
      intdatafile >> data[i][7];  // mminus
      intdatafile >> data[i][8];  // a
      intdatafile >> data[i][9];  // b
      intdatafile >> data[i][10]; // c
      intdatafile >> data[i][11]; // d
      intdatafile >> data[i][12]; // dotplus
      intdatafile >> data[i][13]; // dotminus
    }  
  intdatafile.close();
  //  cout<<"Raw  data:\n";  for(i=0; i<n1ds; i++) cout<<data[i]<<endl;


  // construct the newforms from this data
  nflist.reserve(n1ds);
  for(i=0; i<n1ds; i++) 
    nflist.push_back(newform(data[i],aq[i],ap[i],this));
  //  delete ap; delete aq; delete data;

  if(verbose) 
    {
      cout << "Finished reading oldstyle newform data for N = " << n << endl;
      display();
    }
}

// Construct bases (homology eigenvectors) from eigenvalue lists:
void newforms::makebases()
{
  if(n1ds==0) return;
  if((dim(nflist[0].bplus)>0)&&(plusflag||(dim(nflist[0].bminus)>0))) return;
  if(verbose) cout << "Making homspace..."<<flush;
  makeh1();
  if(verbose) cout << "done." << endl;
  mvp=h1->maninvector(p0); 
  if(verbose) cout << "Making form_finder..."<<flush;
  form_finder splitspace(this, plusflag, nap, 0, 1, cuspidal, verbose);
  if(verbose) cout << "Recovering eigenspace bases with form_finder..."<<endl;
  basisflag=1; j1ds=0;
  vector< vector<long> > eigs(n1ds);
  int i,j;

  for(i=0; i<n1ds; i++) eigs[i]=eiglist(nflist[i]);
  if(verbose>1) 
    {
      cout<<"Before sorting, eig lists are:"<<endl;
      for(i=0; i<n1ds; i++) {vec_out(cout,eigs[i],10);    cout<<endl;}
      cout<<"sorting..."<<endl;
    }
  ::sort(eigs.begin(),eigs.end(),less_apvec_function());
  /*
  vector<long> temp;
  for(i=n1ds-1; i>0; i--)
    for(j=0;j<i;j++)
      {
	if(1==less_apvec(eigs[j+1],eigs[j]))
	  {
	    temp=eigs[j+1]; eigs[j+1]=eigs[j]; eigs[j]=temp;
	  }
      }
  */
  if(verbose>1) 
    {
      cout<<"After sorting, eig lists are:"<<endl;
      for(i=0; i<n1ds; i++) {vec_out(cout,eigs[i],10);    cout<<endl;}
    }
  //  exit(0);
  if(!plusflag) {n1ds=0; nflist.resize(0);}
  splitspace.recover(eigs);
  if(verbose) cout << "...done."<<endl;
  if((n1ds>1)&&(modulus<130000)) // reorder into old order
    {
      if(verbose) 
	{
	  cout<<"Reordering newforms back into old order as N<130000"<<endl;
	  if(verbose>1) cout<<"Before sorting:\n"; display();
	}
      sort(1);
      if(verbose>1) 
	{
	  cout<<"After sorting:\n"; display();
	}
    }
}

//#define OLD_AP_METHOD

vector<long> newforms::apvec(long p) //  computes a[p] for each newform
{
  vector<long> apv(n1ds);
  vec v;
  long i,j,iq,ap; 
  if(::div(p,modulus)) // we already have all the aq
    { 
      if(::div(p*p,modulus))
	for (i=0; i<n1ds; i++) apv[i] = 0;
      else
	{
	  iq = find(plist.begin(),plist.end(),p)-plist.begin();
	  for (i=0; i<n1ds; i++) apv[i] = -nflist[i].aqlist[iq];
	}
      return apv;
    }
  
  // now p is a good prime

  long maxap=(long)(2*sqrt((double)p)); // for validity check

#ifdef OLD_AP_METHOD

  if ( easy)  //   Symbol {0,infinity} non-trivial in all cases
    { 
      v = h1->projmaninvector(p);   //starts at 1
//      cout<<"projmaninvector("<<p0<<") = "<<h1->projmaninvector(p0)<<endl;
//      cout<<"projmaninvector("<<p<<") = "<<v<<endl;
      for (i=0; i<n1ds; i++)
	{ 
	  apv[i] = ap =  1+p-((v[i+1]*nflist[i].np0)/nflist[i].pdot);
	  if((ap>maxap)||(-ap>maxap))
	    {
	      cout<<"Error:  eigenvalue "<<ap<<" for p="<<p
		  <<" for form # "<<(i+1)<<" is outside valid range "
		  <<-maxap<<"..."<<maxap<<endl;
	      abort();
	    }
	}
      return apv;
    }

  // Now easy=0, so we use symbol {q,infinity}

  v = h1->newhecke(p,nq,dq) - (p+1)*initvec;  //starts at 1
  for (i=0; i<n1ds; i++)
    { 
      apv[i] = ap = 1+p- ( (v[i+1]*nflist[i].np0) / nflist[i].qdot );
      if((ap>maxap)||(-ap>maxap))
	{
	  cout<<"Error:  eigenvalue "<<ap<<" for p="<<p
	      <<" for form # "<<(i+1)<<" is outside valid range "
	      <<-maxap<<"..."<<maxap<<endl;
	  abort();
	}
    }
#else // new ap method

  map<long,vec> images; // [j,v] stores image of j'th M-symbol in v
  vec bas, imagej;
  long fac;
  long p2=(p-1)>>1;
  long sl, sg, x1, x2, x3, y1, y2, y3, a, b, c, q, r;
  long u1,u2,u3;
  long ind;
  
  // Compute the image of the necessary M-symbols (hopefully only one)
#if(0)
  matop mlist(p,modulus);
  for(std::set<long>::const_iterator jj=jlist.begin(); jj!=jlist.end(); jj++)
    {
      imagej=vec(n1ds); // initialised to 0
      j=*jj;
      v=(h1->applyop(mlist,h1->freemods[j-1])).as_vec();
      for(i=1; i<=n1ds; i++)
	imagej[i]=v*nflist[i-1].bplus;
      images[j]=imagej;
      cout<<" image is "<<imagej<<endl;
    }
#else
  for(std::set<long>::const_iterator jj=jlist.begin(); jj!=jlist.end(); jj++)
    {
      imagej=vec(n1ds); // initialised to 0
      j=*jj;
      symb s = h1->symbol(h1->freegens[j-1]);
//      cout<<"Computing image of "<<s<<"..."<<flush;
      long u=s.cee(),v=s.dee(); 
      mat& pcd = h1->projcoord;
// Matrix [1,0;0,p]
      ind = h1->coordindex[h1->index2(u,p*v)];
      if(ind>0) imagej+=pcd.row(ind);
      else if(ind<0) imagej-=pcd.row(-ind);
// Matrix [p,0;0,1]
      ind = h1->coordindex[h1->index2(p*u,v)];
      if(ind>0) imagej+=pcd.row(ind);
      else if(ind<0) imagej-=pcd.row(-ind);
// Other matrices
      for(sg=0; sg<2; sg++) // signs
	for(r=1; r<=p2; r++)
	  {
	    a = -p; 
	    b = sg ? -r : r ;
	    u1=u*p; u2=v-u*b;
	    ind = h1->coordindex[h1->index2(u1,u2)];
	    if(ind>0) imagej+=pcd.row(ind);
	    else if(ind<0) imagej-=pcd.row(-ind);
	    while(b!=0)
	      {
		c=mod(a,b); q=(a-c)/b;
		if(q==1) {u3=  u2-u1;} else {u3=q*u2-u1;}
		a=-b; b=c; u1=u2; u2=u3;
		ind = h1->coordindex[h1->index2(u1,u2)];
		if(ind>0) imagej+=pcd.row(ind);
		else if(ind<0) imagej-=pcd.row(-ind);
	      }
	  }    
      images[j]=imagej/(h1->h1denom());
//      cout<<" image is "<<imagej<<endl;
    }
#endif

  for (i=0; i<n1ds; i++)
    {
      vec& bas = nflist[i].bplus;
      if(j0>0) j=j0; else
	{
	  j=1; while(bas[j]==0) j++;
	}
      fac=bas[j];
//      cout<<"i="<<i<<", bas="<<bas<<", using j="<<j<<", factor "<<fac<<endl;
      imagej=images[j];
// recover eigenvalue:
      apv[i]=ap=imagej[i+1]/fac;
// check it is in range:
      if((ap>maxap)||(-ap>maxap))
	{
	  cout<<"Error:  eigenvalue "<<ap<<" for p="<<p
	      <<" for form # "<<(i+1)<<" is outside valid range "
	      <<-maxap<<"..."<<maxap<<endl;
	  abort();
	}
    }  
#endif

  return apv;
}

void newforms::addap(long last) // adds ap for primes up to the last'th prime
{
  if(n1ds==0) return;
  long i, j, p;

#ifdef OLD_AP_METHOD
  if(!easy)
    {
      //  Look for a rational q for which {q,infinity} is nontrivial
      int stillok,foundq=0;
      for (dq = 2; !foundq; dq++)
	{ 
	  if (gcd(dq,modulus)==1)
	    { 
	      for (nq = 1; (nq<=dq/2) && !foundq; nq++)
	    { 
	      if (gcd(nq,dq)==1)
		{ // test candidate q=nq/dq
		  initvec=h1->projcycle(nq,dq);  //starts at 1
		  for (i=0, stillok=1; (i<n1ds) && stillok; i++)
		    { 
		      nflist[i].qdot=nflist[i].pdot-nflist[i].np0*initvec[i+1];
		      stillok = (nflist[i].qdot!=0);
		    }
		  foundq = stillok;
		}
	    }
	    }
	}
      nq--;dq--;
    } // if(easy) ...
#endif
  
  j=0;
  if(verbose>1)  // output the ap already known...
    for(primevar pr(nflist[0].aplist.size()); pr.ok(); pr++, j++) 
      {
	p=(long)pr;
	if(ndiv(p,modulus)) cout<<"p="; else cout<<"q="; 
	cout<<p<<":\t";
	{
	  for (i=0; i<n1ds; i++) cout<<nflist[i].aplist[j]<<"\t";
	  cout<<endl;
	}
      }
  // Now compute and output the rest of the ap...
  for(primevar pr(last,1+nflist[0].aplist.size()); pr.ok(); pr++) 
    {
      p=(long)pr;
      vector<long> apv=apvec(p);
      if(verbose) 
	{
	  if(ndiv(p,modulus)) cout<<"p="; else cout<<"q="; 
	  cout<<p<<":\t";
	  for (i=0; i<n1ds; i++) cout<<apv[i]<<"\t";
	  cout<<endl;
	}
      for (long i=0; i<n1ds; i++) nflist[i].aplist.push_back(apv[i]);
    }
}

void output_to_file_no_newforms(long n, int binflag)
{
  char* name = new char[20];
  if(binflag) 
    sprintf(name,"%s/x%ld",NF_DIR,n);
  else 
    sprintf(name,"%s/e%ld",NF_DIR,n);
  ofstream out(name);
  delete name;
  if(binflag)
    {
      int a=0;
      out.write((char*)&a,sizeof(int));
      out.write((char*)&a,sizeof(int));
      out.write((char*)&a,sizeof(int));
    }
  else
    {
      out<<"0 0 0\n";
    }
  out.close();
   
}
