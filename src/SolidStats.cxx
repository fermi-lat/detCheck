// $Header: /nfs/slac/g/glast/ground/cvs/detCheck/src/SolidStats.cxx,v 1.17 2007/09/25 21:58:31 lsrea Exp $

#include <cmath>
#include <cassert>
#include <ctime>
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
#include "detModel/Sections/Trap.h"
#include "detModel/Sections/Sphere.h"
#include "detModel/Sections/Choice.h"
#include "detModel/Sections/PosXYZ.h"
#include "detModel/Sections/AxisMPos.h"
#include "detModel/Sections/Section.h"
#include "detModel/Materials/MatCollection.h"

#ifdef WIN32
  typedef __int64 int64;
#else
  typedef long long int64;
#endif

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
      delete matIt->second;
    }

  }


  void SolidStats::report(std::string outfileName) {
    bool html = false;
    bool allocStream = false;
    double TO_CU_CM = 0.001;

    if (outfileName.size() == 0) {
      m_out = &std::cout; 
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
      (*m_out) << "<th># Phys. Vol.</th><th>Volume (cm3)</th>";
      (*m_out) << "<th>Mass (g)</th>" ;
      (*m_out) << "<th>Density (g/cm3)</th></tr>"
               << std::endl;
    }
    else {  // just title
      (*m_out) << "Materials Summary" << std::endl;
    }

    // Accumulators for Totals row
    unsigned logCount = 0;
    unsigned physCount = 0;
    double   volTotal = 0.0;
    int64 massTotal = 0;

    // Always output summary per-material information 
    for (MatMapIt mapIt = m_mats.begin(); mapIt != m_mats.end(); ++mapIt) {
      Material* mat = mapIt->second;

      if (!mat->logCount) continue;
      double mass = mat->cuVol * TO_CU_CM * mat->density;
      int64 intMass = (int64) mass;
      int64 intVol = (int64) (mat->cuVol * TO_CU_CM);
      
      if ((mapIt->first).compare("Vacuum")) {
        logCount += mat->logCount;
        physCount += mat->physCount;
        volTotal += mat->cuVol;
        massTotal += intMass;
      }

      if (html) {
        (*m_out) << "<tr><td>" << mapIt->first << "</td><td align='right'>" 
                 << mat->logCount << "</td><td align='right'>" 
                 << mat->physCount << "</td><td align='right'>" 
                 << intVol << "</td><td align='right'>"
                 << intMass << "</td><td align='right'>"
                 << mat->density  
                 << "</td></tr>" << std::endl;
      }
      else {
        (*m_out) << " " << mapIt->first <<  ":  "
                 << "  #Log = " << mat->logCount
                 << "  #Phys = " << mat->physCount 
                 << "  Total volume = " << intVol << " cu cm" 
                 << "  Mass = " <<  intMass << " g" 
                 << "  Density = " << mat->density << " g/cm3"
                 << std::endl;
      }
    }

    // Extra output for html
    if (html) { // Put in totals line, close old table. Then start new one
      int64 intVolTotal = (int64) (volTotal * TO_CU_CM);
      
      (*m_out) << "<tr><td><b>Totals (no vac)</b></td><td align='right'><b>" 
               << logCount << "</b></td><td align='right'><b>" << physCount 
               << "</b></td><td align='right'><b>" << intVolTotal 
               << "</b></td><td align='right'><b>" 
               << massTotal << "</b></td><td></td>";
 
      (*m_out) << "</table><h2 align='center'>Summary by Logical Volume</h2>"
               << "<table cellpadding='3' border='1'>" 
               << "<tr bgcolor='#c0ffc0' align='left'>" << std::endl;
      (*m_out) << "<th>Name</th><th>Volume (cm3)</th><th>Convex vol</th>"
               << "<th> # Phys.</th>"
               << "<th>Total volume</th><th>Material</th>"
               << "<th>Mass (g)</th>" << std::endl;
    
      for (LogVolMapIt logIt = m_logVols.begin(); logIt != m_logVols.end();
           ++logIt) {
        LogVol* log = logIt->second;
        int64 intVol = (int64) (log->vol * TO_CU_CM);
        int64 intConvexVol = (int64) (log->convexVol * TO_CU_CM);
        int64 intTotalVol = (int64) (log->vol * log->nCopy * TO_CU_CM);
        MatMapIt it = m_mats.find(log->matName);
        double density = it->second->density;
        int64 intMass = (int64) (density * log->vol * log->nCopy * TO_CU_CM);

        (*m_out) << "<tr><td>" << log->name;
        if (log->envelope) (*m_out) << "(E)";
        (*m_out) << "</td><td align='right'>" << intVol 
                 << "</td><td align='right'>" << intConvexVol 
                 << "</td><td align='right'>" 
                 << log->nCopy << "</td><td align='right'>" << intTotalVol
                 << "</td><td>" << log->matName << "</td><td align='right'>"
                 <<  intMass << "</td></tr>"
                 << std::endl;
      }
      (*m_out) << "</table>" << std::endl;
      outputVersion();        
      (*m_out) << "</body></html>" << std::endl;
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

    m_gdd = gdd;
    initMaterials();
    
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
      double one = 1.0;
      PI = 2 * (asin(one));
    }
    double rOut = tube->getRout();
    double rIn = tube->getRin();
    double cuVol = PI * tube->getZ() * (rOut * rOut - rIn * rIn);

    registerShape(tube, cuVol);
  }

  void SolidStats::visitTrap(detModel::Trap* trap) {
    double cuVol = trap->getY() * trap->getZ() * 
      0.5 * (trap->getX1() + trap->getX2());
    registerShape(trap, cuVol);
  }

  void SolidStats::visitSphere(detModel::Sphere* sphere) {
    if (PI <= 0.0) {
      double one = 1.0;
      PI = 2 * (asin(one));
    }
    //\todo This formula is only correct if theta has full range.
    double rOut = sphere->getRout();
    double rIn = sphere->getRin();
    double cuVol = PI * (4/3.0) * (rOut * rOut *rOut - rIn * rIn * rIn);
    double phiRange = sphere->getPhiMax() - sphere->getPhiMin();
    cuVol *= (phiRange / (2 * PI));
    registerShape(sphere, cuVol);
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

  void SolidStats::visitIdField(detModel::IdField* ) {
    // Until we figure out what if anything to do with this...
    return;  
  }

  void SolidStats::visitSeg(detModel::Seg* ) {
    return;
  }

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

  void SolidStats::initMaterials() {
    std::map<std::string, detModel::Material*>  gddMats = 
                    m_gdd->getMaterials()->getMaterials();
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

  void SolidStats::outputVersion() {
    unsigned secs = time(0);
    std::string cvsId = m_gdd->getCVSid();
    (*m_out) <<  "<hrule /><br />This page created " 
             <<  ctime((time_t*)(&secs))
             << "<br /> from XML input cvs version: " 
             << cvsId << std::endl
             << "<br /> file path: "
             << detModel::Manager::getPointer()->getNameFile() << std::endl;
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
