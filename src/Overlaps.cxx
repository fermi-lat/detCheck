// $Header: /nfs/slac/g/glast/ground/cvs/detCheck/src/Overlaps.cxx,v 1.13 2007/03/24 06:36:09 jrb Exp $

#include <fstream>
#include <cmath>
#include <time.h>
#include "detCheck/Overlaps.h"
#include "detModel/Gdd.h"
#include "detModel/Sections/Volume.h"
#include "detModel/Management/Manager.h"

// need some or all of the following:

#include "detModel/Sections/BoundingBox.h"
#include "detModel/Sections/Ensemble.h"
#include "detModel/Sections/Sphere.h"
#include "detModel/Sections/Composition.h"
#include "detModel/Sections/Stack.h"
#include "detModel/Sections/Box.h"
#include "detModel/Sections/PosXYZ.h"
#include "detModel/Sections/AxisMPos.h"

namespace detCheck {
  static double PI = 0;    // Set properly at first use

  //  double Overlaps::DEFAULT_EPSILON = 0.000001;
  double Overlaps::DEFAULT_EPSILON = 0.00001;
  Overlaps::Overlaps(detModel::Gdd* gdd) {
    m_gdd = gdd;
    m_eps = DEFAULT_EPSILON;
  }


  bool Overlaps::check(std::string errfileName, bool verbose, bool dump) {

    if (PI <= 0.0) {
      double one = 1.0;
      PI = 2 * (asin(one));
    }
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

    // Identify input as best we can
    time_t curTime = time(0);
    
    (*m_out) << std::endl
             <<"Generated by detCheck diagnostic dumpIds.exe from file " 
             << detModel::Manager::getPointer()->getNameFile() << std::endl
             << "on " << ctime(&curTime) << std::endl;
    
    unsigned int nVol = m_gdd->getVolumesNumber();
    unsigned int iVol;
    bool finalOk = true;
    unsigned int nStack = 0, nCompos = 0;
    
    for (iVol = 0; iVol < nVol; iVol++) {
      bool ok = true;
      m_childrenPrinted = false;

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
          if (verbose) {
            printChildren(ens);
          }
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
          if (verbose) {
            printChildren(ens);
          }
        }
        if (m_dump && !m_childrenPrinted) printChildren(ens);
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

      loc.xBB[0] = mpos->getDx();
      double x = bBox->getXDim(); 
      loc.xBB[1] = loc.xBB[0] + x;
      
      loc.yBB[0] = mpos->getDy(); 
      double y = bBox->getYDim();
      loc.yBB[1] = loc.yBB[0] + y;

      loc.zBB[0] = mpos->getDz(); 
      double z = bBox->getZDim();
      loc.zBB[1] = loc.zBB[0] + z;
      
      // Now fix up stacking direction
      double cm = mpos->getDispCM();

      switch(stack->getAxisDir()) {
      case detModel::Stack::xDir: 
        loc.xBB[0] = cm - x/2; 
        loc.xBB[1] = cm + x/2;
        break;

      case detModel::Stack::yDir: 
        loc.yBB[0] = cm - y/2; 
        loc.yBB[1] = cm + y/2;
        break;

      case detModel::Stack::zDir: 
        loc.zBB[0] = cm - z/2; 
        loc.zBB[1] = cm + z/2;
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
    // storing coordinates of extreme corners, of bounding box,
    // and of what type it is (orthog. box, rot. box or spherical shell)
    std::vector<Position *> positions = compos->getPositions();
    for (unsigned int iPos = 0; iPos < positions.size(); iPos++) {

      // PosXYZ is the only kind of positioning supported for compositions
      PosXYZ *pos = dynamic_cast<PosXYZ *> (positions[iPos]);
      Location loc;
      fillPos(loc, pos);

      locs.push_back(loc);
    }
    bool locsRet = checkLocs(locs);
    bool envRet = checkEnvelope(compos, locs);
    return (locsRet  && envRet);
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
        (*m_out) << "Child #" << iLoc << " [" << locs[iLoc].vol->getName()
                 << "]: (" << locs[iLoc].xBB[0] 
                 << ", " << locs[iLoc].yBB[0] 
                 <<  ", "  << locs[iLoc].zBB[0] << ") & (" 
                 << locs[iLoc].xBB[1] << ", " << locs[iLoc].yBB[1] 
                 << ", " << locs[iLoc].zBB[1] << ")" << std::endl;
      }
      m_childrenPrinted = true;
    }
    return retStatus;
  }

  // Assuming that loc is formed in such a way that x[0] < x[1],
  // y[0] < y[1] etc.
  bool Overlaps::pairOk(Location* loc1, Location* loc2) {
    // First check takes scale of items to be compared into account
    bool ok = 
      (loc1->xBB[0] + m_eps * fabs(loc1->xBB[0]) >= loc2->xBB[1]) || 
      (loc2->xBB[0] + m_eps * fabs(loc2->xBB[0])>= loc1->xBB[1]) ||
      (loc1->yBB[0] + m_eps * fabs(loc1->yBB[0])>= loc2->yBB[1]) || 
      (loc2->yBB[0] + m_eps * fabs(loc2->yBB[0])>= loc1->yBB[1]) ||
      (loc1->zBB[0] + m_eps * fabs(loc1->zBB[0])>= loc2->zBB[1]) || 
      (loc2->zBB[0] + m_eps * fabs(loc2->zBB[0])>= loc1->zBB[1]);
    // However, this sort of comparison can fail if both items are
    // essentially zero, so check for this, too
    if (!ok) {
      ok = (( (fabs(loc1->xBB[0]) < m_eps) && (fabs(loc2->xBB[1]) < m_eps) ) ||
            ( (fabs(loc1->xBB[1]) < m_eps) && (fabs(loc2->xBB[0]) < m_eps) ) ||
            ( (fabs(loc1->yBB[0]) < m_eps) && (fabs(loc2->yBB[1]) < m_eps) ) ||
            ( (fabs(loc1->yBB[1]) < m_eps) && (fabs(loc2->yBB[0]) < m_eps) ) ||
            ( (fabs(loc1->zBB[0]) < m_eps) && (fabs(loc2->zBB[1]) < m_eps) ) ||
            ( (fabs(loc1->zBB[1]) < m_eps) && (fabs(loc2->zBB[0]) < m_eps) ) );
    }

    if (ok) return ok;

    // If not ok, still a chance we have a false positive.
    // Can't do anything for 'other' shapes
    if ((loc1->shapeType == SHAPEotherShape) || 
        (loc2->shapeType == SHAPEotherShape)) return ok;

    // If both shapes were orth boxes, bounding box = shape, so we're done
    if ((loc1->shapeType == SHAPEorthBox) &&
        (loc2->shapeType == SHAPEorthBox) ) return ok;

    // If both are spheres, see if one is inside the other
    if ((loc1->shapeType == SHAPEsphere) &&
        (loc2->shapeType == SHAPEsphere) ) {
      Location* big;
      Location* sml;
      if (loc1->rOut > loc2->rOut) {
        big = loc1; sml = loc2;
      }
      else {
        big = loc2; sml = loc1;
      }
      double dC = 
        sqrt((big->c.px-sml->c.px)*(big->c.px-sml->c.px) +
              (big->c.py-sml->c.py)*(big->c.py-sml->c.py) +
              (big->c.pz-sml->c.pz)*(big->c.pz-sml->c.pz));
      ok = (dC + sml->rOut <= big->rIn);
    }

    // If one is sphere and one is box..
    if (loc1->shapeType == SHAPEsphere) {
      ok = checkSphereBox(loc1, loc2);
    }
    else if (loc2->shapeType == SHAPEsphere) {
      ok = checkSphereBox(loc2, loc1);
    }

    else ok = checkBoxes(loc1, loc2);

    if (!ok) {
      (*m_out) << "Overlapping volumes.  Vertex coords are:" << std::endl;
      (*m_out) << "1st vol:  [" << loc1->vol->getName() << "] (" 
               << loc1->xBB[0] << ", " 
               << loc1->yBB[0] 
               << ", " << loc1->zBB[0] << ") & (" << loc1->xBB[1] 
               << ", " << loc1->yBB[1] <<
        ", " << loc1->zBB[1] << ")" << std::endl;

      (*m_out) << "2nd vol: [" << loc2->vol->getName() << "] (" 
               << loc2->xBB[0] << ", " << loc2->yBB[0] 
               << ", " << loc2->zBB[0] << ") & (" << loc2->xBB[1] 
               << ", " << loc2->yBB[1] << ", " << loc2->zBB[1] 
               << ")" << std::endl;
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
    if (m_childrenPrinted) return;
    std::vector<detModel::Position*> positions = ens->getPositions();
    
    for (unsigned int iPos = 0; iPos < positions.size(); iPos++) {
      (*m_out) << "Child volume #" << iPos << " is " <<
        positions[iPos]->getVolumeRef() << std::endl;
    }
    (*m_out) << std::endl;
    m_childrenPrinted = true;
    return;
  }

  void Overlaps::fillPos(Location& loc, detModel::PosXYZ *pos) {
    detModel::Volume* vol = pos->getVolume();
    loc.bBox = pos->getBBox();

    loc.c.px = pos->getX();   // center position
    loc.c.py = pos->getY();
    loc.c.pz = pos->getZ();

    detModel::Composition* cmp = dynamic_cast<detModel::Composition *> (vol);
    if (cmp) {  // replace volume by its envelope
      vol = cmp->getEnvelope();
    }

    loc.vol = vol;

    // Get bounding box info for all.  
    double dim2 = (loc.bBox->getXDim()) / 2.0;
    loc.xBB[0]  = -dim2 + loc.c.px;
    loc.xBB[1] = dim2 + loc.c.px;
    
    dim2 = (loc.bBox->getYDim()) / 2.0;
    loc.yBB[0] = -dim2 + loc.c.py;
    loc.yBB[1] = dim2 + loc.c.py;

    dim2 = (loc.bBox->getZDim()) / 2.0;
    loc.zBB[0] = -dim2 + loc.c.pz;
    loc.zBB[1] = dim2 + loc.c.pz;


    //Special additional treatment for spheres and boxes
    detModel::Sphere *sphere = dynamic_cast<detModel::Sphere *> (vol);
    detModel::Box *box = dynamic_cast<detModel::Box *> (vol);
    if (sphere != 0) {
      loc.shapeType = SHAPEsphere;
      loc.rOut = sphere->getRout();
      loc.rIn = sphere->getRin();
    }
    else if (box != 0) {
      loc.xDim = box->getX();
      loc.yDim = box->getY();
      loc.zDim = box->getZ();

      loc.xRot = pos->getXRot();
      loc.yRot = pos->getYRot();
      loc.zRot = pos->getZRot();


      // Vertices of box, not rotated and not displaced 
      loc.v[0] = Point(-loc.xDim/2, -loc.yDim/2, -loc.zDim/2);
      loc.v[1] = Point(-loc.xDim/2, -loc.yDim/2, loc.zDim/2);
      loc.v[2] = Point(-loc.xDim/2, loc.yDim/2, -loc.zDim/2);
      loc.v[3] = Point(-loc.xDim/2, loc.yDim/2, loc.zDim/2);
      loc.v[4] = Point(loc.xDim/2, -loc.yDim/2, -loc.zDim/2);
      loc.v[5] = Point(loc.xDim/2, -loc.yDim/2, loc.zDim/2);
      loc.v[6] = Point(loc.xDim/2, loc.yDim/2, -loc.zDim/2);
      loc.v[7] = Point(loc.xDim/2, loc.yDim/2, loc.zDim/2);

      loc.shapeType = SHAPEorthBox;

      if ((fabs(fmod(loc.xRot, 90.0)) > 1 )  &&
          (fabs(fmod(loc.xRot, 90.0)) < 89 ) ) {
        loc.shapeType = SHAPErotBox;
        loc.rotDir = X_ROT;
      } else
      if ((fabs(fmod(loc.yRot, 90.0)) > 1 ) && 
          (fabs(fmod(loc.yRot, 90.0)) < 89 ) ) {
        loc.shapeType = SHAPErotBox;
        loc.rotDir = Y_ROT;
      } else
      if ((fabs(fmod(loc.zRot, 90.0)) > 1 ) && 
           (fabs(fmod(loc.zRot, 90.0)) < 89 ) )  {
        loc.shapeType = SHAPErotBox;
        loc.rotDir = Z_ROT;
      }
    }
    return;
  }

    //    if box isn't orthog, 
    //    Consider cases depending on where center of sphere is w.r.t.
    //    x, y, and z intervals of box.
    //     *if all three of x, y and z coords of rotated sphere are within
    //      box, they certainly collide
    //     *If only two, just need to check distance of center of sphere from
    //      a single face.  Collide iff sphere intersects this face
    //     *If only one coord of sphere center is within box, intersection,
    //      if there is one, occurs with a known edge
    //     *If none, intersection occurs with a known corner.  Check if
    //      that corner is within the sphere.
  const bool Overlaps::checkSphereBox(Location* sphereLoc, Location* boxLoc) {
    Point sphereC(sphereLoc->c);
    Point rotC;   // represents sphere center after rotation into box coords
    bool rotated;
    if (rotated = (boxLoc->shapeType == SHAPErotBox)) {
      // adjust rot sphere center into box coords
      sphereC -= boxLoc->c;
      double rot;
      ROT_DIR dir = boxLoc->rotDir;

      switch(dir) {
      case X_ROT:
        rot = -boxLoc->xRot;
        break;
      case Y_ROT:
        rot = -boxLoc->yRot;
        break;
      case Z_ROT:
        rot = -boxLoc->zRot;
        break;
      case 0:   // shouldn't happen, but if it does nothing to do
        break;
      default:
        // tilt! Probably should throw an exception
        (*m_out) << "Volume " << boxLoc->vol->getName()
                 << " has complex rotation direction " << boxLoc->rotDir
                 << " HELP!" << std::endl;
        exit(1);
      }
      Point::doRot(rot, dir, &sphereC, &rotC);
    }
    else rotC = sphereC;
      
    // Now see where this center is w.r.t unrotated box
    // Use original box dimensions for non-orthog box; else
    // use bounding box dimensions (which are the same, but
    // may be permuted by rotations of 90 degress)
    int outside[3] = {0, 0, 0};
    double halfDim[3];
    unsigned insideCount = 0;
    halfDim[0] = 0.5 * ((rotated) ?  boxLoc->xDim : 
      (boxLoc->xBB[1] - boxLoc->xBB[0]));
    //    if (inside[0] = ((rotC.px < xHalfDim) && (rotC.px > -xHalfDim)))
    //      insideCount++;
    if (rotC.px > halfDim[0]) outside[0] = 1;
    else if (rotC.px <  -halfDim[0]) outside[0] = -1;
    else insideCount++;

    halfDim[1] = 0.5 * ((rotated) ? boxLoc->yDim :
            (boxLoc->yBB[1] - boxLoc->yBB[0]));
    if (rotC.py > halfDim[1]) outside[1] = 1;
    else if (rotC.py <  -halfDim[1]) outside[1] = -1;
    else insideCount++;

    halfDim[2] = 0.5 * ((rotated) ? boxLoc->zDim/2 :
            (boxLoc->zBB[1] - boxLoc->zBB[0]));
    if (rotC.pz > halfDim[2]) outside[2] = 1;
    else if (rotC.pz <  -halfDim[2]) outside[2] = -1;
    else insideCount++;

    switch (insideCount) {
    case 3: 
      return false;
    case 2: {
      if (outside[0] != 0) {
        return (outside[0]*sphereLoc->c.px >= sphereLoc->rOut + halfDim[0]);
      }
      else if (outside[1] != 0) {
        return (outside[1]*sphereLoc->c.py >= sphereLoc->rOut + halfDim[1]);
      }
      else if (outside[2] != 0) {
        return (outside[2]*sphereLoc->c.pz >= sphereLoc->rOut + halfDim[2]);
      }
      break;
    }
    case 1: // If the sphere intersects the box at all, it must intersect
      // the nearest edge.  First see if either corner is in sphere
      {
        Point corner1;
        Point corner2;
        if (!outside[0]) {
          corner1 = 
            Point(-halfDim[0], outside[1]*halfDim[1], outside[2]*halfDim[2]);
          corner2 = 
            Point(halfDim[0], outside[1]*halfDim[1], outside[2]*halfDim[2]);
        } else if (!outside[1]) {
          corner1 = 
            Point(outside[0]* halfDim[0], -halfDim[1], outside[2]*halfDim[2]);
          corner2 = 
            Point(outside[0]*halfDim[0], halfDim[1], outside[2]*halfDim[2]);
        } else {
          corner1 = 
            Point(outside[0]* halfDim[0], outside[1]*halfDim[1], -halfDim[2]);
          corner2 = 
            Point(outside[0]*halfDim[0], outside[1]*halfDim[1], halfDim[2]);
        }
        if (Point::dist(corner1, sphereLoc->c) < sphereLoc->rOut) return false;
        if (Point::dist(corner2, sphereLoc->c) < sphereLoc->rOut) return false;

        // only other way volumes can overlap is for sphere boundary to
        // intersect edge.  See if distance from edge to center of sphere
        // is < outer radius.  Since edge is || to some axis, relatively
        // easy to compute distance to it.
        double toEdgeSq;
        if (!outside[0])  {        
          toEdgeSq =  (sphereLoc->c.py - (outside[1]*halfDim[1])) *
            (sphereLoc->c.py - (outside[1]*halfDim[1]))   +
            (sphereLoc->c.pz - (outside[2]*halfDim[2])) *
            (sphereLoc->c.pz - (outside[2]*halfDim[2]));   
        }
        else if (!outside[1])  {
          toEdgeSq =  (sphereLoc->c.px - (outside[0]*halfDim[0])) *
            (sphereLoc->c.px - (outside[0]*halfDim[0]))   +
            (sphereLoc->c.pz - (outside[2]*halfDim[2])) *
            (sphereLoc->c.pz - (outside[2]*halfDim[2]));   
        }
        else {
          toEdgeSq =  (sphereLoc->c.px - (outside[0]*halfDim[0])) *
            (sphereLoc->c.px - (outside[0]*halfDim[0]))   +
            (sphereLoc->c.py - (outside[1]*halfDim[1])) *
            (sphereLoc->c.py - (outside[1]*halfDim[1]));   
        }
        return (toEdgeSq >= sphereLoc->rOut * sphereLoc->rOut);
        break;
      }
    case 0: // just check if relevant corner intersects sphere 
      {
        Point corner(outside[0]*halfDim[0], outside[1]*halfDim[1], 
                     outside[2]*halfDim[2]);
        return Point::dist(corner, sphereLoc->c) >= sphereLoc->rOut;
      }
    default:  // nothing else can ever happen; this is silly
      return false;
    }
    return false;
  } 

  const bool Overlaps::checkBoxes(Location* /* loc1 */, Location* /* loc2 */) {
    //  TEMPORARY!
    return false;
  }

  void  Overlaps::Point::doRot(double rot, ROT_DIR dir, const Point* initPos, 
                     Point* finalPos) {
    double x = initPos->px;
    double y = initPos->py;
    double z = initPos->pz;

    double rad = (rot*PI)/180.0;
    switch(dir) {
    case X_ROT:
      // rotate in yz plane 
      finalPos->px = x;
      finalPos->py = y*cos(rad) - z*sin(rad);
      finalPos->pz = z*cos(rad) + y*sin(rad);
      break;
    case Y_ROT:
      finalPos->py = y;
      finalPos->pz = z*cos(rad) - x*sin(rad);
      finalPos->px = x*cos(rad) + z*sin(rad);
      break;

    case Z_ROT:
      finalPos->pz = z;
      finalPos->px = x*cos(rad) - y*sin(rad);
      finalPos->py = y*cos(rad) + x*sin(rad);
      break;
    default:
      break;
    }
  }
  double Overlaps::Point::dist(const Point& a, const Point& b) {
    return sqrt((a.px - b.px)*(a.px - b.px) + (a.py - b.py)*(a.py - b.py) + 
                (a.pz - b.pz)*(a.pz - b.pz));
  }
  /*
  const bool Overlaps::Point::rotAbout(double rot, ROT_DIR dir, 
                                       const Point* initPos, 
                                       Point* resultPos) {
    // translate to our local coord.system
    double x = initPos->px - px;
    double y = initPos->py - py;
    double z = initPos->pz - pz;
    double rad = rot*PI/180.0;

    // Do the rotation
    switch(dir) {
    case X_ROT:
      // rotate in yz plane 
      resultPos->px = x;
      resultPos->py = y*cos(rad) - z*sin(rad);
      resultPos->pz = z*cos(rad) + y*sin(rad);
      break;
    case Y_ROT:
      resultPos->py = y;
      resultPos->pz = z*cos(rad) - x*sin(rad);
      resultPos->px = x*cos(rad) + z*sin(rad);
      break;

    case Z_ROT:
      resultPos->pz = z;
      resultPos->px = x*cos(rad) - y*sin(rad);
      resultPos->py = y*cos(rad) + x*sin(rad);
      break;
    default:
    }
    // translate back
    resultPos->px += px;
    resultPos->py += py;
    resultPos->pz += pz;
    return true;
  }
  */
}
