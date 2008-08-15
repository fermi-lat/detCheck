# -*- python -*-
# $Header$ 
# Authors: Joanne Bogart <jrb@slac.stanford.edu>
# Version: detCheck-01-07-00
Import('baseEnv')
Import('listFiles')
Import('packages')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('detCheckLib', depsOnly = 1)
libEnv_OverlapsObj = libEnv.StaticObject('OverlapsObj-libEnv',['src/Overlaps.cxx'])
libEnv_HepRepSectionsVisitorObj = libEnv.StaticObject('libEnv_HepRepSectionsVisitorObj',['src/HepRepSectionsVisitor.cxx'])
detCheck = libEnv.StaticLibrary('detCheck',  [libEnv_OverlapsObj, libEnv_HepRepSectionsVisitorObj])

progEnv.Tool('detCheckLib')
progEnv_OverlapsObj = progEnv.Object('OverlapsObj-progEnv',['src/Overlaps.cxx'])
progEnv_HepRepSectionsVisitorObj = progEnv.Object('HepRepSectionsVisitorObj-progEnv',['src/HepRepSectionsVisitor.cxx'])

if baseEnv['PLATFORM'] != 'win32':
	progEnv.AppendUnique(CCFLAGS = ['-Wno-unused-parameter'])

test = progEnv.Program('test_',['src/overlapsMain.cxx']+[progEnv_OverlapsObj])
heprep = progEnv.Program('heprep',['src/HepRep.cxx']+[progEnv_HepRepSectionsVisitorObj])
dumpIds = progEnv.Program('dumpIds',['src/dumpIds.cxx'])
summary = progEnv.Program('summary',['src/solidStatsMain.cxx']+['src/SolidStats.cxx'])
constsDoc = progEnv.Program('constsDoc',['src/constsDocMain.cxx'])


progEnv.Tool('registerObjects', package = 'detCheck', libraries = [detCheck],testApps = [test], binaries = [heprep,dumpIds,summary,constsDoc],includes = listFiles(['detCheck/*.h']))
