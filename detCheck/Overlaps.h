// $Header: /nfs/slac/g/glast/ground/cvs/detCheck/detCheck/Overlaps.h,v 1.4 2007/03/24 06:35:46 jrb Exp $

#include <string>
#include <iostream>
#include <fstream>

#include "detModel/Gdd.h"
#include "detModel/Sections/Composition.h"
#include "detModel/Sections/Stack.h"
#include "detModel/Sections/AxisMPos.h"

namespace detModel {
  class BoundingBox;
  class Volume;
  class PosXYZ;
}

namespace detCheck {
  /**  This class checks for overlaps among all positioned volumes
       in the given Gdd.  In practice this just means checking that
       "siblings" within an ensemble don't overlap. 

       For now check all volumes, regardless of choice mode.
  */
    enum ROT_DIR {
      NO_ROT = 0,
      X_ROT = 1,
      Y_ROT = 2,
      Z_ROT = 4 };

    enum SHAPE {
      SHAPEotherShape = 0,
      SHAPEorthBox = 1,
      SHAPErotBox = 2,
      SHAPEsphere = 3
    };


  class Overlaps {
  public:

    Overlaps(detModel::Gdd* gdd);
    ~Overlaps() {}

    /** Look for any overlaps in the model; return status and
        write information about overlaps to errfile, defaulting
        to std::cout.

        The actual work is done by checking for overlaps within 
        each ensemble (composition or stack).  Because of the 
        way the Gdd is constructed, there is no chance that a 
        child volume will "stick out" of its enclosing Ensemble.  
        All that remains is to check that siblings don't overlap.  
        The work is done by several private methods.         */
    bool check(std::string errfileName, bool verbose = false, 
               bool dump = false);


    /** Trying to make floating-point comparisons is risky.   If
        epsilon is > 0 "overlap" will be interpreted to mean "overlaps
        by no more than epsilon".  By default Overlaps will make
        comparisons using a small but non-zero DEFAULT_EPSILON.

        This method is only provided for debugging purposes.  It
        returns the old value of epsilon. */
    double setEpsilon(double epsilon);


  private:
    static double DEFAULT_EPSILON;

    detModel::Gdd* m_gdd;
    std::ostream* m_out;
    double        m_eps;
    bool          m_verbose;
    bool          m_dump;
    bool          m_childrenPrinted;
    class Point {
    public:
      double px, py, pz;
      Point(double x=0,double y=0, double z=0) : px(x), py(y), pz(z) {}
      /* According to Stroustrup, default assignment and copy constructor
         will do the right thing
      Point(const Point& other) : px(other.px), py(other.py), pz(other.pz) {}
      Point& operator=(const Point& other) {
        px = other.px; py = other.py; pz = other.pz; return *this;
      }
      */
      Point& operator+=(Point other) {
        px += other.px; py += other.py; pz+= other.pz; return *this;
      }
      Point& operator-=(Point other) {
        px -= other.px; py -= other.py; pz -= other.pz; return *this;
      }

      Point operator+(Point b) { 
        return (*this) += b;
      }

      Point operator-(Point b) { 
        return (*this) -= b;
      }

      static double dist(const Point& a, const Point& b);

      // Find new coordinates when rotating initPos about the
      // point in direction dir.  Angle rot is expressed in degrees.
      // Return false if input is bad (unknown dir)
      //      const bool rotAbout(double rot, ROT_DIR dir, 
      //                          const Point* initPos, Point* finalPos);
      static void doRot(double rot, ROT_DIR dir, const Point* initPos,
                        Point* finalPos);
    };
    class Location {
    public:
      detModel::BoundingBox* bBox;
      detModel::Volume* vol;
      double xBB[2], yBB[2], zBB[2]; // orthog. bounding box corners
      double  rOut, rIn;             // for sphere
      ROT_DIR rotDir;
      double  xRot, yRot, zRot;
      double  xDim, yDim, zDim;  // unrotated box dimensions
      Point c;
      Point v[8]; // 8 vertices of a box, in order (before rotation,
                  // if any) [-x,-y,-z], [-x,-y,+z], [-x, +y, -z] etc.
      unsigned shapeType;  // 1 for orth box, 2 for rot box, 3 for sphere,
                           // 0 for 'other' (e.g. tube, stack)
      Location() : bBox(0), vol(0), rOut(0), rIn(0), rotDir(NO_ROT), 
                   shapeType(0) {};
    };              // end nested Location class

    /// Look for overlaps among (1st-generation) children of
    /// a Composition. 
    bool checkComposition(detModel::Composition* );

    //! Look for overlaps among (1st-generation) children of
    //!
    bool checkStack(detModel::Stack* );

    //! Since an AxisMPos element describes how to position
    //!  n identical volumes there is a chance of overlap among
    //!  them.
    bool checkMPos(detModel::AxisMPos*);

    /// Used in verbose mode; prints names of all child volumes 
    void printChildren(detModel::Ensemble *ens);

    void fillPos(Location& loc, detModel::PosXYZ *pos);
    //! Local class describing locations of two opposite corners
    //! of a positioned bounding box. 

    //! Utility invoked to check pairwise for overlaps among
    //! a sequence of Locations
    bool checkLocs(std::vector<Location>& locs);
    bool pairOk(Location* loc1, Location* loc2);

    //! Extra checking to eliminate false positives if first loc describes
    //! sphere, second describes bo
    const bool checkSphereBox(Location* sphereLoc, Location* boxLoc);
    const bool checkBoxes(Location* box1Loc, Location* box2Loc);

    //! See if all children are inside the envelope (they should be)
    bool checkEnvelope(detModel::Composition*, std::vector<Location>& locs);
  };  //end Overlaps class
}  // end namespace detCheck

