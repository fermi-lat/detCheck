// $Header: /nfs/slac/g/glast/ground/cvs/detCheck/detCheck/SolidStats.h,v 1.6 2002/01/19 00:36:22 jrb Exp $

#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include "detModel/Management/SectionsVisitor.h"

#include "detModel/Gdd.h"

/* May need some of the following. */
#include "detModel/Sections/Composition.h"
#include "detModel/Sections/Stack.h"
#include "detModel/Sections/AxisMPos.h"

namespace detCheck {
  /**  This class accumulates information on all the (physical)
       volumes in the model by material and writes it to the
       specified output.  If the \a verbose flag is set to "true"
       there will also be some per-logical-volume output.

       All the visit.. methods below must be implemented to 
       satisfy the SectionsVisitor interface.

       @author J. Bogart
  */
  class SolidStats : public detModel::SectionsVisitor {
  public:

    SolidStats(std::string topVolume="");
    virtual ~SolidStats();

    /** Produce summary report on constitution of the geometry
        @arg  @b ofile    Output file for report. If empty, a simple
                          table will be output to standard output.
                          If non-empty, the output will be in html.
    */
    void report(std::string ofile);

    /** Provide diagnostic output
        @arg @b filename  File to which output is written, or standard
                          output if string is empty.

        Default is to provide no diagnositic output
    */
    void setDiagnostic(std::string filename);

    // The following are all part of the SectionsVisitor interface
    virtual void visitGdd(detModel::Gdd* gdd);
    virtual void visitSection(detModel::Section* sect);
    virtual void visitEnsemble(detModel::Ensemble* ens);
    virtual void visitBox(detModel::Box* box);
    virtual void visitTube(detModel::Tube* tube);
    virtual void visitSphere(detModel::Sphere* tube);
    virtual void visitPosXYZ(detModel::PosXYZ* pos);
    virtual void visitAxisMPos(detModel::AxisMPos* axisPos);
    virtual void visitIdField(detModel::IdField* field);
    virtual void visitSeg(detModel::Seg* seg);


  private:

    detModel::Gdd* m_gdd;
    /// Section top volume (default) or other requested volume to act as top
    detModel::Volume* m_top;
    /// Name of volume to be treated as top
    std::string   m_topName;
    std::ostream* m_out;
    std::ostream* m_diag;
    bool          m_allocDiag;
    bool          m_verbose;
    static double PI;   // have to initialize
    /// Are we processing an envelope? 
    bool envelopeNow;

    /// Keep track of nCopy multipliers as we recurse through geometry
    std::vector<unsigned> nCopies;

    /// Keep track of accumulated embedded volumes at each level
    std::vector<double> embedCuVol;

    /// Internal data structure to keep track of "logical volume"
    typedef struct s_logVol {
      std::string name;
      std::string matName;

      // Note the following is a bit of a cheat.  In most cases 
      // convexVol = vol.  However if a simple solid is used as an
      // envelope for a composition,  is possible for one (or more) 
      // primitive solid to be inside it, in which case 
      //   vol(outer) = convexVol(outer) - sum[vol(inner)]
      // Worse, it is possible for one physical copy of a logical solid
      // to be used as envelope with embedded inner solids 
      // while another copy of the same logical solid doesn't; i.e.,
      // "volume" is really only a property of physical solids,
      // not logicals. (Maybe only Vacuum-material solids should
      // be allowed as envelopes?)
      //
      // So far this never happens for GLAST.  If it did, the "vol"
      // field below would not have a well-defined value.
      double      convexVol; /**<  in mm**3 */
      double      vol;       /**< in mm**3 */
      unsigned    nCopy;     /**< # of physical copies of this volume */
      bool        envelope;  /**< true if used as envelope for composition */
    }   LogVol;

    /// Internal data structure to keep track of a single material
    typedef struct s_material {
      std::string name;
      double      density;
      /// logical solids made of this material 
      std::vector<LogVol*> logVols; 
      /// # of distinct logical solids made of us 
      unsigned    logCount; 
      unsigned    physCount; /**< total # of phys solids made of the material*/
      double      cuVol;       /**< in mm**3 */
    }   Material;
      
    typedef std::map<std::string, Material*> MatMap;
    typedef MatMap::iterator MatMapIt;
    
    MatMap m_mats;

    typedef std::map<std::string, LogVol*> LogVolMap;
    typedef LogVolMap::iterator  LogVolMapIt;

    LogVolMap m_logVols;


    std::string m_choiceMode;

    /// Internal routine to update cubic volume accumulator @c embedCuVol.
    void accumulateVolume(double cuVol);

    /// Internal routine to compute total copy count from stacked values
    /// in @c copyCount
    unsigned getCopyCount();

    unsigned getLastCount();


    /// Utility to find our data structure for a logical volume, given 
    /// its name.
    LogVol *findLogVol(std::string name);

    /// Utility routine to set up materials map
    void initMaterials();

    /// Utility used by @b report to output xml file version, etc. 
    void outputVersion();

    /** Add a solid volume to solids map if it's not already there,
        initialize or update entries, and update volume accumulator.
        \return pointer to logVol struct
        \return true or false depending on whether shape is already registered.
       If shape has already been registered with a different volume,
       do assert; this indicates an error in the xml input description. */ 
    LogVol*  registerShape(detModel::Shape* shape, double cuVol);

  };  //end SolidStats class
}  // end namespace detCheck

