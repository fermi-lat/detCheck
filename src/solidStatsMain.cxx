//      $Header$  
#include <string>
#include <iostream>
#include <vector>
#include "detModel/Management/Manager.h"
#include "detModel/Management/XercesBuilder.h"
#include "detCheck/SolidStats.h"

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

  char* inFile = "../xml/test-solidStats.xml";
  char* outFile = "";
  char* topVolume = "";
  char* choiceMode= "propagate";


  // Supplied argument overrides default test file
  if (argc > 1) {
    inFile = argv[1];
  }

  // Supplied argument can override default output file (std::cout)
  if (argc > 2) {
    outFile = argv[2];
  }

  if (argc > 3) {
    topVolume = argv[3];
  }

  if (argc > 4) {
    choiceMode = argv[4];
  }

  detModel::Manager* manager = detModel::Manager::getPointer();

  manager->setBuilder(new detModel::XercesBuilder);
  manager->setNameFile(inFile);
  
  manager->setMode(choiceMode);

  // Build the full hierarchy (sections, constants, materials)
  manager->build(detModel::Manager::all);

  detCheck::SolidStats* sStats = 
    new detCheck::SolidStats(std::string(topVolume));

  // Uncomment the line below for the gory details of traversal
  //  sStats->setDiagnostic("diag.txt");

  // Traverse detModel and build up our own data structures
  manager->startVisitor(sStats);

  // Output results
  sStats->report(std::string(outFile));

  return true;
}

