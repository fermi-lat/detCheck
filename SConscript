# -*- python -*-
# $Header: /nfs/slac/g/glast/ground/cvs/detCheck/SConscript,v 1.9 2011/05/22 03:17:29 heather Exp $ 
# Authors: Joanne Bogart <jrb@slac.stanford.edu>
# Version: detCheck-01-07-05
Import('baseEnv')
Import('listFiles')
Import('packages')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

detCheck = libEnv.StaticLibrary('detCheck', ['src/Overlaps.cxx', 'src/SolidStats.cxx', 'src/HepRepSectionsVisitor.cxx'])

progEnv.Tool('detCheckLib')
progEnv_OverlapsObj = progEnv.Object('OverlapsObj-progEnv',['src/Overlaps.cxx'])
progEnv_HepRepSectionsVisitorObj = progEnv.Object('HepRepSectionsVisitorObj-progEnv',['src/HepRepSectionsVisitor.cxx'])
progEnv_SolidStatsObj = progEnv.Object('SolidStatsObj-progEnv', ['src/SolidStats.cxx'])

if baseEnv['PLATFORM'] != 'win32':
    progEnv.AppendUnique(CCFLAGS = ['-Wno-unused-parameter'])

overlapTest = progEnv.Program('overlaps',['src/overlapsMain.cxx'] + [progEnv_OverlapsObj])
heprep = progEnv.Program('heprep',['src/HepRep.cxx'] + [progEnv_HepRepSectionsVisitorObj])

dumpIds = progEnv.Program('dumpIds',['src/dumpIds.cxx'])
summary = progEnv.Program('summary',['src/solidStatsMain.cxx']+[progEnv_SolidStatsObj])
constsDoc = progEnv.Program('constsDoc',['src/constsDocMain.cxx'])


progEnv.Tool('registerTargets', package = 'detCheck',
             staticLibraryCxts = [[detCheck, libEnv]],
	     binaryCxts = [[heprep,progEnv], [dumpIds,progEnv],
                           [summary,progEnv], [constsDoc,progEnv],
                           [overlapTest, progEnv]],
	     includes = listFiles(['detCheck/*.h']))




