// Mainpage for doxygen

/** @mainpage package detCheck
<p>
This package provides tools for examining and validating a detModel geometry
description.  Each tool is implemented as a c++ class and a standalone
program which invokes it. Other applications linking to the library may also 
use the classes.  The entire package may someday become a subpackage 
of detModel.</p>
<p>
To date two functions are provided:</p>
<ul>
 <li>Overlap checking.  See the class Overlaps.  It examines all sibling
positioned volumes (children of the same composition or stack) to see
that they don't overlap.  It also checks that all children of a composition
are contained in its envelope.</li>
 <li>Summary materials statistics.  See the class SolidStats.  It outputs
numbers of volumes made of each material, total cubic volume of each
material, and so forth.</li>
</ul>
<hr>
  \section requirements cmt/requirements
  \include requirements

*/
