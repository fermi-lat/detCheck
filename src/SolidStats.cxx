// $Header: /nfs/slac/g/glast/ground/cvs/detCheck/src/SolidStats.cxx,v 1.2 2002/01/15 23:23:15 jrb Exp $

#include <cmath>
#include <cassert>
#include "detCheck/SolidStats.h"

// may need need some or all of the following:

#include "detModel/Management/Manager.h"
#include "detModel/Sections/Volume.h"
#include "detModel/Sections/BoundingBox.h"
#include "detModel/Sections/Ensemble.h"
#include "detModel/Sections/Composition.h"
#include "detModel/Sections/Stack.h"
#include "detModel/Sections/Shape.h"
#include "detModel/Sections/Box.h"
#include "detModel/Sections/Tube.h"
#include "detModel/Sections/Choice.h"
#include "detModel/Sections/PosXYZ.h"
#include "detModel/Sections/AxisMPos.h"
#include "detModel/Sections/Section.h"
#include "detModel/Materials/MatCollection.h"

namespace detCheck {

  double SolidStats::PI = 0;    // Set properly at first use
  SolidStats::SolidStats(std::string topVolume) : 
    m_gdd(0), m_topName(topVolume), m_out(0), m_verbose(false)
  {
    setRecursive(0);
    detModel::Manager* man = detModel::Manager::getPointer();

    m_choiceMode = man->getMode();
  }

  SolidStats::~SolidStats() {
    // Run through solids map, deleting each individually allocated 
    // solid struct; then do the same for the materials map
    for (LogVolMapIt logIt = m_logVols.begin(); logIt != m_logVols.end();
         ++logIt) {
      delete logIt->second;
    }

    for (MatMapIt matIt = m_mats.begin(); matIt != m_mats.end();
         ++matIt) {
      //      delete matIt->second->logVols;
      delete matIt->second;
    }

  }

  void SolidStats::report(std::string errfileName, bool verbose, bool html) {
    m_verbose = verbose;
    bool allocStream = false;
    if (errfileName.size() == 0) {
      m_out = &std::cout;  // or out = new ostream(std::out);  ?
    }
    else {
      m_out = new std::ofstream(errfileName.c_str());
      allocStream = true;
    }

    // Here is where all the work goes to output the information saved
    // while visiting..
    if (html) {
      (*m_out) << 
        "<html><head><title>Materials Summary</title></head>" << std::endl;
      (*m_out) << "<body> <h2>Materials Summary</h2>" << std::endl;
      (*m_out) << "<table> cellpadding='3' border='1'>" << std::endl;
      (*m_out) << "<tr bgcolor='#c0ffc0' align ='left'>";
      (*m_out) << "<th>Material Name</th><th># Log. Vol.</th>";
      (*m_out) << "<th># Phys. Vol.</th><th>Volume (cubic mm)</th></tr>" << std::endl;
    }
    else {  // just title
      (*m_out) << "Materials Summary" << std::endl;
    }

    // Always output summary per-material information 
    for (MatMapIt mapIt = m_mats.begin(); mapIt != m_mats.end(); ++mapIt) {
      Material* mat = mapIt->second;
      if (html) {
        (*m_out) << "<tr><td>" << mapIt->first << "</td><td>" << mat->logCount
                 << "</td><td>" << mat->physCount << "</td><td>" 
                 << mat->cuVol << "</td></tr>" << std::endl;
      }
      else {
        (*m_out) << " " << mapIt->first <<  ":" << std::endl;
        (*m_out) << "#Log volumes = " << mat->logCount
                 << " #Phys volumes = " << mat->physCount 
                 << " Total volume = " << mat->cuVol << " cu mm" << std::endl;
      }
    }

    // TO DO: Extra output for verbose will go here

    if (html) {
      (*m_out) << "</table></body></html>" << std::endl;
    }
      

    if (allocStream) delete m_out;
    return;
  }

 
  void SolidStats::visitGdd(detModel::Gdd* gdd) {

    initMaterials(gdd);
    
    // Do the actual visiting:
    (gdd->getSections())[0]->AcceptNotRec(this);

    // Now fill in remaining statistics in materials data structures
    // by iterating through logical volumes map
    for (LogVolMapIt volIt = m_logVols.begin(); volIt != m_logVols.end();
         ++volIt) {
      LogVol *logVol = volIt->second;

      MatMapIt mapIt = m_mats.find(logVol->matName);
      assert (mapIt != m_mats.end());
      
      Material* mat = mapIt->second;
      
      mat->logVols.push_back(logVol);
      ++(mat->logCount);
      mat->physCount += logVol->nCopy;
      mat->cuVol += logVol->nCopy * logVol->vol;
    }
  }

