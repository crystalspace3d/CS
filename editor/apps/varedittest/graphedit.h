/*
  Copyright (C) 2011 Christian Van Brussel, Eutyche Mukuama, Dodzi de Souza
      Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef __WX_GRAPHEDIT_H__
#define __WX_GRAPHEDIT_H__

#include <stdarg.h>

#include "csutil/array.h"
#include "csutil/csstring.h"
//#include "csutil/ref.h"

//#include "wx/wxsf/ShapeBase.h"

class csOptionDescription;
class csVariant;

/// Warning, this is part of the old implementation. No longer used, pending deletion.
class GraphNodeFactory
{
 public:
  GraphNodeFactory ();
  ~GraphNodeFactory ();
  void SetName (const char* name);
  const char* GetName () const;

  void AddParameter (csOptionDescription& description);
  size_t GetParameterCount () const;
  const csOptionDescription* GetParameterDescription (size_t index) const;

  // SetIcon/GetIcon

 private:
  csString name;
  csArray<csOptionDescription> optionDescriptions;
};
//-----------------GraphNode---------------------

class GraphNode// : public wxSFShapeBase
{
 public:
  GraphNode (GraphNodeFactory* _factory);

  GraphNodeFactory* GetFactory () const;

  void SetName (const char* name);
  const char* GetName () const;

  //size_t GetAnchorCount () const;
  //GraphAnchor* GetAnchor (size_t index) const;

  csVariant* GetParameter (size_t index);
  // TODO: remove old value?
  virtual void UpdateParameter (size_t index, csVariant* oldValue, csVariant* newValue) = 0;

  // virtual save/load

 protected:
  GraphNodeFactory* factory;
  csString name;
  csArray<csVariant> parameters;
};
/*
class GraphLink : public wxSFArrowBase
{
 public:
  GraphAnchor* GetSourceAnchor () const;
  void SetSourceAnchor (GraphAnchor* anchor);
  GraphAnchor* GetTargetAnchor () const;
  void SetTargetAnchor (GraphAnchor* anchor);
};

class GraphAnchor
{
 public:
  GraphNode* GetNode () const;
  bool IsSource () const;
  bool IsTarget () const;
  GraphLink* GetLink () const;
  GraphAnchor* GetLinkedAnchor () const;
};

class Graph ()
{
 public:
  //virtual void NewGraph () = 0;
  //virtual bool SaveGraph (const char* filename) = 0;
  //virtual bool LoadGraph (const char* filename) = 0;

  //virtual GraphNode* CreateNode (GraphNodeFactory* factory, GraphNode* parent = nullptr) = 0;
  virtual GraphNode* CreateNode (GraphNodeFactory* factory) = 0;
  virtual void RemoveNode (GraphNode* node) = 0;
  //virtual void ReparentNode (GraphNode* node, GraphNode* oldParent, GraphNode* newParent) = 0;

  virtual GraphLink* CreateLink (GraphAnchor* source, GraphAnchor* target) = 0;
  virtual void RemoveLink (GraphLink* link) = 0;
};

class GraphEditor
{
 public:
  void AddNodeFactory (GraphNodeFactory* factory);
  size_t GetNodeFactoryCount () const;
  GraphNodeFactory* GetNodeFactory (size_t index) const;
  //GraphNodeFactory* FindNodeFactory (const char* name) const;

  void SetGraph (Graph* graph);
  Graph* GetGraph ();
};
*/

#endif // __WX_GRAPHEDIT_H__
