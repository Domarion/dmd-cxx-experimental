
/* Compiler implementation of the D programming language
 * Copyright (C) 1999-2021 by The D Language Foundation, All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 * https://github.com/dlang/dmd/blob/master/src/dmd/globals.h
 */

#pragma once

#include "root/dcompat.hpp"
#include "root/ctfloat.hpp"
#include "root/outbuffer.hpp"
#include "root/filename.hpp"
#include "compiler.hpp"

// Can't include arraytypes.h here, need to declare these directly.
template <typename TYPE> struct Array;

typedef unsigned char Diagnostic;
enum
{
    DIAGNOSTICerror,  // generate an error
    DIAGNOSTICinform, // generate a warning
    DIAGNOSTICoff     // disable diagnostic
};

// The state of array bounds checking
typedef unsigned char CHECKENABLE;
enum
{
    CHECKENABLEdefault, // initial value
    CHECKENABLEoff,     // never do bounds checking
    CHECKENABLEon,      // always do bounds checking
    CHECKENABLEsafeonly // do bounds checking only in @safe functions
};

typedef unsigned char CHECKACTION;
enum
{
    CHECKACTION_D,        // call D assert on failure
    CHECKACTION_C,        // call C assert on failure
    CHECKACTION_halt      // cause program halt on failure
};

enum CPU
{
    x87,
    mmx,
    sse,
    sse2,
    sse3,
    ssse3,
    sse4_1,
    sse4_2,
    avx,                // AVX1 instruction set
    avx2,               // AVX2 instruction set
    avx512,             // AVX-512 instruction set

    // Special values that don't survive past the command line processing
    baseline,           // (default) the minimum capability CPU
    native              // the machine the compiler is being run on
};

enum CppStdRevision
{
    CppStdRevisionCpp17 = 201703
};

// Put command line switches in here
struct Param
{
    bool obj = true;           // write object file
    bool link = true;          // perform link
    bool dll;           // generate shared dynamic library
    bool lib;           // write library file instead of object file(s)
    bool multiobj;      // break one object file into multiple ones
    bool oneobj;        // write one object file instead of multiple ones
    bool trace;         // insert profiling hooks
    bool verbose;       // verbose compile
    bool vcg_ast;       // write-out codegen-ast
    bool showColumns;   // print character (column) numbers in diagnostics
    bool vtls;          // identify thread local variables
    char vgc;           // identify gc usage
    bool vfield;        // identify non-mutable field variables
    bool vcomplex = true;      // identify complex/imaginary type usage
    char symdebug;      // insert debug symbolic information
    bool symdebugref;   // insert debug information for all referenced types, too
    bool alwaysframe;   // always emit standard stack frame
    bool optimize;      // run optimizer
    bool map;           // generate linker .map file
    bool is64bit = (sizeof(size_t) == 8);       // generate 64 bit code
    bool isLP64;        // generate code for LP64
    bool isLinux = true;       // generate code for linux
    Diagnostic useDeprecated = DIAGNOSTICinform;
    bool stackstomp;    // add stack stomping code
    bool useUnitTests;  // generate unittest code
    bool useInline = false;     // inline expand functions
    bool useDIP25;      // implement http://wiki.dlang.org/DIP25
    bool release;       // build release version
    bool preservePaths; // true means don't strip path from source file
    Diagnostic warnings = DIAGNOSTICoff;
    bool pic;           // generate position-independent-code for shared libs
    bool color;         // use ANSI colors in console output
    bool cov;           // generate code coverage data
    unsigned char covPercent;   // 0..100 code coverage percentage required
    bool nofloat;       // code should not pull in floating point support
    bool ignoreUnsupportedPragmas;      // rather than error on them
    bool enforcePropertySyntax;
    bool useModuleInfo = true; // generate runtime module information
    bool useTypeInfo = true;   // generate runtime type information
    bool useExceptions = true; // support exception handling
    bool betterC;       // be a "better C" compiler; no dependency on D runtime
    bool addMain;       // add a default main() function
    bool allInst;       // generate code for all template instantiations
    bool vsafe;         // use enhanced @safe checking
    unsigned cplusplus = CppStdRevisionCpp17;     // version of C++ name mangling to support
    bool showGaggedErrors;  // print gagged errors anyway

    CPU cpu;                // CPU instruction set to target

    CHECKENABLE useInvariants = CHECKENABLEdefault;     // generate class invariant checks
    CHECKENABLE useIn = CHECKENABLEdefault;             // generate precondition checks
    CHECKENABLE useOut = CHECKENABLEdefault;            // generate postcondition checks
    CHECKENABLE useArrayBounds = CHECKENABLEdefault;    // when to generate code for array bounds checks
    CHECKENABLE useAssert = CHECKENABLEdefault;         // when to generate code for assert()'s
    CHECKENABLE useSwitchError = CHECKENABLEdefault;    // check for switches without a default
    CHECKENABLE boundscheck = CHECKENABLEdefault;       // state of -boundscheck switch

    CHECKACTION checkAction = CHECKACTION_D;       // action to take when bounds, asserts or switch defaults are violated

    unsigned errorLimit = 20;

    DString  argv0;    // program name
    Strings modFileAliasStrings; // array of char*'s of -I module filename alias strings
    Strings *imppath;     // array of char*'s of where to look for import modules
    Strings *fileImppath; // array of char*'s of where to look for file import modules
    DString objdir;    // .obj/.lib file output directory
    DString objname;   // .obj file output name
    DString libname;   // .lib file output name