  void SolidStats::visitSection(detModel::Section* sect) {
    // If we have a non-null top volume, attempt to find it.
    if (m_topName.size() != 0) {
      m_top = m_gdd->getVolumeByName(m_topName);
      if (m_top == 0) m_topName = "";
    }

    // If top volume name is null,  use section's top volume
    if (m_topName.size() == 0) {
      m_top = sect->getTopVolume();
      m_topName = m_top->getName();
    }
    m_top->AcceptNotRec(this);

    // Increment copy count for top volume since it won't be
    // positioned inside anything else
    LogVol* ourVol = findLogVol(m_top->getName());
    ++(ourVol->nCopy);
  }

  void SolidStats::visitEnsemble(detModel::Ensemble* ens) {

    std::string matName = "Vacuum";
    detModel::BoundingBox* bBox = ens->getBBox();
    if (detModel::Composition* comp = 
        dynamic_cast<detModel::Composition*>(ens)) {
      detModel::Shape* env = comp->getEnvelope();
      env->AcceptNotRec(this);
      // Also need to set field in logVol saying this is an envelope
      LogVol* envLog = findLogVol(env->getName());
      envLog->envelope = true;

      matName = env->getMaterial();
      bBox = env->getBBox();
    }
    double convexVol = bBox->getXDim() * bBox->getYDim() * bBox->getZDim();
    double cuVol = convexVol;
    LogVol* ourLogVol = findLogVol(ens->getName());

    
    //    LogVolMapIt it;
    //if ((it  = m_logVols.find(ens->getName())) != m_logVols.end() ) {
    if (ourLogVol == 0) {
      ourLogVol = new LogVol();
      ourLogVol->name = ens->getName();
      ourLogVol->matName = matName;
      ourLogVol->convexVol = convexVol;
      ourLogVol->vol = 0;
      ourLogVol->nCopy = 0;
      ourLogVol->envelope = false;

      m_logVols[ens->getName()] = ourLogVol;
    }
    else {
      assert(ourLogVol->convexVol == convexVol);  // probably not necessary
      assert(ourLogVol->envelope == false);
    }

    std::vector<detModel::Position*>::iterator posIt;
    std::vector<detModel::Position*> positions = ens->getPositions();

    for (posIt = positions.begin(); posIt != positions.end(); posIt++) {
      detModel::Position *pos = *posIt;

      // Causes volume associated with positioning elt. to be registered
      // if it isn't already  and updates nCopy 
      pos->AcceptNotRec(this);

      detModel::Volume* ourVol = pos->getVolume();
      // If it was a Choice, resolve it first
      if (detModel::Choice* ch = dynamic_cast<detModel::Choice*>(ourVol)) {
        ourVol = ch->getVolumeByMode(m_choiceMode);
      }

      LogVol* logVol = findLogVol(ourVol->getName());
      //      LogVol* logVol = findLogVol(pos->getVolumeRef()); 
      assert(logVol != 0);


      unsigned nCopy = 1;
      if (detModel::AxisMPos* mpos = 
          dynamic_cast<detModel::AxisMPos*>(pos) ) {
        nCopy = mpos->getNcopy();
      }
      cuVol -= nCopy * logVol->vol;
    }
    assert(cuVol > 0.0);
    ourLogVol->vol = cuVol;
  }

