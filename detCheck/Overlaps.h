// $Header: /nfs/slac/g/glast/ground/cvs/detCheck/detCheck/Overlaps.h,v 1.2 2003/06/27 17:03:56 jrb Exp $

#include <string>
#include <iostream>
#include <fstream>

#include "detModel/Gdd.h"
#include "detModel/Sections/Composition.h"
#include "detModel/Sections/Stack.h"
#include "detModel/Sections/AxisMPos.h"

namespace detCheck {
  /**  This class checks for overlaps among all positioned volumes
       in the given Gdd.  In practice this just means checking that
       "siblings" within an ensemble don't overlap. 

       For now check all volumes, regardless of choice mode.
  */
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

    //! Local class describing locations of two opposite corners
    //! of a positioned bounding box.  Can use in bitmask to
    //! express multiple rotations
    enum ROT_DIR {
      X_ROT = 1,
      Y_ROT = 2,
      Z_ROT = 4 };

    class Point {
    public:
      double px, py, pz;
      // Find new coordinates when rotating initPos about the
      // point in direction dir.  Angle rot is expressed in degrees.
      // Return false if input is bad (unknown dir)
      const bool rotAbout(double rot, ROT_DIR dir, 
                          const Point* initPos, Point* finalPos);
    };
    class Location {
    public:
      double x[2], y[2], z[2]; // orthog. bounding box corners
      double  rOut, rIn;             // for sphere
      double  xRot, yRot, zRot;
      double  xDim, yDim, zDim;  // unrotated box dimensions
      Point c;
      Point v[8]; // 8 vertices of a box, in order (before rotation,
                  // if any) [-x,-y,-z], [-x,-y,+z], [-x, +y, -z] etc.
      unsigned shapeType;   // 0 for orthog box, 1 for rot box, 2 for sphere
      Location() : rOut(0), rIn(0), shapeType(0) {};
    };              // end nested Location class

    //! Utility invoked to check pairwise for overlaps among
    //! a sequence of Locations
    bool checkLocs(std::vector<Location>& locs);
    bool pairOk(Location* loc1, Location* loc2);

    //! See if all children are inside the envelope (they should be)
    bool checkEnvelope(detModel::Composition*, std::vector<Location>& locs);
  };  //end Overlaps class
}  // end namespace detCheck

