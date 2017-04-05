//
//Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.
//

//
// Implement the top-level of interface to the compiler/linker,
// as defined in ShaderLang.h
//
#include "SymbolTable.h"
#include "ParseHelper.h"
#include "../Include/ShHandle.h"
#include "Initialisation.h"

#define SH_EXPORTING
#include "../Public/ShaderLangExt.h"

#include "Include/ResourceLimits.h"
#include "Initialize.h"

extern "C" int InitPreprocessor(void);
extern "C" int FinalizePreprocessor(void);
extern void SetGlobalPoolAllocatorPtr(TPoolAllocator* poolAllocator);

bool generateBuiltInSymbolTable(const TBuiltInResource* resources, TInfoSink&, TSymbolTable*, EShLanguage language = EShLangCount);
bool initializeSymbolTable(TBuiltInStrings* BuiltInStrings, EShLanguage language, TInfoSink& infoSink, const TBuiltInResource *resources, TSymbolTable*);

//
// A symbol table for each language.  Each has a different
// set of built-ins, and we want to preserve that from
// compile to compile.
//
TSymbolTable SymbolTables[EShLangCount];

TPoolAllocator* PerProcessGPA = 0;
//
// This is the platform independent interface between an OGL driver
// and the shading language compiler/linker.
//

//
// Driver must call this first, once, before doing any other
// compiler/linker operations.
//
int ShInitialize()
{
    TInfoSink infoSink;
    bool ret = true;

    if (!InitProcess())
        return 0;

    // This method should be called once per process. If its called by multiple threads, then 
    // we need to have thread synchronization code around the initialization of per process
    // global pool allocator
    if (!PerProcessGPA) { 
        TPoolAllocator *builtInPoolAllocator = new TPoolAllocator(true);
        builtInPoolAllocator->push();
        TPoolAllocator* gPoolAllocator = &GlobalPoolAllocator;
        SetGlobalPoolAllocatorPtr(builtInPoolAllocator);

        TSymbolTable symTables[EShLangCount];
        generateBuiltInSymbolTable(0, infoSink, symTables);

        PerProcessGPA = new TPoolAllocator(true);
        PerProcessGPA->push();
        SetGlobalPoolAllocatorPtr(PerProcessGPA);

        SymbolTables[EShLangVertex].copyTable(symTables[EShLangVertex]);
        SymbolTables[EShLangFragment].copyTable(symTables[EShLangFragment]);

        SetGlobalPoolAllocatorPtr(gPoolAllocator);

        symTables[EShLangVertex].pop();
        symTables[EShLangFragment].pop();

        builtInPoolAllocator->popAll();
        delete builtInPoolAllocator;        

    }

    return ret ? 1 : 0;
}

//
// Driver calls these to create and destroy compiler/linker
// objects.
//

ShHandle ShConstructCompiler(const EShLanguage language, int debugOptions)
{
    if (!InitThread())
        return 0;

    TShHandleBase* base = static_cast<TShHandleBase*>(ConstructCompiler(language, debugOptions));
    
    return reinterpret_cast<void*>(base);
}

ShHandle ShConstructLinker(const EShExecutable executable, int debugOptions)
{
    if (!InitThread())
        return 0;

    TShHandleBase* base = static_cast<TShHandleBase*>(ConstructLinker(executable, debugOptions));

    return reinterpret_cast<void*>(base);
}

ShHandle ShConstructUniformMap()
{
    if (!InitThread())
        return 0;

    TShHandleBase* base = static_cast<TShHandleBase*>(ConstructUniformMap());

    return reinterpret_cast<void*>(base);
}

void ShDestruct(ShHandle handle)
{
    if (handle == 0)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);

    if (base->getAsCompiler())
        DeleteCompiler(base->getAsCompiler());
    else if (base->getAsLinker())
        DeleteLinker(base->getAsLinker());
    else if (base->getAsUniformMap())
        DeleteUniformMap(base->getAsUniformMap());
}

//
// Cleanup symbol tables
//
int __fastcall ShFinalize()
{  
  if (PerProcessGPA) {
    PerProcessGPA->popAll();
    delete PerProcessGPA;
  }
  return 1;
}

//
// This function should be called only once by the Master Dll. Currently, this is being called for each thread 
// which is incorrect. This is required to keep the Sh interface working for now and will eventually be called 
// from master dll once.
//
bool generateBuiltInSymbolTable(const TBuiltInResource* resources, TInfoSink& infoSink, TSymbolTable* symbolTables, EShLanguage language)
{
    TBuiltIns builtIns;
    
    if (resources) {
        builtIns.initialize(*resources);
        initializeSymbolTable(builtIns.getBuiltInStrings(), language, infoSink, resources, symbolTables);
    } else {
        builtIns.initialize();
        initializeSymbolTable(builtIns.getBuiltInStrings(), EShLangVertex, infoSink, resources, symbolTables);
        initializeSymbolTable(builtIns.getBuiltInStrings(), EShLangFragment, infoSink, resources, symbolTables);
    }

    return true;
}

