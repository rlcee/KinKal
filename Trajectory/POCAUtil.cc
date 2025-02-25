/*

   code based on MuteUtilities/src/TwoLinePCA:

   Given two lines in 3D, compute the distance of closest
   approach between the two lines.  The lines are
   specified in point-slope form.

   Original author Rob Kutschke

 */

#include "KinKal/Trajectory/POCAUtil.hh"
#include <math.h>
#include <stdexcept>

using namespace std;
using namespace ROOT::Math;

namespace KinKal {

  POCAUtil::POCAUtil( VEC3 const& p1, VEC3 const& t1,  VEC3 const& p2,VEC3 const& t2, double cut):
    _p1(p1),
    _t1(t1.unit()),
    _p2(p2),
    _t2(t2.unit()),
    _s1(0.),
    _s2(0.),
    _dca(0.),
    _dca2d(0.),
    _closeToParallel(false)
  {

    double c(_t1.Dot(_t2));
    double sinsq(1.-c*c);

    if ( sinsq < cut ){
      _closeToParallel = true;
      _pca1 = p1;
      _pca2 = p2;

    }

    else {

      VEC3 delta(_p1-_p2);
      double dDotT1 = delta.Dot(_t1);
      double dDotT2 = delta.Dot(_t2);

      _s1 =  (dDotT2*c-dDotT1)/sinsq;
      _s2 = -(dDotT1*c-dDotT2)/sinsq;

      _pca1 = _p1 + _t1*_s1;
      _pca2 = _p2 + _t2*_s2;
    }

    VEC3 diff = (_pca1-_pca2);
    _dca   = sqrt(diff.Mag2());
    _dca2d = sqrt(diff.Perp2());

  }

  POCAUtil::~POCAUtil(){}

}
