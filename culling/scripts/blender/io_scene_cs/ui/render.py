import os
import bpy
from io_scene_cs.utilities import rnaType, GetExportPath

from io_scene_cs.utilities import WalkTestPath, HasCrystalSpace, GetPreferences
from io_scene_cs.io import Export


class csSettingsPanel():
  bl_space_type = "PROPERTIES"
  bl_region_type = "WINDOW"
  bl_context = "render"
  # COMPAT_ENGINES must be defined in each subclass, external engines can add themselves here

  @classmethod
  def poll(cls, context):
    rd = context.scene.render
    return (rd.engine in cls.COMPAT_ENGINES)


@rnaType
class RENDER_PT_csSettingsPanel(csSettingsPanel, bpy.types.Panel):
  bl_label = "Crystal Space Export"
  COMPAT_ENGINES = {'CRYSTALSPACE'}
  
  def draw(self, context):
    layout = self.layout
    
    
    rd = context.scene.render
    row = layout.row(align=True)
    row.operator("io_scene_cs.export", text="Export", icon='SCENE_DATA')
    if not GetPreferences().library:
      if HasCrystalSpace():
        row.operator("io_scene_cs.export_run", text="Play", icon='GAME')
        if context.active_object.type == 'MESH':
          row.operator("io_scene_cs.export_view", text="View", icon='RENDER_STILL')
        row = layout.row()
        row.prop(GetPreferences(), "console")
        row.prop(GetPreferences(), "verbose")
        row.prop(GetPreferences(), "silent")
        row.prop(GetPreferences(), "bugplug")
      else:
        row.label(text="'walktest' isn't available!")
    
    
    row = layout.row(align=True)
    box = row.box()
    
    row = box.row()
    row.prop(GetPreferences(), "exportOnlyCurrentScene")
    
    row = box.row()
    row.prop(GetPreferences(), "library")
    row = box.row()
    if not GetPreferences().library:
      row.prop(GetPreferences(), "sharedMaterial")
    row = box.row()
    row.prop(GetPreferences(), "enableDoublesided")
    row = box.row()
    row.prop(GetPreferences(), "exportPath")
      

#@rnaType    
class B2CS_OT_export(bpy.types.Operator):
  "Export"
  bl_idname = "io_scene_cs.export"
  bl_label = "Export"

  def execute(self, context): 
    exportPath = GetExportPath()
    Export(exportPath)

    return {'FINISHED'}

#@rnaType    
class B2CS_OT_export_run(bpy.types.Operator):
  "Export and Run"
  bl_idname = "io_scene_cs.export_run"
  bl_label = "Export and Run."

  def execute(self, context): 
    exportPath = GetExportPath()
    Export(exportPath)
    
    options = " "
    if GetPreferences().console:
      options += "-console "
    if GetPreferences().verbose:
      options += "-verbose=-scf "
    if GetPreferences().silent:
      options += "-silent "
    if GetPreferences().bugplug:
      options += "-plugin=bugplug "
     
    #import commands  
    #output = commands.getstatusoutput(WalkTestPath() + options + path)
    #print(output)
    
    import shlex, subprocess
    print(WalkTestPath())
    args = shlex.split(WalkTestPath() + options + exportPath)
    print(args)
    output = subprocess.call(args)
    print(output)

    return {'FINISHED'}

VIEWMESH_INSTANCE = None

#@rnaType    
class B2CS_OT_export_view(bpy.types.Operator):
  "View selected mesh in Viewmesh"
  bl_idname = "io_scene_cs.export_view"
  bl_label = "View selected mesh in Viewmesh"

  def execute(self, context): 
    exportPath = GetExportPath()
    Export(exportPath)
    
    global VIEWMESH_INSTANCE
    
    if VIEWMESH_INSTANCE and VIEWMESH_INSTANCE.poll() is None:
      print('Reloading...')
      import signal
      VIEWMESH_INSTANCE.send_signal(signal.SIGINT)
    else:
      
      #TODO: damnit why isn't it loading the materials library??!!
      options = ' -R="'+exportPath+'"'
      options += ' -L="materials"'
      options += ' -factory="'+bpy.context.active_object.data.uname+'"'
      options += ' "/tmp/viewmesh/factories/'+bpy.context.active_object.data.uname+'"'
      
      exe = WalkTestPath().replace('walktest', 'viewmesh')
      
      import shlex, subprocess
      print(exe)
      args = shlex.split(exe + options)
      print(args)
      process = subprocess.Popen(args)
      print(process.pid)
      VIEWMESH_INSTANCE = process

    return {'FINISHED'}
