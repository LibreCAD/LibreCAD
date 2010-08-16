BUILDING DXFLIB ON WINDOWS WITH MS VC++ 6.0 IDE

This file describes how to build DXFLIB and the test application using
the MicroSoft Visual C++ 6.0 IDE.  I do not believe the project and
workspace files are backward compatible with earlier versions of VC++.
Instructions and files for earlier versions are welcome.  Instructions
and patches for building DXFLIB on Windows with command line tools
(particularly gcc and autoconf) would also be welcome.  Please send
them to rob.campbell@att.net.

The following files should be in the DXFLIB install directory:

dxflib.dsp
    Project file for the dxflib library.  Roughly equivalent to a
    makefile.  Generates dxfd.lib (debug version) and dxf.lib (release
	version).

dxflib_test.dsp
    Project file for the test application.  Generates testd.exe
    (debug version) and test.exe (release version).

dxflib.dsw
    Workspace file.  Loads both projects.

Loading dxflib.dsw should automatically load both projects.  They can
be worked with just like any other Visual C++ project.  dxflib must be
built before dxflib_test can be built.

The VC++ IDE doesn't support environment variables (at least, I haven't
figured out how to use them), so some paths are hardcoded in the
project settings.  You will probably need to change them.

In dxflib.dsp:

Output files: c:\home\rob\lib
    Defined on the General page of the project settings notebook.  Use
	the same value for Debug and Release.

    Change to where you would like dxf.lib and dxfd.lib placed.  The
    directory must be one that is specified on the Directories page of
    the Options dialog ("Tools:Options...", not "Project:Settings...").

In dxflib_test.dsp:

Output files: c:\home\rob\bin
    Defined on the General page of the project settings notebook.  Use
	the same value for Debug and Release.

    Change to where you would like test.exe and testd.exe placed.  The
    directory must be one that is specified on the Directories page of
    the Options dialog ("Tools:Options...", not "Project:Settings...").

Additional include directories: c:\home\rob\
    Defined on the C++ page of the project settings notebook, under the
	Preprocessor category.  It should be set to the dxflib install
	directory, since the #include directives are relative to that.
	
Working directory: c:\home\rob\dxflib\test
    Defined on the Debug page of the project settings notebook.  Should
	be the location of demo.dxf.