bool initializeSymbolTable(TBuiltInStrings* BuiltInStrings, EShLanguage language, TInfoSink& infoSink, const TBuiltInResource* resources, TSymbolTable* symbolTables)
{
    TIntermediate intermediate(infoSink);
    TSymbolTable* symbolTable;
    
    if (resources)
        symbolTable = symbolTables;
    else
        symbolTable = &symbolTables[language];

    TParseContext parseContext(*symbolTable, intermediate, language, infoSink);

    GlobalParseContext = &parseContext;
    
    setInitialState();

    assert (symbolTable->isEmpty() || symbolTable->atSharedBuiltInLevel());
       
    //
    // Parse the built-ins.  This should only happen once per
    // language symbol table.
    //
    // Push the symbol table to give it an initial scope.  This
    // push should not have a corresponding pop, so that built-ins
    // are preserved, and the test for an empty table fails.
    //

    symbolTable->push();
    
    //Initialize the Preprocessor
    int ret = InitPreprocessor();
    if (ret) {
        infoSink.info.message(EPrefixInternalError,  "Unable to intialize the Preprocessor");
        return false;
    }
    
    for (TBuiltInStrings::iterator i  = BuiltInStrings[parseContext.language].begin();
                                    i != BuiltInStrings[parseContext.language].end();
                                    ++i) {
        const char* builtInShaders[1];
        int builtInLengths[1];

        builtInShaders[0] = (*i).c_str();
        builtInLengths[0] = (int) (*i).size();

        if (PaParseStrings(const_cast<char**>(builtInShaders), builtInLengths, 1, parseContext) != 0) {
            infoSink.info.message(EPrefixInternalError, "Unable to parse built-ins");
            return false;
        }
    }

    if (resources) {
        IdentifyBuiltIns(parseContext.language, *symbolTable, *resources);
    } else {                                       
        IdentifyBuiltIns(parseContext.language, *symbolTable);
    }

    FinalizePreprocessor();

    return true;
}


//
// Do an actual compile on the given strings.  The result is left 
// in the given compile object.
//
// Return:  The return value of ShCompile is really boolean, indicating
// success or failure.
//
int ShCompile(
    const ShHandle handle,
    const char* const shaderStrings[],
    const int numStrings,
    const EShOptimizationLevel optLevel,
    const TBuiltInResource* resources,
    int debugOptions
    )
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (compiler == 0)
        return 0;
    
    GlobalPoolAllocator.push();
    compiler->infoSink.info.erase();
    compiler->infoSink.debug.erase();

    if (numStrings == 0)
        return 1;

    TIntermediate intermediate(compiler->infoSink);
    TSymbolTable symbolTable(SymbolTables[compiler->getLanguage()]);
    
    generateBuiltInSymbolTable(resources, compiler->infoSink, &symbolTable, compiler->getLanguage());

    TParseContext parseContext(symbolTable, intermediate, compiler->getLanguage(), compiler->infoSink);
    parseContext.initializeExtensionBehavior();

    GlobalParseContext = &parseContext;
    
    setInitialState();

    InitPreprocessor();    
    //
    // Parse the application's shaders.  All the following symbol table
    // work will be throw-away, so push a new allocation scope that can
    // be thrown away, then push a scope for the current shader's globals.
    //
    bool success = true;
    
    symbolTable.push();
    if (!symbolTable.atGlobalLevel())
        parseContext.infoSink.info.message(EPrefixInternalError, "Wrong symbol table level");

    if (parseContext.insertBuiltInArrayAtGlobalLevel())
        success = false;

    int ret = PaParseStrings(const_cast<char**>(shaderStrings), 0, numStrings, parseContext);
    if (ret)
        success = false;

    if (success && parseContext.treeRoot) {
        if (optLevel == EShOptNoGeneration)
            parseContext.infoSink.info.message(EPrefixNone, "No errors.  No code generation or linking was requested.");
        else {
            success = intermediate.postProcess(parseContext.treeRoot, parseContext.language);

            if (success) {

                if (debugOptions & EDebugOpIntermediate)
                    intermediate.outputTree(parseContext.treeRoot);

                //
                // Call the machine dependent compiler
                //
                if (! compiler->compile(parseContext.treeRoot))
                    success = false;
            }
        }
    } else if (!success) {
        parseContext.infoSink.info.prefix(EPrefixError);
        parseContext.infoSink.info << parseContext.numErrors << " compilation errors.  No code generated.\n\n";
        success = false;
        if (debugOptions & EDebugOpIntermediate)
            intermediate.outputTree(parseContext.treeRoot);
    }

    intermediate.remove(parseContext.treeRoot);

    //
    // Ensure symbol table is returned to the built-in level,
    // throwing away all but the built-ins.
    //
    while (! symbolTable.atSharedBuiltInLevel())
        symbolTable.pop();

    FinalizePreprocessor();
    //
    // Throw away all the temporary memory used by the compilation process.
    //
    GlobalPoolAllocator.pop();

    return success ? 1 : 0;
}

