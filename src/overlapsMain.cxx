
#include <string>
#include <iostream>
#include <vector>
#include "detModel/Management/Manager.h"
#include "detModel/Management/XercesBuilder.h"
#include "detCheck/Overlaps.h"

/// Look for overlaps in a geometric representation.
/// Arguments (with their [defaults]) are
///     xml file to be parsed              [../xml/test-overlap.xml]
///     file for output                    [std::cout]
///     verbose mode                       [false]
/// Invoking this program with any value at all for the third argument
/// will turn on verbose mode.
int main(int argc, char* argv[]) {

  char* inFile = "../xml/test-overlap.xml";
  char* outFile = "";

  // Supplied argument overrides default test file
  if (argc > 1) {
    inFile = argv[1];
  }

  // Supplied argument can override default output file (std::cout)
  if (argc > 2) {
    outFile = argv[2];
  }

  bool verbose = (argc > 3);

  // We retrieve the manager pointer (it is a singleton, so it is not possible
  // to create it in the usual way)
  detModel::Manager* manager = detModel::Manager::getPointer();

  // Set the builder and the file name
  manager->setBuilder(new detModel::XercesBuilder);
  manager->setNameFile(inFile);
  
  // We set the mode for the choice elements in the XML file
  manager->setMode("digi");

  // We build the hierarchy; in that case we build all, i.e. both the constants
  // the sections and the materials
  manager->build(detModel::Manager::all);

  detModel::Gdd* gdd = manager->getGdd();

  detCheck::Overlaps* oCheck = new detCheck::Overlaps(gdd);

  bool ok = oCheck->check(std::string(outFile), verbose);

  return ok;
}

