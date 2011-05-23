#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "idents/VolumeIdentifier.h"

#include "CLHEP/Vector/Rotation.h"
#include "CLHEP/Geometry/Point3D.h"
#include "CLHEP/Vector/ThreeVector.h"
#include "CLHEP/Geometry/Transform3D.h"

#include "detModel/Materials/MatCollection.h"
#include "detModel/Management/Manager.h"
#include "detModel/Sections/Section.h"
#include "detModel/Gdd.h"
#include "detModel/Sections/Box.h"
#include "detModel/Sections/Tube.h"
#include "detModel/Sections/Trap.h"
#include "detModel/Sections/Composition.h"
#include "detModel/Sections/PosXYZ.h"
#include "detModel/Sections/Stack.h"
#include "detModel/Sections/AxisMPos.h"
#include "detModel/Sections/IdField.h"
#include "detModel/Sections/Position.h"
#include "detModel/Utilities/ColorCreator.h"
#include "detModel/Utilities/Color.h"
#include "detModel/Utilities/Vector.h"

#include "xmlUtil/id/IdDict.h"
#include "xmlUtil/id/Identifier.h"

#include "HepRepSectionsVisitor.h"

namespace detModel{

HepRepSectionsVisitor::HepRepSectionsVisitor(std::string nvol)
{
  //  typedef map<std::string,Color*>M1;

  setRecursive(0);
  actualVolume = nvol;

  //  Manager* manager = Manager::getPointer();
  //  Gdd* g = manager->getGdd();

  HepGeom::Transform3D start;
  m_actualTransform.push_back(start);
  
};

HepRepSectionsVisitor::~HepRepSectionsVisitor()
{
  out.close();
}


void HepRepSectionsVisitor::visitGdd(Gdd* gdd)
{
  typedef std::vector<Section*>sec;
  std::vector <Section*>::iterator i;

  if (m_outputFile.size() == 0) {
    if (actualVolume.size() == 0 ) {
      m_outputFile = "sections.heprep";
    }
    else m_outputFile = actualVolume + ".heprep";
  }
  out.open(m_outputFile.c_str());

  m_actualID = m_idPrefix;
  m_gdd = gdd;
  colorsMap = gdd->getMaterials()->getMaterialColors();
  sec s = gdd->getSections();
  for(i=s.begin(); i!=s.end();i++)
    (*i)->AcceptNotRec(this);


}
  
void  HepRepSectionsVisitor::visitSection(Section* section)
{
  Volume* vol=0;
  std::vector <Volume*>::iterator v;
  
  if (actualVolume == "")
    {
      std::vector<Volume*> volumes = section->getVolumes();
      
      for(v=volumes.begin(); v!=volumes.end(); v++){
	if(Ensemble* ens = dynamic_cast<Ensemble*>(*v))
	  {
	    if (ens == section->getTopVolume())
	      vol = ens;
	  }
      }
    }
  else
      {
      //      Manager* manager = Manager::getPointer();
      //      if (manager->getGdd()->getVolumeByName(actualVolume))
      if (m_gdd->getVolumeByName(actualVolume))
	vol = m_gdd->getVolumeByName(actualVolume);
      else
	{
	  std::cout << "No such volume" << std::endl;
	  exit(0);
	}
      }

  /// Here we start the visit of the hierarchy.
    // First we build the TypeTree
   out << "<heprep>" << std::endl;
    out << "<typetree name=\"detModel\" version=\"1.0\">" << std::endl;
    setMode("type");
    vol->AcceptNotRec(this);
    out << "</typetree>" << std::endl;
    
    /// Now for the Instance Tree
    out << "<instancetree name=\"detModel\" version=\"1.0\" reqtypetree=\"\" typetreename=\"detModel\" typetreeversion=\"1.0\"> " << std::endl;
    setMode("instance");
    vol->AcceptNotRec(this);
    out << "</instancetree>" << std::endl;
    
    // Lets close the HepRep
    out << "</heprep>" << std::endl;
      }

void  HepRepSectionsVisitor::visitEnsemble(Ensemble* ensemble)
{
  m_actualName.push_back(ensemble->getName());

  std::vector <Position*>::iterator i;
  typedef std::vector<Position*> pos;
  
  if (m_mode == "type")
    {
      out << "<type name =\"" <<  ensemble->getName() << "\" >" << std::endl;
      if (m_types.size() == 0)
        {
          CLHEP::Hep3Vector t = m_prefixTransform.getTranslation();

          out << "<attvalue name=\"PosTopVolume\" value =\"(" 
              <<  t.x() << ", " 
              <<  t.y() << ", " 
              <<  t.z() << ")\" showlabel =\"\"/>" << std::endl;      
        }
    }
  else
    {
      out << "<instance type=\"" << getFullName()  << "\">" << std::endl;
      out << "<attvalue name=\"ID\" value =\""<< m_actualID.name() << "\" showlabel =\"\"/>" << std::endl;
      
      xmlUtil::Identifier identifier;
      
      for(int ii=0;ii<m_actualID.size();ii++)
          identifier.append(m_actualID[ii]);

      out << "<attvalue name=\"IDname\" value =\""<< m_idDictionary->getNameSeqString(identifier) << "\" showlabel =\"\"/>" << std::endl;      

      HepGeom::Transform3D atr = m_actualTransform.back();
      CLHEP::Hep3Vector t = atr.getTranslation();

      out << "<attvalue name=\"Pos\" value =\"(" 
          <<  t.x() << ", " 
          <<  t.y() << ", " 
          <<  t.z() << ")\" showlabel =\"\"/>" << std::endl;      
    }
  
  /// Here the positioned volumes are visited
  pos p = ensemble->getPositions();
  std::vector<std::string> types;
  for(i=p.begin(); i!=p.end();i++)
    if(m_mode == "instance")
    {
      (*i)->AcceptNotRec(this);
    }
    else
      {
          std::string name = (*i)->getVolume()->getName();
          std::vector<std::string>::iterator j;
          j = std::find(types.begin(), types.end(),name);
          if (j ==types.end())
              {
                  (*i)->AcceptNotRec(this);          
                  types.push_back(name);   
              }
      }

  if (m_mode == "type")
    {
      out << "</type>" << std::endl;
    }
  else
    {
      out << "</instance>" << std::endl;
    }

    m_actualName.pop_back();
  
  /*      
  /// Here the envelope is visited if the ensamble is a composition
  if (Composition* comp = dynamic_cast<Composition*>(ensemble))
    {
      out << "/ForceWireframe 1" << std::endl;
      comp->getEnvelope()->AcceptNotRec(this);
     out << "/ForceWireframe 0" << std::endl;
    }
  */
}


void  HepRepSectionsVisitor::visitBox(Box* box)
{
  m_actualName.push_back(box->getName());

  typedef std::map<std::string, Color*> M;
  M::const_iterator j; 

  j = colorsMap.find(box->getMaterial());
  if (j == colorsMap.end()) return;

  if (m_mode == "type")
    {
      out << "<type name =\"" <<  box->getName() << "\" >" << std::endl;
      out << "<attvalue name=\"DrawAs\" value =\"Prism\" showlabel =\"\"/>" << std::endl;
      out << "<attvalue name=\"Material\" value =\""<< box->getMaterial() << "\" showlabel =\"\"/>" << std::endl;
      out << "<attvalue name=\"Dim\" value =\"(" 
          <<  box->getX() << ", " 
          <<  box->getY() << ", " 
          <<  box->getZ() << ")\" showlabel =\"\"/>" << std::endl;

      out << "<attvalue name=\"Color\" value =\"" <<
        j->second->getRed() << "," <<
        j->second->getGreen() << "," <<
        j->second->getBlue() << "\" showlabel =\"\"/>" << std::endl;      
      
      out << "<attvalue name=\"Sensitive\" value =\""<< box->getSensitive() << "\" showlabel =\"\"/>" << std::endl;
      out << "</type>" << std::endl;
      m_types.push_back(box->getName());
    }
  else
    {
      // Here we build the full qualified Name of the Type
      out << "<instance type=\"" << getFullName()  << "\">" << std::endl;
      out << "<attvalue name=\"ID\" value =\""<< m_actualID.name() << "\" showlabel =\"\"/>" << std::endl;
      xmlUtil::Identifier identifier;
      for(int ii=0;ii<m_actualID.size();ii++)
          identifier.append(m_actualID[ii]);

      out << "<attvalue name=\"IDname\" value =\""<< m_idDictionary->getNameSeqString(identifier) << "\" showlabel =\"\"/>" << std::endl;      
      HepGeom::Point3D<double> v(0,0,0);
      HepGeom::Point3D<double> v1;
      double dx = box->getX()/2;
      double dy = box->getY()/2;
      double dz = box->getZ()/2;
      HepGeom::Transform3D atr = m_actualTransform.back();
      out << "<attvalue name=\"Pos\" value =\"(" 
          <<  (atr*v).x() << ", " 
          <<  (atr*v).y() << ", " 
          <<  (atr*v).z() << ")\" showlabel =\"\"/>" << std::endl;

      v.setX(dx); v.setY(dy); v.setZ(dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;
      v.setX(-dx); v.setY(dy); v.setZ(dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl; 
     v.setX(-dx); v.setY(-dy); v.setZ(dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;
      v.setX(dx); v.setY(-dy); v.setZ(dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;

      v.setX(dx); v.setY(dy); v.setZ(-dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() << "\" z = \"" 
          << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;
      v.setX(-dx); v.setY(dy); v.setZ(-dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() << "\" z = \"" 
          << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;
      v.setX(-dx); v.setY(-dy); v.setZ(-dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() << "\" z = \"" 
          << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;
      v.setX(dx); v.setY(-dy); v.setZ(-dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() << "\" z = \"" 
          << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;

      out << "</instance>" << std::endl;
   }

      m_actualName.pop_back();


  /*  
  out << "/ColorRGB " << j->second->getRed() << " " 
      <<  j->second->getGreen() << " " 
      <<  j->second->getBlue()  << std::endl;
    

  out << "/Box " <<
    box->getX()/2 << " " << box->getY()/2 << " " << box->getZ()/2 << " " << std::endl;

  out << "/ForceWireframe 0" << std::endl;

  */
}

void  HepRepSectionsVisitor::visitTrap(Trap* trap)
{
  m_actualName.push_back(trap->getName());

  typedef std::map<std::string, Color*> M;
  M::const_iterator j; 

  j = colorsMap.find(trap->getMaterial());
  if (j == colorsMap.end()) return;
  double x = (trap->getX1() + trap->getX2())/2;
  double y = trap->getY();
  double z = trap->getZ();

  //  x = 0.5 * (trap->getX1() + trap->getX2());
  if (m_mode == "type")
    {
      out << "<type name =\"" <<  trap->getName() << "\" >" << std::endl;
      out << "<attvalue name=\"DrawAs\" value =\"Prism\" showlabel =\"\"/>" 
          << std::endl;
      out << "<attvalue name=\"Material\" value =\""<< trap->getMaterial() 
          << "\" showlabel =\"\"/>" << std::endl;
      out << "<attvalue name=\"Dim\" value =\"(" 
          <<  x << ", " 
          <<  y << ", " 
          <<  z << ")\" showlabel =\"\"/>" << std::endl;

      out << "<attvalue name=\"Color\" value =\"" <<
        j->second->getRed() << "," <<
        j->second->getGreen() << "," <<
        j->second->getBlue() << "\" showlabel =\"\"/>" << std::endl;      
      
      out << "<attvalue name=\"Sensitive\" value =\""<< trap->getSensitive() 
          << "\" showlabel =\"\"/>" << std::endl;
      out << "</type>" << std::endl;
      m_types.push_back(trap->getName());
    }
  else
    {
      // Here we build the full qualified Name of the Type
      out << "<instance type=\"" << getFullName()  << "\">" << std::endl;
      out << "<attvalue name=\"ID\" value =\""<< m_actualID.name() 
          << "\" showlabel =\"\"/>" << std::endl;
      xmlUtil::Identifier identifier;
      for(int ii=0;ii<m_actualID.size();ii++)
        identifier.append(m_actualID[ii]);

      out << "<attvalue name=\"IDname\" value =\""
          << m_idDictionary->getNameSeqString(identifier) 
          << "\" showlabel =\"\"/>" << std::endl;      
      // (x,y) components of vertices in local coords are
      //   ( halfTop + halfDiff, y/2),
      //   (-halfTop + halfDiff, y/2)
      //    (-halfBot - halfDiff, -y/2)
      //   (halfBot - halfDiff, -y/2),
      HepGeom::Point3D<double> v(0,0,0);
      HepGeom::Point3D<double> v1;

      double halfBotX = trap->getX1()/2;
      double halfTopX = trap->getX2()/2;
      double halfDiff = trap->getXDiff()/2;
      double dy = y/2;
      double dz = z/2;
      HepGeom::Transform3D atr = m_actualTransform.back();
      out << "<attvalue name=\"Pos\" value =\"(" 
          <<  (atr*v).x() << ", " 
          <<  (atr*v).y() << ", " 
          <<  (atr*v).z() << ")\" showlabel =\"\"/>" << std::endl;

      v.setX(halfTopX+halfDiff); v.setY(dy); v.setZ(dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
          << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;
      v.setX(-halfTopX+halfDiff); v.setY(dy); v.setZ(dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
          << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl; 
     v.setX(-halfBotX-halfDiff); v.setY(-dy); v.setZ(dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
          << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;
      v.setX(halfBotX-halfDiff); v.setY(-dy); v.setZ(dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
          << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;

      v.setX(halfTopX+halfDiff); v.setY(dy); v.setZ(-dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
          << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;
      v.setX(-halfTopX+halfDiff); v.setY(dy); v.setZ(-dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
          << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl; 
     v.setX(-halfBotX-halfDiff); v.setY(-dy); v.setZ(-dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
          << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;
      v.setX(halfBotX-halfDiff); v.setY(-dy); v.setZ(-dz); v1 = (atr*v);
      out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
          << "\" z = \"" << v1.z() << "\">" << std::endl;
      out << "</point>" << std::endl;

      out << "</instance>" << std::endl;
   }
  m_actualName.pop_back();
}


void  HepRepSectionsVisitor::visitTube(Tube* tube)
{
  typedef HepGeom::Point3D<double> HepPoint3D;
  m_actualName.push_back(tube->getName());

  typedef std::map<std::string, Color*> M;
  M::const_iterator j; 

  j = colorsMap.find(tube->getMaterial());
  if (j == colorsMap.end()) return;

  if (m_mode == "type")
    {
      out << "<type name =\"" <<  tube->getName() << "\" >" << std::endl;
      out << "<attvalue name=\"DrawAs\" value =\"Prism\" showlabel =\"\"/>" 
          << std::endl;
      out << "<attvalue name=\"Material\" value =\""<< tube->getMaterial() 
          << "\" showlabel =\"\"/>" << std::endl;
      out << "<attvalue name=\"Dim\" value =\"(" 
          <<  tube->getZ() << ", " 
          <<  tube->getRin() << ", " 
          <<  tube->getRout() << ")\" showlabel =\"\"/>" << std::endl;

      out << "<attvalue name=\"Color\" value =\"" <<
        j->second->getRed() << "," <<
        j->second->getGreen() << "," <<
        j->second->getBlue() << "\" showlabel =\"\"/>" << std::endl;      
      
      out << "<attvalue name=\"Sensitive\" value =\""<< tube->getSensitive() 
          << "\" showlabel =\"\"/>" << std::endl;
      out << "</type>" << std::endl;
      m_types.push_back(tube->getName());
    }
  else
    {
      // Here we build the full qualified Name of the Type
      out << "<instance type=\"" << getFullName()  << "\">" << std::endl;
      out << "<attvalue name=\"ID\" value =\""<< m_actualID.name() 
          << "\" showlabel =\"\"/>" << std::endl;
      xmlUtil::Identifier identifier;
      for(int ii=0;ii<m_actualID.size();ii++)
          identifier.append(m_actualID[ii]);

      out << "<attvalue name=\"IDname\" value =\""
          << m_idDictionary->getNameSeqString(identifier) 
          << "\" showlabel =\"\"/>" << std::endl;      
      HepPoint3D v(0,0,0);
      HepPoint3D v1;
      double dz = tube->getZ()/2;
      // double rin = tube->getRin();
      double rout = tube->getRout();
      HepGeom::Transform3D atr = m_actualTransform.back();

      unsigned int n = 20;
      double angle;
          
      for(unsigned int i=0; i<n; i++)
      {
        angle = 2*M_PI*((double)i/n);
        v.setX(rout*sin(angle)); v.setY(rout*cos(angle)); v.setZ(-dz); 
        v1 = (atr*v);           
        out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
            << "\" z = \"" << v1.z() << "\">" << std::endl;
        out << "</point>" << std::endl;
      }
      for(unsigned int i=0; i<n; i++)
      {
        angle = 2*M_PI*((double)i/n);
        v.setX(rout*sin(angle)); v.setY(rout*cos(angle)); 
        v.setZ(dz); v1 = (atr*v);           
        out << "<point x=\"" << v1.x() <<"\" y = \""  << v1.y() 
            << "\" z = \"" << v1.z() << "\">" << std::endl;
        out << "</point>" << std::endl;        
      }            

      out << "</instance>" << std::endl;
   }

  m_actualName.pop_back();

}

void  HepRepSectionsVisitor::visitPosXYZ(PosXYZ* pos)
{
  CLHEP::HepRotation rot;
  rot.rotateX(pos->getXRot()*M_PI/180);
  rot.rotateY(pos->getYRot()*M_PI/180);
  rot.rotateZ(pos->getZRot()*M_PI/180);

  CLHEP::Hep3Vector t(pos->getX(), pos->getY(), pos->getZ());  
  HepGeom::Transform3D tr(rot,t);
  HepGeom::Transform3D atr = (m_actualTransform.back())*tr;
  unsigned int i;

  idents::VolumeIdentifier tempID = m_actualID;

  /// Set the identifier
    for (i=0;i<(pos->getIdFields()).size();i++)
      m_actualID.append((int) pos->getIdFields()[i]->getValue());
  
  m_actualTransform.push_back(atr);
  pos->getVolume()->AcceptNotRec(this);
  m_actualTransform.pop_back();

  m_actualID = tempID;
}


void  HepRepSectionsVisitor::visitAxisMPos(AxisMPos* pos)
{
  unsigned int i,j ;
  unsigned int n;
  idents::VolumeIdentifier tempID = m_actualID;
  HepGeom::Transform3D atr;

  if (m_mode=="type")
    {
      pos->getVolume()->AcceptNotRec(this);      
    }
  else
    {
      n = pos->getNcopy();
      for(i=0;i<n;i++)
        {
        switch(pos->getAxisDir()){
        case (Stack::xDir):
          {
            //            HepRotation rot(pos->getRotation()*M_PI/180, 0, 0);
            CLHEP::HepRotation rot(-pos->getRotation()*M_PI/180, 0, 0);
            CLHEP::Hep3Vector t(pos->getDx()+pos->getDisp(i), pos->getDy(), 
                                pos->getDz());  
            HepGeom::Transform3D tr(rot,t);
            atr = (m_actualTransform.back())*tr;
          }
          break;
        case (Stack::yDir):
          {
            //            HepRotation rot(0,pos->getRotation()*M_PI/180, 0);
            CLHEP::HepRotation rot(0,-pos->getRotation()*M_PI/180, 0);
            CLHEP::Hep3Vector t(pos->getDx(), pos->getDy()+pos->getDisp(i), 
                                pos->getDz());  
            HepGeom::Transform3D tr(rot,t);
            atr = (m_actualTransform.back())*tr;
          }
          break;
        case (Stack::zDir):
          {
            //            HepRotation rot(0,0,pos->getRotation()*M_PI/180);
            CLHEP::HepRotation rot(0,0,-pos->getRotation()*M_PI/180);
            CLHEP::Hep3Vector t(pos->getDx(), pos->getDy(), 
                                pos->getDz()+pos->getDisp(i));  
            HepGeom::Transform3D tr(rot,t);
            atr = (m_actualTransform.back())*tr;
          }
          break;
        }  

        /// ID stuff
          for(j=0;j<pos->getIdFields().size();j++)
            {	
              m_actualID.append((int)(pos->getIdFields()[j]->getValue())+
                                (int)(pos->getIdFields()[j]->getStep()*i));
            }



        m_actualTransform.push_back(atr);
        pos->getVolume()->AcceptNotRec(this);
        m_actualTransform.pop_back();

        m_actualID = tempID;
      }
    }
}

void  HepRepSectionsVisitor::visitIdField(IdField*)
{

}

void  HepRepSectionsVisitor::visitSeg(Seg*)
{

}


// Return the full qualified Name of the Type
std::string HepRepSectionsVisitor::getFullName()
{
  std::string fullName = "";

  for (unsigned int i=0;i<m_actualName.size()-1; i++)
  {
    fullName = fullName + m_actualName[i] + "/";
  }
  fullName = fullName + m_actualName[m_actualName.size()-1];

  return fullName;
}
  

}
