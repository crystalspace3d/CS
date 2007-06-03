/*
    Copyright (C) 2007 by Scott Johnson

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This application is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this application; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "appcolladaconvert.h"

using namespace std;

CS_IMPLEMENT_APPLICATION

ColladaConversionUtility::ColladaConversionUtility()
{
	SetApplicationName("crystalspace.colladaconvertor");
}

bool ColladaConversionUtility::OnInitialize(int argc, char* argv[])
{
	if (!csInitializer::RequestPlugins(GetObjectRegistry(),
    CS_REQUEST_VFS,
    CS_REQUEST_OPENGL3D,
    CS_REQUEST_ENGINE,
    CS_REQUEST_REPORTER,
    CS_REQUEST_REPORTERLISTENER,
    CS_REQUEST_END))
    return ReportError("Failed to initialize plugins!");

  csBaseEventHandler::Initialize(GetObjectRegistry());
  if (!RegisterQueue(GetObjectRegistry(), csevAllEvents(GetObjectRegistry())))
    return ReportError("Failed to set up event handler!");
  
    /* Load up the XML Read Document System, instead of the default */
    plugManager = csQueryRegistry<iPluginManager> (GetObjectRegistry());

	/* Deprecated */
    //csPluginLoader* plugload = new csPluginLoader(GetObjectRegistry());
	//plugload->RequestPlugin("crystalspace.documentsystem.xmlread", "iDocumentSystem");
	//plugload->LoadPlugins();
	//docSystem = csQueryRegistry<iDocumentSystem> (GetObjectRegistry());
	docSystem = csLoadPlugin<iDocumentSystem> (plugManager, "crystalspace.documentsystem.xmlread");

	if (!docSystem.IsValid())
	{
		ReportWarning("Warning: Document system invalid.  Defaulting to Tiny XML Document System.");
		docSystem.AttachNew(new csTinyDocumentSystem());
	}
	
	colladaDocument = docSystem->CreateDocument();
	if (!colladaDocument.IsValid())
	{
		return ReportError("Error: Unable to create new COLLADA document.  Terminating.");
	}

	fileSystem = csQueryRegistry<iVFS> (GetObjectRegistry());
	if (!fileSystem)
	{
		return ReportError("Error: Unable to fetch VFS.  Terminating.");
	}

	// for testing only
	fileSystem->Mount("/test/", TESTDIR);
	if (!fileSystem->ChDir("/test/"))
	{
		return ReportError("Error: Unable to change directory to VFS mount /test.  Terminating.");
	}

	colladaFile = fileSystem->Open(TESTFILE, VFS_FILE_READ);
	if (!colladaFile.IsValid())
	{
		cout << "Attempting to open: " << TESTDIR << TESTFILE << endl;
		return ReportError("Error: Unable to open COLLADA file on VFS.  Terminating.");
	}

	// This works
	csRef<iDataBuffer> buf = colladaFile->GetAllData();
	//cout << buf->GetData() << endl;
	
	// This does not work
	colladaDocument->Parse(colladaFile);
	//int position = colladaFile->GetPos();
	//cout << "Position in file: " << position << endl;
	//colladaDocument->Parse(buf);

	csRef<iDocumentNode> root = colladaDocument->GetRoot();
	
	// we need to start at the root node
	// for our purposes, this will be the node <COLLADA>
	root = root->GetNode("COLLADA"); 
	csRef<iDocumentAttribute> ver = root->GetAttribute("version");
	if (!ver.IsValid())
	{
		cout << "Unable to find attribute named version!" << endl;
		//cout << "Node value: " << root->GetValue() << endl;
	}

	else
	{
		//string warn = "COLLADA Version: ";
		//warn.append(ver->GetValue());
		cout << "COLLADA Version: " << ver->GetValue() << endl;

		//ReportWarning(warn.c_str());
	}

	//cout << "Everything seems good, proceeding!" << endl;

  return true;
}

bool ColladaConversionUtility::Application()
{
	if (!OpenApplication(GetObjectRegistry()))
		return ReportError("Error: Unable to fetch Object Registry!");

  // We don't need the runloop to start
  //Run();

  return true;
}

int main(int argc, char** argv)
{
	return csApplicationRunner<ColladaConversionUtility>::Run (argc, argv);
}