
#include <string>
#include <iostream>
#include <vector>
#include "detModel/Management/Manager.h"
#include "detModel/Management/XercesBuilder.h"
#include "detCheck/SolidStats.h"

/// Stand-alone program which invokes the SolidStats sections visitor 
/// to produce statistics on solids by material
/// Arguments (with their [defaults]) are
///     xml file to be parsed              [../xml/test-solidStats.xml]
///     file for output                    [std::cout]
///     top volume name                    ["*", i.e., top volume of section]
///     <choice> mode                      ["propagate"]
///     verbose mode                       [false]
/// Invoking this program with any value at all for the last argument
/// will turn on verbose mode.
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

  bool verbose = (argc > 5);

  bool html = false;       

  detModel::Manager* manager = detModel::Manager::getPointer();

  manager->setBuilder(new detModel::XercesBuilder);
  manager->setNameFile(inFile);
  
  manager->setMode(choiceMode);

  // Build the full hierarchy (sections, constants, materials)
  manager->build(detModel::Manager::all);

  detCheck::SolidStats* sStats = 
    new detCheck::SolidStats(std::string(topVolume));

  sStats->setDiagnostic("diag.txt");
  // Traverse detModel and build up our own data structures
  manager->startVisitor(sStats);

  // Output results
  sStats->report(std::string(outFile), verbose, html);

  return true;
}

