//
// test basic functions of ClosestApproach using KTraj and Line
//
#include "KinKal/Trajectory/Line.hh"
#include "KinKal/Trajectory/ClosestApproach.hh"
#include "KinKal/Trajectory/PointClosestApproach.hh"
#include "KinKal/Trajectory/PiecewiseClosestApproach.hh"
#include "KinKal/Trajectory/ParticleTrajectory.hh"
#include "KinKal/General/BFieldMap.hh"
#include "KinKal/General/PhysicalConstants.h"

#include <iostream>
#include <stdio.h>
#include <iostream>
#include <getopt.h>

#include "TH1F.h"
#include "TSystem.h"
#include "THelix.h"
#include "TFile.h"
#include "TPolyLine3D.h"
#include "TAxis3D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TVector3.h"
#include "TPolyLine3D.h"
#include "TPolyMarker3D.h"
#include "TLegend.h"
#include "TGraph.h"
#include "TRandom3.h"
#include "TH2F.h"
#include "TDirectory.h"
#include "TProfile.h"
#include "TProfile2D.h"

using namespace KinKal;
using namespace std;
// avoid confusion with root
using KinKal::Line;

void print_usage() {
  printf("Usage: ClosestApproachTest --charge i--gap f --tmin f --tmax f --phi f --vprop f --costheta f --eta f\n");
}

