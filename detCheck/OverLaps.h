// $Header:  $

#include <map>
#include <string>
#include "detModel/Gdd.h"
#include "detModel/Sections/Volume.h"

// need some or all of the following:

#include "detModel/Sections/BoundingBox.h"
#include "detModel/Sections/Position.h"
#include "detModel/Sections/Ensemble.h"
#include "detModel/Sections/Composition.h"
#include "detModel/Sections/Stack.h"
#include "detModel/Sections/Box.h"
#include "detModel/Sections/PosXYZ.h"

namespace detCheck {
  class Overlaps {
    Overlaps(detModel::Gdd* gdd);       
    ~Overlaps();

    //! Look for any overlaps in the model; return status and
    //! write information about overlaps to errfile, defaulting
    //! to std::cout
    int check(std::string errfile="");

    // someday might implement similar method which takes a volume
    // name as argument and only looks for overlaps within that
    // volume (and its children).
  };
}