  void SolidStats::visitBox(detModel::Box* box) {
    // Get its volume
    double cuVol = box->getX() * box->getY() * box->getZ();
    registerShape(box, cuVol);

  }

  void SolidStats::visitTube(detModel::Tube* tube) {
    if (PI <= 0.0) {
      PI = asin(-1);
    }
    double cuVol = PI * tube->getZ() * 
      (tube->getRout()*tube->getRout() -
       tube->getRin()*tube->getRin());
    registerShape(tube, cuVol);
  }

  void SolidStats::visitPosXYZ(detModel::PosXYZ* pos) {
    // First visit our volume   
    detModel::Volume* ourVol = pos->getVolume();

    // If it was a Choice, resolve it first
    if (detModel::Choice* ch = dynamic_cast<detModel::Choice*>(ourVol)) {
      ourVol = ch->getVolumeByMode(m_choiceMode);
    }
    ourVol->AcceptNotRec(this);

    // Increment its copy count.
    LogVol* logVol = findLogVol(ourVol->getName());
    assert(logVol != 0);
    logVol->nCopy++;
  }

  void SolidStats::visitAxisMPos(detModel::AxisMPos* axisPos) {
    // First visit our volume   
    detModel::Volume* ourVol = axisPos->getVolume();

    // If it was a Choice, resolve it first.
    if (detModel::Choice* ch = dynamic_cast<detModel::Choice*>(ourVol)) {
      ourVol = ch->getVolumeByMode(m_choiceMode);
    }
    ourVol->AcceptNotRec(this);

    // Increment its copy count.
    LogVol* logVol = findLogVol(ourVol->getName());
    assert(logVol != 0);
    logVol->nCopy += axisPos->getNcopy();
  }

  void SolidStats::visitIdField(detModel::IdField* field) {
    // Until we figure out what if anything to do with this...
    return;  
  }

  void SolidStats::visitSeg(detModel::Seg* seg) {
    return;
  }

  bool SolidStats::registerShape(detModel::Shape* shape, double cuVol) {
    LogVol* logVol;

    logVol = findLogVol(shape->getName());

    if (!logVol) {
      logVol = new LogVol();
      logVol->name = shape->getName();
      logVol->matName =  shape->getMaterial();
      logVol->vol = cuVol;
      detModel::BoundingBox *bBox = shape->getBBox();
      logVol->convexVol = bBox->getXDim() * bBox->getYDim() * bBox->getZDim();
      logVol->nCopy = 0;
      logVol->envelope = false;

      m_logVols[shape->getName()] = logVol;
      return false;
    }
    else {

      assert(logVol->vol == cuVol);
      return true;
    }
  }

  SolidStats::LogVol *SolidStats::findLogVol(std::string name) {
    LogVolMapIt it;

    if (  (it  = m_logVols.find(name)) == m_logVols.end() ) return 0;

    return it->second;
  }

  void SolidStats::initMaterials(detModel::Gdd* gdd) {
    std::map<std::string, detModel::Material*>  gddMats = 
                    gdd->getMaterials()->getMaterials();
    typedef std::map<std::string, detModel::Material*>::iterator MaterialsIt;

    for (MaterialsIt gddMatIt = gddMats.begin(); gddMatIt != gddMats.end();
         gddMatIt++) {
      detModel::Material* detMat = gddMatIt->second;

      //   fill in name, density, set accumulators to zero
      Material* mat = new Material();
      mat->name = detMat->getName();
      mat->density = detMat->getDensity();
      mat->logCount = mat->physCount = 0;
      mat->cuVol = 0;

      m_mats[detMat->getName()] = mat;
    }

  }

  /*
  void Overlaps::printChildren(detModel::Ensemble *ens) {
    std::vector<detModel::Position*> positions = ens->getPositions();
    
    for (unsigned int iPos = 0; iPos < positions.size(); iPos++) {
      (*m_out) << "Child volume #" << iPos << " is " <<
        positions[iPos]->getVolumeRef() << std::endl;
    }
    (*m_out) << std::endl;
    return;
  }
  */
}
