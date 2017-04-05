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

/**
 * This is bison grammar and production code for parsing the OpenGL 2.0 shading
 * languages.
 */
%{

/* Based on:
ANSI C Yacc grammar

In 1985, Jeff Lee published his Yacc grammar (which is accompanied by a 
matching Lex specification) for the April 30, 1985 draft version of the 
ANSI C standard.  Tom Stockfisch reposted it to net.sources in 1987; that
original, as mentioned in the answer to question 17.25 of the comp.lang.c
FAQ, can be ftp'ed from ftp.uu.net, file usenet/net.sources/ansi.c.grammar.Z.
 
I intend to keep this version as close to the current C Standard grammar as 
possible; please let me know if you discover discrepancies. 

Jutta Degener, 1995 
*/

#include "SymbolTable.h"
#include "ParseHelper.h"
#include "../Public/ShaderLang.h"

#ifdef _WIN32
    #define YYPARSE_PARAM parseContext
    #define YYPARSE_PARAM_DECL TParseContext&
    #define YY_DECL int yylex(YYSTYPE* pyylval, TParseContext& parseContext)
    #define YYLEX_PARAM parseContext
#else
    #define YYPARSE_PARAM parseContextLocal
    #define parseContext (*((TParseContext*)(parseContextLocal)))
    #define YY_DECL int yylex(YYSTYPE* pyylval, void* parseContextLocal)
    #define YYLEX_PARAM (void*)(parseContextLocal)
    extern void yyerror(char*);    
#endif

#define FRAG_VERT_ONLY(S, L) {                                                  \
    if (parseContext.language != EShLangFragment &&                             \
        parseContext.language != EShLangVertex) {                               \
        parseContext.error(L, " supported in vertex/fragment shaders only ", S, "", "");   \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define VERTEX_ONLY(S, L) {                                                     \
    if (parseContext.language != EShLangVertex) {                               \
        parseContext.error(L, " supported in vertex shaders only ", S, "", "");            \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define FRAG_ONLY(S, L) {                                                       \
    if (parseContext.language != EShLangFragment) {                             \
        parseContext.error(L, " supported in fragment shaders only ", S, "", "");          \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define PACK_ONLY(S, L) {                                                       \
    if (parseContext.language != EShLangPack) {                                 \
        parseContext.error(L, " supported in pack shaders only ", S, "", "");              \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define UNPACK_ONLY(S, L) {                                                     \
    if (parseContext.language != EShLangUnpack) {                               \
        parseContext.error(L, " supported in unpack shaders only ", S, "", "");            \
        parseContext.recover();                                                            \
    }                                                                           \
}

#define PACK_UNPACK_ONLY(S, L) {                                                \
    if (parseContext.language != EShLangUnpack &&                               \
        parseContext.language != EShLangPack) {                                 \
        parseContext.error(L, " supported in pack/unpack shaders only ", S, "", "");       \
        parseContext.recover();                                                            \
    }                                                                           \
}
%}
%union {
    struct {
        TSourceLoc line;
        union {
            TString *string;
            float f;
            int i;
            bool b;
        };
        TSymbol* symbol;
    } lex;
    struct {
        TSourceLoc line;
        TOperator op;
        union {
            TIntermNode* intermNode;
            TIntermNodePair nodePair;
            TIntermTyped* intermTypedNode;
            TIntermAggregate* intermAggregate;
        };
        union {
            TPublicType type;
            TQualifier qualifier;
            TFunction* function;
            TParameter param;
            TTypeLine typeLine;
            TTypeList* typeList;
        };
    } interm;
}

%{
#ifndef _WIN32
    extern int yylex(YYSTYPE*, void*);
#endif
%}

%pure_parser /* Just in case is called from multiple threads */
%expect 1 /* One shift reduce conflict because of if | else */
%token <lex> ATTRIBUTE CONST_QUAL BOOL_TYPE FLOAT_TYPE INT_TYPE
%token <lex> BREAK CONTINUE DO ELSE FOR IF DISCARD RETURN
%token <lex> BVEC2 BVEC3 BVEC4 IVEC2 IVEC3 IVEC4 VEC2 VEC3 VEC4
%token <lex> MATRIX2 MATRIX3 MATRIX4 IN_QUAL OUT_QUAL INOUT_QUAL UNIFORM VARYING
%token <lex> STRUCT VOID_TYPE WHILE
%token <lex> SAMPLER1D SAMPLER2D SAMPLER3D SAMPLERCUBE SAMPLER1DSHADOW SAMPLER2DSHADOW

%token <lex> IDENTIFIER TYPE_NAME FLOATCONSTANT INTCONSTANT BOOLCONSTANT
%token <lex> FIELD_SELECTION
%token <lex> LEFT_OP RIGHT_OP
%token <lex> INC_OP DEC_OP LE_OP GE_OP EQ_OP NE_OP
%token <lex> AND_OP OR_OP XOR_OP MUL_ASSIGN DIV_ASSIGN ADD_ASSIGN
%token <lex> MOD_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%token <lex> SUB_ASSIGN

%token <lex> LEFT_PAREN RIGHT_PAREN LEFT_BRACKET RIGHT_BRACKET LEFT_BRACE RIGHT_BRACE DOT
%token <lex> COMMA COLON EQUAL SEMICOLON BANG DASH TILDE PLUS STAR SLASH PERCENT
%token <lex> LEFT_ANGLE RIGHT_ANGLE VERTICAL_BAR CARET AMPERSAND QUESTION

%type <interm> assignment_operator constructor_identifier unary_operator
%type <interm.intermTypedNode> variable_identifier primary_expression postfix_expression
%type <interm.intermTypedNode> expression integer_expression assignment_expression
%type <interm.intermTypedNode> unary_expression multiplicative_expression additive_expression
%type <interm.intermTypedNode> relational_expression equality_expression 
%type <interm.intermTypedNode> conditional_expression constant_expression
%type <interm.intermTypedNode> logical_or_expression logical_xor_expression logical_and_expression
%type <interm.intermTypedNode> shift_expression and_expression exclusive_or_expression inclusive_or_expression
%type <interm.intermTypedNode> function_call initializer condition conditionopt

%type <interm.intermNode> translation_unit function_definition
%type <interm.intermNode> statement simple_statement
%type <interm.intermAggregate>  statement_list compound_statement 
%type <interm.intermNode> declaration_statement selection_statement expression_statement
%type <interm.intermNode> declaration external_declaration
%type <interm.intermNode> for_init_statement compound_statement_no_new_scope
%type <interm.nodePair> selection_rest_statement for_rest_statement
%type <interm.intermNode> iteration_statement jump_statement statement_no_new_scope
%type <interm> single_declaration init_declarator_list

%type <interm> parameter_declaration parameter_declarator parameter_type_specifier
%type <interm.qualifier> parameter_qualifier

%type <interm.type> type_qualifier fully_specified_type type_specifier 
%type <interm.type> struct_specifier 
%type <interm.typeLine> struct_declarator 
%type <interm.typeList> struct_declarator_list struct_declaration struct_declaration_list
%type <interm.function> function_header function_declarator function_identifier
%type <interm.function> function_header_with_parameters function_call_header 
%type <interm> function_call_header_with_parameters function_call_header_no_parameters function_call_generic function_prototype

%start translation_unit 
%%

variable_identifier 
    : IDENTIFIER {
        // The symbol table search was done in the lexical phase
        const TSymbol* symbol = $1.symbol;
        const TVariable* variable;
        if (symbol == 0) {
            parseContext.error($1.line, "undeclared identifier", $1.string->c_str(), "");
            parseContext.recover();
            TType type(EbtFloat);
            TVariable* fakeVariable = new TVariable($1.string, type);
            parseContext.symbolTable.insert(*fakeVariable);
            variable = fakeVariable;
        } else {
            // This identifier can only be a variable type symbol 
            if (! symbol->isVariable()) {
                parseContext.error($1.line, "variable expected", $1.string->c_str(), "");
                parseContext.recover();
            }
            variable = static_cast<const TVariable*>(symbol);
        }

        // don't delete $1.string, it's used by error recovery, and the pool
        // pop will reclaim the memory

        if (variable->getType().getQualifier() == EvqConst ) {
            constUnion* constArray = variable->getConstPointer();
            TType t(variable->getType());
            $$ = parseContext.intermediate.addConstantUnion(constArray, t, $1.line);        
        } else
            $$ = parseContext.intermediate.addSymbol(variable->getUniqueId(), 
                                                     variable->getName(), 
                                                     variable->getType(), $1.line);
    }
    ;

primary_expression
    : variable_identifier {
        $$ = $1;
    }
    | INTCONSTANT {
        //
        // INT_TYPE is only 16-bit plus sign bit for vertex/fragment shaders, 
        // check for overflow for constants
        //
        if (abs($1.i) >= (1 << 16)) {
            parseContext.error($1.line, " integer constant overflow", "", "");
            parseContext.recover();
        }
        constUnion *unionArray = new constUnion[1];
        unionArray->iConst = $1.i;
        $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), $1.line);
    }
    | FLOATCONSTANT {
        constUnion *unionArray = new constUnion[1];
        unionArray->fConst = $1.f;
        $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtFloat, EvqConst), $1.line);
    }
    | BOOLCONSTANT {
        constUnion *unionArray = new constUnion[1];
        unionArray->bConst = $1.b;
        $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $1.line);
    }
    | LEFT_PAREN expression RIGHT_PAREN {
        $$ = $2;
    }
    ;

postfix_expression
    : primary_expression { 
        $$ = $1;
    } 
    | postfix_expression LEFT_BRACKET integer_expression RIGHT_BRACKET {
        if (!$1->isArray() && !$1->isMatrix() && !$1->isVector()) {
            if ($1->getAsSymbolNode())
                parseContext.error($2.line, " left of '[' is not of type array, matrix, or vector ", $1->getAsSymbolNode()->getSymbol().c_str(), "");
            else
                parseContext.error($2.line, " left of '[' is not of type array, matrix, or vector ", "expression", "");
            parseContext.recover();
        }
        if ($1->getType().getQualifier() == EvqConst && !$1->isArray() && $3->getQualifier() == EvqConst) {
             if ($1->isVector()) {  // constant folding for vectors
                TVectorFields fields;
                fields.num = 1;
                fields.offsets[0] = $3->getAsConstantUnion()->getUnionArrayPointer()->iConst; // need to do it this way because v.xy sends fields integer array
                $$ = parseContext.addConstVectorNode(fields, $1, $2.line);
            } else if ($1->isMatrix()) { // constant folding for matrices
                $$ = parseContext.addConstMatrixNode($3->getAsConstantUnion()->getUnionArrayPointer()->iConst, $1, $2.line);
            }
        } else {
            if ($3->getQualifier() == EvqConst) {
                if (($1->isVector() || $1->isMatrix()) && $1->getType().getNominalSize() <= $3->getAsConstantUnion()->getUnionArrayPointer()->iConst && !$1->isArray() ) {
                    parseContext.error($2.line, "", "[", "field selection out of range '%d'", $3->getAsConstantUnion()->getUnionArrayPointer()->iConst);
                    parseContext.recover();
                } else {
                    if ($1->isArray()) {
                        if ($1->getType().getArraySize() == 0) {
                            if ($1->getType().getMaxArraySize() <= $3->getAsConstantUnion()->getUnionArrayPointer()->iConst) {
                                if (parseContext.arraySetMaxSize($1->getAsSymbolNode(), $1->getTypePointer(), $3->getAsConstantUnion()->getUnionArrayPointer()->iConst, true, $2.line))
                                    parseContext.recover(); 
                            } else {
                                if (parseContext.arraySetMaxSize($1->getAsSymbolNode(), $1->getTypePointer(), 0, false, $2.line))
                                    parseContext.recover(); 
                            }
                        } else if ( $3->getAsConstantUnion()->getUnionArrayPointer()->iConst >= $1->getType().getArraySize()) {
                            parseContext.error($2.line, "", "[", "array index out of range '%d'", $3->getAsConstantUnion()->getUnionArrayPointer()->iConst);
                            parseContext.recover();
                        }
                    }
                    $$ = parseContext.intermediate.addIndex(EOpIndexDirect, $1, $3, $2.line);
                }
            } else {
                if ($1->isArray() && $1->getType().getArraySize() == 0) {
                    parseContext.error($2.line, "", "[", "array must be redeclared with a size before being indexed with a variable");
                    parseContext.recover();
                }
                
                $$ = parseContext.intermediate.addIndex(EOpIndexIndirect, $1, $3, $2.line);
            }
        } 
        if ($$ == 0) {
            constUnion *unionArray = new constUnion[1];
            unionArray->fConst = 0.0;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtFloat, EvqConst), $2.line);
        } else if ($1->isArray()) {
            if ($1->getType().getStruct())
                $$->setType(TType($1->getType().getStruct(), $1->getType().getTypeName()));
            else
                $$->setType(TType($1->getBasicType(), EvqTemporary, $1->getNominalSize(), $1->isMatrix()));
        } else if ($1->isMatrix() && $1->getType().getQualifier() == EvqConst)         
            $$->setType(TType($1->getBasicType(), EvqConst, $1->getNominalSize()));     
        else if ($1->isMatrix())            
            $$->setType(TType($1->getBasicType(), EvqTemporary, $1->getNominalSize()));     
        else if ($1->isVector() && $1->getType().getQualifier() == EvqConst)          
            $$->setType(TType($1->getBasicType(), EvqConst));     
        else if ($1->isVector())       
            $$->setType(TType($1->getBasicType(), EvqTemporary));
        else
            $$->setType($1->getType()); 
    }
    | function_call {
        $$ = $1;
    }
    | postfix_expression DOT FIELD_SELECTION {        
        if ($1->isArray()) {
            parseContext.error($3.line, "cannot apply dot operator to an array", ".", "");
            parseContext.recover();
        }

        if ($1->isVector()) {
            TVectorFields fields;
            if (! parseContext.parseVectorFields(*$3.string, $1->getNominalSize(), fields, $3.line)) {
                fields.num = 1;
                fields.offsets[0] = 0;
                parseContext.recover();
            }

            if ($1->getType().getQualifier() == EvqConst) { // constant folding for vector fields
                $$ = parseContext.addConstVectorNode(fields, $1, $3.line);
                if ($$ == 0) {
                    parseContext.recover();
                    $$ = $1;
                }
                else
                    $$->setType(TType($1->getBasicType(), EvqConst, (int) (*$3.string).size()));
            } else {
                if (fields.num == 1) {
                    constUnion *unionArray = new constUnion[1];
                    unionArray->iConst = fields.offsets[0];
                    TIntermTyped* index = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), $3.line);
                    $$ = parseContext.intermediate.addIndex(EOpIndexDirect, $1, index, $2.line);
                    $$->setType(TType($1->getBasicType()));
                } else {
                    TString vectorString = *$3.string;
                    TIntermTyped* index = parseContext.intermediate.addSwizzle(fields, $3.line);                
                    $$ = parseContext.intermediate.addIndex(EOpVectorSwizzle, $1, index, $2.line);
                    $$->setType(TType($1->getBasicType(),EvqTemporary, (int) vectorString.size()));  
                }
            }
        } else if ($1->isMatrix()) {
            TMatrixFields fields;
            if (! parseContext.parseMatrixFields(*$3.string, $1->getNominalSize(), fields, $3.line)) {
                fields.wholeRow = false;
                fields.wholeCol = false;
                fields.row = 0;
                fields.col = 0;
                parseContext.recover();
            }

            if (fields.wholeRow || fields.wholeCol) {
                parseContext.error($2.line, " non-scalar fields not implemented yet", ".", "");
                parseContext.recover();
                constUnion *unionArray = new constUnion[1];
                unionArray->iConst = 0;
                TIntermTyped* index = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), $3.line);
                $$ = parseContext.intermediate.addIndex(EOpIndexDirect, $1, index, $2.line);                
                $$->setType(TType($1->getBasicType(), EvqTemporary, $1->getNominalSize()));
            } else {
                constUnion *unionArray = new constUnion[1];
                unionArray->iConst = fields.col * $1->getNominalSize() + fields.row;
                TIntermTyped* index = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), $3.line);
                $$ = parseContext.intermediate.addIndex(EOpIndexDirect, $1, index, $2.line);                
                $$->setType(TType($1->getBasicType()));
            }
        } else if ($1->getBasicType() == EbtStruct) {
            bool fieldFound = false;
            TTypeList* fields = $1->getType().getStruct();
            if (fields == 0) {
                parseContext.error($2.line, "structure has no fields", "Internal Error", "");
                parseContext.recover();
                $$ = $1;
            } else {
                unsigned int i;
                for (i = 0; i < fields->size(); ++i) {
                    if ((*fields)[i].type->getFieldName() == *$3.string) {
                        fieldFound = true;
                        break;
                    }                
                }
                if (fieldFound) {
                    if ($1->getType().getQualifier() == EvqConst) {
                        $$ = parseContext.addConstStruct(*$3.string, $1, $2.line);
                        if ($$ == 0) {
                            parseContext.recover();
                            $$ = $1;
                        }
                        else {
                            $$->setType(*(*fields)[i].type);
                            // change the qualifier of the return type, not of the structure field
                            // as the structure definition is shared between various structures.
                            $$->getTypePointer()->changeQualifier(EvqConst);
                        }
                    } else {
                        constUnion *unionArray = new constUnion[1];
                        unionArray->iConst = i;
                        TIntermTyped* index = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtInt, EvqConst), $3.line);
                        $$ = parseContext.intermediate.addIndex(EOpIndexDirectStruct, $1, index, $2.line);                
                        $$->setType(*(*fields)[i].type);
                    }
                } else {
                    parseContext.error($2.line, " no such field in structure", $3.string->c_str(), "");
                    parseContext.recover();
                    $$ = $1;
                }
            }
        } else {
            parseContext.error($2.line, " field selection requires structure, vector, or matrix on left hand side", $3.string->c_str(), "");
            parseContext.recover();
            $$ = $1;
        }
        // don't delete $3.string, it's from the pool
    }
    | postfix_expression INC_OP {
        if (parseContext.lValueErrorCheck($2.line, "++", $1))
            parseContext.recover();
        $$ = parseContext.intermediate.addUnaryMath(EOpPostIncrement, $1, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.unaryOpError($2.line, "++", $1->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    | postfix_expression DEC_OP {
        if (parseContext.lValueErrorCheck($2.line, "--", $1))
            parseContext.recover();
        $$ = parseContext.intermediate.addUnaryMath(EOpPostDecrement, $1, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.unaryOpError($2.line, "--", $1->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    ;

integer_expression 
    : expression {
        if (parseContext.integerErrorCheck($1, "[]"))
            parseContext.recover();
        $$ = $1; 
    }
    ;

function_call 
    : function_call_generic {
        TFunction* fnCall = $1.function;
        TOperator op = fnCall->getBuiltInOp();
        
        if (op != EOpNull) {
            //
            // Then this should be a constructor.
            //
            TType type(EbtVoid);  // use this to get the type back
            if (parseContext.constructorErrorCheck($1.line, $1.intermNode, *fnCall, op, &type)) {
                $$ = 0;
            } else {
                //
                // It's a constructor, of type 'type'.
                //
                $$ = parseContext.addConstructor($1.intermNode, &type, op, fnCall, $1.line);
            }
            
            if ($$ == 0) {        
                parseContext.recover();
                $$ = parseContext.intermediate.setAggregateOperator(0, op, $1.line);
            }
            $$->setType(type);
        } else {
            //
            // Not a constructor.  Find it in the symbol table.
            //
            const TFunction* fnCandidate;
            bool builtIn;
            fnCandidate = parseContext.findFunction($1.line, fnCall, &builtIn);
            if (fnCandidate) {
                //
                // A declared function.  But, it might still map to a built-in
                // operation.
                //
                op = fnCandidate->getBuiltInOp();
                if (builtIn && op != EOpNull) {
                    //
                    // A function call mapped to a built-in operation.
                    //
                    if (fnCandidate->getParamCount() == 1) {
                        //
                        // Treat it like a built-in unary operator.
                        //
                        $$ = parseContext.intermediate.addUnaryMath(op, $1.intermNode, 0, parseContext.symbolTable);
                        if ($$ == 0)  {
                            parseContext.error($1.intermNode->getLine(), " wrong operand type", "Internal Error", 
                                "built in unary operator function.  Type: %s",
                                static_cast<TIntermTyped*>($1.intermNode)->getCompleteString().c_str());
                            YYERROR;
                        }
                    } else {
                        $$ = parseContext.intermediate.setAggregateOperator($1.intermAggregate, op, $1.line);
                    }
                } else {
                    // This is a real function call
                    
                    $$ = parseContext.intermediate.setAggregateOperator($1.intermAggregate, EOpFunctionCall, $1.line);
                    $$->setType(fnCandidate->getReturnType());                   
                    
                    // this is how we know whether the given function is a builtIn function or a user defined function
                    // if builtIn == false, it's a userDefined -> could be an overloaded builtIn function also
                    // if builtIn == true, it's definitely a builtIn function with EOpNull
                    if (!builtIn) 
                        $$->getAsAggregate()->setUserDefined(); 
                    $$->getAsAggregate()->setName(fnCandidate->getMangledName());

                    TQualifier qual;
                    TQualifierList& qualifierList = $$->getAsAggregate()->getQualifier();
                    for (int i = 0; i < fnCandidate->getParamCount(); ++i) {
                        qual = (*fnCandidate)[i].type->getQualifier();
                        if (qual == EvqOut || qual == EvqInOut) {
                            if (parseContext.lValueErrorCheck($$->getLine(), "assign", $$->getAsAggregate()->getSequence()[i]->getAsTyped())) {
                                parseContext.error($1.intermNode->getLine(), "Constant value cannot be passed for 'out' or 'inout' parameters.", "Error", "");
                                parseContext.recover();
                            }
                        }
                        qualifierList.push_back(qual);
                    }
                }
                $$->setType(fnCandidate->getReturnType());
            } else {
                // error message was put out by PaFindFunction()
                // Put on a dummy node for error recovery
                constUnion *unionArray = new constUnion[1];
                unionArray->fConst = 0.0;
                $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtFloat, EvqConst), $1.line);
                parseContext.recover();
            }
        }
        delete fnCall;
    }
    ;

function_call_generic
    : function_call_header_with_parameters RIGHT_PAREN {
        $$ = $1;
        $$.line = $2.line;
    }
    | function_call_header_no_parameters RIGHT_PAREN {
        $$ = $1;
        $$.line = $2.line;
    }
    ;
    
function_call_header_no_parameters 
    : function_call_header VOID_TYPE {
        $$.function = $1;
        $$.intermNode = 0;
    }
    | function_call_header {
        $$.function = $1;
        $$.intermNode = 0;
    }
    ;

function_call_header_with_parameters
    : function_call_header assignment_expression {
        TParameter param = { 0, new TType($2->getType()) };
        $1->addParameter(param);
        $$.function = $1;
        $$.intermNode = $2;
    }
    | function_call_header_with_parameters COMMA assignment_expression {
        TParameter param = { 0, new TType($3->getType()) };
        $1.function->addParameter(param);
        $$.function = $1.function;
        $$.intermNode = parseContext.intermediate.growAggregate($1.intermNode, $3, $2.line);
    }
    ;

function_call_header 
    : function_identifier LEFT_PAREN {
        $$ = $1;
    }
    ;

function_identifier
    : constructor_identifier {
        if ($1.op == EOpConstructStruct) {
            TString tempString = "";
            TFunction *function = new TFunction(&tempString, *($1.type.userDef), $1.op);
            $$ = function;
        }
        else {
            TFunction *function = new TFunction($1.op);
            $$ = function;
        }
    }
    | IDENTIFIER {
        if (parseContext.reservedErrorCheck($1.line, *$1.string)) 
            parseContext.recover();
        TType type(EbtVoid);
        TFunction *function = new TFunction($1.string, type);
        $$ = function;        
    }
    ;

// Grammar Note:  Constructors look like functions, but lexical anaylsis recognized most of them as keywords.

//
// Don't go through the symbol table for constructors.  
// Their parameters will be verified algorithmically.
//
constructor_identifier
    : FLOAT_TYPE {                                   $$.line = $1.line; $$.op = EOpConstructFloat; }
    | INT_TYPE   {                                   $$.line = $1.line; $$.op = EOpConstructInt;   }
    | BOOL_TYPE  {                                   $$.line = $1.line; $$.op = EOpConstructBool;  }
    | VEC2       {                                   $$.line = $1.line; $$.op = EOpConstructVec2;  }
    | VEC3       {                                   $$.line = $1.line; $$.op = EOpConstructVec3;  }
    | VEC4       {                                   $$.line = $1.line; $$.op = EOpConstructVec4;  }
    | BVEC2      { FRAG_VERT_ONLY("bvec2", $1.line); $$.line = $1.line; $$.op = EOpConstructBVec2; }
    | BVEC3      { FRAG_VERT_ONLY("bvec3", $1.line); $$.line = $1.line; $$.op = EOpConstructBVec3; }
    | BVEC4      { FRAG_VERT_ONLY("bvec4", $1.line); $$.line = $1.line; $$.op = EOpConstructBVec4; }
    | IVEC2      { FRAG_VERT_ONLY("ivec2", $1.line); $$.line = $1.line; $$.op = EOpConstructIVec2; }
    | IVEC3      { FRAG_VERT_ONLY("ivec3", $1.line); $$.line = $1.line; $$.op = EOpConstructIVec3; }
    | IVEC4      { FRAG_VERT_ONLY("ivec4", $1.line); $$.line = $1.line; $$.op = EOpConstructIVec4; }
    | MATRIX2    { FRAG_VERT_ONLY("mat2", $1.line);  $$.line = $1.line; $$.op = EOpConstructMat2;  }
    | MATRIX3    { FRAG_VERT_ONLY("mat3", $1.line);  $$.line = $1.line; $$.op = EOpConstructMat3;  }
    | MATRIX4    { FRAG_VERT_ONLY("mat4", $1.line);  $$.line = $1.line; $$.op = EOpConstructMat4;  }
    | TYPE_NAME  {                                   
        TType& structure = static_cast<TVariable*>($1.symbol)->getType();
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtStruct, qual, 1, false, false, &structure, $1.line }; 
        $$.type = t;
        $$.line = $1.line; 
        $$.op = EOpConstructStruct; 
    }    
    ;

unary_expression
    : postfix_expression {
        $$ = $1;
    }
    | INC_OP unary_expression {
        if (parseContext.lValueErrorCheck($1.line, "++", $2))
            parseContext.recover();
        $$ = parseContext.intermediate.addUnaryMath(EOpPreIncrement, $2, $1.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.unaryOpError($1.line, "++", $2->getCompleteString());
            parseContext.recover();
            $$ = $2;
        }
    }
    | DEC_OP unary_expression {
        if (parseContext.lValueErrorCheck($1.line, "--", $2))
            parseContext.recover();
        $$ = parseContext.intermediate.addUnaryMath(EOpPreDecrement, $2, $1.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.unaryOpError($1.line, "--", $2->getCompleteString());
            parseContext.recover();
            $$ = $2;
        }
    }
    | unary_operator unary_expression {
        if ($1.op != EOpNull) {
            $$ = parseContext.intermediate.addUnaryMath($1.op, $2, $1.line, parseContext.symbolTable);
            if ($$ == 0) {
                char* errorOp = "";
                switch($1.op) {
                case EOpNegative:   errorOp = "-"; break;
                case EOpLogicalNot: errorOp = "!"; break;
                case EOpBitwiseNot: errorOp = "~"; break;
				default: break;
                }
                parseContext.unaryOpError($1.line, errorOp, $2->getCompleteString());
                parseContext.recover();
                $$ = $2;
            }
        } else
            $$ = $2;
    }
    ;
// Grammar Note:  No traditional style type casts.

unary_operator
    : PLUS  { $$.line = $1.line; $$.op = EOpNull; }
    | DASH  { $$.line = $1.line; $$.op = EOpNegative; }
    | BANG  { $$.line = $1.line; $$.op = EOpLogicalNot; }
    | TILDE { PACK_UNPACK_ONLY("~", $1.line);  
              $$.line = $1.line; $$.op = EOpBitwiseNot; }
    ;
// Grammar Note:  No '*' or '&' unary ops.  Pointers are not supported.

multiplicative_expression
    : unary_expression { $$ = $1; }
    | multiplicative_expression STAR unary_expression {
        FRAG_VERT_ONLY("*", $2.line);
        $$ = parseContext.intermediate.addBinaryMath(EOpMul, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "*", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    | multiplicative_expression SLASH unary_expression {
        FRAG_VERT_ONLY("/", $2.line); 
        $$ = parseContext.intermediate.addBinaryMath(EOpDiv, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "/", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    | multiplicative_expression PERCENT unary_expression {
        PACK_UNPACK_ONLY("%", $2.line);
        $$ = parseContext.intermediate.addBinaryMath(EOpMod, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "%", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression PLUS multiplicative_expression {  
        $$ = parseContext.intermediate.addBinaryMath(EOpAdd, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "+", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    | additive_expression DASH multiplicative_expression {
        $$ = parseContext.intermediate.addBinaryMath(EOpSub, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "-", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        } 
    }
    ;

shift_expression
    : additive_expression { $$ = $1; }
    | shift_expression LEFT_OP additive_expression {
        PACK_UNPACK_ONLY("<<", $2.line);
        $$ = parseContext.intermediate.addBinaryMath(EOpLeftShift, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "<<", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    | shift_expression RIGHT_OP additive_expression {
        PACK_UNPACK_ONLY(">>", $2.line);
        $$ = parseContext.intermediate.addBinaryMath(EOpRightShift, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, ">>", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    ;

relational_expression
    : shift_expression { $$ = $1; }
    | relational_expression LEFT_ANGLE shift_expression { 
        $$ = parseContext.intermediate.addBinaryMath(EOpLessThan, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "<", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $2.line);
        }
    }
    | relational_expression RIGHT_ANGLE shift_expression  { 
        $$ = parseContext.intermediate.addBinaryMath(EOpGreaterThan, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, ">", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $2.line);
        }
    }
    | relational_expression LE_OP shift_expression  { 
        $$ = parseContext.intermediate.addBinaryMath(EOpLessThanEqual, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "<=", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $2.line);
        }
    }
    | relational_expression GE_OP shift_expression  { 
        $$ = parseContext.intermediate.addBinaryMath(EOpGreaterThanEqual, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, ">=", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $2.line);
        }
    }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression EQ_OP relational_expression  { 
        $$ = parseContext.intermediate.addBinaryMath(EOpEqual, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "==", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $2.line);
        }
    }
    | equality_expression  NE_OP relational_expression { 
        $$ = parseContext.intermediate.addBinaryMath(EOpNotEqual, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "!=", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $2.line);
        }
    }
    ;

and_expression
    : equality_expression { $$ = $1; }
    | and_expression AMPERSAND equality_expression {
        PACK_UNPACK_ONLY("&", $2.line);
        $$ = parseContext.intermediate.addBinaryMath(EOpAnd, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "&", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    ;

exclusive_or_expression
    : and_expression { $$ = $1; }
    | exclusive_or_expression CARET and_expression {
        PACK_UNPACK_ONLY("^", $2.line);
        $$ = parseContext.intermediate.addBinaryMath(EOpExclusiveOr, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "^", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    ;

inclusive_or_expression
    : exclusive_or_expression { $$ = $1; }
    | inclusive_or_expression VERTICAL_BAR exclusive_or_expression {
        PACK_UNPACK_ONLY("|", $2.line);
        $$ = parseContext.intermediate.addBinaryMath(EOpInclusiveOr, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "|", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    ;

logical_and_expression
    : inclusive_or_expression { $$ = $1; }
    | logical_and_expression AND_OP inclusive_or_expression {
        $$ = parseContext.intermediate.addBinaryMath(EOpLogicalAnd, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "&&", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $2.line);
        }
    }
    ;

logical_xor_expression
    : logical_and_expression { $$ = $1; }
    | logical_xor_expression XOR_OP logical_and_expression  { 
        $$ = parseContext.intermediate.addBinaryMath(EOpLogicalXor, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "^^", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $2.line);
        }
    }
    ;

logical_or_expression
    : logical_xor_expression { $$ = $1; }
    | logical_or_expression OR_OP logical_xor_expression  { 
        $$ = parseContext.intermediate.addBinaryMath(EOpLogicalOr, $1, $3, $2.line, parseContext.symbolTable);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, "||", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            constUnion *unionArray = new constUnion[1];
            unionArray->bConst = false;
            $$ = parseContext.intermediate.addConstantUnion(unionArray, TType(EbtBool, EvqConst), $2.line);
        }
    }
    ;

conditional_expression
    : logical_or_expression { $$ = $1; }
    | logical_or_expression QUESTION expression COLON assignment_expression {
       if (parseContext.boolErrorCheck($2.line, $1))
            parseContext.recover();
       
        $$ = parseContext.intermediate.addSelection($1, $3, $5, $2.line);
        if ($3->getType() != $5->getType())
            $$ = 0;
            
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, ":", $3->getCompleteString(), $5->getCompleteString());
            parseContext.recover();
            $$ = $5;
        }
    }
    ;

assignment_expression
    : conditional_expression { $$ = $1; }
    | unary_expression assignment_operator assignment_expression {
        if (parseContext.lValueErrorCheck($2.line, "assign", $1))
            parseContext.recover();
        $$ = parseContext.intermediate.addAssign($2.op, $1, $3, $2.line);
        if ($$ == 0) {
            parseContext.assignError($2.line, "assign", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $1;
        }
    }
    ;

assignment_operator
    : EQUAL        {                                    $$.line = $1.line; $$.op = EOpAssign; }
    | MUL_ASSIGN   { FRAG_VERT_ONLY("*=", $1.line);     $$.line = $1.line; $$.op = EOpMulAssign; }
    | DIV_ASSIGN   { FRAG_VERT_ONLY("/=", $1.line);     $$.line = $1.line; $$.op = EOpDivAssign; }
    | MOD_ASSIGN   { PACK_UNPACK_ONLY("%=", $1.line);   $$.line = $1.line; $$.op = EOpModAssign; }
    | ADD_ASSIGN   {                                    $$.line = $1.line; $$.op = EOpAddAssign; }
    | SUB_ASSIGN   {                                    $$.line = $1.line; $$.op = EOpSubAssign; }
    | LEFT_ASSIGN  { PACK_UNPACK_ONLY("<<=", $1.line);  $$.line = $1.line; $$.op = EOpLeftShiftAssign; }
    | RIGHT_ASSIGN { PACK_UNPACK_ONLY("<<=", $1.line);  $$.line = $1.line; $$.op = EOpRightShiftAssign; }
    | AND_ASSIGN   { PACK_UNPACK_ONLY("&=",  $1.line);  $$.line = $1.line; $$.op = EOpAndAssign; }
    | XOR_ASSIGN   { PACK_UNPACK_ONLY("^=",  $1.line);  $$.line = $1.line; $$.op = EOpExclusiveOrAssign; }
    | OR_ASSIGN    { PACK_UNPACK_ONLY("|=",  $1.line);  $$.line = $1.line; $$.op = EOpInclusiveOrAssign; }
    ;

expression
    : assignment_expression {
        $$ = $1;
    }
    | expression COMMA assignment_expression {
        $$ = parseContext.intermediate.addComma($1, $3, $2.line);
        if ($$ == 0) {
            parseContext.binaryOpError($2.line, ",", $1->getCompleteString(), $3->getCompleteString());
            parseContext.recover();
            $$ = $3;
        }
    }
    ;

constant_expression
    : conditional_expression {
        if (parseContext.constErrorCheck($1))
            parseContext.recover();
        $$ = $1;
    }
    ;

declaration
    : function_prototype SEMICOLON   { $$ = 0; }
    | init_declarator_list SEMICOLON { 
        if ($1.intermAggregate)
            $1.intermAggregate->setOperator(EOpSequence); 
        $$ = $1.intermAggregate; 
    }
    ;

function_prototype 
    : function_declarator RIGHT_PAREN  {
        //
        // Multiple declarations of the same function are allowed.
        //
        // If this is a definition, the definition production code will check for redefinitions 
        // (we don't know at this point if it's a definition or not).
        //
        // Redeclarations are allowed.  But, return types and parameter qualifiers must match.
        //        
        TFunction* prevDec = static_cast<TFunction*>(parseContext.symbolTable.find($1->getMangledName()));
        if (prevDec) {
            if (prevDec->getReturnType() != $1->getReturnType()) {
                parseContext.error($2.line, "overloaded functions must have the same return type", $1->getReturnType().getBasicString(), "");
                parseContext.recover();
            }
            for (int i = 0; i < prevDec->getParamCount(); ++i) {
                if ((*prevDec)[i].type->getQualifier() != (*$1)[i].type->getQualifier()) {
                    parseContext.error($2.line, "overloaded functions must have the same parameter qualifiers", (*$1)[i].type->getQualifierString(), "");
                    parseContext.recover();
                }
            }
        }
        
        //
        // If this is a redeclaration, it could also be a definition,
        // in which case, we want to use the variable names from this one, and not the one that's
        // being redeclared.  So, pass back up this declaration, not the one in the symbol table.
        //
        $$.function = $1;
        $$.line = $2.line;

        parseContext.symbolTable.insert(*$$.function);
    }
    ;

function_declarator 
    : function_header {
        $$ = $1;
    }
    | function_header_with_parameters { 
        $$ = $1;  
    }
    ;


function_header_with_parameters
    : function_header parameter_declaration {
        // Add the parameter 
        $$ = $1;
        if ($2.param.type->getBasicType() != EbtVoid)
            $1->addParameter($2.param);
        else
            delete $2.param.type;
    }
    | function_header_with_parameters COMMA parameter_declaration {   
        //
        // Only first parameter of one-parameter functions can be void
        // The check for named parameters not being void is done in parameter_declarator 
        //
        if ($3.param.type->getBasicType() == EbtVoid) {
            //
            // This parameter > first is void
            //
            parseContext.error($2.line, "cannot be an argument type except for '(void)'", "void", "");
            parseContext.recover();
            delete $3.param.type;
        } else {
            // Add the parameter 
            $$ = $1; 
            $1->addParameter($3.param);
        }
    }
    ;

function_header 
    : fully_specified_type IDENTIFIER LEFT_PAREN {
        if ($1.qualifier != EvqGlobal && $1.qualifier != EvqTemporary) {
            parseContext.error($2.line, "no qualifiers allowed for function return", getQualifierString($1.qualifier), "");
            parseContext.recover();
        }
        // make sure a sampler is not involved as well...
        if (parseContext.structQualifierErrorCheck($2.line, $1))
            parseContext.recover();
        
        // Add the function as a prototype after parsing it (we do not support recursion) 
        TFunction *function;
        TType type($1);
        function = new TFunction($2.string, type);
        $$ = function;
    }
    ;

parameter_declarator
    // Type + name 
    : type_specifier IDENTIFIER {
        if ($1.type == EbtVoid) {
            parseContext.error($2.line, "illegal use of type 'void'", $2.string->c_str(), "");
            parseContext.recover();
        }
        if (parseContext.reservedErrorCheck($2.line, *$2.string))
            parseContext.recover();
        TParameter param = {$2.string, new TType($1)};
        $$.line = $2.line;
        $$.param = param;
    }
    | type_specifier IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET {
        // Check that we can make an array out of this type
        if ($1.array) {
            parseContext.error($3.line, "cannot declare arrays of this type", TType($1).getCompleteString().c_str(), "");
            parseContext.recover();
        }
        if (parseContext.reservedErrorCheck($2.line, *$2.string))
            parseContext.recover();
        $1.array = true;
        TType* type = new TType($1);        
        if ($4->getAsConstantUnion())
            type->setArraySize($4->getAsConstantUnion()->getUnionArrayPointer()->iConst);
        TParameter param = { $2.string, type };
        $$.line = $2.line;
        $$.param = param;
    }
    ;

parameter_declaration 
    // 
    // The only parameter qualifier a parameter can have are 
    // IN_QUAL, OUT_QUAL, INOUT_QUAL, or CONST.
    //
    
    //
    // Type + name 
    //
    : type_qualifier parameter_qualifier parameter_declarator {
        $$ = $3;
        if (parseContext.paramErrorCheck($3.line, $1.qualifier, $2, $$.param.type))
            parseContext.recover();
    }
    | parameter_qualifier parameter_declarator {
        $$ = $2;
        if (parseContext.parameterSamplerErrorCheck($2.line, $1, *$2.param.type))
            parseContext.recover();
        if (parseContext.paramErrorCheck($2.line, EvqTemporary, $1, $$.param.type))
            parseContext.recover();
    }
    //
    // Only type 
    //
    | type_qualifier parameter_qualifier parameter_type_specifier {
        $$ = $3;
        if (parseContext.paramErrorCheck($3.line, $1.qualifier, $2, $$.param.type))
            parseContext.recover();
    }
    | parameter_qualifier parameter_type_specifier {
        $$ = $2;
        if (parseContext.parameterSamplerErrorCheck($2.line, $1, *$2.param.type))
            parseContext.recover();
        if (parseContext.paramErrorCheck($2.line, EvqTemporary, $1, $$.param.type))
            parseContext.recover();
    }
    ;
    
parameter_qualifier
    : /* empty */ {
        $$ = EvqIn;
    }
    | IN_QUAL {
        $$ = EvqIn;
    }
    | OUT_QUAL {
        $$ = EvqOut;
    }
    | INOUT_QUAL {
        $$ = EvqInOut;
    }
    ;

parameter_type_specifier 
    : type_specifier {
        TParameter param = { 0, new TType($1) };
        $$.param = param;
        
    }
    | type_specifier LEFT_BRACKET constant_expression RIGHT_BRACKET {
        // Check that we can make an array out of this type 
        if ($1.array) {
            parseContext.error($2.line, "cannot declare arrays of this type", TType($1).getCompleteString().c_str(), "");
            parseContext.recover();
        }
        $1.array = true;
        TType* type = new TType($1);       
        if ($3->getAsConstantUnion())
            type->setArraySize($3->getAsConstantUnion()->getUnionArrayPointer()->iConst);

        TParameter param = { 0, type };
        $$.line = $2.line;
        $$.param = param;
    }
    ;

init_declarator_list
    : single_declaration {
        $$ = $1;
    } 
    | init_declarator_list COMMA IDENTIFIER {
        $$ = $1;
        if (parseContext.structQualifierErrorCheck($3.line, $1.type))
            parseContext.recover();
        
        if (parseContext.nonInitErrorCheck($3.line, *$3.string, $$.type))
            parseContext.recover();
    }
    | init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET {
        $$ = $1;
        if (parseContext.structQualifierErrorCheck($3.line, $1.type))
            parseContext.recover();
            
        if (parseContext.arrayErrorCheck($4.line, *$3.string, $$.type, 0))
            parseContext.recover();
    }
    | init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET {
        $$ = $1;
        if (parseContext.structQualifierErrorCheck($3.line, $1.type))
            parseContext.recover();
            
        if (parseContext.arrayErrorCheck($4.line, *$3.string, $$.type, $5))
            parseContext.recover();
    }
    | init_declarator_list COMMA IDENTIFIER EQUAL initializer {
        $$ = $1;
        if (parseContext.structQualifierErrorCheck($3.line, $1.type))
            parseContext.recover();
        
        TIntermNode* intermNode;
        if (!parseContext.executeInitializer($3.line, *$3.string, $1.type, $5, intermNode)) {
            //
            // build the intermediate representation
            //
            if (intermNode)
                $$.intermAggregate = parseContext.intermediate.growAggregate($1.intermNode, intermNode, $4.line);
            else
                $$.intermAggregate = $1.intermAggregate;
        } else {
            parseContext.recover();
            $$.intermAggregate = 0;
        }
    }
    ;

single_declaration 
    : fully_specified_type {
        $$.type = $1;
        $$.intermAggregate = 0;
    }    
    | fully_specified_type IDENTIFIER {
        $$.intermAggregate = 0;
        $$.type = $1;
        if (parseContext.structQualifierErrorCheck($2.line, $1))
            parseContext.recover();
        
        if (parseContext.nonInitErrorCheck($2.line, *$2.string, $$.type))
            parseContext.recover();
    }
    | fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET {
        $$.intermAggregate = 0;
        $$.type = $1;
        if (parseContext.structQualifierErrorCheck($2.line, $1))
            parseContext.recover();
        
        if (parseContext.arrayErrorCheck($3.line, *$2.string, $$.type, 0))
            parseContext.recover();
    }
    | fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET {
        $$.intermAggregate = 0;
        $$.type = $1;
        if (parseContext.structQualifierErrorCheck($2.line, $1))
            parseContext.recover();
        
        if (parseContext.arrayErrorCheck($3.line, *$2.string, $$.type, $4))
            parseContext.recover();
    }
    | fully_specified_type IDENTIFIER EQUAL initializer {
        $$.type = $1;
        if (parseContext.structQualifierErrorCheck($2.line, $1))
            parseContext.recover();
        
        TIntermNode* intermNode;
        if (!parseContext.executeInitializer($2.line, *$2.string, $1, $4, intermNode)) {
            //
            // Build intermediate representation
            //
            if (intermNode)
                $$.intermAggregate = parseContext.intermediate.makeAggregate(intermNode, $3.line);
            else
                $$.intermAggregate = 0;
        } else {
            parseContext.recover();
            $$.intermAggregate = 0;
        }
    }
    
//
// Place holder for the pack/unpack languages.
//
//    | buffer_specifier {
//        $$.intermAggregate = 0;
//    }
    ;

// Grammar Note:  No 'enum', or 'typedef'.

//
// Place holder for the pack/unpack languages.
//
//%type <interm> buffer_declaration
//%type <interm.type> buffer_specifier input_or_output buffer_declaration_list 
//buffer_specifier
//    : input_or_output LEFT_BRACE buffer_declaration_list RIGHT_BRACE {
//    }
//    ;
//
//input_or_output
//    : INPUT {
//        if (parseContext.globalErrorCheck($1.line, parseContext.symbolTable.atGlobalLevel(), "input"))
//            parseContext.recover();
//        UNPACK_ONLY("input", $1.line);
//        $$.qualifier = EvqInput;        
//    }
//    | OUTPUT {
//        if (parseContext.globalErrorCheck($1.line, parseContext.symbolTable.atGlobalLevel(), "output"))
//            parseContext.recover();
//        PACK_ONLY("output", $1.line);
//        $$.qualifier = EvqOutput;
//    }
//    ;

//
// Place holder for the pack/unpack languages.
//
//buffer_declaration_list
//    : buffer_declaration {
//    }
//    | buffer_declaration_list buffer_declaration {
//    }
//    ;

//
// Input/output semantics:
//   float must be 16 or 32 bits
//   float alignment restrictions?
//   check for only one input and only one output
//   sum of bitfields has to be multiple of 32
//

//
// Place holder for the pack/unpack languages.
//
//buffer_declaration
//    : type_specifier IDENTIFIER COLON constant_expression SEMICOLON {
//        if (parseContext.reservedErrorCheck($2.line, *$2.string, parseContext))
//            parseContext.recover();
//        $$.variable = new TVariable($2.string, $1);
//        if (! parseContext.symbolTable.insert(*$$.variable)) {
//            parseContext.error($2.line, "redefinition", $$.variable->getName().c_str(), "");
//            parseContext.recover();
//            // don't have to delete $$.variable, the pool pop will take care of it
//        }
//    }
//    ;

fully_specified_type
    : type_specifier {
        $$ = $1;
    }
    | type_qualifier type_specifier  { 
        TPublicType t = { $2.type, $1.qualifier, $2.size, $2.matrix, false, $2.userDef, 0 };
        if ($1.qualifier == EvqAttribute &&
            ($2.type == EbtBool || $2.type == EbtInt)) {
            parseContext.error($2.line, "cannot be bool or int", getQualifierString($1.qualifier), "");
            parseContext.recover();
        }
        if (($1.qualifier == EvqVaryingIn || $1.qualifier == EvqVaryingOut) &&
            ($2.type == EbtBool || $2.type == EbtInt)) {
            parseContext.error($2.line, "cannot be bool or int", getQualifierString($1.qualifier), "");
            parseContext.recover();
        }
        $$ = t;
    }
    ;

type_qualifier
    : CONST_QUAL { 
        TPublicType t = { EbtVoid,  EvqConst,     1, false, false, 0 }; 
        $$ = t; 
    }
    | ATTRIBUTE { 
        VERTEX_ONLY("attribute", $1.line);
        if (parseContext.globalErrorCheck($1.line, parseContext.symbolTable.atGlobalLevel(), "attribute"))
            parseContext.recover();
        TPublicType t = { EbtVoid,  EvqAttribute, 1, false, false, 0 }; 
        $$ = t; 
    }
    | VARYING {
        if (parseContext.globalErrorCheck($1.line, parseContext.symbolTable.atGlobalLevel(), "varying"))
            parseContext.recover();
        TPublicType t = { EbtVoid,  EvqVaryingIn, 1, false, false, 0 };
        if (parseContext.language == EShLangVertex)
            t.qualifier = EvqVaryingOut;
        $$ = t; 
    }
    | UNIFORM {
        if (parseContext.globalErrorCheck($1.line, parseContext.symbolTable.atGlobalLevel(), "uniform"))
            parseContext.recover();
        TPublicType t = { EbtVoid,  EvqUniform,   1, false, false, 0 }; 
        $$ = t;
    }
    ;

type_specifier
    : VOID_TYPE {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtVoid, qual, 1, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | FLOAT_TYPE {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 1, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | INT_TYPE {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtInt, qual, 1, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | BOOL_TYPE {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtBool, qual, 1, false, false, 0, $1.line }; 
        $$ = t; 
    }
//    | UNSIGNED INT_TYPE { 
//        PACK_UNPACK_ONLY("unsigned", $1.line); 
//        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
//        TPublicType t = { EbtInt, qual, 1, false, false, 0, $1.line }; 
//        $$ = t; 
//    }
    | VEC2 {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 2, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | VEC3 {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 3, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | VEC4 {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 4, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | BVEC2 {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtBool, qual, 2, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | BVEC3 {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtBool, qual, 3, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | BVEC4 {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtBool, qual, 4, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | IVEC2 {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtInt, qual, 2, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | IVEC3 {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtInt, qual, 3, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | IVEC4 {
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtInt, qual, 4, false, false, 0, $1.line }; 
        $$ = t; 
    }
    | MATRIX2 {
        FRAG_VERT_ONLY("mat2", $1.line); 
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 2, true, false, 0, $1.line }; 
        $$ = t; 
    }
    | MATRIX3 { 
        FRAG_VERT_ONLY("mat3", $1.line); 
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 3, true, false, 0, $1.line }; 
        $$ = t; 
    }
    | MATRIX4 { 
        FRAG_VERT_ONLY("mat4", $1.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtFloat, qual, 4, true, false, 0, $1.line }; 
        $$ = t; 
    } 
    | SAMPLER1D {
        FRAG_VERT_ONLY("sampler1D", $1.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler1D, qual, 1, false, false, 0, $1.line }; 
        $$ = t;
    } 
    | SAMPLER2D {
        FRAG_VERT_ONLY("sampler2D", $1.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler2D, qual, 1, false, false, 0, $1.line }; 
        $$ = t;
    } 
    | SAMPLER3D {
        FRAG_VERT_ONLY("sampler3D", $1.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler3D, qual, 1, false, false, 0, $1.line }; 
        $$ = t;
    } 
    | SAMPLERCUBE {
        FRAG_VERT_ONLY("samplerCube", $1.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSamplerCube, qual, 1, false, false, 0, $1.line }; 
        $$ = t;
    } 
    | SAMPLER1DSHADOW {
        FRAG_VERT_ONLY("sampler1DShadow", $1.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler1DShadow, qual, 1, false, false, 0, $1.line }; 
        $$ = t;
    } 
    | SAMPLER2DSHADOW {
        FRAG_VERT_ONLY("sampler2DShadow", $1.line);
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtSampler2DShadow, qual, 1, false, false, 0, $1.line }; 
        $$ = t;
    }
    | struct_specifier {
        FRAG_VERT_ONLY("struct", $1.line);
        $$ = $1;
        $$.qualifier = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
    }
    | TYPE_NAME {     
        //
        // This is for user defined type names.  The lexical phase looked up the 
        // type.
        //
        TType& structure = static_cast<TVariable*>($1.symbol)->getType();
        TQualifier qual = parseContext.symbolTable.atGlobalLevel() ? EvqGlobal : EvqTemporary;
        TPublicType t = { EbtStruct, qual, 1, false, false, &structure, $1.line }; 
        $$ = t;
    }
    ;

struct_specifier
    : STRUCT IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE {
        TType* structure = new TType($4, *$2.string);
        TVariable* userTypeDef = new TVariable($2.string, *structure, true);
        if (! parseContext.symbolTable.insert(*userTypeDef)) {
            parseContext.error($2.line, "redefinition", $2.string->c_str(), "struct");
            parseContext.recover();
        }
        TPublicType t = { EbtStruct, EvqTemporary, 1, false, false, structure, $1.line };
        $$ = t;
    }
    | STRUCT LEFT_BRACE struct_declaration_list RIGHT_BRACE {
        TType* structure = new TType($3, TString(""));
        TPublicType t = { EbtStruct, EvqTemporary, 1, false, false, structure, $1.line };
        $$ = t;
    }
    ;
    
struct_declaration_list
    : struct_declaration {
        $$ = $1;
    }
    | struct_declaration_list struct_declaration {
        $$ = $1;
        for (unsigned int i = 0; i < $2->size(); ++i) {
            for (unsigned int j = 0; j < $$->size(); ++j) {
                if ((*$$)[j].type->getFieldName() == (*$2)[i].type->getFieldName()) {
                    parseContext.error((*$2)[i].line, "duplicate field name in structure:", "struct", (*$2)[i].type->getFieldName().c_str());
                    parseContext.recover();
                }
            }
            $$->push_back((*$2)[i]);
        }
    }
    ;
    
struct_declaration
    : type_specifier struct_declarator_list SEMICOLON {
        $$ = $2;
        
        if (parseContext.voidErrorCheck($1.line, (*$2)[0].type->getFieldName(), $1)) {
            parseContext.recover();
        }
        for (unsigned int i = 0; i < $$->size(); ++i) {
            //
            // Careful not to replace already know aspects of type, like array-ness
            //
            (*$$)[i].type->setType($1.type, $1.size, $1.matrix, $1.userDef);
            if ($1.userDef)
                (*$$)[i].type->setTypeName($1.userDef->getTypeName());
        }
    }
    ;
    
struct_declarator_list
    : struct_declarator {
        $$ = NewPoolTTypeList();
        $$->push_back($1);
    }
    | struct_declarator_list COMMA struct_declarator {
        $$->push_back($3);
    }
    ;
    
struct_declarator
    : IDENTIFIER {
        $$.type = new TType(EbtVoid);
        $$.line = $1.line;
        $$.type->setFieldName(*$1.string);
    }
    | IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET {
        $$.type = new TType(EbtVoid);
        $$.line = $1.line;
        $$.type->setFieldName(*$1.string);
        
        if ($3->getAsConstantUnion() == 0 || $3->getAsConstantUnion()->getBasicType() != EbtInt ||
            $3->getAsConstantUnion()->getUnionArrayPointer()->iConst <= 0) {
            parseContext.error($2.line, "structure field array size must be a positive integer", $1.string->c_str(), "");
            parseContext.recover();
        } else {           
            $$.type->setArraySize($3->getAsConstantUnion()->getUnionArrayPointer()->iConst);
        }
    }
    ;

initializer
    : assignment_expression { $$ = $1; }
    ;

declaration_statement 
    : declaration { $$ = $1; }
    ;

statement
    : compound_statement  { $$ = $1; }
    | simple_statement    { $$ = $1; }
    ;

// Grammar Note:  No labeled statements; 'goto' is not supported.

simple_statement 
    : declaration_statement { $$ = $1; }
    | expression_statement  { $$ = $1; } 
    | selection_statement   { $$ = $1; }
    | iteration_statement   { $$ = $1; }
    | jump_statement        { $$ = $1; }
    ;

compound_statement
    : LEFT_BRACE RIGHT_BRACE { $$ = 0; }
    | LEFT_BRACE { parseContext.symbolTable.push(); } statement_list { parseContext.symbolTable.pop(); } RIGHT_BRACE {
        if ($3 != 0)            
            $3->setOperator(EOpSequence); 
        $$ = $3;
    }
    ;

statement_no_new_scope 
    : compound_statement_no_new_scope { $$ = $1; }
    | simple_statement                { $$ = $1; }
    ;

compound_statement_no_new_scope 
    // Statement that doesn't create a new scope, for selection_statement, iteration_statement 
    : LEFT_BRACE RIGHT_BRACE { 
        $$ = 0; 
    }
    | LEFT_BRACE statement_list RIGHT_BRACE { 
        if ($2)
            $2->setOperator(EOpSequence); 
        $$ = $2; 
    }
    ;

statement_list
    : statement {
        $$ = parseContext.intermediate.makeAggregate($1, 0); 
    }
    | statement_list statement { 
        $$ = parseContext.intermediate.growAggregate($1, $2, 0);
    }
    ;

expression_statement
    : SEMICOLON  { $$ = 0; }
    | expression SEMICOLON  { $$ = static_cast<TIntermNode*>($1); }
    ;

selection_statement
    : IF LEFT_PAREN expression RIGHT_PAREN selection_rest_statement { 
        if (parseContext.boolErrorCheck($1.line, $3))
            parseContext.recover();
        $$ = parseContext.intermediate.addSelection($3, $5, $1.line);
    }
    ;

selection_rest_statement 
    : statement ELSE statement {
        $$.node1 = $1;
        $$.node2 = $3;
    }
    | statement { 
        $$.node1 = $1;
        $$.node2 = 0;
    }
    ;

// Grammar Note:  No 'switch'.  Switch statements not supported.

condition
    // In 1996 c++ draft, conditions can include single declarations 
    : expression {
        $$ = $1;
        if (parseContext.boolErrorCheck($1->getLine(), $1))
            parseContext.recover();          
    }
    | fully_specified_type IDENTIFIER EQUAL initializer {
        TIntermNode* intermNode;
        if (parseContext.structQualifierErrorCheck($2.line, $1))
            parseContext.recover();
        if (parseContext.boolErrorCheck($2.line, $1))
            parseContext.recover();
        
        if (!parseContext.executeInitializer($2.line, *$2.string, $1, $4, intermNode))
            $$ = $4;
        else {
            parseContext.recover();
            $$ = 0;
        }
    }
    ;

iteration_statement
    : WHILE LEFT_PAREN { parseContext.symbolTable.push(); ++parseContext.loopNestingLevel; } condition RIGHT_PAREN statement_no_new_scope { 
        parseContext.symbolTable.pop();
        $$ = parseContext.intermediate.addLoop($6, $4, 0, true, $1.line);
        --parseContext.loopNestingLevel;
    }
    | DO { ++parseContext.loopNestingLevel; } statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON {
        if (parseContext.boolErrorCheck($8.line, $6))
            parseContext.recover();
                    
        $$ = parseContext.intermediate.addLoop($3, $6, 0, false, $4.line);
        --parseContext.loopNestingLevel;
    }
    | FOR LEFT_PAREN { parseContext.symbolTable.push(); ++parseContext.loopNestingLevel; } for_init_statement for_rest_statement RIGHT_PAREN statement_no_new_scope {
        parseContext.symbolTable.pop();
        $$ = parseContext.intermediate.makeAggregate($4, $2.line);
        $$ = parseContext.intermediate.growAggregate(
                $$,
                parseContext.intermediate.addLoop($7, reinterpret_cast<TIntermTyped*>($5.node1), reinterpret_cast<TIntermTyped*>($5.node2), true, $1.line),
                $1.line);
        $$->getAsAggregate()->setOperator(EOpSequence);
        --parseContext.loopNestingLevel;
    }
    ;

for_init_statement 
    : expression_statement {
        $$ = $1; 
    } 
    | declaration_statement {
        $$ = $1;
    }
    ;

conditionopt 
    : condition { 
        $$ = $1; 
    }
    | /* May be null */ { 
        $$ = 0; 
    }
    ;

for_rest_statement 
    : conditionopt SEMICOLON { 
        $$.node1 = $1;
        $$.node2 = 0;
    }
    | conditionopt SEMICOLON expression  {
        $$.node1 = $1;
        $$.node2 = $3;
    }
    ;

jump_statement
    : CONTINUE SEMICOLON {
        if (parseContext.loopNestingLevel <= 0) {
            parseContext.error($1.line, "continue statement only allowed in loops", "", "");
            parseContext.recover();
        }        
        $$ = parseContext.intermediate.addBranch(EOpContinue, $1.line);
    }
    | BREAK SEMICOLON {
        if (parseContext.loopNestingLevel <= 0) {
            parseContext.error($1.line, "break statement only allowed in loops", "", "");
            parseContext.recover();
        }        
        $$ = parseContext.intermediate.addBranch(EOpBreak, $1.line);
    }
    | RETURN SEMICOLON {
        $$ = parseContext.intermediate.addBranch(EOpReturn, $1.line);
        if (parseContext.currentFunctionType->getBasicType() != EbtVoid) {
            parseContext.error($1.line, "non-void function must return a value", "return", "");
            parseContext.recover();
        }
    }
    | RETURN expression SEMICOLON {        
        $$ = parseContext.intermediate.addBranch(EOpReturn, $2, $1.line);
        parseContext.functionReturnsValue = true;
        if (parseContext.currentFunctionType->getBasicType() == EbtVoid) {
            parseContext.error($1.line, "void function cannot return a value", "return", "");
            parseContext.recover();
        } else if (*(parseContext.currentFunctionType) != $2->getType()) {
            parseContext.error($1.line, "function return is not matching type:", "return", "");
            parseContext.recover();
        }
    }
    | DISCARD SEMICOLON {
        FRAG_ONLY("discard", $1.line);
        $$ = parseContext.intermediate.addBranch(EOpKill, $1.line);
    }        
    ;

// Grammar Note:  No 'goto'.  Gotos are not supported.

translation_unit
    : external_declaration { 
        $$ = $1; 
        parseContext.treeRoot = $$; 
    }
    | translation_unit external_declaration {
        $$ = parseContext.intermediate.growAggregate($1, $2, 0);
        parseContext.treeRoot = $$;
    }
    ;

external_declaration
    : function_definition { 
        $$ = $1; 
    }
    | declaration { 
        $$ = $1; 
    }
    ;

function_definition
    : function_prototype {
        TFunction& function = *($1.function);
        TFunction* prevDec = static_cast<TFunction*>(parseContext.symbolTable.find(function.getMangledName()));
        //
        // Note:  'prevDec' could be 'function' if this is the first time we've seen function
        // as it would have just been put in the symbol table.  Otherwise, we're looking up
        // an earlier occurance.
        //
        if (prevDec->isDefined()) {
            //
            // Then this function already has a body.
            //
            parseContext.error($1.line, "function already has a body", function.getName().c_str(), "");
            parseContext.recover();
        }
        prevDec->setDefined();
        
        //
        // Raise error message if main function takes any parameters or return anything other than void
        //
        if (function.getName() == "main") {
            if (function.getParamCount() > 0) {
                parseContext.error($1.line, "function cannot take any parameter(s)", function.getName().c_str(), "");
                parseContext.recover();
            }
            if (function.getReturnType().getBasicType() != EbtVoid) {
                parseContext.error($1.line, "", function.getReturnType().getBasicString(), "main function cannot return a value" );
                parseContext.recover();
            }            
        }
   
        //
        // New symbol table scope for body of function plus its arguments
        //
        parseContext.symbolTable.push();
        
        //
        // Remember the return type for later checking for RETURN statements.
        //
        parseContext.currentFunctionType = &(prevDec->getReturnType());
        parseContext.functionReturnsValue = false;
        
        // 
        // Insert parameters into the symbol table.
        // If the parameter has no name, it's not an error, just don't insert it 
        // (could be used for unused args).
        //
        // Also, accumulate the list of parameters into the HIL, so lower level code
        // knows where to find parameters.
        //
        TIntermAggregate* paramNodes = new TIntermAggregate;
        for (int i = 0; i < function.getParamCount(); i++) {
            TParameter& param = function[i];
            if (param.name != 0) {
                TVariable *variable = new TVariable(param.name, *param.type);
                // 
                // Insert the parameters with name in the symbol table.
                //
                if (! parseContext.symbolTable.insert(*variable)) {
                    parseContext.error($1.line, "redefinition", variable->getName().c_str(), "");
                    parseContext.recover();
                    delete variable;
                }
                //
                // Transfer ownership of name pointer to symbol table.
                //
                param.name = 0;
                
                //
                // Add the parameter to the HIL
                //                
                paramNodes = parseContext.intermediate.growAggregate(
                                               paramNodes, 
                                               parseContext.intermediate.addSymbol(variable->getUniqueId(),
                                                                       variable->getName(),
                                                                       variable->getType(), $1.line), 
                                               $1.line);
            } else {
                paramNodes = parseContext.intermediate.growAggregate(paramNodes, parseContext.intermediate.addSymbol(0, "", *param.type, $1.line), $1.line);
            }
        }
        parseContext.intermediate.setAggregateOperator(paramNodes, EOpParameters, $1.line);
        $1.intermAggregate = paramNodes;
        parseContext.loopNestingLevel = 0;
    }
    compound_statement_no_new_scope {
        //?? Check that all paths return a value if return type != void ?
        //   May be best done as post process phase on intermediate code
        if (parseContext.currentFunctionType->getBasicType() != EbtVoid && ! parseContext.functionReturnsValue) {
            parseContext.error($1.line, "function does not return a value:", "", $1.function->getName().c_str());
            parseContext.recover();
        }
        parseContext.symbolTable.pop();
        $$ = parseContext.intermediate.growAggregate($1.intermAggregate, $3, 0);
        parseContext.intermediate.setAggregateOperator($$, EOpFunction, $1.line);
        $$->getAsAggregate()->setName($1.function->getMangledName().c_str());
        $$->getAsAggregate()->setType($1.function->getReturnType());
        
        // store the pragma information for debug and optimize and other vendor specific 
        // information. This information can be queried from the parse tree
        $$->getAsAggregate()->setOptimize(parseContext.contextPragma.optimize);
        $$->getAsAggregate()->setDebug(parseContext.contextPragma.debug);
        $$->getAsAggregate()->addToPragmaTable(parseContext.contextPragma.pragmaTable);
    }
    ;

%%
