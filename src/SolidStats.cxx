// $Header: /nfs/slac/g/glast/ground/cvs/detCheck/src/SolidStats.cxx,v 1.6 2002/01/17 05:44:02 jrb Exp $

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
    m_gdd(0), m_topName(topVolume), m_out(0), m_diag(0), 
    m_allocDiag(false), m_verbose(false)
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

  void SolidStats::report(std::string outfileName) {
    bool html = false;
    bool allocStream = false;

    if (outfileName.size() == 0) {
      m_out = &std::cout;  // or out = new ostream(std::out);  ?
    }
    else {
      m_out = new std::ofstream(outfileName.c_str());
      allocStream = true;
      html = true;
    }

    // Here is where all the work goes to output the information saved
    // while visiting..
    if (html) {
      (*m_out) << 
        "<html><head><title>Materials Summary</title></head>" << std::endl;
      (*m_out) << "<body> <h2 align = 'center'>Materials Summary</h2>" 
               << std::endl;
      (*m_out) << "<table cellpadding='3' border='1'>" << std::endl;
      (*m_out) << "<tr bgcolor='#c0ffc0' align ='left'>";
      (*m_out) << "<th>Material Name</th><th># Log. Vol.</th>";
      (*m_out) << "<th># Phys. Vol.</th><th>Volume (cubic mm)</th></tr>" 
               << std::endl;
    }
    else {  // just title
      (*m_out) << "Materials Summary" << std::endl;
    }

    // Always output summary per-material information 
    for (MatMapIt mapIt = m_mats.begin(); mapIt != m_mats.end(); ++mapIt) {
      Material* mat = mapIt->second;

      if (!mat->logCount) continue;

      if (html) {
        (*m_out) << "<tr><td>" << mapIt->first << "</td><td align='right'>" 
                 << mat->logCount << "</td><td align='right'>" 
                 << mat->physCount << "</td><td>" 
                 << mat->cuVol << "</td></tr>" << std::endl;
      }
      else {
        (*m_out) << " " << mapIt->first <<  ":  "
                 << "  #Log = " << mat->logCount
                 << "  #Phys = " << mat->physCount 
                 << "  Total volume = " << mat->cuVol << " cu mm" << std::endl;
      }
    }

    // Extra output for html
    if (html) { // first close old table, then start new one
      (*m_out) << "</table><h2 align='center'>Summary by Logical Volume</h2>"
               << "<table cellpadding='3' border='1'>" 
               << "<tr bgcolor='#c0ffc0' align='left'>" << std::endl;
      (*m_out) << "<th>Name</th><th>Volume (cu mm)</th><th>Convex vol</th>"
               << "<th> # Phys.</th>"
               << "<th>Total volume</th><th>Material</th></tr>"
               << std::endl;
    
      for (LogVolMapIt logIt = m_logVols.begin(); logIt != m_logVols.end();
           ++logIt) {
        LogVol* log = logIt->second;
        (*m_out) << "<tr><td>" << log->name;
        if (log->envelope) (*m_out) << "(E)";
        (*m_out) << "</td><td>" << log->vol << "</td><td>" 
                 << log->convexVol << "</td><td align='right'>" 
                 << log->nCopy << "</td><td>" << (log->vol*log->nCopy)
                 << "</td><td>" << log->matName << "</td></tr>" 
                 << std::endl;
      }
        
      (*m_out) << "</table></body></html>" << std::endl;
    }
      

    if (allocStream) delete m_out;
    if (m_allocDiag) delete m_diag;
    return;
  }

  void SolidStats::setDiagnostic(std::string filename) {
    if (filename.size() == 0) {
      m_diag = &std::cout;
    }
    else {
      m_diag = new std::ofstream(filename.c_str());
      m_allocDiag = true;
    }
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

      // Don't include envelopes; their cubic volume has already
      // been handled by the associated composition.
      if (logVol->envelope) continue;

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
    // Initialize stacks and other state information
    nCopies.push_back(1);
    embedCuVol.push_back(0);
    envelopeNow = false;

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
    // *** I think this was a mistake
    //    LogVol* ourVol = findLogVol(m_top->getName());
    /////  ++(ourVol->nCopy);
  }

  void SolidStats::visitEnsemble(detModel::Ensemble* ens) {

    std::string matName = "Vacuum";
    detModel::BoundingBox* bBox = ens->getBBox();
    if (detModel::Composition* comp = 
        dynamic_cast<detModel::Composition*>(ens)) {
      detModel::Shape* env = comp->getEnvelope();
      envelopeNow = true;
      env->AcceptNotRec(this);
      envelopeNow = false;

      matName = env->getMaterial();
      bBox = env->getBBox();

      if (m_diag) {
        *m_diag << "Starting to process composition volume "
               << ens->getName() << std::endl;
      }
    }
    else {
      if (m_diag) {
        *m_diag << "Starting to process stack volume " 
               << ens->getName() << std::endl;
      }
    }
    double convexVol = bBox->getXDim() * bBox->getYDim() * bBox->getZDim();
    double cuVol = convexVol;
    LogVol* ourLogVol = findLogVol(ens->getName());

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

    // Update our copy count
    ourLogVol->nCopy += getCopyCount();

    std::vector<detModel::Position*>::iterator posIt;
    std::vector<detModel::Position*> positions = ens->getPositions();

    // Initialize our entry on the cubic volume accumulator stack
    embedCuVol.push_back(0);

    for (posIt = positions.begin(); posIt != positions.end(); posIt++) {
      detModel::Position *pos = *posIt;

      // Updates nCopy appropriately and causes associated logical
      // volume to be visited.
      pos->AcceptNotRec(this);
    }

    // Use cubic volume accumulator to compute our volume, properly
    // subtracting volumes of embedded things
    ourLogVol->vol = convexVol - embedCuVol[embedCuVol.size() - 1];
    embedCuVol.pop_back();

    // Finally, add our (convex) volume to parent volume's accumulator
    // entry
    accumulateVolume(convexVol);
    if (m_diag) {
      *m_diag << "finished processing ensemble " << ens->getName() 
              << std::endl;
      *m_diag << "   nCopy: " << ourLogVol->nCopy
              << "  convex volume: " << ourLogVol->convexVol
              << "   volume: " << ourLogVol->vol << std::endl;
    }
  }

  void SolidStats::visitBox(detModel::Box* box) {
    // Get its volume
    double cuVol = box->getX() * box->getY() * box->getZ();
    registerShape(box, cuVol);

  }

  void SolidStats::visitTube(detModel::Tube* tube) {
    if (PI <= 0.0) {
      PI = 2 * (asin(1));
    }
    double rOut = tube->getRout();
    double rIn = tube->getRin();
    double cuVol = PI * tube->getZ() * (rOut * rOut - rIn * rIn);

    registerShape(tube, cuVol);
  }

  void SolidStats::visitPosXYZ(detModel::PosXYZ* pos) {
    // First visit our volume   
    detModel::Volume* ourVol = pos->getVolume();

    // NOTE:  May not need this any more
    // If it was a Choice, resolve it first
    if (detModel::Choice* ch = dynamic_cast<detModel::Choice*>(ourVol)) {
      ourVol = ch->getVolumeByMode(m_choiceMode);
    }
    nCopies.push_back(1);       // PosXYZ by definition positions only 1 copy
    ourVol->AcceptNotRec(this);
    nCopies.pop_back();
  }

  void SolidStats::visitAxisMPos(detModel::AxisMPos* axisPos) {
    // First visit our volume   
    detModel::Volume* ourVol = axisPos->getVolume();

    // If it was a Choice, resolve it first.
    if (detModel::Choice* ch = dynamic_cast<detModel::Choice*>(ourVol)) {
      ourVol = ch->getVolumeByMode(m_choiceMode);
    }

    // Push copy count
    nCopies.push_back(axisPos->getNcopy());
    ourVol->AcceptNotRec(this);

    // Get rid of our copy count (last entry)
    nCopies.pop_back();
  }

  void SolidStats::visitIdField(detModel::IdField* field) {
    // Until we figure out what if anything to do with this...
    return;  
  }

  void SolidStats::visitSeg(detModel::Seg* seg) {
    return;
  }

  //  bool SolidStats::registerShape(detModel::Shape* shape, double cuVol) {
  SolidStats::LogVol* SolidStats::registerShape(detModel::Shape* shape, 
                                                double cuVol) {
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
      logVol->envelope = envelopeNow;

      m_logVols[shape->getName()] = logVol;
      //      return false;
    }
    else {

      assert(logVol->vol == cuVol);
      //      return true;
    }
    if (!logVol->envelope) logVol->nCopy += getCopyCount();

    if (m_diag) {
      *m_diag << "   finished processing shape " << shape->getName() 
              << std::endl;
      *m_diag << "      nCopy: " << logVol->nCopy
              << "   volume: " << logVol->vol << std::endl;
    }

    // Accumulator has only to do with our immediately enclosing volume,
    // so relevant copy count is just the top entry on the copy count stack
    // If we're an envelope we don't participate in this at all; our
    // associated composition takes care of it
    if (!logVol->envelope) accumulateVolume(cuVol * getLastCount());
    return logVol;
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

  unsigned SolidStats::getCopyCount() {
    unsigned result = 1;

    for (unsigned i = 0; i < nCopies.size(); i++) {
      result *= nCopies[i];
    }
    return result;
  }

  unsigned SolidStats::getLastCount() {
    return nCopies[nCopies.size() - 1];
  }


  void SolidStats::accumulateVolume(double cuVol) {
    double accum = embedCuVol[embedCuVol.size() - 1] + cuVol;
    embedCuVol.pop_back();
    embedCuVol.push_back(accum);
  }
}
