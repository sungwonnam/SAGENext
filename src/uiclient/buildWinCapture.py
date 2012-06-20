############################################################################
#
# SAGE Pointer - Captures your cursor and allows you to control SAGE
#              - Shares your desktop on a SAGE display (need VNC server running)
#              - Drag-and-drop multimedia files to display on a SAGE display
#
# Copyright (C) 2010 Electronic Visualization Laboratory,
# University of Illinois at Chicago
#
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following disclaimer
#    in the documentation and/or other materials provided with the distribution.
#  * Neither the name of the University of Illinois at Chicago nor
#    the names of its contributors may be used to endorse or promote
#    products derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Direct questions, comments etc about SAGE UI to www.evl.uic.edu/cavern/forum
#
# Author: Ratko Jagodic
#        
############################################################################

####  this is used for building on Windows
####  without this we'll get some ugly looking controls


manifest = """
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity
    version="5.0.0.0"
    processorArchitecture="x86"
    name="%(prog)s"
    type="win32"
  />
  <description>%(prog)s</description>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel
            level="asInvoker"
            uiAccess="false">
        </requestedExecutionLevel>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <dependency>
    <dependentAssembly>
      <assemblyIdentity
            type="win32"
            name="Microsoft.VC90.CRT"
            version="9.0.21022.8"
            processorArchitecture="x86"
            publicKeyToken="1fc8b3b9a1e18e3b">
      </assemblyIdentity>
    </dependentAssembly>
  </dependency>
  <dependency>
    <dependentAssembly>
        <assemblyIdentity
            type="win32"
            name="Microsoft.Windows.Common-Controls"
            version="6.0.0.0"
            processorArchitecture="X86"
            publicKeyToken="6595b64144ccf1df"
            language="*"
        />
    </dependentAssembly>
  </dependency>
</assembly>
"""
            
#### end windows crap




VERSION = "1.3.1"
NAME = "Sage Pointer"
DESCRIPTION = "SAGE Pointer v"+str(VERSION)

import sys, glob



#----------------------     MAC    ---------------------------
# 
# DEPENDENCIES:  python2.5, wxPython, py2app
#
# MAKE BINARY WITH:  run "GO" script 
#

if sys.platform == 'darwin':
    import py2app
    from setuptools import setup
    
    # A custom plist for letting it associate with all files.  LSItemContentTypes
    myPlist = dict( CFBundleDocumentTypes = [dict(CFBundleTypeExtensions=["*"], CFBundleTypeRole="Viewer"),],
                    CFBundleURLTypes = [dict(CFBundleURLSchemes=["http"], CFBundleTypeRole="Viewer"),],
                    )

    opts = dict(dist_dir="sagePointer",
                iconfile="sagePointerIcon.icns", plist=myPlist)

    setup(app=['sagePointer.py'],
	  setup_requires=["py2app"],
          name = NAME,
          version = str(VERSION),
	  options=dict(py2app=opts),
          data_files = [(".", ["splash_small.png", "macCapture"])]
	  )
	  


#--------------------     WINDOWS   -------------------------
# 
# DEPENDENCIES:  python, wxPython, py2exe, pyWin32, pyHook
# wxPython isn't needed for SAGENext
#
# MAKE BINARY WITH: run "GO" script
#

elif sys.platform == 'win32':
    import py2exe
    from distutils.core import setup, Distribution
    opts = dict(dist_dir="winCapture")
    setup(windows=[{"script":"winCapture.py",
                    "icon_resources":[(1, "sagePointerIcon.ico")],
                    "other_resources": [(24,1,manifest)],
                    "dll_excludes":[ "mswsock.dll", "powrprof.dll" ],
                    }],
	  name = NAME,
	  version = str(VERSION),
	  description = DESCRIPTION,
	  options=dict(py2exe=opts),
          data_files = [(".", ["splash_small.png", "gdiplus.dll", "msvcp90.dll", "msvcr90.dll"])]
	  )
	  



