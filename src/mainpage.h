// Mainpage for doxygen

/** @mainpage package detCheck
<p>
This package provides tools for examining and validating a detModel geometry
description.  Each tool is implemented as a c++ class and a standalone
program which invokes it. Other applications linking to the library may also 
use the classes.  The entire package may someday become a subpackage 
of detModel.</p>
<p>
To date five functions are provided:</p>
<ul>
 <li>Overlap checking, executable test.exe.
  See the class Overlaps and main program 
  overlapsMain.cxx.  It examines all sibling
positioned volumes (children of the same composition or stack) to see
that they don't overlap.  It also checks that all children of a composition
are contained in its envelope.  There is also a dump mode which will
output volume dimensions and positions even when there are no overlaps.</li>
 <li>Documentation of constants, executable constsDoc.exe.  
See the main program constsDocMain.cxx,
  which invokes detModel utilities to produce html output.</li>
 <li>Summary materials statistics, executable summary.exe.  
See the class SolidStats and main
program solidStatsMain.cxx.  It outputs
numbers of volumes made of each material, total cubic volume of each
material, and so forth.</li>
 <li>Creation of heprep file from xml desscription, suitable
as input for <a href="http://www.fisica.uniud.it/~riccardo/research/fred/">
FRED</a>
 </li>
 <li>Dump of geometry information (centroid position; X,Y,Z dimensions)
for all active elements, executable dumpIds.exe
 </li>
</ul>
<p>These programs are invoked as follows (optional arguments are in
[square brackets])</p>
<pre>
   <b>test.exe</b> aPath/myGeoInput.xml [anotherPath/output.txt [verbose [dump] ] ]
</pre>
<p>where the last two arguments are boolean.  By default both are false,
but if present (any string) they are true.</p>
<pre>
   <b>constsDoc.exe</b> aPath/myGeoInput.xml anotherPath/myGeoOutput.html

   <b>summary.exe</b> aPath/myGeoInput.xml  anotherPath/mySummary.html [topVolume  [choice-mode] ]

   <b>heprep.exe</b> aPath/myGeoInput.xml [topVolume [outputFilename] ]
</pre>
<p>If neither of the optional arguments is specified, the topVolume specified
in the xml file will be used and the output filename will be sections.heprep.
If the output file name is not specified but topVolume is, the output will
be named after it, e.g. LAT.heprep for topVolume LAT.</p>

<pre>
   <b>dumpIds.exe</b> aPath/myGeoInput.xml
</pre>
<p>See sample output from constsDoc.exe, summary.exe and dumpIds.exe at
<a href="http://www.slac.stanford.edu/exp/glast/ground/software/geometry/data/RELEASED/">http://www.slac.stanford.edu/exp/glast/ground/software/geometry/data/RELEASED/</a></p>
<hr>
  \section requirements cmt/requirements
  \include requirements

*/
