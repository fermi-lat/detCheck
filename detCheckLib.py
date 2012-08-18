# $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/detCheck/detCheckLib.py,v 1.3 2009/01/23 00:21:06 ecephas Exp $
def generate(env, **kw):
    if not kw.get('depsOnly', 0):
        env.Tool('addLibrary', library = ['detCheck'])
        if env['PLATFORM'] == "win32" and env.get('CONTAINERNAME','') == 'GlastRelease':
            env.Tool('findPkgPath', package = 'detCheck') 
            env.Tool('findPkgPath', package = 'detModel') 
            env.Tool('findPkgPath', package = 'xmlBase') 
            env.Tool('findPkgPath', package = 'facilities') 
    env.Tool('addLibrary', library = env['xercesLibs'])
    env.Tool('detModelLib')
    env.Tool('identsLib')
    if kw.get('incsOnly', 0) == 1: 
        env.Tool('findPkgPath', package = 'facilities')        
        env.Tool('findPkgPath', package = 'idents')        
        env.Tool('findPkgPath', package = 'xmlUtil')        
        env.Tool('findPkgPath', package = 'xmlBase')        
        env.Tool('findPkgPath', package = 'detModel')        

def exists(env):
    return 1;
