#!/usr/bin/env python
#==============================================================================
#
#    CVS Snapshot Generation Script
#    Copyright (C) 2000 by Eric Sunshine <sunshine@sunshineco.com>
#
#    This library is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Library General Public
#    License as published by the Free Software Foundation; either
#    version 2 of the License, or (at your option) any later version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Library General Public License for more details.
#
#    You should have received a copy of the GNU Library General Public
#    License along with this library; if not, write to the Free
#    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#==============================================================================
#------------------------------------------------------------------------------
# snapshot.py
#
#    A tool for generating snapshots and 'diffs' of a module within a CVS
#    repository.
#
#    Author's note: This script can certainly be improved.  Better error
#    handling, more options (such as --verbose, --quiet, etc.), better
#    abstraction and generalization, are all future possibilities.  There is
#    room for improvement.
#
#------------------------------------------------------------------------------
import commands, glob, grp, os, re, string, sys, tempfile, time

#------------------------------------------------------------------------------
# Configuration Section
#    cvsroot - CVSROOT setting for CVS.
#    fixcvsroot - The CVSROOT setting which should appear in each CVS/Root
#        file within the snapshot.  May be None if it is identical to the
#        original cvsroot setting.  This setting is useful in cases where the
#        CVSROOT value used for performing the check out differs from the one
#        users will later need when updating the snapshot from CVS.
#    cvsmodule - The module to checkout from the CVS repository.
#    moduledir - The name of the directory which is created when the module
#        is checked out from CVS (frequently identical to cvsmodule).
#    ownergroup - The group name which will be given to the 'chgrp' command
#        when directories are created.  Assigning group ownership to a
#        directory allows others in the group to manipulate the contents of
#        the directory.  May be None.
#    packprefix - Prefix used to compose the final package name of the form
#        prefix-YYYY-MM-DD-*.ext".
#    snapdir - Directory where snapshot packages will be placed.
#    keepsnaps - Number of recent packages to retain.
#    archivers - A tuple of archivers used to generate the project packages.
#        Each tuple is a dictionary with the following keys.  The key "name"
#        specifies the name of the directory under "snapdir" into which this
#        archived package will be placed.  The key "dir" is a dictionary
#        describing how to archive a directory into a single package.  The
#        key "file" is a dictionary describing how to archive a single file
#        into a package.  The "dir" and "file" dictionaries contain the
#        following keys.  The key "ext" is the file extension for the
#        generated file.  The key "cmd" is the actual command template which
#        describes how to generate the given archive.  It may contain the
#        meta-tokens @S and @D.  The token @S is replaced with the name of
#        the source directory or file which is being archived, and @D is
#        replaced with the final destination package name.
#------------------------------------------------------------------------------

cvsroot = ":pserver:anonymous@cvs1:/cvsroot/crystal"
fixcvsroot = ":pserver:anonymous@cvs.crystal.sourceforge.net:/cvsroot/crystal"
cvsmodule = "crystal"
moduledir = "CS"
ownergroup = "crystal"
packprefix = "cs-"
snapdir = "/home/groups/ftp/pub/crystal/cvs-snapshots"
keepsnaps = 4

archivers = (
    {"name": "gzip",
     "dir": {"ext": "tgz", "cmd": "tar --create --file=- @S | gzip > @D"},
     "file": {"ext": "gz", "cmd": "gzip --stdout @S > @D"}},
    {"name": "bzip2",
     "dir": {"ext": "tar.bz2", "cmd": "tar --create --file=- @S | bzip2 > @D"},
     "file": {"ext": "bz2", "cmd": "bzip2 --stdout @S > @D"}},
    {"name": "zip",
     "dir": {"ext": "zip", "cmd": "zip -q -r @D @S"},
     "file": {"ext": "zip", "cmd": "zip -q @D @S"}})

#------------------------------------------------------------------------------
# Directory Stack Class
#------------------------------------------------------------------------------
class DirStack:
    stack = []

    def pushdir(self, dir):
        self.stack.append(os.getcwd())
        os.chdir(dir)

    def popdir(self):
        os.chdir(self.stack[-1])
        del self.stack[-1]

