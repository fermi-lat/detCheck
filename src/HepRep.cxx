/// Test program for the generic model

#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <vector>

#include "HepRepSectionsVisitor.h"

#include "detModel/Management/Manager.h"
#include "detModel/Management/VrmlSectionsVisitor.h"
#include "detModel/Management/CountMaterial.h"
#include "detModel/Management/IDmapBuilder.h"
#include "detModel/Management/PrinterSectionsVisitor.h"
#include "detModel/Management/HtmlConstantsVisitor.h"
#include "detModel/Management/PrinterMaterialsVisitor.h"
#include "detModel/Management/XercesBuilder.h"
#include "detModel/Materials/MatCollection.h"
#include "detModel/Materials/Material.h"
#include "detModel/Utilities/Color.h"
#include "detModel/Utilities/PositionedVolume.h"
#include "detModel/Sections/Volume.h"
#include "detModel/Sections/Shape.h"
#include "detModel/Sections/Box.h"
#include "detModel/Gdd.h"


/* This basic test needs two argument; the xml file to use and the volume 
   name to use as the mother volume (for example oneCAL). If only the filename
   is specified, the topVolume specified in the section is used as the mother
   volume.

   Ex.
   ./test.exe ../../../xmlUtil/v2r1/xml/flight.xml oneTKR
   
   The test program produce a sections.wrl file with the HEPREP representation 
   of the geometry (be careful, if you have a big geometry the file can be
   huge).
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

  // We start the VRMLSectionsVisitor to build the vrml file
  // If we don't specify a string in the constructor, it will build the
  // vrml file for all the volumes placed in the topVolume, otherwise it
  // will build the vrml file for the specified volume. The output is
  // placed in sections.vrml
  std::string volName("");
  if (argc > 2) volName = std::string(argv[2]);

  detModel::IDmapBuilder idMap(volName);
  manager->startVisitor(&idMap);
  idMap.summary(std::cout);
  
  detModel::HepRepSectionsVisitor* visitor;
  visitor = new detModel::HepRepSectionsVisitor(volName);  
  visitor->setIDPrefix(idMap.getIDPrefix());
  visitor->setPrefixTransform(idMap.getTransform3DPrefix());

  visitor->setMode("digi");
  // We retrieve the hierarchy entry point, i.e. the GDD object. It
  // contains all the relevant information
  detModel::Gdd* g = manager->getGdd();

  // Retrieve the materials, generate the colors and set some 
  // transparency values
  detModel::MatCollection* mats = g->getMaterials();  
  mats->generateColor();
  
  // We start the heprep visitor
  manager->startVisitor(visitor);

  delete visitor;
  delete manager;
  return(0);
}





