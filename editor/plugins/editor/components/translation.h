/*
    Copyright (C) 2012 by Andrei Bârsan

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CSEDITOR_COMPONENT_TRANSLATION_H__
#define __CSEDITOR_COMPONENT_TRANSLATION_H__

#include "ivaria/translator.h"
#include "ieditor/editor.h"
// #include "csutil/scf_implementation.h"

using namespace CS::EditorApp;

CS_PLUGIN_NAMESPACE_BEGIN(CSEditor)
{
  class EditorTranslation
    : public scfImplementation2<EditorTranslation, iEditorComponent, iTranslator>
  {
  public:
    EditorTranslation (iBase* parent);
    virtual ~EditorTranslation ();

    //-- iEditorComponent
    virtual bool Initialize (iEditor* editor);
    virtual void Update ();
    virtual void Save (iDocumentNode* node) const;
    virtual bool Load (iDocumentNode* node);

    //-- iTranslator
    virtual const char* GetMsg (const char* src) const;

  private:
    iObjectRegistry* object_reg;
    iEditor* editor;
    csRef<iTranslator> translator;
  };
}
CS_PLUGIN_NAMESPACE_END(CSEditor)

#endif // __CSEDITOR_COMPONENT_TRANSLATION_H__