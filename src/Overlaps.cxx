// $Header: /nfs/slac/g/glast/ground/cvs/detCheck/src/Overlaps.cxx,v 1.6 2003/07/10 17:56:01 jrb Exp $

#include <fstream>
#include <cmath>
#include "detCheck/Overlaps.h"
#include "detModel/Gdd.h"
#include "detModel/Sections/Volume.h"

// need some or all of the following:

#include "detModel/Sections/BoundingBox.h"
#include "detModel/Sections/Ensemble.h"
#include "detModel/Sections/Composition.h"
#include "detModel/Sections/Stack.h"
#include "detModel/Sections/Box.h"
#include "detModel/Sections/PosXYZ.h"
#include "detModel/Sections/AxisMPos.h"

namespace detCheck {

  //  double Overlaps::DEFAULT_EPSILON = 0.000001;
  double Overlaps::DEFAULT_EPSILON = 0.00001;
  Overlaps::Overlaps(detModel::Gdd* gdd) {
    m_gdd = gdd;
    m_eps = DEFAULT_EPSILON;
  }

  bool Overlaps::check(std::string errfileName, bool verbose, bool dump) {
    //    m_verbose = verbose;
    m_verbose = verbose;
    m_dump = dump;
    bool allocStream = false;
    if (errfileName.size() == 0) {
      m_out = &std::cout;  // or out = new ostream(std::out);  ?
    }
    else {
      m_out = new std::ofstream(errfileName.c_str());
      allocStream = true;
    }
    
    unsigned int nVol = m_gdd->getVolumesNumber();
    unsigned int iVol;
    bool finalOk = true;
    unsigned int nStack = 0, nCompos = 0;
    
    for (iVol = 0; iVol < nVol; iVol++) {
      bool ok = true;

      detModel::Volume *vol = m_gdd->getOrderedVolume(iVol);
      if (detModel::Stack *ens = 
          dynamic_cast<detModel::Stack*>(vol) ) {
        if (verbose) {
          (*m_out) << "Checking stack volume " << nStack << " named " 
                   << vol->getName() << std::endl;
        }
        ok = checkStack(ens);
        if (!ok) {
          (*m_out) << "Overlap check failed in volume #" << iVol 
                   << ", stack volume #" 
                   << nStack << " " 
                   << vol->getName() << std::endl;
          if (verbose) printChildren(ens);
        }
        ++nStack;
      }
      else if (detModel::Composition *ens =
          dynamic_cast<detModel::Composition*>(vol) ) {
        if (verbose) {
          (*m_out) << "Checking composition volume " << nCompos << " named " 
                   << vol->getName() << std::endl;
        }
        ok = checkComposition(ens);
        if (!ok) {
          (*m_out) << "Overlap check failed in volume #" << iVol 
                   << ", Composition volume #" 
                   << nCompos << " " 
                   << vol->getName() << std::endl;
          if (verbose) printChildren(ens);
        }
        if (m_dump) printChildren(ens);
        ++nCompos;
      }
      finalOk = finalOk & ok;
      //      if (!ok) {
      //        if (allocStream) delete m_out;
      //        return ok;
      //      }
    }       //  iteration over volumes
    if (finalOk) {
      (*m_out) << "Congratulations!  No overlaps found." << std::endl;
    }
    if (allocStream) delete m_out;
    return finalOk;
  }
  bool Overlaps::checkStack(detModel::Stack* stack) {
    using namespace detModel;

    std::vector<Location> locs;

    // Step 1 is to look at the child elements one by one,
    // storing coordinates of extreme corners.
    std::vector<Position *> positions = stack->getPositions();
    for (unsigned int iPos = 0; iPos < positions.size(); iPos++) {
      
      // only possibility for positioning elt. within a stack
      AxisMPos *mpos = dynamic_cast<AxisMPos*>(positions[iPos]);
      BoundingBox* bBox = mpos->getBBox();

      // a little extra processing to check that evenly-spaced
      // components don't intersect
      bool ok = checkMPos(mpos);
      if (!ok) {
        (*m_out) << "Error in evenly-spaced components named " <<
          mpos->getVolumeRef() << std::endl;
        //        return ok;
      }
      // Now compute displacements; store result of folding
      // BBox coords with displacement, gap, etc. in locs[iPos]

      // First ignore stacking direction.  In non-stacking directions,
      // displacement is just dx, dy or dz.  Use parent coordinate system 
      // with origin at the vertex of the stack
      Location loc;

      loc.x[0] = mpos->getDx();
      double x = bBox->getXDim(); 
      loc.x[1] = loc.x[0] + x;
      
      loc.y[0] = mpos->getDy(); 
      double y = bBox->getYDim();
      loc.y[1] = loc.y[0] + y;

      loc.z[0] = mpos->getDz(); 
      double z = bBox->getZDim();
      loc.z[1] = loc.z[0] + z;
      
      // Now fix up stacking direction
      double cm = mpos->getDispCM();

      switch(stack->getAxisDir()) {
      case detModel::Stack::xDir: 
        loc.x[0] = cm - x/2; 
        loc.x[1] = cm + x/2;
        break;

      case detModel::Stack::yDir: 
        loc.y[0] = cm - y/2; 
        loc.y[1] = cm + y/2;
        break;

      case detModel::Stack::zDir: 
        loc.z[0] = cm - z/2; 
        loc.z[1] = cm + z/2;
        break;
      }
      
      locs.push_back(loc);
    }
    return checkLocs(locs);
  }

