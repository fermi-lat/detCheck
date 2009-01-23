# -*- python -*-
# $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/detCheck/SConscript,v 1.1 2008/08/15 21:22:46 ecephas Exp $ 
# Authors: Joanne Bogart <jrb@slac.stanford.edu>
# Version: detCheck-01-07-00
Import('baseEnv')
Import('listFiles')
Import('packages')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('detCheckLib', depsOnly = 1)
#libEnv_OverlapsObj = libEnv.StaticObject('OverlapsObj-libEnv',['src/Overlaps.cxx'])
#libEnv_HepRepSectionsVisitorObj = libEnv.StaticObject('libEnv_HepRepSectionsVisitorObj',['src/HepRepSectionsVisitor.cxx'])
#detCheck = libEnv.StaticLibrary('detCheck',  [libEnv_OverlapsObj, libEnv_HepRepSectionsVisitorObj])

detCheck = libEnv.StaticLibrary('detCheck', ['src/Overlaps.cxx', 'src/SolidStats.cxx', 'src/HepRepSectionsVisitor.cxx'])

progEnv.Tool('detCheckLib')
progEnv_OverlapsObj = progEnv.Object('OverlapsObj-progEnv',['src/Overlaps.cxx'])
progEnv_HepRepSectionsVisitorObj = progEnv.Object('HepRepSectionsVisitorObj-progEnv',['src/HepRepSectionsVisitor.cxx'])
progEnv_SolidStatsObj = progEnv.Object('SolidStatsObj-progEnv', ['src/SolidStats.cxx'])

if baseEnv['PLATFORM'] != 'win32':
	progEnv.AppendUnique(CCFLAGS = ['-Wno-unused-parameter'])

test = progEnv.Program('test_',['src/overlapsMain.cxx'] + [progEnv_OverlapsObj])
heprep = progEnv.Program('heprep',['src/HepRep.cxx'] + [progEnv_HepRepSectionsVisitorObj])

dumpIds = progEnv.Program('dumpIds',['src/dumpIds.cxx'])
summary = progEnv.Program('summary',['src/solidStatsMain.cxx']+[progEnv_SolidStatsObj])
constsDoc = progEnv.Program('constsDoc',['src/constsDocMain.cxx'])


progEnv.Tool('registerObjects', package = 'detCheck', libraries = [detCheck],
	     testApps = [test],
	     binaries = [heprep,dumpIds,summary,constsDoc],
	     includes = listFiles(['detCheck/*.h']))



