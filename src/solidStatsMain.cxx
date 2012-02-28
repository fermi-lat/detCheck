//      $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/detCheck/src/solidStatsMain.cxx,v 1.6 2004/03/19 19:06:32 jrb Exp $  
#include <string>
#include <iostream>
#include <vector>
#include "detModel/Management/Manager.h"
#include "detModel/Management/XercesBuilder.h"
#include "detCheck/SolidStats.h"
#include "facilities/commonUtilities.h"

/** 
    @file solidStatsMain.cxx

    Stand-alone program which invokes the SolidStats sections visitor 
    to produce statistics on solids by material
    Arguments (with their [defaults]) are
      @a @b file  xml file to be parsed    [../xml/test-solidStats.xml]
      @a @b outfile  file for output       [std::cout]
      @a @b vol     top volume name        ["", i.e., top volume of section]
      @a @b mode     <choice> mode         ["propagate"]
*/
int main(int argc, char* argv[]) {

  std::string inFile = "$(XMLGEODBSXMLPATH)/flight/flight.xml";
  std::string outFile = "";
  std::string topVolume = "oneCAL";
  std::string choiceMode= "propagate";

  facilities::commonUtilities::setupEnvironment();
  
  // Supplied argument overrides default test file
  if (argc > 1) {
    inFile = std::string(argv[1]);
  }

  // Supplied argument can override default output file (std::cout)
  if (argc > 2) {
    outFile = std::string(argv[2]);
  }

  if (argc > 3) {
    topVolume = std::string(argv[3]);
  }

  if (argc > 4) {
    choiceMode = std::string(argv[4]);
  }

  detModel::Manager* manager = detModel::Manager::getPointer();

  manager->setBuilder(new detModel::XercesBuilder);
  manager->setNameFile(inFile);
  
  manager->setMode(choiceMode);

  // Build the full hierarchy (sections, constants, materials)
  if (!manager->build(detModel::Manager::all)) {
    std::cerr << "Unable to build geometry" << std::endl;
    return -1;
  }


  detCheck::SolidStats* sStats = 
    new detCheck::SolidStats(topVolume);

  // Uncomment the line below for the gory details of traversal
  //  sStats->setDiagnostic("diag.txt");

  // Traverse detModel and build up our own data structures
  manager->startVisitor(sStats);

  // Output results
  sStats->report(outFile);

  return true;
}