//
// Do an actual link on the given compile objects.
//
// Return:  The return value of is really boolean, indicating
// success or failure.
//
int ShLink(
    const ShHandle linkHandle,
    const ShHandle compHandles[],
    const int numHandles,
    ShHandle uniformMapHandle,
    short int** uniformsAccessed,
    int* numUniformsAccessed)

{
    if (!InitThread())
        return 0;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(linkHandle);
    TLinker* linker = static_cast<TLinker*>(base->getAsLinker());
    if (linker == 0)
        return 0;

    int returnValue;
    GlobalPoolAllocator.push();
    returnValue = ShLinkExt(linkHandle, compHandles, numHandles);
    GlobalPoolAllocator.pop();

    if (returnValue)
        return 1;

    return 0;
}
//
// This link method will be eventually used once the ICD supports the new linker interface
//
int ShLinkExt(
    const ShHandle linkHandle,
    const ShHandle compHandles[],
    const int numHandles)
{
    if (linkHandle == 0 || numHandles == 0)
        return 0;

    THandleList cObjects;

    {// support MSVC++6.0
        for (int i = 0; i < numHandles; ++i) {
            if (compHandles[i] == 0)
                return 0;
            TShHandleBase* base = reinterpret_cast<TShHandleBase*>(compHandles[i]);
            if (base->getAsLinker()) {
                cObjects.push_back(base->getAsLinker());
            }
            if (base->getAsCompiler())
                cObjects.push_back(base->getAsCompiler());
    
    
            if (cObjects[i] == 0)
                return 0;
        }
    }

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(linkHandle);
    TLinker* linker = static_cast<TLinker*>(base->getAsLinker());

    if (linker == 0)
        return 0;

    linker->infoSink.info.erase();

    {// support MSVC++6.0
        for (int i = 0; i < numHandles; ++i) {
            if (cObjects[i]->getAsCompiler()) {
                if (! cObjects[i]->getAsCompiler()->linkable()) {
                    linker->infoSink.info.message(EPrefixError, "Not all shaders have valid object code.");                
                    return 0;
                }
            }
        }
    }

    bool ret = linker->link(cObjects);

    return ret ? 1 : 0;
}

//
// ShSetEncrpytionMethod is a place-holder for specifying
// how source code is encrypted.
//
void ShSetEncryptionMethod(ShHandle handle)
{
    if (handle == 0)
        return;
}

//
// Return any compiler/linker/uniformmap log of messages for the application.
//
const char* ShGetInfoLog(const ShHandle handle)
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TInfoSink* infoSink;

    if (base->getAsCompiler())
        infoSink = &(base->getAsCompiler()->getInfoSink());
    else if (base->getAsLinker())
        infoSink = &(base->getAsLinker()->getInfoSink());

    infoSink->info << infoSink->debug.c_str();
    return infoSink->info.c_str();
}

//
// Return the resulting binary code from the link process.  Structure
// is machine dependent.
//
const void* ShGetExecutable(const ShHandle handle)
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    
    TLinker* linker = static_cast<TLinker*>(base->getAsLinker());
    if (linker == 0)
        return 0;

    return linker->getObjectCode();
}

//
// Let the linker know where the application said it's attributes are bound.
// The linker does not use these values, they are remapped by the ICD or
// hardware.  It just needs them to know what's aliased.
//
// Return:  The return value of is really boolean, indicating
// success or failure.
//
// This is to preserve the old linker API, P20 code can use the generic 
// ShConstructBinding() and ShAddBinding() APIs
//
int ShSetVirtualAttributeBindings(const ShHandle handle, const ShBindingTable* table)
{    
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TLinker* linker = static_cast<TLinker*>(base->getAsLinker());

    if (linker == 0)
        return 0;
   
    linker->setAppAttributeBindings(table);

    return 1;
}

//
// Let the linker know where the predefined attributes have to live.
// This is to preserve the old linker API, P20 code can use the generic 
// ShConstructBinding() and ShAddBinding() APIs
//
int ShSetFixedAttributeBindings(const ShHandle handle, const ShBindingTable* table)
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TLinker* linker = static_cast<TLinker*>(base->getAsLinker());

    if (linker == 0)
        return 0;

    linker->setFixedAttributeBindings(table);
    return 1;
}

//
// Some attribute locations are off-limits to the linker...
//
int ShExcludeAttributes(const ShHandle handle, int *attributes, int count)
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TLinker* linker = static_cast<TLinker*>(base->getAsLinker());
    if (linker == 0)
        return 0;

    linker->setExcludedAttributes(attributes, count);

    return 1;
}

//
// Return the index for OpenGL to use for knowing where a uniform lives.
//
// Return:  The return value of is really boolean, indicating
// success or failure.
//
// We dont have to change this code for now since the TUniformMap being
// passed back to ICD by the linker is the same as being used for the old P10 linker
//
int ShGetUniformLocation(const ShHandle handle, const char* name)
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return -1;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TUniformMap* uniformMap= base->getAsUniformMap();
    if (uniformMap == 0)
        return -1;

    return uniformMap->getLocation(name);
}
