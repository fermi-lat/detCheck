/// Diagnostic program to dump all ids

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <cmath>

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

#include "xmlUtil/id/IdDict.h"

using detModel::IDmapBuilder;

/* This basic test needs one argument: the xml file to use.

   Ex.
   ./dumpIds.exe ../../../xmlGeoDbs/v2r1/xml/flight/flight.xml 
   
   The test program produces an ascii file with one line per sensitive volume,
   with its id

*/
int main(int argc, char* argv[]) {

  // We need the XML flight as input to the test executable
  if (argc == 1) {
    std::cout << "Invoke as follows: " << std::endl;
    std::cout << "    ./dumpIds.exe infileSpec [outfileSpec [myTopVolume]] " 
              << std::endl;

    std::cout << "Default creates the file dumpIds.txt in default directory" 
              << std::endl;
    return 0;
  }

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
  if (!manager->build(detModel::Manager::all)) {
    std::cerr << "Unable to build geometry" << std::endl;
    return -1;
  }

  std::string outfile("dumpIds.txt");
  if (argc > 2) outfile = std::string(argv[2]);

  std::string volName("");
  if (argc > 3) volName = std::string(argv[3]);

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

  xmlUtil::IdDict* pDict = g->getIdDictionary();

  // Open output file
  std::ofstream out;
  out.open(outfile.c_str());

  const IDmapBuilder::IdVector* idVectors = idMap.getIdVector();
  
  unsigned int iVec = 0;
  for  (iVec = 0; iVec < (idVectors->size()); iVec++) {
    idents::VolumeIdentifier volIdent = (*idVectors)[iVec];
    out << ((*idVectors)[iVec]).name() << "  " << std::endl;

    xmlUtil::Identifier identifier;
    unsigned iField;
    for (iField = 0; iField < volIdent.size(); iField++) {
      identifier.append(volIdent[iField]);
    }
    out << *(pDict->getNamedId(identifier)) << "  " << std::endl;    
    const detModel::PositionedVolume* pPosVol = 
      idMap.getPositionedVolumeByID(volIdent);

    Hep3Vector trans = pPosVol->getTranslation();
    HepRotation rot=pPosVol->getRotation();
    double phi = rot.getPhi();
    double theta = rot.getTheta();
    bool isRot = (fabs(phi) + fabs(theta)) > .001;
    out << "center: (" << trans.getX() << ", " << trans.getY() << ", " ;
    out << trans.getZ() << ") " ;
    if (isRot) out << "(unrotated) ";
    out << "X,Y,Z dim: " ;
    detModel::BoundingBox* pBox = pPosVol->getVolume()->getBBox();

    out << pBox->getXDim() << ", " << pBox->getYDim() << ", " ;
    out << pBox->getZDim()  << std::endl;

    if (isRot) {
    out << " and rotation angles theta=" << theta << " phi=" << phi 
        << std::endl << std::endl;
    }
    else out << std::endl;
  }

  delete manager;
  return(0);
}