template <class KTRAJ>
int ClosestApproachTest(int argc, char **argv) {
  using TCA = ClosestApproach<KTRAJ,Line>;
  using TCAP = PointClosestApproach<KTRAJ>;
  using PCA = PiecewiseClosestApproach<KTRAJ,Line>;
  using PTRAJ = ParticleTrajectory<KTRAJ>;
  int opt;
  double mom(105.0), cost(0.7), phi(0.5);
  int icharge(-1);
  double pmass(0.511), oz(0.0), ot(0.0);
  double tmin(-10.0), tmax(10.0);
  double wlen(1000.0); //length of the wire
  double gap(2.0); // distance between Line and KTRAJ
  double vprop(0.7);
  double eta(0.0);
  unsigned nstep(50),ntstep(10);

  static struct option long_options[] = {
    {"charge",     required_argument, 0, 'q'  },
    {"costheta",     required_argument, 0, 'c'  },
    {"gap",     required_argument, 0, 'g'  },
    {"tmin",     required_argument, 0, 't'  },
    {"tmax",     required_argument, 0, 'd'  },
    {"phi",     required_argument, 0, 'f'  },
    {"vprop",     required_argument, 0, 'v'  },
    {"eta",     required_argument, 0, 'e'  },
  };

  int long_index =0;
  while ((opt = getopt_long_only(argc, argv,"",
          long_options, &long_index )) != -1) {
    switch (opt) {
      case 'c' : cost = atof(optarg);
                 break;
      case 'q' : icharge = atoi(optarg);
                 break;
      case 'g' : gap = atof(optarg);
                 break;
      case 't' : tmin = atof(optarg);
                 break;
      case 'd' : tmax = atof(optarg);
                 break;
      case 'f' : phi = atof(optarg);
                 break;
      case 'v' : vprop = atof(optarg);
                 break;
      case 'e' : eta = atof(optarg);
                 break;
      default: print_usage();
               exit(EXIT_FAILURE);
    }
  }
  // create helix
  VEC3 bnom(0.0,0.0,1.0);
  UniformBFieldMap BF(bnom); // 1 Tesla
  VEC4 origin(0.0,0.0,oz,ot);
  double sint = sqrt(1.0-cost*cost);
  MOM4 momv(mom*sint*cos(phi),mom*sint*sin(phi),mom*cost,pmass);
  KTRAJ ktraj(origin,momv,icharge,bnom);
  TFile tpfile((KTRAJ::trajName()+"ClosestApproach.root").c_str(),"RECREATE");
  TCanvas* ttpcan = new TCanvas("ttpcan","DToca",1200,800);
  ttpcan->Divide(3,2);
  TCanvas* dtpcan = new TCanvas("dtpcan","DDoca",1200,800);
  dtpcan->Divide(3,2);
  std::vector<TGraph*> dtpoca, ttpoca;
  std::vector<double> pchange = {10,0.1,0.0001,10,0.01,0.1};
  for(size_t ipar=0;ipar<NParams();ipar++){
    typename KTRAJ::ParamIndex parindex = static_cast<typename KTRAJ::ParamIndex>(ipar);
    dtpoca.push_back(new TGraph(nstep*ntstep));
    string ts = KTRAJ::paramTitle(parindex)+string(" DOCA Change;#Delta DOCA (exact);#Delta DOCA (derivative)");
    dtpoca.back()->SetTitle(ts.c_str());
    ttpoca.push_back(new TGraph(nstep*ntstep));
    ts = KTRAJ::paramTitle(parindex)+string(" TOCA Change;#Delta TOCA (exact);#Delta TOCA (derivative)");
    ttpoca.back()->SetTitle(ts.c_str());
  }
  for(unsigned itime=0;itime < ntstep;itime++){
    double time = tmin + itime*(tmax-tmin)/(ntstep-1);
    // create tline perp to trajectory at the specified time, separated by the specified gap
    VEC3 pos, dir;
    pos = ktraj.position3(time);
    dir = ktraj.direction(time);
    VEC3 perp1 = ktraj.direction(time,MomBasis::perpdir_);
    VEC3 perp2 = ktraj.direction(time,MomBasis::phidir_);
    // choose a specific direction for DOCA
    // the line traj must be perp. to this and perp to the track
    VEC3 docadir = cos(eta)*perp1 + sin(eta)*perp2;
    VEC3 pdir = sin(eta)*perp1 - cos(eta)*perp2;
    double pspeed = CLHEP::c_light*vprop; // vprop is relative to c
    VEC3 pvel = pdir*pspeed;
    // shift the position
    VEC3 ppos = pos + gap*docadir;
    // create the Line
    Line tline(ppos, time, pvel, wlen);
    // create ClosestApproach from these
    CAHint tphint(time,time);
    TCA tp(ktraj,tline,tphint,1e-8);
    // test: delta vector should be perpendicular to both trajs
    VEC3 del = tp.delta().Vect();
    auto pd = tp.particleDirection();
    auto sd = tp.sensorDirection();
    double dp = del.Dot(pd);
    if(dp>1e-9) cout << "CA delta not perpendicular to particle direction" << endl;
    double ds = del.Dot(sd);
    if(ds>1e-9) cout << "CA delta not perpendicular to sensor direction" << endl;
    // test PointClosestApproach
    VEC4 pt(ppos.X(),ppos.Y(),ppos.Z(),time-1.0);
    TCAP tpp(ktraj,pt,1e-8);
    if(fabs(fabs(tpp.doca()) - gap) > 1e-8) cout << "Point DOCA not correct " << endl;

    // test against a piece-traj
    PTRAJ ptraj(ktraj);
    PCA pca(ptraj,tline,tphint,1e-8);
    if(tp.status() != ClosestApproachData::converged)cout << "ClosestApproach status " << tp.statusName() << " doca " << tp.doca() << " dt " << tp.deltaT() << endl;
    if(tpp.status() != ClosestApproachData::converged)cout << "PointClosestApproach status " << tpp.statusName() << " doca " << tpp.doca() << " dt " << tpp.deltaT() << endl;
    if(pca.status() != ClosestApproachData::converged)cout << "PiecewiseClosestApproach status " << pca.statusName() << " doca " << pca.doca() << " dt " << pca.deltaT() << endl;
    VEC3 thpos, tlpos;
    thpos = tp.particlePoca().Vect();
    tlpos = tp.sensorPoca().Vect();
    double refd = tp.doca();
    double reft = tp.deltaT();
    //  cout << " Helix Pos " << pos << " ClosestApproach KTRAJ pos " << thpos << " ClosestApproach Line pos " << tlpos << endl;
    //  cout << " ClosestApproach particlePoca " << tp.particlePoca() << " ClosestApproach sensorPoca " << tp.sensorPoca()  << " DOCA " << refd << endl;
    //  cout << "ClosestApproach dDdP " << tp.dDdP() << " dTdP " << tp.dTdP() << endl;
    // test against numerical derivatives
    // range to change specific parameters; most are a few mm
    for(size_t ipar=0;ipar<NParams();ipar++){
      double dstep = pchange[ipar]/(nstep-1);
      double dstart = -0.5*pchange[ipar];
      for(unsigned istep=0;istep<nstep;istep++){
        // compute exact change in DOCA
        auto dvec = ktraj.params().parameters();
        double dpar = dstart + dstep*istep;
        dvec[ipar] += dpar;
        Parameters pdata(dvec,ktraj.params().covariance());
        KTRAJ dktraj(pdata,ktraj);
        TCA dtp(dktraj,tline,tphint,1e-9);
        double xd = dtp.doca();
        double xt = dtp.deltaT();
        // now derivatives
        double dd = tp.dDdP()[ipar]*dpar;
        double dt = tp.dTdP()[ipar]*dpar;
        dtpoca[ipar]->SetPoint(istep,xd-refd,dd);
        ttpoca[ipar]->SetPoint(istep,xt-reft,dt);
      }
    }
  }
  for(size_t ipar=0;ipar<NParams();ipar++){
    dtpcan->cd(ipar+1);
    dtpoca[ipar]->Draw("A*");
  }
  for(size_t ipar=0;ipar<NParams();ipar++){
    ttpcan->cd(ipar+1);
    ttpoca[ipar]->Draw("A*");
  }
  dtpcan->Write();
  ttpcan->Write();
  tpfile.Write();
  tpfile.Close();
  return 0;
}