    bool doDocComments;  // process embedded documentation comments
    DString docdir;      // write documentation file to docdir directory
    DString docname;     // write documentation file to docname
    Strings ddocfiles;  // macro include files for Ddoc

    bool doHdrGeneration;  // process embedded documentation comments
    DString hdrdir;        // write 'header' file to docdir directory
    DString hdrname;       // write 'header' file to docname
    bool hdrStripPlainFunctions; // strip the bodies of plain (non-template) functions

    bool doJsonGeneration;    // write JSON file
    DString jsonfilename;     // write JSON file to jsonfilename

    Strings *debugids;     // debug identifiers

    Strings *versionids;   // version identifiers

    DString defaultlibname = "libphobos2.a";     // default library for non-debug builds
    DString debuglibname;       // default library for debug builds
    DString mscrtlib;           // MS C runtime library

    DString moduleDepsFile;     // filename for deps output
    OutBuffer *moduleDeps;      // contents to be written to deps file

    // Hidden debug switches
    bool debugb;
    bool debugc;
    bool debugf;
    bool debugr;
    bool debugx;
    bool debugy;

    bool run;           // run resulting executable
    Strings runargs;    // arguments for executable

    // Linker stuff
    Strings objfiles;
    Strings linkswitches;
    Strings libfiles;
    Strings dllfiles;
    DString deffile;
    DString resfile;
    DString exefile;
    DString mapfile;
};

typedef unsigned structalign_t;
// magic value means "match whatever the underlying C compiler does"
// other values are all powers of 2
#define STRUCTALIGN_DEFAULT ((structalign_t) ~0)

struct Global
{
    DString inifilename = nullptr;
    DString mars_ext = "d";
    DString obj_ext = "o";
    DString lib_ext = "a";
    DString dll_ext = "so";
    DString doc_ext = "html";            // for Ddoc generated files
    DString ddoc_ext = "ddoc";           // for Ddoc macro include files
    DString hdr_ext = "di";            // for D 'header' import files
    DString cxxhdr_ext;         // for C/C++ 'header' files
    DString json_ext = "json";           // for JSON files
    DString map_ext = "map";            // for .map files
    bool run_noext = true;             // allow -run sources without extensions.

    DString copyright = "Copyright (C) 1999-2021 by The D Language Foundation, All Rights Reserved";
    DString written = "written by Walter Bright";
    const char *main_d = "__main.d";         // dummy filename for dummy main()
    Strings *path;        // Array of char*'s which form the import lookup path
    Strings *filePath;    // Array of char*'s which form the file import lookup path

    DString version = "v"
        #include "verstr.hpp"
            ;         // Compiler version string
    DString vendor = "Digital Mars D";          // Compiler backend name

    Param params;
    unsigned errors;       // number of errors reported so far
    unsigned warnings;     // number of warnings reported so far
    FILE *stdmsg = stdout;          // where to send verbose messages
    unsigned gag;          // !=0 means gag reporting of errors & warnings
    unsigned gaggedErrors; // number of errors reported while gagged
    unsigned gaggedWarnings; // number of warnings reported while gagged

    void* console;         // opaque pointer to console for controlling text attributes

    Array<class Identifier*>* versionids; // command line versions and predefined versions
    Array<class Identifier*>* debugids;   // command line debug versions and predefined versions

    enum { recursionLimit = 500 }; // number of recursive template expansions before abort

    /* Start gagging. Return the current number of gagged errors
     */
    unsigned startGagging();

    /* End gagging, restoring the old gagged state.
     * Return true if errors occurred while gagged.
     */
    bool endGagging(unsigned oldGagged);

    /*  Increment the error count to record that an error
     *  has occurred in the current context. An error message
     *  may or may not have been printed.
     */
    void increaseErrorCount();
};

extern Global global;

// Be careful not to care about sign when using dinteger_t
// use this instead of integer_t to
// avoid conflicts with system #include's
using dinteger_t = uint64_t;
// Signed and unsigned variants
using sinteger_t = int64_t;
using uinteger_t = uint64_t;

using d_int8 = int8_t;
using d_uns8 = uint8_t;
using d_int16 = int16_t;
using d_uns16 = uint16_t;
using d_int32 = int32_t;
using d_uns32 = uint32_t;
using d_int64 = int64_t;
using d_uns64 = uint64_t;

// file location
struct Loc
{
    const char *filename; // either absolute or relative to cwd
    unsigned linnum;
    unsigned charnum;

    Loc()
    {
        linnum = 0;
        charnum = 0;
        filename = nullptr;
    }

    Loc(const char *filename, unsigned linnum, unsigned charnum);

    const char *toChars() const;
    bool equals(const Loc& loc);
};

enum LINK
{
    LINKdefault,
    LINKd,
    LINKc,
    LINKcpp,
    LINKsystem
};

enum CPPMANGLE
{
    CPPMANGLEdefault,
    CPPMANGLEstruct,
    CPPMANGLEclass
};

enum MATCH
{
    MATCHnomatch,       // no match
    MATCHconvert,       // match with conversions
    MATCHconst,         // match with conversion to const
    MATCHexact          // exact match
};

enum PINLINE
{
    PINLINEdefault,      // as specified on the command line
    PINLINEnever,        // never inline
    PINLINEalways        // always inline
};

typedef uinteger_t StorageClass;
