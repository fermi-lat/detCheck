#ifndef HEPREPSECTIONSVISITOR_H
#define HEPREPSECTIONSVISITOR_H
#include "detModel/Management/SectionsVisitor.h"
#include "detModel/Utilities/Vector.h"
#include <fstream>
#include <vector>
#include <map>
#include "idents/VolumeIdentifier.h"

#include "CLHEP/Geometry/Transform3D.h"

namespace xmlUtil{
  class IdDict;
}

namespace detModel{

  class Gdd;
  class Color;
  /*
   * This is a concrete implementation of a sectionsVisitor that produces
   * a DAWN file with the geometry. 
   */
  class HepRepSectionsVisitor : public SectionsVisitor {
    
  public:
    
    HepRepSectionsVisitor(std::string);
    virtual ~HepRepSectionsVisitor();
    
    /**
     * This is the visitor for the SectionsContainer 
     */
    virtual void visitGdd(Gdd*);
    
    /**
     * This is the visitor for the Section 
     */
    virtual void visitSection(Section*);
    
    /**
     * This is the visitor for the GDDcomposition 
     */
    virtual void visitEnsemble(Ensemble*);
    
    /**
     * This is the visitor for the GDDbox 
     */
    virtual void visitBox(Box*);

    /**
     * This is the visitor for the Tube 
     */
    virtual void visitTube(Tube*);

    /**
     * This is the visitor for the Trapezoidal solid
     */
    virtual void visitTrap(Trap*);

    /**
     * This is the visitor for the Sphere 
     */
    virtual void visitSphere(Sphere*){};
    
    /**
     * This is the visitor for the GDDposXYZ 
     */
    virtual void visitPosXYZ(PosXYZ*);
    
    /**
     * This is the visitor for the GDDaxisMPos 
     */
    virtual void visitAxisMPos(AxisMPos*);
    
    /**
     * This is the visitor for the GDDaxisMPos 
     */
    virtual void visitIdField(IdField*);
    
    /**
     * This is the visitor for the GDDseg 
     */
    virtual void visitSeg(Seg*);

    /**
     * Set the visiting mode
     */
    void setMode(std::string m){m_mode = m;};

    /**
     * Get the visiting mode
     */
    std::string getMode(){return m_mode;};

    /**
     * Set output file name
     */
    void setOutputFile(std::string name){m_outputFile = name;};

    void setIDPrefix(idents::VolumeIdentifier pr){m_idPrefix = pr;};

    void setPrefixTransform(HepGeom::Transform3D t){m_prefixTransform = t;};

    void setIdDictionary(xmlUtil::IdDict* id){m_idDictionary = id;};

    /// Return the full qualified name of the actual instance
    std::string getFullName();    
  private:
    std::string actualVolume;
    std::ofstream out;

    std::string m_mode;
    std::string m_outputFile;

    std::vector<HepGeom::Transform3D> m_actualTransform;
    HepGeom::Transform3D m_prefixTransform;
    std::vector<std::string> m_types;

    /// This is used to build the full qualified name
    std::vector<std::string> m_actualName;
    
    /// This is the identifiers used during the m_volMap building
    idents::VolumeIdentifier m_actualID;    
    idents::VolumeIdentifier m_idPrefix;    
      
    xmlUtil::IdDict* m_idDictionary;
    /// This map holds the colors for the material
    std::map <std::string, Color*> colorsMap;
    Gdd* m_gdd;
  };
}
#endif //HEPREPSECTIONSVISITOR_H







