//   h1clist.cc  --  Outputs table of curves computing a_i, r, |T|
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
//                

#include <iomanip>
#include "moddata.h"
#include "symb.h"
#include "oldforms.h"
#include "homspace.h"
#include "cperiods.h"     //from qcurves, for computing conductors
#include "newforms.h"
#include "periods.h"
#include "points.h"

#define AUTOLOOP
#define BOOKORDER       // if defined, sorts newforms/curves into order
                        // in the Book (relevant up to 500 only)
#include "curvesort.cc"
#define CURVE_IS_ONE_FIELD // outputs      [a1,a2,a3,a4,a6]
                           // else outputs a1 a2 a3 a4 a6

int main(void)
{
  int prec0 = 10; 
  int prec = prec0;
  set_precision(prec);
 int limit,n=1; 
 char* code = new char[20];
#ifdef AUTOLOOP
 cerr<<"Enter first and last N: ";cin>>n>>limit; 
 n--; cerr<<endl;
#ifdef CURVE_IS_ONE_FIELD
 cerr << "    N\tC\t#\t[a1,a2,a3,a4,a6]\tr\t|T|\tdeg(phi)" << endl;
#else
 cerr << "    N\tC\t#\ta1 a2  a3 a4          a6\tr\t|T|\tdeg(phi)" << endl;
#endif
 cerr << "----------------------------------------------------------------------" << endl;

 while (n<limit) { n++;
#else
 while (n>0) { cout<<"Enter level: "; cin>>n;
#endif
 if (n>0)
{
  newforms nf(n,0); 
  int noldap=25;
  nf.createfromdata(1,noldap,0); // do not create from scratch if data absent
  int nnf = nf.n1ds; 
  int inf = 1; 
 for(int xi=0; xi<nnf; xi++)
   { int i=xi;
     codeletter(xi,code);
#ifdef BOOKORDER
     i=booknumber0(n,i);
#endif
     newform& nfi = nf.nflist[i];
     int degphi = nfi.degphi;
     ldash1 x(&nf, &nfi);
     bigfloat rperiod;
     int r = x.rank();

     prec = prec0;
     set_precision(prec);
     Curve C = nf.getcurve(i, -1, rperiod);
     Curvedata CD;
     CurveRed CR;
     bigint nc; int nt;
     bigint a1,a2,a3,a4,a6;
     if (!C.isnull())
       {
	 C = nf.getcurve(i, -1, rperiod);
         CD = Curvedata(C,1);  // The 1 causes minimalization
         CR = CurveRed(CD);
         nc = getconductor(CR);
         if(n!=nc) C = Curve();  // wrong conductor; reset to null curve
       }
    while (C.isnull() && (prec<100))
       {
	 prec += 10;
	 set_precision(prec);
	 C = nf.getcurve(i, -1, rperiod);
         CD = Curvedata(C,1);  // The 1 causes minimalization
         CR = CurveRed(CD);
         nc = getconductor(CR);
         if(n!=nc) C = Curve();  // wrong conductor; reset to null curve
       }
     if (C.isnull())
       {
	 cout<<setw(6)<<n<<code<<": bad curve"<<endl;
	 continue;
       }

     CD.getai(a1,a2,a3,a4,a6);
     nt = ntorsion(CD);
     //     cout.form("%4d\t%s\t1\t",n,code);
     cout<<setw(6)<<n<<"\t"<<code<<"\t1\t";
#ifdef CURVE_IS_ONE_FIELD
     cout<<"["<<a1<<","<<a2<<","<<a3<<","<<a4<<","<<a6<<"]";
#else
     cout<<a1<<" "<<a2<<" "<<a3<<" "<<a4<<" "<<a6;
#endif
     cout << "\t" << r << "\t" << nt << "\t" << degphi << endl;
   }
}       // end of if(n)
}       // end of while()
 delete[] code;
}       // end of main()
