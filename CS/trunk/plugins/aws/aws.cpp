#include "cssysdef.h"
#include "aws.h"
#include "awsprefs.h"
#include "awsslot.h"

CS_IMPLEMENT_PLUGIN

SCF_IMPLEMENT_IBASE(awsWindow)
  SCF_IMPLEMENTS_INTERFACE(iAwsWindow)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_IBASE (awsComponent)
  SCF_IMPLEMENTS_INTERFACE (iAwsComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_IBASE (awsSink)
  SCF_IMPLEMENTS_INTERFACE (iAwsSink)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_IBASE (awsSlot)
  SCF_IMPLEMENTS_INTERFACE (iAwsSlot)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_IBASE (awsSource)
  SCF_IMPLEMENTS_INTERFACE (iAwsSource)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_IBASE (awsSinkManager)
  SCF_IMPLEMENTS_INTERFACE (iAwsSinkManager)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (awsSinkManager::eiComponent)
   SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_IBASE (awsPrefManager)
  SCF_IMPLEMENTS_INTERFACE (iAwsPrefManager)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_IBASE (awsManager)
  SCF_IMPLEMENTS_INTERFACE (iAws)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iEventHandler)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (awsManager::eiComponent)
   SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (awsManager::eiEventHandler)
   SCF_IMPLEMENTS_INTERFACE (iEventHandler)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_FACTORY (awsManager)
SCF_IMPLEMENT_FACTORY (awsPrefManager)
SCF_IMPLEMENT_FACTORY (awsSinkManager)

SCF_EXPORT_CLASS_TABLE (aws)                                                                                      
  SCF_EXPORT_CLASS (awsManager, "crystalspace.window.alternatemanager", "Crystal Space alternate window manager") 
  SCF_EXPORT_CLASS (awsPrefManager, "crystalspace.window.preferencemanager", "Crystal Space window preference manager") 
  SCF_EXPORT_CLASS (awsSinkManager, "crystalspace.window.sinkmanager", "Crystal Space window sink manager") 
SCF_EXPORT_CLASS_TABLE_END                                                                                        