#------------------------------------------------------------------------------
# Snapshot Class
#------------------------------------------------------------------------------
class Snapshot:
    def timenow(self):
        return time.asctime(time.gmtime(time.time())) + " UTC"

    def __init__(self):
        self.packtemplate = packprefix + "????-??-??.??????"
        self.packbase = packprefix + time.strftime(
            "%Y-%m-%d.%H%M%S", time.gmtime(time.time()))
        self.linkbase = packprefix + "current-snapshot"
        self.workdir = os.path.join(snapdir, "transient")
        self.diffext = ".diff"
        self.diffname = self.packbase + self.diffext
        self.logdir  = os.path.join(snapdir, "logs")
        self.logext = ".log"
        self.logname = self.packbase + self.logext
        self.logpath = os.path.join(self.logdir, self.logname)
        self.logfile = None
        self.stamppath = os.path.join(self.logdir, "lastrun.timestamp")
        self.timestamp = self.timenow()
        self.hasdiff = None
        self.dirstack = DirStack()

    def log(self, msg):
        s = msg + "\n"
        sys.stdout.write(s)
        if self.logfile:
            self.logfile.write(s)

    def run(self, cmd):
        rc = commands.getstatusoutput(cmd)
        if len(rc[1]) > 0:
            self.log(rc[1])
        return (rc[0] == 0)

    def removefile(self, path):
        if os.path.exists(path):
            os.remove(path)

    def makedirectory(self, path):
        if not os.path.exists(path) :
            os.mkdir(path)
	try:
            os.chmod(path, 0775)
        except Exception, e:
            self.log("Error making directory group writable: " +
                     path + '; reason: ' + str(e))
        if ownergroup:
            try:
                os.chown(path, os.getuid(), grp.getgrnam(ownergroup)[2])
            except Exception, e:
                self.log('Error setting group ownership "' + ownergroup +
                         '" on ' + path + '; reason: ' + str(e))

    def openlog(self):
        if not self.logfile:
            try:
                self.logfile = open(self.logpath, "w")
            except IOError, e:
                self.log("Error opening log file: " + self.logpath + " " +
                         repr(e.args))

    def closelog(self):
        if self.logfile:
            self.logfile.close()
            self.logfile = None

    def writetimestamp(self):
        file = open(self.stamppath, "w")
        file.write(self.timestamp + "\n")
        file.close()

    def readtimestamp(self):
        stamp = None
        if os.path.exists(self.stamppath):
            file = open(self.stamppath, "r")
            stamp = string.strip(file.readline())
        return stamp

    def purge(self, pattern):
        files = glob.glob(pattern)
        blast = len(files) - keepsnaps
        if blast > 0:
            files.sort()
            for i in range(0, blast):
                self.log("Purging old file: " + os.path.basename(files[i]))
                os.remove(files[i])

    def purgeold(self):
        self.purge(os.path.join(self.logdir, self.packtemplate + self.logext))
        for dict in archivers:
            self.purge(os.path.join(snapdir, dict["name"], self.packtemplate +
                                    "." + dict["dir"]["ext"]))
            self.purge(os.path.join(snapdir, dict["name"], self.packtemplate +
                                    self.diffext + "." + dict["file"]["ext"]))

    def purgetransient(self):
        self.dirstack.pushdir(snapdir)
        self.log("Purging working directory: " + self.builddir)
        # Remove our local work directory.
        self.run("rm -rf " + self.builddir)
        # Attempt to remove parent work directory.  This will only succeed if
        # it is empty (i.e. no other snapshots are in progress).
        try:
            os.rmdir(self.workdir)
            self.dirstack.popdir()
        except Exception:
            self.dirstack.popdir()
            pass

    def preparetransient(self):
        tempfile.tempdir = self.workdir
        self.builddir = tempfile.mktemp();
        self.log("Creating working directory: " + self.builddir)
        self.makedirectory(self.workdir)
        self.makedirectory(self.builddir)

    def findcvsdirs(self, dir):
        dirs = []
        rc = commands.getstatusoutput(
            "find " + dir + " -type d -name CVS -print -prune")
        if rc[0] == 0:
            dirs = string.split(rc[1])
        else: # 'find' command returned error.
            if len(rc[1]) > 0:
                self.log(rc[1])
        return dirs

    def stripstickyinfo(self, path): # Strip trailing sticky date or tag.
        file = open(path, "r")
        data = file.read()
        file.close()
        file = open(path, "w")
        file.write(re.sub("(?m)(^/.+)(/[TD].+?)$", "\g<1>/", data))
        file.close()

    def purgestickytags(self, dirs):
        self.log("Removing CVS sticky tags")
        for dir in dirs:
            self.removefile(os.path.join(dir, "Tag"))
            self.removefile(os.path.join(dir, "Entries.Static"))
            self.stripstickyinfo(os.path.join(dir, "Entries"))

    def patchcvsroot(self, dirs):
        if fixcvsroot:
            self.log("Patching CVS/Root entries")
            newroot = fixcvsroot + "\n"
            for dir in dirs:
                try:
                    file = open(os.path.join(dir, "Root"), "w")
                    file.write(newroot)
                    file.close()
                    file = None
                except IOError, e:
                    self.log("Error patching Root in " + dir + " " +
                             repr(e.args))

    def checkout(self, datewanted, outdir):
        self.log("Retrieving module " + cvsmodule + " for " + datewanted)
        self.makedirectory(outdir)
        self.dirstack.pushdir(outdir)
        rc = self.run("cvs -Q -d " + cvsroot + " checkout -D '" + datewanted +
                      "' " + cvsmodule)
        self.dirstack.popdir()
        if rc:
            dirs = self.findcvsdirs(os.path.join(outdir, moduledir))
            if len(dirs) > 0:
                self.patchcvsroot(dirs)
                self.purgestickytags(dirs)
        return rc

    def gendiff(self):
        oldstamp = self.readtimestamp()
        if oldstamp:
            olddir = "old"
            oldpath = os.path.join(self.builddir, "old")
            if self.checkout(oldstamp, oldpath):
                self.log("Generating diff of " + oldstamp + " & " +
                         self.timestamp)
                self.dirstack.pushdir(self.builddir)
                self.run("diff -crN " + os.path.join(olddir, moduledir) +
                         " " + moduledir + " > " + self.diffname)
                self.dirstack.popdir()
                self.hasdiff = 1

    def genpackage(self, dirname, dict, src, dst):
        outdir = os.path.join(snapdir, dirname)
        self.makedirectory(outdir)
        target = os.path.join(outdir, dst + "." + dict["ext"])
        cmd = string.replace(
            string.replace(dict["cmd"], "@S", src), "@D", target)
        return self.run(cmd)

    def genpackages(self):
        self.dirstack.pushdir(self.builddir)
        for dict in archivers:
            name = dict["name"]
            self.log("Generating '" + name + "' packages")
            if self.genpackage(name, dict["dir"], moduledir, self.packbase):
                if self.hasdiff:
                    self.genpackage(name, dict["file"], self.diffname,
                                    self.diffname)
        self.dirstack.popdir()
        self.writetimestamp()
        
    def makelink(self, ext, src, linkname):
        src = src + "." + ext
        linkname = linkname + "." + ext
        self.removefile(linkname)
        os.symlink(src, linkname)
                            
    def makelinks(self):
        for dict in archivers:
            name = dict["name"]
            self.log("Linking to current '" + name + "' packages")
            self.dirstack.pushdir(os.path.join(snapdir, name))
            self.makelink(dict["dir"]["ext"], self.packbase, self.linkbase)
            if self.hasdiff:
                self.makelink(dict["file"]["ext"],
                              self.packbase + self.diffext,
                              self.linkbase + self.diffext)
            self.dirstack.popdir()

    def dobulk(self):
        if self.checkout(self.timestamp, self.builddir):
            self.gendiff()
            self.genpackages()
            self.makelinks()
            self.purgeold()

    def doall(self):
        self.makedirectory(snapdir)
        self.makedirectory(self.logdir)
        self.openlog()
        self.log("BEGIN: " + self.timenow())
        try:
            self.preparetransient()
            os.chdir(self.builddir)
            try:
                self.dobulk()
            except Exception, e:
                self.log("A fatal exception occurred: " + str(e))
            os.chdir(snapdir)
            self.purgetransient()
        finally:
            self.log("END: " + self.timenow())
            self.closelog()

tool = Snapshot()
tool.doall()
