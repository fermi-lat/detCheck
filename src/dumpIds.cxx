/// Diagnostic program to dump all ids

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <vector>

#include "detModel/Management/Manager.h"
#include "detModel/Management/IDmapBuilder.h"
#include "detModel/Management/XercesBuilder.h"
#include "detModel/Materials/MatCollection.h"
#include "detModel/Materials/Material.h"
#include "detModel/Utilities/Color.h"
#include "detModel/Utilities/PositionedVolume.h"
#include "detModel/Sections/Volume.h"
#include "detModel/Sections/Shape.h"
#include "detModel/Sections/Box.h"
#include "detModel/Gdd.h"

using detModel::IDmapBuilder;

/* This basic test needs one argument: the xml file to use.

   Ex.
   ./test.exe ../../../xmlUtil/v2r1/xml/flight.xml 
   
   The test program produces an ascii file with one line per sensitive volume,
   with its id

*/
int main(int argc, char* argv[]) {

  // We need the XML flight as input to the test executable
  if (argc == 1) return 0;

  // We retrive the manager pointer (it is a singleton, so it is not possible
  // to create it in the usual way)
  detModel::Manager* manager = detModel::Manager::getPointer();

  // Set the builder and the file name
  manager->setBuilder(new detModel::XercesBuilder);
  manager->setNameFile(argv[1]);
  
  // We set the mode for the choice elements in the XML file
  //manager->setMode("digi");

  // We build the hierarchy; in that case we build all, i.e. both the constants
  // the sections and the materials
  manager->build(detModel::Manager::all);

  std::string volName("");
  if (argc > 2) volName = std::string(argv[2]);

  // We retrieve the hierarchy entry point, i.e. the GDD object. It
  // contains all the relevant information
  detModel::Gdd* g = manager->getGdd();
  
  // An example; we retrieve some info from the xml file
  std::cout << "XML file contains " << g->getVolumesNumber() << " volumes." << std::endl;
  std::cout << "XML file contains " << g->getMaterialsNumber() << " materials." << std::endl;
  std::cout << "XML file contains " << g->getConstantsNumber() << " constants." << std::endl;

  detModel::IDmapBuilder idMap(volName);
  manager->startVisitor(&idMap);
  idMap.summary(std::cout);

  std::cout << "Now for id list: " << std::endl;

  // Open output file
  std::ofstream out;
  out.open("dumpIds.txt");

  const IDmapBuilder::IdVector* idVectors = idMap.getIdVector();
  //  const std::vector<idents::VolumeIdentifier> * idVectors = idMap.getIdVector();
  

  int iVec = 0;
  for  (iVec = 0; iVec < (idVectors->size()); iVec++) {
    out << ((*idVectors)[iVec]).name() << std::endl;
  }

  delete manager;
  return(0);
}





