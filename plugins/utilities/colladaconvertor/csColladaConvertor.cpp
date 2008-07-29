/*
  Copyright (C) 2007 by Scott Johnson

  This application is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This application is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this application; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
#include "iutil/objreg.h"
#include "iutil/plugin.h"
#include "iutil/document.h"
#include "csutil/xmltiny.h"
#include "iutil/string.h"
#include "csutil/hash.h"
#include "csutil/scfstr.h"
#include "csutil/scfstringarray.h"
#include "csColladaConvertor.h"
#include "csColladaClasses.h"

#include "csutil/custom_new_disable.h"
#include <string>
#include <sstream>
#include "csutil/custom_new_enable.h"

using std::string;
using std::stringstream;

CS_IMPLEMENT_PLUGIN

CS_PLUGIN_NAMESPACE_BEGIN (ColladaConvertor)
{

  SCF_IMPLEMENT_FACTORY(csColladaConvertor)

    // =============== Error Reporting and Handling Functions ===============
  void csColladaConvertor::Report(int severity, const char* msg, ...)
  {
    va_list argList;
    va_start(argList, msg);

    csRef<iReporter> rep = csQueryRegistry<iReporter>(obj_reg);
    if (rep.IsValid())
    {
      rep->ReportV(severity, "crystalspace.utilities.colladaconvertor", msg, argList); 
    }
    else
    {
      csPrintfV(msg, argList);
      csPrintf("\n");
    }

    va_end(argList);
  }

  void csColladaConvertor::SetWarnings(bool toggle)
  {
    warningsOn = toggle;
  }

  void csColladaConvertor::CheckColladaFilenameValidity(const char* str)
  {
    std::string filePath = str;
    size_t index = filePath.find(".", 0);

    if (index == std::string::npos)
    {
      Report(CS_REPORTER_SEVERITY_WARNING, "Warning:  No file extension detected on filename.  File is possibly not a COLLADA file.");
    }
    else
    {
      std::string ext = filePath.substr(index);

      if (ext != ".dae" && ext != ".DAE")
      {
        std::string warningMsg = "Warning:  File extension \'";
        warningMsg.append(ext);
        warningMsg.append("\' does not conform to expected COLLADA standard file extension \'.dae\'.  File is possibly not a COLLADA file.");
        Report(CS_REPORTER_SEVERITY_WARNING, warningMsg.c_str());
      }
    }
  }

  const char* csColladaConvertor::CheckColladaValidity(iFile *file)
  {
    Report(CS_REPORTER_SEVERITY_WARNING, "Warning: The function CheckColladaValidity(iFile* file) has not yet been implemented.");
    return 0;
  }

  // =============== Constructors/ Destructors ===============

  csColladaConvertor::csColladaConvertor(iBase* parent) :
    scfImplementationType(this, parent), 
    docSys(0),
    warningsOn(false),
    obj_reg(0),
    lastEffectId(-1),
    csOutputReady(false),
    outputFileType(CS_NO_FILE),
    colladaReady(false)      
  {
  }

  csColladaConvertor::~csColladaConvertor()
  {
    colladaElement.Invalidate();
    colladaFile.Invalidate();
    csFile.Invalidate();
    csTopNode.Invalidate();
    delete docSys;
    materialsList.DeleteAll();
  }

  // =============== Plugin Initialization ===============

  bool csColladaConvertor::Initialize (iObjectRegistry* reg)
  {
    obj_reg = reg;

    // create our own document system, since we will be reading and
    // writing to the XML files
    docSys = new csTinyDocumentSystem();

    // get a pointer to the virtual file system
    fileSys = csQueryRegistryOrLoad<iVFS>(obj_reg, "crystalspace.kernel.vfs");

    return fileSys.IsValid();
  }

  // =============== File Loading ===============

  const char* csColladaConvertor::Load(const char *str)
  {
    csRef<iFile> filePtr;

    // only do a consistency check for collada filename if warnings are on
    if (warningsOn)
    {
      CheckColladaFilenameValidity(str);
    }

    filePtr = fileSys->Open(str, VFS_FILE_READ);

    if (!filePtr.IsValid())
    {
      std::string warningMsg = "Unable to open file: ";
      warningMsg.append(str);
      warningMsg.append(".  File not loaded.");
      Report(CS_REPORTER_SEVERITY_WARNING, warningMsg.c_str());
      return "Unable to open file";
    }

    const char* retVal = Load(filePtr);
    filePtr.Invalidate();
    return retVal;
  }

  const char* csColladaConvertor::Load(iString *str)
  {
    csRef<iFile> filePtr;

    if (!fileSys.IsValid())
    {
      Report(CS_REPORTER_SEVERITY_WARNING,
	  "Unable to access file system.  File not loaded.");
      return "Unable to access file system";
    }

    // only do a consistency check for collada file if warnings are on
    if (warningsOn)
    {
      CheckColladaFilenameValidity(str->GetData());
    }

    filePtr = fileSys->Open(str->GetData(), VFS_FILE_READ);

    if (!filePtr.IsValid())
    {
      std::string warningMsg = "Unable to open file: ";
      warningMsg.append(str->GetData());
      warningMsg.append(".  File not loaded.");
      Report(CS_REPORTER_SEVERITY_WARNING, warningMsg.c_str());
      return "Unable to open file";
    }

    const char* retVal = Load(filePtr);
    filePtr.Invalidate();
    return retVal;
  }

  const char* csColladaConvertor::Load(iFile *file)
  {
    colladaFile = docSys->CreateDocument();
    colladaFile->Parse(file);
    csRef<iDocumentNode> rootNode = colladaFile->GetRoot();
    rootNode = rootNode->GetNode("COLLADA");
    if (!rootNode.IsValid())
    {
      Report(CS_REPORTER_SEVERITY_ERROR,
	  "Error: Unable to find COLLADA node.  File not loaded.");
      return "Unable to find COLLADA node";
    }

    colladaElement = rootNode;
    csOutputReady = false;
    colladaReady = true;

    return 0;
  }

  const char* csColladaConvertor::Load(iDataBuffer *db)
  {
    colladaFile = docSys->CreateDocument();
    colladaFile->Parse(db);
    csRef<iDocumentNode> rootNode = colladaFile->GetRoot();
    rootNode = rootNode->GetNode("COLLADA");
    if (!rootNode.IsValid())
    {
      Report(CS_REPORTER_SEVERITY_ERROR,
	  "Error: Unable to find COLLADA node.  File not loaded.");
      return "Unable to find COLLADA node";
    }  

    colladaElement = rootNode;
    csOutputReady = false;
    colladaReady = true;

    return 0;
  }

  // =============== File Writing ===============

  const char* csColladaConvertor::Write(const char* filepath)
  {
    // sanity check
    if (!csOutputReady)
    {
      if (warningsOn)
      {
        Report(CS_REPORTER_SEVERITY_WARNING,
          "Warning: Crystal Space document not ready for writing.");
      }

      return "Crystal Space document not ready for writing";
    }

    const char* errorString = csFile->Write(fileSys, filepath);

    if (errorString)
    {
      std::string errorMsg = "Warning: An error occurred writing to file: ";
      errorMsg.append(errorString);
      Report(CS_REPORTER_SEVERITY_ERROR, errorMsg.c_str()); 
      return "An error occurred writing to file";
    }

    return 0;
  }

  // =============== Accessor Functions =============== 

  csColladaMaterial* csColladaConvertor::FindMaterial(const char* accessorString)
  {
    // Using csArrayCmp would be more efficient I believe, but
    // I have no idea how to use this comparator.  ;)
    //csArrayCmp<csColladaMaterial, const char*> functor;

    if (warningsOn)
    {
      Report(CS_REPORTER_SEVERITY_WARNING, "Inside FindMaterial()");
    }

    csArray<csColladaMaterial>::Iterator matIter = materialsList.GetIterator();
    while (matIter.HasNext())
    {
      csString accessConverted(accessorString);

      if (warningsOn)
      {
        Report(CS_REPORTER_SEVERITY_WARNING, "Creating currentMat...");
      }

      csColladaMaterial *currentMat = new csColladaMaterial(matIter.Next());

      if (warningsOn)
      {
        Report(CS_REPORTER_SEVERITY_WARNING, "Done. Accessor string: %s", accessConverted.GetData());
      }

      if (currentMat->GetID().Compare(accessConverted.GetData()))
      {
        if (warningsOn)
        {
          Report(CS_REPORTER_SEVERITY_WARNING, "Returning...");
        }
        return currentMat;
      }

      else
      {
        delete currentMat;
      }
    }

    if (warningsOn)
    {
      Report(CS_REPORTER_SEVERITY_WARNING, "Returning...");
    }

    return 0;
  }

  csColladaEffect& csColladaConvertor::GetEffect(size_t index)
  {
    return effectsList.Get(index);
  }

  size_t csColladaConvertor::GetEffectIndex(const csColladaEffect& effect)
  {
    return 0;
  }

  // =============== Mutator Functions ===============

  const char* csColladaConvertor::SetOutputFiletype(csColladaFileType filetype)
  {
    if (filetype == CS_NO_FILE)
    {
      Report(CS_REPORTER_SEVERITY_ERROR, "Error: Unable to set output file type to CS_NO_FILE.");
      return "Unable to set output file type to CS_NO_FILE";
    }

    outputFileType = filetype;
    return 0;
  }

  bool csColladaConvertor::InitializeCrystalSpaceDocument()
  {
    if (outputFileType == CS_NO_FILE)
    {
      if (warningsOn)
      {
        Report(CS_REPORTER_SEVERITY_WARNING, "Warning: No output file type specified.  Use SetOutputFiletype() first");
      }

      return false;
    }

    csFile = docSys->CreateDocument();
    csRef<iDocumentNode> rootNode = csFile->CreateRoot();

    /* Some identifying commentary */
    csRef<iDocumentNode> comment = rootNode->CreateNodeBefore(CS_NODE_COMMENT, 0);
    comment->SetValue("FILE GENERATED AUTOMATICALLY - DO NOT EDIT");
    csRef<iDocumentNode> comment2 = rootNode->CreateNodeBefore(CS_NODE_COMMENT, comment);
    comment2->SetValue("Created with CrystalSpace COLLADA Convertor v1.0");
    comment.Invalidate();
    comment2.Invalidate();

    csRef<iDocumentNode> newNode = rootNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    if(!newNode.IsValid())
    {
      if (warningsOn)
      {
        Report(CS_REPORTER_SEVERITY_WARNING, "Warning: Unable to create first element.");
      }

      return false;
    }

    if (outputFileType == CS_MAP_FILE)
    {
      newNode->SetValue("world"); 
      csTopNode = csFile->GetRoot()->GetNode("world");
    }

    else 
    {
      newNode->SetValue("library");
      csTopNode = csFile->GetRoot()->GetNode("library");
    }

    csOutputReady = true;
    return true;
  }

  // =============== Basic Utility Functions ===============

  // None, anymore ;)


  // =============== Conversion Functions ===============
  // This is the warp core of the implementation.  ;)
  // Currently, the core has been ejected, so it's missing.  Engineering is still
  // trying to locate it on sensors.  :)

  const char* csColladaConvertor::Convert()
  {
    if (!csOutputReady)
    {
      if (!InitializeCrystalSpaceDocument())
      {
        if (warningsOn)
        {
          Report(CS_REPORTER_SEVERITY_ERROR, "Error: Unable to initialize output document.");
        }
        return "Unable to initialize output document";
      }
      else
      {
        if (warningsOn)
        {
          Report(CS_REPORTER_SEVERITY_NOTIFY, "Success.");
        }
      }
    }

    if (!colladaReady)
    {
      Report(CS_REPORTER_SEVERITY_ERROR, "Error: COLLADA file has not been loaded.");
      return "COLLADA file not loaded";
    }

    // ConvertEffects() needs to be called first, so that there actually *is* a materials
    // list from which to assign materials in ConvertGeometry()
    csRef<iDocumentNode> materialsNode = colladaElement->GetNode("library_materials");
    if (!materialsNode.IsValid())
    {
      Report(CS_REPORTER_SEVERITY_ERROR, "Error: Unable to find <library_materials> element");
      return "Unable to find library_materials.";
    }

    if (warningsOn)
    {
      Report(CS_REPORTER_SEVERITY_WARNING, "Beginning to convert effects.");
    }

    ConvertEffects();

    if (warningsOn)
    {
      Report(CS_REPORTER_SEVERITY_WARNING, "Done converting effects.");
    }

    csRef<iDocumentNode> geoNode = colladaElement->GetNode("library_geometries");
    if (!geoNode.IsValid())
    {
      Report(CS_REPORTER_SEVERITY_ERROR, "Error: Unable to find <library_geometries> element.");
      return "Unable to find library_geometries.";
    }

    if (warningsOn)
    {
      Report(CS_REPORTER_SEVERITY_WARNING, "Beginning to convert geometry");
    }

    ConvertGeometry(geoNode);

    if (warningsOn)
    {
      Report(CS_REPORTER_SEVERITY_WARNING, "Done converting geometry");
    }

    if (outputFileType == CS_MAP_FILE)
    {
      csRef<iDocumentNode> camerasNode, lightsNode, visualScenesNode;
      camerasNode = colladaElement->GetNode("library_cameras");
      lightsNode = colladaElement->GetNode("library_lights");
      visualScenesNode = colladaElement->GetNode("library_visual_scenes");

      if (warningsOn)
      {
        Report(CS_REPORTER_SEVERITY_WARNING, "Beginning to convert scene data");
      }

      ConvertScene(camerasNode, lightsNode, visualScenesNode);

      if (warningsOn)
      {
        Report(CS_REPORTER_SEVERITY_WARNING, "Done converting scene data");
      }
    }

    return 0;
  }

  bool csColladaConvertor::ConvertGeometry(iDocumentNode *geometrySection)
  {
    csRef<iDocumentNodeIterator> geometryIterator;

    // get an iterator over all <geometry> elements
    geometryIterator = geometrySection->GetNodes("geometry");

    if (!geometryIterator.IsValid())
    {
      if (warningsOn)
      {
        Report(CS_REPORTER_SEVERITY_WARNING, "Warning: Unable to get iterator over geometry nodes.");
      }

      return false;
    }

    // variable definitions, so we don't have to run the constructors each
    // time during an iteration of the while loops
    csRef<iDocumentNode> currentGeometryElement;
    csRef<iDocumentAttribute> currentGeometryID;
    csRef<iDocumentNodeIterator> meshIterator;
    csRef<iDocumentNode> currentMeshElement;
    csRef<iDocumentNode> currentVerticesElement;
    csRef<iDocumentNodeIterator> convexMeshIterator;
    csColladaMesh *mesh;

    // while the iterator is not empty
    while (geometryIterator->HasNext())
    {
      // retrieve next <geometry> element and store as currentGeometryElement
      currentGeometryElement = geometryIterator->Next();

      // get value of id attribute and store as currentGeometryID
      currentGeometryID = currentGeometryElement->GetAttribute("id");

      if (currentGeometryID.IsValid())
      {
        if (warningsOn)
        {
          std::string notifyMsg = "Current Geometry Node ID: ";
          notifyMsg.append(currentGeometryID->GetValue());
          Report(CS_REPORTER_SEVERITY_NOTIFY, notifyMsg.c_str());
        }
      }

      // let's make sure that we output a warning in the event that the
      // user attempts to convert a <convex_mesh> element, as this
      // isn't supported yet
      /// @todo Add support for <convex_mesh> elements.
      convexMeshIterator = currentGeometryElement->GetNodes("convex_mesh");
      if (convexMeshIterator->HasNext() && warningsOn)
      {
        Report(CS_REPORTER_SEVERITY_WARNING, "Warning: <convex_mesh> element detected.  This system does not currently support this element type.  It will not be converted.");
      }

      // get an iterator over all <mesh> child elements
      meshIterator = currentGeometryElement->GetNodes("mesh");

      if (!meshIterator.IsValid())
      {
        if (warningsOn)
        {
          Report(CS_REPORTER_SEVERITY_WARNING, "Warning: Unable to get iterator over mesh nodes.");
        }

        return false;
      }

      // while this iterator is not empty
      while (meshIterator->HasNext())
      {
        // get next mesh element as currentMeshElement
        currentMeshElement = meshIterator->Next();

        // retrieve <vertices> element from currentMeshElement as
        // currentVerticesElement
        currentVerticesElement = currentMeshElement->GetNode("vertices");

        if(!currentVerticesElement.IsValid())
        {
          if (warningsOn)
          {
            Report(CS_REPORTER_SEVERITY_WARNING, "Warning: Unable to retrieve vertices element from mesh.");
          }

          return false;
        }

        mesh = new csColladaMesh(currentMeshElement, this);

        mesh->WriteXML(csTopNode);

        delete mesh;
      }
    }

    return true;
  }

  csRef<iDocumentNode> csColladaConvertor::GetSourceElement(const char* name, iDocumentNode* parent)
  {
    csRef<iDocumentNode> returnValue;
    csRef<iDocumentNodeIterator> iter;
    bool found = false;
    iter = parent->GetNodes("source");

    while(iter->HasNext())
    {
      returnValue = iter->Next();
      std::string comparisonString(returnValue->GetAttributeValue("id"));
      if (comparisonString.compare(name) == 0)
      {
        found = true;
        break;
      }
    }

    if (!found)
    {
      return 0;
    }

    return returnValue;
  }

  // ConvertEffects() is the generic function for converting textures, materials, shaders
  // and all related items.
  bool csColladaConvertor::ConvertEffects()
  {
    Report(CS_REPORTER_SEVERITY_WARNING, "Warning: ConvertEffects() functionality not fully implemented.  Use at your own risk!");

    // Convert textures
    csRef<iDocumentNode> imagesNode = colladaElement->GetNode("library_images");
    if(imagesNode.IsValid())
    {
      csRef<iDocumentNode> texturesNode = csTopNode->CreateNodeBefore(CS_NODE_ELEMENT);
      texturesNode->SetValue("textures");
      csRef<iDocumentNodeIterator> textureNodes = imagesNode->GetNodes("image");
      while(textureNodes->HasNext())
      {
        csRef<iDocumentNode> texture = textureNodes->Next();
        csRef<iDocumentNode> newTexture = texturesNode->CreateNodeBefore(CS_NODE_ELEMENT);
        newTexture->SetValue("texture");
        newTexture->SetAttribute("name", texture->GetAttributeValue("id"));

        csRef<iDocumentNode> textureFile = newTexture->CreateNodeBefore(CS_NODE_ELEMENT);
        textureFile->SetValue("file");

        csRef<iDocumentNode> textureFileContents = textureFile->CreateNodeBefore(CS_NODE_TEXT);
        textureFileContents->SetValue(texture->GetAttributeValue("id"));

        // TODO: Alpha, Class.
      }
    }

    // Convert materials
    csRef<iDocumentNode> materialsNode = colladaElement->GetNode("library_materials");
    csRef<iDocumentNode> effectsNode = colladaElement->GetNode("library_effects");
    if(materialsNode.IsValid() && effectsNode.IsValid())
    {
      csRef<iDocumentNode> newMaterialsNode = csTopNode->CreateNodeBefore(CS_NODE_ELEMENT);
      newMaterialsNode->SetValue("materials");

      csRef<iDocumentNodeIterator> materialNodes = materialsNode->GetNodes("material");
      csRef<iDocumentNodeIterator> effectNodes = effectsNode->GetNodes("effect");

      while(materialNodes->HasNext() && effectNodes->HasNext())
      {
        csRef<iDocumentNode> material = materialNodes->Next();
        csRef<iDocumentNode> effect = effectNodes->Next();

        csRef<iDocumentNode> newMaterial = newMaterialsNode->CreateNodeBefore(CS_NODE_ELEMENT);
        newMaterial->SetValue("material");
        newMaterial->SetAttribute("name", material->GetAttributeValue("id"));

        csColladaMaterial nextMaterial = csColladaMaterial(this);
        nextMaterial.SetID(material->GetAttributeValue("id"));
        //nextMaterial.SetInstanceEffect(effect); // Needs work.
        materialsList.Push(nextMaterial);

        // Common profile.
        csRef<iDocumentNode> effectCommon = effect->GetNode("profile_COMMON");
        if(effectCommon.IsValid())
        {
          csRef<iDocumentNode> surface;
          csRef<iDocumentNodeIterator> newparams = effectCommon->GetNodes("newparam");
          while(newparams->HasNext())
          {
            csRef<iDocumentNode> surface = newparams->Next()->GetNode("surface");
            if(!surface)
            {
              continue;
            }

            if(surface->GetNode("init_from"))
            {
              csRef<iDocumentNode> texture = newMaterial->CreateNodeBefore(CS_NODE_ELEMENT);
              texture->SetValue("texture");
              csRef<iDocumentNode> textureContents = texture->CreateNodeBefore(CS_NODE_TEXT);
              textureContents->SetValue(surface->GetNode("init_from")->GetContentsValue());
            }
          }
        }

        // TODO: Shaders.
      }

      return true;
    }

    return false;
  }

  bool csColladaConvertor::ConvertScene(iDocumentNode *camerasSection, iDocumentNode *lightsSection, iDocumentNode *visualScenesSection)
  {
    Report(CS_REPORTER_SEVERITY_WARNING, "Warning: ConvertScene() functionality not fully implemented.  Use at your own risk!");

    if (outputFileType != CS_MAP_FILE)
    {
      Report(CS_REPORTER_SEVERITY_WARNING, "Warning: Conversion of scenes is invalid except for Crystal Space map files.  Continuing blithely...");
      return false;
    }

    // Get camera IDs
    csArray<csString> cameraIDs;
    if(camerasSection)
    {
      csRef<iDocumentNodeIterator> cameraNodes = camerasSection->GetNodes("camera");
      while(cameraNodes->HasNext())
      {
        cameraIDs.Push(cameraNodes->Next()->GetAttributeValue("id"));
      }
    }

    // Get lights
    csHash<csColladaLight, csString> lights;
    if(lightsSection)
    {
      csRef<iDocumentNodeIterator> lightNodes = lightsSection->GetNodes("light");
      while(lightNodes->HasNext())
      {
        csRef<iDocumentNode> lightNode = lightNodes->Next();
        lightNode = lightNode->GetNode("technique_common");

        // Point lights.
        if(lightNode &&
           lightNode->GetNode("point"))
        {
          csColladaLight light;

          // TODO: Radius, attenuation, more?

          csStringArray lightColour;
          lightColour.SplitString(lightNode->GetNode("point")->GetNode("color")->GetContentsValue(), " ");
          light.colour = csColor(atof(lightColour[0]), atof(lightColour[1]), atof(lightColour[2]));

          lights.Put(lightNode->GetParent()->GetAttributeValue("id"), light);
        }
      }
    }

    // For each scene (<visual_scene> node), the first level nodes will be converted to 
    // a sector in crystal space.
    csRef<iDocumentNodeIterator> visualSceneIterator = visualScenesSection->GetNodes("visual_scene");
    csRef<iDocumentNode> currentVisualSceneElement;
    while(visualSceneIterator->HasNext())
    {
      csRef<iDocumentNode> visualSceneElement = visualSceneIterator->Next();
      csRef<iDocumentNodeIterator> sectors = visualSceneElement->GetNodes("node");
      while(sectors->HasNext())
      {
        csRef<iDocumentNode> sector = sectors->Next();

        // Check that it really is a sector.
        bool cameraTarget = false;
        for(size_t i=0; i<cameraIDs.GetSize(); i++)
        {
          csString id = cameraIDs[i];
          id.Truncate(id.FindLast('-'));
          cameraTarget |= id.Append(".Target-node").Compare(sector->GetAttributeValue("id"));
        }

        if(sector->GetAttribute("name") && !cameraTarget)
        {
          // Write sector.
          csRef<iDocumentNode> currentSectorElement = csTopNode->CreateNodeBefore(CS_NODE_ELEMENT);
          currentSectorElement->SetValue("sector");
          currentSectorElement->SetAttribute("name", sector->GetAttributeValue("name"));

          // Write children.
          csRef<iDocumentNodeIterator> sectorNodes = sector->GetNodes("node");
          while(sectorNodes->HasNext())
          {
            csRef<iDocumentNode> child = sectorNodes->Next();
            // Write collision plugin.
            // Write meshobj.
            // Write portal.
            // Write light.
            if(child->GetNode("instance_light"))
            {
              csRef<iDocumentNode> newLight = currentSectorElement->CreateNodeBefore(CS_NODE_ELEMENT);
              newLight->SetValue("light");
              newLight->SetAttribute("name", child->GetAttributeValue("name"));

              // Calculate centre.
              csStringArray sectorPos;
              csStringArray centerPos;

              sectorPos.SplitString(sector->GetNode("matrix")->GetContentsValue(), " ");
              centerPos.SplitString(child->GetNode("matrix")->GetContentsValue(), " ");

              float x = atof(sectorPos[3]) + atof(centerPos[3]);
              float y = atof(sectorPos[11]) + atof(centerPos[11]);
              float z = atof(sectorPos[7]) + atof(centerPos[7]);

              csRef<iDocumentNode> centreNode = newLight->CreateNodeBefore(CS_NODE_ELEMENT);
              centreNode->SetValue("center");
              centreNode->SetAttributeAsFloat("x", x);
              centreNode->SetAttributeAsFloat("y", y);
              centreNode->SetAttributeAsFloat("z", z);

              // Write colour.
              csString url = child->GetNode("instance_light")->GetAttributeValue("url");
              csColladaLight light = lights.Get(url.Slice(1), csColladaLight());
              csRef<iDocumentNode> lightColour = newLight->CreateNodeBefore(CS_NODE_ELEMENT);
              lightColour->SetValue("color");
              lightColour->SetAttributeAsFloat("red", light.colour.red);
              lightColour->SetAttributeAsFloat("green", light.colour.green);
              lightColour->SetAttributeAsFloat("blue", light.colour.blue);
            }
          }
        }
      }
    }

    // Next, we'll convert the cameras
    // A camera will convert to a start location in the world file
    // A start location consists of: 
    //   * A sector to start in.
    //   * A position to start at.
    //   * An up vector. (optional)
    //   * A forward vector. (optional)

    // For each camera.
    for(size_t i=0; i<cameraIDs.GetSize(); i++)
    {
      // For each scene.
      visualSceneIterator = visualScenesSection->GetNodes("visual_scene");
      while(visualSceneIterator->HasNext())
      {
        // For each sector;
        csRef<iDocumentNode> visualSceneElement = visualSceneIterator->Next();
        csRef<iDocumentNodeIterator> sectors = visualSceneElement->GetNodes("node");
        while(sectors->HasNext())
        {
          csRef<iDocumentNode> sector = sectors->Next();

          // Not a sector if it's a camera target.
          csString id = cameraIDs[i];
          if(id.Append(".Target-node").Compare(sector->GetAttributeValue("id")))
          {
            continue;
          }

          // For each node (possible camera).
          csRef<iDocumentNodeIterator> nodes = sector->GetNodes("node");
          while(nodes->HasNext())
          {
            csRef<iDocumentNode> node = nodes->Next();
            if(node->GetNode("instance_camera"))
            {
              // Is it the right camera?
              csString url = node->GetNode("instance_camera")->GetAttributeValue("url");
              if(url.Slice(1).Compare(cameraIDs[i]))
              {
                // Create start node.
                csRef<iDocumentNode> startNode = csTopNode->CreateNodeBefore(CS_NODE_ELEMENT);
                startNode->SetValue("start");
                startNode->SetAttribute("name", node->GetAttributeValue("name"));
                csRef<iDocumentNode> sectorNode = startNode->CreateNodeBefore(CS_NODE_ELEMENT);
                sectorNode->SetValue("sector");
                csRef<iDocumentNode> sectorNodeContents = sectorNode->CreateNodeBefore(CS_NODE_TEXT);
                sectorNodeContents->SetValue(sector->GetAttributeValue("name"));

                // Calculate position.
                csStringArray sectorPos;
                csStringArray cameraPos;

                sectorPos.SplitString(sector->GetNode("matrix")->GetContentsValue(), " ");
                cameraPos.SplitString(node->GetNode("matrix")->GetContentsValue(), " ");

                float x = atof(sectorPos[3]) + atof(cameraPos[3]);
                float y = atof(sectorPos[11]) + atof(cameraPos[11]);
                float z = atof(sectorPos[7]) + atof(cameraPos[7]);

                csRef<iDocumentNode> positionNode = startNode->CreateNodeBefore(CS_NODE_ELEMENT);
                positionNode->SetValue("position");
                positionNode->SetAttributeAsFloat("x", x);
                positionNode->SetAttributeAsFloat("y", y);
                positionNode->SetAttributeAsFloat("z", z);

                // TODO: up and forward vectors.
              }
            }
          }
        }
      }
    }

    return true;
  }

  bool csColladaConvertor::ConvertRiggingAnimation(iDocumentNode *riggingSection)
  {
    Report(CS_REPORTER_SEVERITY_WARNING, "Warning: ConvertRiggingAnimation() functionality not fully implemented.  Use at your own risk!");
    return true;
  }

  bool csColladaConvertor::ConvertPhysics(iDocumentNode *physicsSection)
  {
    Report(CS_REPORTER_SEVERITY_WARNING, "Warning: ConvertPhysics() functionality not fully implemented.  Use at your own risk!");
    return true;
  }

}
CS_PLUGIN_NAMESPACE_END(ColladaConvertor)
