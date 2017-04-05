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

#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED

#include "../Include/Common.h"
#include "../Include/BaseTypes.h"

//
// Need to have association of line numbers to types in a list for building structs.
//
class TType;
struct TTypeLine {
    TType* type;
    int line;
};
typedef TVector<TTypeLine> TTypeList;

inline TTypeList* NewPoolTTypeList()
{
	void* memory = GlobalPoolAllocator.allocate(sizeof(TTypeList));
	return new(memory) TTypeList;
}

//
// This is a workaround for a problem with the yacc stack,  It can't have
// types that the compiler thinks non-trivial constructors.  It should 
// just be used while recognizing the grammar, not anything else.  Pointers
// could be used, but also trying to avoid lots of memory management overhead.
//
// Not as bad as it looks, there is no actual assumption that the fields
// match up or are name the same or anything like that.
//
class TPublicType {
public:
    TBasicType type;
    TQualifier qualifier;
    int size;          // size of vector or matrix, not size of array
    bool matrix;
    bool array;
    TType* userDef;
    int line;
};

typedef std::map<TTypeList*, TTypeList*> TStructureMap;
typedef std::map<TTypeList*, TTypeList*>::iterator TStructureMapIterator;
//
// Base class for things that have a type.
//
class TType {
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)
    explicit TType(TBasicType t, TQualifier q = EvqTemporary, int s = 1, bool m = false, bool a = false) :
                            type(t), qualifier(q), size(s), matrix(m), array(a), arraySize(0), structure(0),
                            structureSize(0), maxArraySize(0), arrayInformationType(0), fieldName(0), typeName(0), mangled(0)
                            { }
    explicit TType(TPublicType p) :  
                            type(p.type), qualifier(p.qualifier), size(p.size), matrix(p.matrix), array(p.array), arraySize(0), 
                            structure(0), structureSize(0), maxArraySize(0), arrayInformationType(0), fieldName(0), typeName(0), mangled(0)
                            {
                              if (p.userDef) {
                                  structure = p.userDef->getStruct();
                                  structureSize = setStructSize(p.userDef->getStruct());
                                  typeName = NewPoolTString(p.userDef->getTypeName().c_str());
                              }
                            }
    explicit TType(TTypeList* userDef, TString n) : 
                            type(EbtStruct), qualifier(EvqTemporary), size(1), matrix(false), array(false), arraySize(0),
                            structure(userDef), maxArraySize(0), arrayInformationType(0), fieldName(0), mangled(0) {
                                structureSize = setStructSize(userDef);
								typeName = NewPoolTString(n.c_str());
                            }
	explicit TType() {}
    virtual ~TType() {}

	TType (const TType& type) { *this = type; }
    
	void copyType(const TType& copyOf, TStructureMap& remapper)
	{
		type = copyOf.type;
		qualifier = copyOf.qualifier;
		size = copyOf.size;
		matrix = copyOf.matrix;
		array = copyOf.array;
		arraySize = copyOf.arraySize;
		
		TStructureMapIterator iter;
		if (copyOf.structure) {
	        if ((iter = remapper.find(structure)) == remapper.end()) {
				// create the new structure here
				structure = NewPoolTTypeList();
				for (unsigned int i = 0; i < copyOf.structure->size(); ++i) {
					TTypeLine typeLine;
					typeLine.line = (*copyOf.structure)[i].line;
					typeLine.type = (*copyOf.structure)[i].type->clone(remapper);
					structure->push_back(typeLine);
				}
			} else {
				structure = iter->second;
			}
		} else
			structure = 0;

		fieldName = 0;
		if (copyOf.fieldName)
			fieldName = NewPoolTString(copyOf.fieldName->c_str());
		typeName = 0;
		if (copyOf.typeName)
			typeName = NewPoolTString(copyOf.typeName->c_str());
		
		mangled = 0;
		if (copyOf.mangled)
			mangled = NewPoolTString(copyOf.mangled->c_str());

		structureSize = copyOf.structureSize;
		maxArraySize = copyOf.maxArraySize;
		assert (copyOf.arrayInformationType == 0);
		arrayInformationType = 0; // arrayInformationType should not be set for builtIn symbol table level
	}

	TType* clone(TStructureMap& remapper)
	{
		TType *newType = new TType();
		newType->copyType(*this, remapper);

		return newType;
	}
    
    int setStructSize(TTypeList* userDef)
    {
        int stSize = 0;
        for (TTypeList::iterator tl = userDef->begin(); tl != userDef->end(); tl++) {
            if (((*tl).type)->isArray()) { 
				if (((*tl).type)->getStruct()) {
					int structSize = setStructSize(((*tl).type)->getStruct());
					stSize += structSize * ((*tl).type)->getArraySize();
				} else {
                stSize += ((*tl).type)->getInstanceSize() * ((*tl).type)->getArraySize();
				}
            } else if (((*tl).type)->isMatrix() || ((*tl).type)->isVector()){
                stSize += ((*tl).type)->getInstanceSize();
            } else if (((*tl).type)->getStruct()) {
                //?? We should actually be calling getStructSize() function and not setStructSize. This problem occurs in case
                // of nested/embedded structs.
                stSize += setStructSize(((*tl).type)->getStruct());
            } else 
                stSize += 1;
        }
        structureSize = stSize;
        return stSize;
    }

    virtual void setType(TBasicType t, int s, bool m, bool a, int aS = 0)
                            { type = t; size = s; matrix = m; array = a; arraySize = aS; }
    virtual void setType(TBasicType t, int s, bool m, TType* userDef = 0)
                            { type = t; 
                              size = s; 
                              matrix = m; 
                              if (userDef)
                                  structure = userDef->getStruct(); 
                              // leave array information intact.
                            }
    virtual void setTypeName(const TString& n) { typeName = NewPoolTString(n.c_str()); }
    virtual void setFieldName(const TString& n) { fieldName = NewPoolTString(n.c_str()); }
    virtual const TString& getTypeName() const
    { 
		assert (typeName);    		
    	return *typeName; 
    }

    virtual const TString& getFieldName() const
    { 
    	assert (fieldName);
		return *fieldName; 
    }
    
    virtual TBasicType getBasicType() const { return type; }
    virtual TQualifier getQualifier() const { return qualifier; }
    virtual void changeQualifier(TQualifier q) { qualifier = q; }

    // One-dimensional size of single instance type
    virtual int getNominalSize() const { return size; }  
    
    // Full-dimensional size of single instance of type
    virtual int getInstanceSize() const  
    {
        if (matrix)
            return size * size;
        else
            return size;
    }
    
	virtual bool isMatrix() const { return matrix ? true : false; }
    virtual bool isArray() const  { return array ? true : false; }
    int getArraySize() const { return arraySize; }
    void setArraySize(int s) { array = true; arraySize = s; }
    void setMaxArraySize (int s) { maxArraySize = s; }
    int getMaxArraySize () const { return maxArraySize; }
    void setArrayInformationType(TType* t) { arrayInformationType = t; }
    TType* getArrayInformationType() { return arrayInformationType; }
    virtual bool isVector() const { return size > 1 && !matrix; }
    static char* getBasicString(TBasicType t) {
        switch (t) {
        case EbtVoid:            return "void";              break;
        case EbtFloat:           return "float";             break;
        case EbtInt:             return "int";               break;
        case EbtBool:            return "bool";              break;
        case EbtSampler1D:       return "sampler1D";         break;
        case EbtSampler2D:       return "sampler2D";         break;
        case EbtSampler3D:       return "sampler3D";         break;
        case EbtSamplerCube:     return "samplerCube";       break;
        case EbtSampler1DShadow: return "sampler1DShadow";   break;
        case EbtSampler2DShadow: return "sampler2DShadow";   break;
        case EbtStruct:          return "structure";         break;
        default:                 return "unknown type";
        }
    }
    const char* getBasicString() const { return TType::getBasicString(type); }
    const char* getQualifierString() const { return ::getQualifierString(qualifier); }
    TTypeList* getStruct() { return structure; }
    int getStructSize() const { return structureSize; }
    TTypeList* getStruct() const { return structure; }
    TString& getMangledName() {
        if (!mangled) {
			mangled = NewPoolTString("");
            buildMangledName(*mangled);            
            *mangled+=';';
        }
    
        return *mangled;
    }
    bool operator==(const TType& right) const {
        return      type == right.type   &&
                    size == right.size   &&
                  matrix == right.matrix &&
                   array == right.array  &&
               structure == right.structure;
        // don't check the qualifier, it's not ever what's being sought after
    }
    bool operator!=(const TType& right) const {
        return !operator==(right);
    }
    TString getCompleteString() const;
        
protected:
    void buildMangledName(TString&);

	TBasicType type      : 6;
	TQualifier qualifier : 7;
	int size             : 8; // size of vector or matrix, not size of array
	unsigned int matrix  : 1;
	unsigned int array   : 1;

    int arraySize;
    TTypeList* structure;      // 0 unless this is a struct
    int structureSize;
    int maxArraySize;
    TType* arrayInformationType;
    TString *fieldName;         // for structure field names
    TString *typeName;          // for structure field type name
    TString *mangled;

};

#endif // _TYPES_INCLUDED_