  bool Overlaps::checkMPos(detModel::AxisMPos* mpos) {
    if (mpos->getNcopy() == 1) return true;

    // Otherwise, since everything is uniform, sufficient to check that
    // the first two copies don't overlap.  

    // If using shift, need to know
    //  -- size of bounding box in stacking dimension
    //  -- value of shift
    double shift = mpos->getShift();
    detModel::BoundingBox *bBox = mpos->getVolume()->getBBox();

    if (shift != 0) {

      double boxDim;
      switch(mpos->getAxisDir() ) {
      case (detModel::Stack::xDir):
        boxDim = bBox->getXDim();
        break;
      case (detModel::Stack::yDir):
        boxDim = bBox->getYDim();
        break;
      case (detModel::Stack::zDir):
        boxDim = bBox->getZDim();
        break;
      }
      if (shift + m_eps >= boxDim) {
        return true;
      }
      else {
        if (m_verbose) {
          (*m_out) << "shift was " << shift 
                   << ", relevant box dimension was " << boxDim
                   << std::endl;
        }
        return false;
      }
    }
    else  {
      if (mpos->getGap() >= 0.0)  return true;
      else if (m_verbose) {
        (*m_out) << "Bad gap of " << mpos->getGap() << std::endl;
      }
      return false;
    }
  }

  bool Overlaps::checkComposition(detModel::Composition* compos) {
    using namespace detModel;
    std::vector<Overlaps::Location> locs;
    
      // Step 1 is to look at the child elements one by one,
    // storing coordinates of extreme corners.
    std::vector<Position *> positions = compos->getPositions();
    for (unsigned int iPos = 0; iPos < positions.size(); iPos++) {

      // PosXYZ is the only kind of positioning supported for compositions
      PosXYZ *pos = dynamic_cast<PosXYZ *> (positions[iPos]);
      BoundingBox* bBox = pos->getBBox();
      Location loc;

      // Now compute displacements; store result of folding
      // BBox coords with displacement, gap, etc. in locs[iPos]
      double dim2 = (bBox->getXDim()) / 2.0;
      double dsp = pos->getX();
      loc.x[0] = -dim2 + dsp;
      loc.x[1] = dim2 + dsp;

      dim2 = (bBox->getYDim()) / 2.0;
      dsp = pos->getY();
      loc.y[0] = -dim2 + dsp;
      loc.y[1] = dim2 + dsp;

      dim2 = (bBox->getZDim()) / 2.0;
      dsp = pos->getZ();
      loc.z[0] = -dim2 + dsp;
      loc.z[1] = dim2 + dsp;

      locs.push_back(loc);
    }
    return (checkLocs(locs)  && checkEnvelope(compos, locs));
  }

  bool Overlaps::checkLocs(std::vector<Location>& locs) {
    unsigned int i, j;
    bool ok = true;
    for (i = 1; i < locs.size(); i++) {
      for (j = 0; j < i; j++) {
        if (!pairOk(&locs[j], &locs[i]) ) {
          (*m_out) << "Overlap between child volumes #" << j <<
            " and " << i << std::endl;
          ok = false;
        }
      }
    }
    return ok;
  }

