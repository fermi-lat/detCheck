#include <string>
#include <iostream>
#include "detModel/Management/Manager.h"
#include "detModel/Management/XercesBuilder.h"
#include "detModel/Management/HtmlConstantsVisitor.h"
/** Make html documentation for constants for specified input geometry
    description using the detModel HtmlConstantsVisitor.
    The program has two arguments, both required: 
      input and output file specification, respectively.
*/
#define BAD_CALL 1

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cout << "Input xml file spec and output html file spec" << std::endl;
    std::cout << "are required arguments 1 and 2, resp. " << std::endl;
    std::cout << "Have a nice day. " << std::endl;
    return BAD_CALL;
  }

  char* inFile = argv[1];
  char* outFile = argv[2];

  detModel::Manager* manager = detModel::Manager::getPointer();

  // Set the builder and the file name
  manager->setBuilder(new detModel::XercesBuilder);
  manager->setNameFile(inFile);
  
  // We build the hierarchy.  All we need are constants.
  manager->build(detModel::Manager::constants);

  detModel::HtmlConstantsVisitor* vis = 
    new detModel::HtmlConstantsVisitor(std::string(outFile));

  manager->startVisitor(vis);

  return 0;
}