  bool Overlaps::checkEnvelope(detModel::Composition* compos,
                               std::vector<Location>& locs) {
    detModel::BoundingBox *envBB = compos->getEnvelope()->getBBox();
    detModel::BoundingBox *compBB = compos->getBBox();
    bool retStatus =
      ((envBB->getXDim() + m_eps*envBB->getXDim() >= compBB->getXDim() ) &&
       (envBB->getYDim() + m_eps*envBB->getYDim() >= compBB->getYDim() ) &&
       (envBB->getZDim() + m_eps*envBB->getZDim() >= compBB->getZDim() ) );

    if ((!m_dump) && (retStatus)) {        return true; }
    else {
      *m_out << "Envelope " << compos->getEnvelopeRef() 
             << " for composition volume " 
             << compos->getName() << std::endl;
    }
    if (!retStatus) {
    (*m_out) << "Envelope " << compos->getEnvelopeRef() 
             << " too small for volume " 
             << compos->getName() << std::endl;
    }

    // print out coordinates for all children and size of envelope for
    // the real fans
    if (m_verbose) { 
      double maxX = (envBB->getXDim())/2;
      double maxY = (envBB->getYDim())/2;
      double maxZ = (envBB->getZDim())/2;
      (*m_out) << "Envelope half-dimensions: " << maxX
               << ", " << maxY
               << ", " << maxZ << std::endl;
      // locs next
      (*m_out) << "Child positions (min and max corners) " << std::endl;
      for (unsigned iLoc = 0; iLoc < locs.size(); iLoc++) {
        (*m_out) << "   (" << locs[iLoc].x[0] << ", " << locs[iLoc].y[0] 
                 <<  ", "  << locs[iLoc].z[0] << ") and (" 
                 << locs[iLoc].x[1] << ", " << locs[iLoc].y[1] 
                 << ", " << locs[iLoc].z[1] << ")" << std::endl;
      }
    }
    return retStatus;
  }

  // Assuming that loc is formed in such a way that x[0] < x[1],
  // y[0] < y[1] etc.
  bool Overlaps::pairOk(Location* loc1, Location* loc2) {
    // First check takes scale of items to be compared into account
    bool ok = 
      (loc1->x[0] + m_eps * abs(loc1->x[0]) >= loc2->x[1]) || 
      (loc2->x[0] + m_eps * abs(loc2->x[0])>= loc1->x[1]) ||
      (loc1->y[0] + m_eps * abs(loc1->y[0])>= loc2->y[1]) || 
      (loc2->y[0] + m_eps * abs(loc2->y[0])>= loc1->y[1]) ||
      (loc1->z[0] + m_eps * abs(loc1->z[0])>= loc2->z[1]) || 
      (loc2->z[0] + m_eps * abs(loc2->z[0])>= loc1->z[1]);
    // However, this sort of comparison can fail if both items are
    // essentially zero, so check for this, too
    if (!ok) {
      ok = (( (abs(loc1->x[0]) < m_eps) && (abs(loc2->x[1]) < m_eps) ) ||
            ( (abs(loc1->x[1]) < m_eps) && (abs(loc2->x[0]) < m_eps) ) ||
            ( (abs(loc1->y[0]) < m_eps) && (abs(loc2->y[1]) < m_eps) ) ||
            ( (abs(loc1->y[1]) < m_eps) && (abs(loc2->y[0]) < m_eps) ) ||
            ( (abs(loc1->z[0]) < m_eps) && (abs(loc2->z[1]) < m_eps) ) ||
            ( (abs(loc1->z[1]) < m_eps) && (abs(loc2->z[0]) < m_eps) ) );
    }

    if (!ok) {
      (*m_out) << "Overlapping volumes.  Vertex coords are:" << std::endl;
      (*m_out) << "1st volume:  (" << loc1->x[0] << ", " << loc1->y[0] <<
        ", " << loc1->z[0] << ") and (" << loc1->x[1] << ", " << loc1->y[1] <<
        ", " << loc1->z[1] << ")" << std::endl;
      (*m_out) << "2nd volume:  (" << loc2->x[0] << ", " << loc2->y[0] <<
        ", " << loc2->z[0] << ") and (" << loc2->x[1] << ", " << loc2->y[1] <<
        ", " << loc2->z[1] << ")" << std::endl;
    }
    return ok;
  }

  double Overlaps::setEpsilon(double epsilon) {
    double old = m_eps;
    if (epsilon < 0.0) return m_eps;  // epsilon < 0 makes no sense
    m_eps = epsilon;
    return old;
  }

  void Overlaps::printChildren(detModel::Ensemble *ens) {
    std::vector<detModel::Position*> positions = ens->getPositions();
    
    for (unsigned int iPos = 0; iPos < positions.size(); iPos++) {
      (*m_out) << "Child volume #" << iPos << " is " <<
        positions[iPos]->getVolumeRef() << std::endl;
    }
    (*m_out) << std::endl;
    return;
  }
}
