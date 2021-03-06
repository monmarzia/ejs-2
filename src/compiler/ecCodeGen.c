/**
    ecCodeGen.c - Ejscript code generator
  
    This module generates code for a program that is represented by an in-memory AST set of nodes.
  
    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejsCompiler.h"

/********************************** Defines ***********************************/
/*
    State level macros. Enter/Leave manage state and inheritance of state.
 */
#undef ENTER
#define ENTER(a)    if (ecEnterState(a) < 0) { return; } else

#undef LEAVE
#define LEAVE(cp)   ecLeaveState(cp)

#define SAVE_ONLEFT(cp)                                     \
    if (1) {                                                \
            cp->state->saveOnLeft = cp->state->onLeft;      \
            cp->state->onLeft = 0;                          \
    } else

#define RESTORE_ONLEFT(cp)                                  \
    cp->state->onLeft = cp->state->saveOnLeft

/***************************** Forward Declarations ***************************/

static void     addDebug(EcCompiler *cp, EcNode *np);
static void     addDebugLine(EcCompiler *cp, EcCodeGen *code, int offset, wchar *source);
static void     addException(EcCompiler *cp, uint tryStart, uint tryEnd, EjsType *catchType, uint handlerStart, 
                    uint handlerEnd, int numBlocks, int numStack, int flags);
static void     addJump(EcCompiler *cp, EcNode *np, int kind);
static void     addModule(EcCompiler *cp, EjsModule *mp);
static EcCodeGen *allocCodeBuffer(EcCompiler *cp);
static void     badNode(EcCompiler *cp, EcNode *np);
static void     copyCodeBuffer(EcCompiler *cp, EcCodeGen *dest, EcCodeGen *code);
static void     createInitializer(EcCompiler *cp, EjsModule *mp);
static void     discardBlockItems(EcCompiler *cp, int preserve);
static void     discardStackItems(EcCompiler *cp, int preserve);
static void     emitNamespace(EcCompiler *cp, EjsNamespace *nsp);
static int      flushModule(MprFile *file, EcCodeGen *code);
static void     genBinaryOp(EcCompiler *cp, EcNode *np);
static void     genBlock(EcCompiler *cp, EcNode *np);
static void     genBreak(EcCompiler *cp, EcNode *np);
static void     genBoundName(EcCompiler *cp, EcNode *np);
static void     genCall(EcCompiler *cp, EcNode *np);
static void     genCatchArg(EcCompiler *cp, EcNode *np);
static void     genClass(EcCompiler *cp, EcNode *child);
static void     genClassName(EcCompiler *cp, EjsType *type);
static void     genContinue(EcCompiler *cp, EcNode *np);
static void     genDassign(EcCompiler *cp, EcNode *np);
static void     genDirectives(EcCompiler *cp, EcNode *np, bool saveResult);
static void     genDo(EcCompiler *cp, EcNode *np);
static void     genDot(EcCompiler *cp, EcNode *np, EcNode **rightMost);
static void     genError(EcCompiler *cp, EcNode *np, char *fmt, ...);
static void     genEndFunction(EcCompiler *cp, EcNode *np);
static void     genExpressions(EcCompiler *cp, EcNode *np);
static void     genField(EcCompiler *cp, EcNode *np);
static void     genFor(EcCompiler *cp, EcNode *np);
static void     genForIn(EcCompiler *cp, EcNode *np);
static void     genFunction(EcCompiler *cp, EcNode *np);
static void     genHash(EcCompiler *cp, EcNode *np);
static void     genIf(EcCompiler *cp, EcNode *np);
static void     genLeftHandSide(EcCompiler *cp, EcNode *np);
static void     genLiteral(EcCompiler *cp, EcNode *np);
static void     genLogicalOp(EcCompiler *cp, EcNode *np);
static void     genModule(EcCompiler *cp, EcNode *np);
static void     genName(EcCompiler *cp, EcNode *np);
static void     genNameExpr(EcCompiler *cp, EcNode *np);
static void     genNew(EcCompiler *cp, EcNode *np);
static void     genArrayLiteral(EcCompiler *cp, EcNode *np);
static void     genObjectLiteral(EcCompiler *cp, EcNode *np);
static void     genProgram(EcCompiler *cp, EcNode *np);
static void     genPragmas(EcCompiler *cp, EcNode *np);
static void     genPostfixOp(EcCompiler *cp, EcNode *np);
static void     genReturn(EcCompiler *cp, EcNode *np);
static void     genSuper(EcCompiler *cp, EcNode *np);
static void     genSwitch(EcCompiler *cp, EcNode *np);
static void     genThis(EcCompiler *cp, EcNode *np);
static void     genThrow(EcCompiler *cp, EcNode *np);
static void     genTry(EcCompiler *cp, EcNode *np);
static void     genUnaryOp(EcCompiler *cp, EcNode *np);
static void     genUnboundName(EcCompiler *cp, EcNode *np);
static void     genUseNamespace(EcCompiler *cp, EcNode *np);
static void     genVar(EcCompiler *cp, EcNode *np);
static void     genVarDefinition(EcCompiler *cp, EcNode *np);
static void     genWith(EcCompiler *cp, EcNode *np);
static int      getCodeLength(EcCompiler *cp, EcCodeGen *code);
static EcNode   *getNextNode(EcCompiler *cp, EcNode *np, int *next);
static EcNode   *getPrevNode(EcCompiler *cp, EcNode *np, int *next);
static int      getStackCount(EcCompiler *cp);
static int      mapToken(EcCompiler *cp, int tokenId);
static MprFile  *openModuleFile(EcCompiler *cp, cchar *filename);
static void     orderModule(EcCompiler *cp, MprList *list, EjsModule *mp);
static void     patchJumps(EcCompiler *cp, int kind, int target);
static void     popStack(EcCompiler *cp, int count);
static void     processNode(EcCompiler *cp, EcNode *np);
static void     processModule(EcCompiler *cp, EjsModule *mp);
static void     pushStack(EcCompiler *cp, int count);
static void     setCodeBuffer(EcCompiler *cp, EcCodeGen *saveCode);
static void     setFunctionCode(EcCompiler *cp, EjsFunction *fun, EcCodeGen *code);
static void     setStack(EcCompiler *cp, int count);

/************************************ Code ************************************/
/*
    Generate code for evaluating conditional compilation directives
 */
PUBLIC void ecGenConditionalCode(EcCompiler *cp, EcNode *np, EjsModule *mp)
{
    ENTER(cp);

    addModule(cp, mp);
    genDirectives(cp, np, 1);

    if (cp->errorCount > 0) {
        ecRemoveModule(cp, mp);
        LEAVE(cp);
        return;
    }
    createInitializer(cp, mp);
    ecRemoveModule(cp, mp);
    LEAVE(cp);
}


/*
    Top level for code generation. Loop through the AST nodes recursively.
 */
PUBLIC int ecCodeGen(EcCompiler *cp)
{
    EjsModule   *mp;
    EcNode      *np;
    MprList     *modules;
    int         next, i, count;

    if (ecEnterState(cp) < 0) {
        return EJS_ERR;
    }
    count = mprGetListLength(cp->nodes);
    for (i = 0; i < count && !cp->error; i++) {
        np = mprGetItem(cp->nodes, i);
        cp->fileState = cp->state;
        cp->fileState->strict = cp->strict;
        if (np) {
            processNode(cp, np);
        }
    }
    if (cp->error) {
        return EJS_ERR;
    }

    /*
        Open once if merging into a single output file
     */
    if (cp->outputFile) {
        for (next = 0; (mp = mprGetNextItem(cp->modules, &next)) != 0; ) {
            if (next <= 1 || mp->globalProperties || mp->hasInitializer || 
                    ejsCompareAsc(cp->ejs, mp->name, EJS_DEFAULT_MODULE) != 0) {
                break;
            }
        }
        if (openModuleFile(cp, cp->outputFile) == 0) {
            return EJS_ERR;
        }
    }

    /*
        Now generate code for all the modules
     */
    modules = mprCreateList(-1, 0);
    for (next = 0; (mp = mprGetNextItem(cp->modules, &next)) != 0; ) {
        orderModule(cp, modules, mp);
    }
    for (next = 0; (mp = mprGetNextItem(modules, &next)) != 0 && !cp->fatalError; ) {
        //  TODO -- remove this test. Should be able to add to a loaded module??
        assure(!mp->loaded);
        if (mp->loaded) {
            continue;
        }
        /*
            Don't generate the default module unless it contains some real code or definitions and 
            we have more than one module.
         */
        if (mprGetListLength(cp->modules) == 1 || mp->globalProperties || mp->hasInitializer || 
                ejsCompareAsc(cp->ejs, mp->name, EJS_DEFAULT_MODULE) != 0) {
            mp->initialized = 0;
            processModule(cp, mp);
        }
    }
    cp->modules = modules;

    if (cp->outputFile) {
        if (flushModule(cp->file, cp->state->code) < 0) {
            genError(cp, 0, "Cannot write to module file %s", cp->outputFile);
        }
        mprCloseFile(cp->file);
    }
    cp->file = 0;
    ecLeaveState(cp);
    return (cp->fatalError) ? EJS_ERR : 0;
}


static void orderModule(EcCompiler *cp, MprList *list, EjsModule *mp)
{
    EjsModule   *dp;
    int         next;
    
    mp->visited = 1;
    for (next = 0; (dp = mprGetNextItem(mp->dependencies, &next)) != 0; ) {
        if (mprLookupItem(list, dp) < 0 && mprLookupItem(cp->modules, dp) >= 0) {
            if (!dp->visited) {
                orderModule(cp, list, dp);
            }
        }
    }
    if (mprLookupItem(list, mp) < 0) {
        mprAddItem(list, mp);
    }
    mp->visited = 0;
}


static void genArgs(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    int         next;

    ENTER(cp);
    assure(np->kind == N_ARGS);

    cp->state->needsValue = 1;

    next = 0;
    while ((child = getNextNode(cp, np, &next)) && !cp->error) {
        if (child->kind == N_ASSIGN_OP) {
            child->needDup = 1;
        }
        processNode(cp, child);
        child->needDup = 0;
    }
    LEAVE(cp);
}


static void genSpread(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    int         next;

    ENTER(cp);

    assure(np->kind == N_SPREAD);

    next = 0;
    while ((child = getNextNode(cp, np, &next)) && !cp->error) {
        if (child->kind == N_ASSIGN_OP) {
            child->needDup = 1;
        }
        processNode(cp, child);
        child->needDup = 0;
    }
    ecEncodeOpcode(cp, EJS_OP_SPREAD);
    LEAVE(cp);
}


/*
    Generate an assignment expression
 */
static void genAssignOp(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);

    assure(np->kind == N_ASSIGN_OP);
    assure(np->left);
    assure(np->right);

    state = cp->state;
    state->onLeft = 0;

    /*
        Dup the object on the stack so it is available for subsequent operations
     */
    if (np->needDupObj) {
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
    }

    /*
        Process the expression on the right. Leave the result on the stack.
     */
    if (np->right->kind == N_ASSIGN_OP) {
        np->right->needDup = 1;
    }

    state->needsValue = 1;
    processNode(cp, np->right);
    state->needsValue = 0;

    if (np->needDupObj) {
        /*
            Get the object on the top above the value
         */
        ecEncodeOpcode(cp, EJS_OP_SWAP);
    }

    /*
        If this expression is part of a function argument, the result must be preserved.
     */
    if (np->needDup || state->next->needsValue) {
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
    }

    /*
        Store to the left hand side
     */
    genLeftHandSide(cp, np->left);
    LEAVE(cp);
}


static void genBinaryOp(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);

    state = cp->state;
    state->needsValue = 1;

    assure(np->kind == N_BINARY_OP);

    switch (np->tokenId) {
    case T_LOGICAL_AND:
    case T_LOGICAL_OR:
        genLogicalOp(cp, np);
        break;

    default:
        if (np->left) {
            processNode(cp, np->left);
        }
        if (np->right) {
            processNode(cp, np->right);
        }
        ecEncodeOpcode(cp, mapToken(cp, np->tokenId));
        popStack(cp, 2);
        pushStack(cp, 1);
        break;
    }
    assure(state == cp->state);
    LEAVE(cp);
}


static void genBreak(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);

    state = cp->state;
    discardBlockItems(cp, state->code->blockMark);
    if (state->captureFinally) {
        ecEncodeOpcode(cp, EJS_OP_CALL_FINALLY);
    } else if (cp->state->captureBreak) {
        ecEncodeOpcode(cp, EJS_OP_END_EXCEPTION);
    }
    if (state->code->jumps == 0 || !(state->code->jumpKinds & EC_JUMP_BREAK)) {
        genError(cp, np, "Illegal break statement");
    } else {
        discardStackItems(cp, state->code->breakMark);
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        addJump(cp, np, EC_JUMP_BREAK);
        ecEncodeInt32(cp, 0);
    }
    LEAVE(cp);
}


static void genBlock(EcCompiler *cp, EcNode *np)
{
    EjsNamespace    *namespace;
    EcState         *state;
    EjsBlock        *block;
    EjsLookup       *lookup;
    EcNode          *child;
    int             next;

    ENTER(cp);

    state = cp->state;
    block = (EjsBlock*) np->blockRef;

    if (block && np->createBlockObject) {
        state->prevBlockState = cp->blockState;
        cp->blockState = state;

        lookup = &np->lookup;
        if (lookup->slotNum >= 0) {
            assure(lookup->bind);
            ecEncodeOpcode(cp, EJS_OP_OPEN_BLOCK);
            ecEncodeNum(cp, lookup->slotNum);
            ecEncodeNum(cp, lookup->nthBlock);
            state->code->blockCount++;
        }
        /*
            Emit block namespaces
         */
        if (block->namespaces.length > 0) {
            for (next = 0; ((namespace = (EjsNamespace*) mprGetNextItem(&block->namespaces, &next)) != 0); ) {
                if (namespace->value->value[0] == '-') {
                    emitNamespace(cp, namespace);
                }
            }
        }
        state->letBlock = block;
        state->letBlockNode = np;

        next = 0;
        while ((child = getNextNode(cp, np, &next))) {
            processNode(cp, child);
        }
        if (lookup->slotNum >= 0) {
            assure(lookup->bind);
            ecEncodeOpcode(cp, EJS_OP_CLOSE_BLOCK);
            state->code->blockCount--;
        }
        cp->blockState = state->prevBlockState;
        ecAddNameConstant(cp, np->qname);

    } else {
        next = 0;
        while ((child = getNextNode(cp, np, &next))) {
            processNode(cp, child);
        }
    }
    LEAVE(cp);
}


/*
    Block scope variable reference
 */
static void genBlockName(EcCompiler *cp, int slotNum, int nthBlock)
{
    int         code;

    assure(slotNum >= 0);

    code = (!cp->state->onLeft) ?  EJS_OP_GET_BLOCK_SLOT :  EJS_OP_PUT_BLOCK_SLOT;
    ecEncodeOpcode(cp, code);
    ecEncodeNum(cp, slotNum);
    ecEncodeNum(cp, nthBlock);
    pushStack(cp, (cp->state->onLeft) ? -1 : 1);
}


static void genContinue(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    discardBlockItems(cp, cp->state->code->blockMark);
    if (cp->state->captureFinally) {
        ecEncodeOpcode(cp, EJS_OP_CALL_FINALLY);
    } else if (cp->state->captureBreak) {
        ecEncodeOpcode(cp, EJS_OP_END_EXCEPTION);
    }
    if (cp->state->code->jumps == 0 || !(cp->state->code->jumpKinds & EC_JUMP_CONTINUE)) {
        genError(cp, np, "Illegal continue statement");
    } else {
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        addJump(cp, np, EC_JUMP_CONTINUE);
        ecEncodeInt32(cp, 0);
    }
    LEAVE(cp);
}


static void genDelete(EcCompiler *cp, EcNode *np)
{
    Ejs         *ejs;
    EcNode      *left, *lright;

    ENTER(cp);
    assure(np);

    ejs = cp->ejs;
    left = np->left;
    assure(left);

    switch (left->kind) {
    case N_DOT:
        processNode(cp, left->left);
        lright = left->right;
        if (lright->kind == N_QNAME) {
            /* delete obj.name */
            genNameExpr(cp, lright);
            ecEncodeOpcode(cp, EJS_OP_DELETE_NAME_EXPR);
            popStack(cp, 3);
            pushStack(cp, 1);
        } else {
            /* delete obj[expr] */
            ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
            ecEncodeConst(cp, ESV(empty));
            processNode(cp, lright);
            ecEncodeOpcode(cp, EJS_OP_DELETE_NAME_EXPR);
            popStack(cp, 2);
            pushStack(cp, 1);
        }
        break;

    case N_QNAME:
        /* delete space::name */
        genNameExpr(cp, left);
        ecEncodeOpcode(cp, EJS_OP_DELETE_SCOPED_NAME_EXPR);
        popStack(cp, 2);
        pushStack(cp, 1);
        break;

    default:
        assure(0);
    }
    LEAVE(cp);
}


/*
    Global variable
 */
static void genGlobalName(EcCompiler *cp, int slotNum)
{
    int     code;

    assure(slotNum >= 0);

    code = (!cp->state->onLeft) ?  EJS_OP_GET_GLOBAL_SLOT :  EJS_OP_PUT_GLOBAL_SLOT;
    ecEncodeOpcode(cp, code);
    ecEncodeNum(cp, slotNum);

    pushStack(cp, (cp->state->onLeft) ? -1 : 1);
}


/*
    Function local variable or argument reference
 */
static void genLocalName(EcCompiler *cp, int slotNum)
{
    int     code;

    assure(slotNum >= 0);

    if (slotNum < 10) {
        code = (!cp->state->onLeft) ?  EJS_OP_GET_LOCAL_SLOT_0 :  EJS_OP_PUT_LOCAL_SLOT_0;
        ecEncodeOpcode(cp, code + slotNum);

    } else {
        code = (!cp->state->onLeft) ?  EJS_OP_GET_LOCAL_SLOT :  EJS_OP_PUT_LOCAL_SLOT;
        ecEncodeOpcode(cp, code);
        ecEncodeNum(cp, slotNum);
    }
    pushStack(cp, (cp->state->onLeft) ? -1 : 1);
}


/*
    Generate code for a logical operator. Called by genBinaryOp
  
    (expression OP expression)
 */
static void genLogicalOp(EcCompiler *cp, EcNode *np)
{
    EcState     *state;
    EcCodeGen   *saveCode, *rightCode;
    int         doneIfTrue, rightLen;

    ENTER(cp);

    state = cp->state;
    saveCode = state->code;
    rightCode = 0;

    assure(np->kind == N_BINARY_OP);

    switch (np->tokenId) {
    case T_LOGICAL_AND:
        doneIfTrue = 0;
        break;

    case T_LOGICAL_OR:
        doneIfTrue = 1;
        break;

    default:
        doneIfTrue = 1;
        assure(0);
        ecEncodeOpcode(cp, mapToken(cp, np->tokenId));
        break;
    }

    /*
        Process the conditional test. Put the pop for the branch here prior to the right hand side.
     */
    processNode(cp, np->left);
    ecEncodeOpcode(cp, EJS_OP_DUP);
    pushStack(cp, 1);
    popStack(cp, 1);

    assure(np->right);
    if (np->right) {
        state->code = allocCodeBuffer(cp);
        rightCode = state->code;
        /*
            Evaluating right hand side, so we must pop the left side duped value.
         */
        ecEncodeOpcode(cp, EJS_OP_POP);
        popStack(cp, 1);
        processNode(cp, np->right);
    }
    rightLen = (int) mprGetBufLength(rightCode->buf);

    /*
        Now copy the code to the output code buffer
     */
    setCodeBuffer(cp, saveCode);

    /*
        Jump to done if we know the result due to lazy evalation.
     */
    if (rightLen > 0 && rightLen < 0x7f && cp->optimizeLevel > 0) {
        ecEncodeOpcode(cp, (doneIfTrue) ? EJS_OP_BRANCH_TRUE_8: EJS_OP_BRANCH_FALSE_8);
        ecEncodeByte(cp, rightLen);
    } else {
        ecEncodeOpcode(cp, (doneIfTrue) ? EJS_OP_BRANCH_TRUE: EJS_OP_BRANCH_FALSE);
        ecEncodeInt32(cp, rightLen);
    }
    copyCodeBuffer(cp, state->code, rightCode);
    assure(state == cp->state);
    LEAVE(cp);
}


/*
    Generate a property name reference based on the object already pushed.
    The owning object (pushed on the VM stack) may be an object or a type.
 */
static void genPropertyName(EcCompiler *cp, int slotNum)
{
    EcState     *state;
    int         code;

    assure(slotNum >= 0);

    state = cp->state;

    assure(0);
    if (slotNum < 10) {
        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_SLOT_0 :  EJS_OP_PUT_OBJ_SLOT_0;
        ecEncodeOpcode(cp, code + slotNum);

    } else {
        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_SLOT :  EJS_OP_PUT_OBJ_SLOT;
        ecEncodeOpcode(cp, code);
        ecEncodeNum(cp, slotNum);
    }

    popStack(cp, 1);
    pushStack(cp, (state->onLeft) ? -1 : 1);
}


/*
    Generate a class property name reference
    The owning object (pushed on the VM stack) may be an object or a type. We must access its base class.
 */
static void genBaseClassPropertyName(EcCompiler *cp, int slotNum, int nthBase)
{
    EcState     *state;
    int         code;

    assure(slotNum >= 0);

    state = cp->state;

    assure(0);
    code = (!cp->state->onLeft) ?  EJS_OP_GET_TYPE_SLOT : EJS_OP_PUT_TYPE_SLOT;

    ecEncodeOpcode(cp, code);
    ecEncodeNum(cp, slotNum);
    ecEncodeNum(cp, nthBase);

    popStack(cp, 1);
    pushStack(cp, (state->onLeft) ? -1 : 1);
}


/*
    Generate a class property name reference
    The owning object (pushed on the VM stack) may be an object or a type. We must access its base class.
 */
static void genThisBaseClassPropertyName(EcCompiler *cp, EjsType *type, int slotNum)
{
    EcState     *state;
    int         code, nthBase;

    assure(0);
    assure(slotNum >= 0);
    assure(type && ejsIsType(ejs, type));
    state = cp->state;

    /*
        Count based up from object 
     */
    for (nthBase = 0; type->baseType; type = type->baseType) {
        nthBase++;
    }
    code = (!state->onLeft) ?  EJS_OP_GET_THIS_TYPE_SLOT :  EJS_OP_PUT_THIS_TYPE_SLOT;
    ecEncodeOpcode(cp, code);
    ecEncodeNum(cp, slotNum);
    ecEncodeNum(cp, nthBase);
    pushStack(cp, (state->onLeft) ? -1 : 1);
}


/*
    Generate a class name reference or a global reference.
 */
static void genClassName(EcCompiler *cp, EjsType *type)
{
    Ejs         *ejs;
    EcState     *state;
    int         slotNum;

    assure(type);

    ejs = cp->ejs;
    state = cp->state;

    if (type == ejs->global) {
        ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);
        pushStack(cp, 1);
        return;
    }
    slotNum = ejsLookupProperty(ejs, ejs->global, type->qname);
    if (cp->bind && slotNum < ES_global_NUM_CLASS_PROP) {
        //  MOB - WARNING: this won't work if classes are implemented like Record.
        assure(slotNum >= 0);
        genGlobalName(cp, slotNum);

    } else if (type == state->currentClass &&
            (!state->inFunction || (state->currentFunction && state->currentFunction->staticMethod))) {
        ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
        pushStack(cp, 1);

    } else {
        ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);
        pushStack(cp, 1);
        ecEncodeOpcode(cp, EJS_OP_GET_OBJ_NAME);
        ecEncodeName(cp, type->qname);
        popStack(cp, 1);
        pushStack(cp, 1);
    }
}


/*
    Generate a property reference in the current object
 */
static void genPropertyViaThis(EcCompiler *cp, int slotNum)
{
    EcState         *state;
    int             code;

    assure(slotNum >= 0);
    assure(0);
    state = cp->state;

    /*
        Property in the current "this" object
     */
    if (slotNum < 10) {
        code = (!state->onLeft) ?  EJS_OP_GET_THIS_SLOT_0 :  EJS_OP_PUT_THIS_SLOT_0;
        ecEncodeOpcode(cp, code + slotNum);

    } else {
        code = (!state->onLeft) ?  EJS_OP_GET_THIS_SLOT :  EJS_OP_PUT_THIS_SLOT;
        ecEncodeOpcode(cp, code);
        ecEncodeNum(cp, slotNum);
    }
    pushStack(cp, (cp->state->onLeft) ? -1 : 1);
}


/*
    Generate code for a bound name reference. We already know the slot for the property and its owning type.
 */
static void genBoundName(EcCompiler *cp, EcNode *np)
{
    Ejs         *ejs;
    EcState     *state;
    EjsLookup   *lookup;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;
    lookup = &np->lookup;

    assure(lookup->slotNum >= 0);
    assure(lookup->bind);

    if (lookup->obj == ejs->global) {
        /*
            Global variable
         */
        if (lookup->slotNum < 0 || lookup->slotNum > ES_global_NUM_CLASS_PROP) {
            lookup->bind = 0;
            genUnboundName(cp, np);

        } else {
            genGlobalName(cp, lookup->slotNum);
        }
#if OLD
    } else if (ejsIsFunction(ejs, lookup->obj) && lookup->nthBlock == 0) {
        genLocalName(cp, lookup->slotNum);
#else
    } else if (lookup->obj == (EjsObj*) state->currentFunction->activation) {
        genLocalName(cp, lookup->slotNum);
#endif

    } else if ((ejsIsBlock(ejs, lookup->obj) || ejsIsFunction(ejs, lookup->obj)) && 
            (!ejsIsType(ejs, lookup->obj) && !ejsIsPrototype(ejs, lookup->obj))) {
        genBlockName(cp, lookup->slotNum, lookup->nthBlock);

    } else if (lookup->useThis) {
        if (lookup->instanceProperty) {
            /*
                Property being accessed via the current object "this" or an explicit object?
             */
            genPropertyViaThis(cp, lookup->slotNum);
        } else {
            genThisBaseClassPropertyName(cp, (EjsType*) lookup->obj, lookup->slotNum);
        }

    } else if (!state->currentObjectNode) {
        if (lookup->instanceProperty) {
            genBlockName(cp, lookup->slotNum, lookup->nthBlock);

        } else {
            /*
                Static property with no explicit object. ie. Not "obj.property". The property was found via a scope search.
                We ignore nthBase as we use the actual type (lookup->obj) where the property was found.
             */
            if (state->inClass && state->inFunction && state->currentFunction->staticMethod) {
                genThisBaseClassPropertyName(cp, (EjsType*) lookup->obj, lookup->slotNum);
                
            } else {
                if (state->inFunction && ejsIsA(ejs, (EjsObj*) state->currentClass, (EjsType*) lookup->obj)) {
                    genThisBaseClassPropertyName(cp, (EjsType*) lookup->obj, lookup->slotNum);
                    
                } else {
                    SAVE_ONLEFT(cp);
                    genClassName(cp, (EjsType*) lookup->obj);
                    RESTORE_ONLEFT(cp);
                    genPropertyName(cp, lookup->slotNum);
                }
            }
        }

    } else {
        /*
            Explicity object. ie. "obj.property". The object in a dot expression is already pushed on the stack.
            Determine if we can access the object itself or if we need to use the type of the object to access
            static properties.
         */
        if (lookup->instanceProperty) {
            genPropertyName(cp, lookup->slotNum);

        } else {
            /*
                Property is in the nth base class from the object already pushed on the stack (left hand side).
             */
            genBaseClassPropertyName(cp, lookup->slotNum, lookup->nthBase);
        }
    }
    LEAVE(cp);
}


static void processNodeGetValue(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);
    cp->state->needsValue = 1;
    processNode(cp, np);
    LEAVE(cp);
}


static int genCallArgs(EcCompiler *cp, EcNode *np) 
{
    if (np == 0) {
        return 0;
    }
    processNode(cp, np);
    return mprGetListLength(np->children);
}


static void genCallSequence(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EcNode          *left, *right;
    EcState         *state;
    EjsFunction     *fun;
    EjsLookup       *lookup;
    int             fast, argc, staticMethod;    
        
    ejs = cp->ejs;
    state = cp->state;
    left = np->left;
    right = np->right;
    lookup = &np->left->lookup;
    argc = 0;
    
    if (!lookup->bind || lookup->slotNum < 0) {
        /*
            Unbound or Function expression or instance variable containing a function. Cannot use fast path op codes below.
         */
        if (left->kind == N_QNAME && !(left->name.nameExpr || left->name.qualifierExpr)) {
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_SCOPED_NAME);
            ecEncodeName(cp, np->qname);
            
        } else if (left->kind == N_DOT && left->right->kind == N_QNAME && 
                   !(left->right->name.nameExpr || left->right->name.qualifierExpr)) {
            processNodeGetValue(cp, left->left);
            if (state->dupLeft) {
                ecEncodeOpcode(cp, EJS_OP_DUP);
                pushStack(cp, 1);
                state->dupLeft = 0;
            }
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_NAME);
            ecEncodeName(cp, np->qname);
            popStack(cp, 1);
            
        } else {
            /*
                Could be an arbitrary expression on the left. Need a consistent way to save the right most
                object before the property. 
             */
            processNodeGetValue(cp, left);
            ecEncodeOpcode(cp, EJS_OP_LOAD_THIS_LOOKUP);
            pushStack(cp, 1);
            ecEncodeOpcode(cp, EJS_OP_SWAP);
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL);
            popStack(cp, 2);
        }
        ecEncodeNum(cp, argc); 
        popStack(cp, argc);
        return;
    }
        
    fun = (EjsFunction*) lookup->ref;
    staticMethod = (ejsIsFunction(ejs, fun) && fun->staticMethod);
        
    /*
        Use fast opcodes when the call sequence is bindable and either:
            expression.name()
            name
     */
    fast = (left->kind == N_DOT && left->right->kind == N_QNAME) || left->kind == N_QNAME;      
        
    if (!fast) {
        /*
            Resolve a reference to a function expression
            TODO REFACTOR needThis. Example: (function (s) { print(s);})("hello");
         */
        if (left->kind == N_EXPRESSIONS) {
            if (left->right == 0) {
                left->left->needThis = 1;
            } else {
                left->right->needThis = 1;
            }
        } else {
            left->needThis = 1;
        }
        processNodeGetValue(cp, left);
        argc = genCallArgs(cp, right);
        ecEncodeOpcode(cp, EJS_OP_CALL);
        popStack(cp, 2);
        ecEncodeNum(cp, argc); 
        popStack(cp, argc);
        return;
    }
    if (staticMethod) {
        assure(ejsIsType(ejs, lookup->obj));
        if (state->currentClass && state->inFunction && 
                lookup->obj != EST(Object)) {
            /*
                Calling a static method from within a class or subclass. So we can use "this".
             */
            argc = genCallArgs(cp, right);
            assure(0);
            ecEncodeOpcode(cp, EJS_OP_CALL_THIS_STATIC_SLOT);
            ecEncodeNum(cp, lookup->slotNum);
            if (!state->currentFunction->staticMethod) {
                /*
                    If calling from within an instance function, need to step over the instance also
                 */
                lookup->nthBase++;
            }
            ecEncodeNum(cp, lookup->nthBase);
            
        } else if (left->kind == N_DOT && left->right->kind == N_QNAME) {
            /*
                Calling a static method with an explicit object or expression. Call via the object.
             */
            processNode(cp, left->left);
            argc = genCallArgs(cp, right);
            assure(0);
            ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_STATIC_SLOT);
            ecEncodeNum(cp, lookup->slotNum);
            if (lookup->ownerIsType) {
                lookup->nthBase--;
            }
            ecEncodeNum(cp, lookup->nthBase);
            popStack(cp, 1);
            
        } else {
            /*
                Foreign static method. Call directly on the correct class type object.
             */
            genClassName(cp, (EjsType*) lookup->obj);
            argc = genCallArgs(cp, right);
            assure(0);
            ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_STATIC_SLOT);
            ecEncodeNum(cp, lookup->slotNum);
            ecEncodeNum(cp, 0);
            popStack(cp, 1);
        }
        
    } else {
        // pushStack(cp, 1);
        if (left->kind == N_DOT && left->right->kind == N_QNAME) {
            if (left->left->kind == N_THIS) {
                lookup->useThis = 1;
            }
        }
        
        if (lookup->useThis && !lookup->instanceProperty) {
            argc = genCallArgs(cp, right);
            assure(0);
            ecEncodeOpcode(cp, EJS_OP_CALL_THIS_SLOT);
            ecEncodeNum(cp, lookup->slotNum);
            
        } else if (lookup->obj == ejs->global) {
            /*
                Instance function or type being invoked as a constructor (e.g. Date(obj))
             */
            argc = genCallArgs(cp, right);
            assure(0);
            ecEncodeOpcode(cp, EJS_OP_CALL_GLOBAL_SLOT);
            ecEncodeNum(cp, lookup->slotNum);
            
        } else if (lookup->instanceProperty && left->left) {
            processNodeGetValue(cp, left->left);
            argc = genCallArgs(cp, right);
            assure(0);
            ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_INSTANCE_SLOT);
            ecEncodeNum(cp, lookup->slotNum);
            popStack(cp, 1);
            
        } else if (ejsIsType(ejs, lookup->obj) || ejsIsPrototype(ejs, lookup->obj)) {
            if (left->kind == N_DOT && left->right->kind == N_QNAME) {
                processNodeGetValue(cp, left->left);
                argc = genCallArgs(cp, right);
                assure(0);
                ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_SLOT);
                assure(lookup->slotNum >= 0);
                ecEncodeNum(cp, lookup->slotNum);
                popStack(cp, 1);
                
            } else {
                left->needThis = 1;
                processNodeGetValue(cp, left);
                argc = genCallArgs(cp, right);
                ecEncodeOpcode(cp, EJS_OP_CALL);
                popStack(cp, 2);
            }
            
        } else if (ejsIsBlock(ejs, lookup->obj)) {
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_BLOCK_SLOT);
            ecEncodeNum(cp, lookup->slotNum);
            ecEncodeNum(cp, lookup->nthBlock);
        }
    }
    ecEncodeNum(cp, argc); 
    popStack(cp, argc);
}


/*
    Code generation for function calls
 */
static void genCall(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EcNode          *left, *right;
    EcState         *state;
    EjsFunction     *fun;
    int             argc, hasResult;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;
    left = np->left;
    right = np->right;
    fun = (EjsFunction*) np->lookup.ref;    
    
    if (left->kind == N_NEW && !left->newExpr.callConstructors) {
        processNode(cp, left);
        LEAVE(cp);
        return;
    }
    if (left->kind == N_NEW) {
        processNode(cp, left);
        argc = genCallArgs(cp, right);
        ecEncodeOpcode(cp, EJS_OP_CALL_CONSTRUCTOR);
        ecEncodeNum(cp, argc);
        popStack(cp, argc);
        LEAVE(cp);
        return;
    }
    genCallSequence(cp, np);

    /*
        Process the function return value. Call by ref has a this pointer plus method reference plus args
     */
    hasResult = 0;
    if (fun && ejsIsFunction(ejs, fun)) {
        if (fun->resultType && fun->resultType != EST(Void)) {
            hasResult = 1;

        } else if (fun->hasReturn || ejsIsType(ejs, fun)) {
            /*
                Untyped function, but it has a return stmt.
                We don't do data flow to make sure all return cases have returns (sorry).
             */
            hasResult = 1;
        }
        if (state->needsValue && !hasResult) {
            genError(cp, np, "Function call does not return a value.");
        }
    }
    /*
        If calling a type as a constructor (Date()), must push result
     */
    if (state->needsValue || ejsIsType(ejs, np->lookup.ref)) {
        ecEncodeOpcode(cp, EJS_OP_PUSH_RESULT);
        pushStack(cp, 1);
    }
    LEAVE(cp);
}


static void genCatchArg(EcCompiler *cp, EcNode *np)
{
    ecEncodeOpcode(cp, EJS_OP_PUSH_CATCH_ARG);
    pushStack(cp, 1);
}


/*
    Code is injected before existing code
 */
static int injectCode(Ejs *ejs, EjsFunction *fun, EcCodeGen *extra)
{
    EjsCode     *old;
    EjsEx       *ex;
    EjsDebug    *debug;
    uchar       *byteCode;
    int         next, i, len, codeLen, extraCodeLen;

    if (extra == NULL || extra->buf == NULL) {
        return 0;
    }
    old = fun->body.code;
    codeLen = (fun->body.code) ? old->codeLen : 0;
    extraCodeLen = (int) mprGetBufLength(extra->buf);
    len = codeLen + extraCodeLen;

    if (extraCodeLen == 0 || len == 0) {
        return 0;
    }
    if ((byteCode = mprAllocZeroed(len)) == 0) {
        return MPR_ERR_MEMORY;
    }
    mprMemcpy(byteCode, extraCodeLen, mprGetBufStart(extra->buf), extraCodeLen);
    if (codeLen) {
        mprMemcpy(&byteCode[extraCodeLen], codeLen, old->byteCode, codeLen);
    }
    ejsSetFunctionCode(ejs, fun, old->module, byteCode, len, extra->debug);

    debug = old->debug;
    if (debug && debug->numLines > 0) {
        for (i = 0; i < debug->numLines; i++) {
            if (ejsAddDebugLine(ejs, &fun->body.code->debug, debug->lines[i].offset + extraCodeLen, 
                    debug->lines[i].source) < 0) {
                return MPR_ERR_MEMORY;
            }
        }
    }

    /*
        Recreate all exception handlers
     */
    for (i = 0; i < old->numHandlers; i++) {
        ex = old->handlers[i];
        ex->tryStart += extraCodeLen;
        ex->tryEnd += extraCodeLen;
        ex->handlerStart += extraCodeLen;
        ex->handlerEnd += extraCodeLen;
        ejsAddException(ejs, fun, ex->tryStart, ex->tryEnd, ex->catchType, ex->handlerStart, ex->handlerEnd, 
            ex->numBlocks, ex->numStack, ex->flags, -1);
    }
    for (next = 0; (ex = mprGetNextItem(extra->exceptions, &next)) != 0; ) {
        ejsAddException(ejs, fun, ex->tryStart, ex->tryEnd, ex->catchType, ex->handlerStart, ex->handlerEnd, 
            ex->numBlocks, ex->numStack, ex->flags, -1);
    }
    return 0;
}


/*
    Process a class node.
 */
static void genClass(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EjsType         *type, *baseType;
    EjsFunction     *constructor;
    EcCodeGen       *code;
    EcState         *state;
    EcNode          *constructorNode;
    EjsName         qname;

    ENTER(cp);
    assure(np->kind == N_CLASS);

    ejs = cp->ejs;
    state = cp->state;
    type = (EjsType*) np->klass.ref;
    assure(type);

    state->inClass = 1;
    state->inFunction = 0;

    /*
        Op code to define the class. This goes into the module code buffer. DefineClass will capture the current scope
        including the internal namespace for this file.
        OPT See above todo
     */
    ecEncodeOpcode(cp, EJS_OP_DEFINE_CLASS);
    ecEncodeGlobal(cp, (EjsObj*) type, type->qname);

    state->letBlock = (EjsBlock*) type;
    state->varBlock = (EjsBlock*) type;
    state->currentClass = type;
    state->currentClassNode = np;

    constructorNode = np->klass.constructor;

    /*
        Create code buffers to hold the static and instance level initialization code. The AST module will always
        create a constructor node for us if there is instance level initialization code. We currently put the class
        initialization code in the constructor. Static variable initialization code will go into the current
        module buffer (cp->currentModule) and will be run when the module is loaded. 
        BUG - CLASS INITIALIZATION ORDERING.
     */
    state->code = state->currentModule->code;

    /*
        Create a code buffer for static initialization code and set it as the default buffer
     */
    state->code = state->staticCodeBuf = allocCodeBuffer(cp);

    if (type->constructor.block.pot.isFunction) {
        state->instanceCodeBuf = allocCodeBuffer(cp);
    }

    /*
        The current code buffer is the static initializer buffer. genVar will redirect to the instanceCodeBuf as required.
     */
    processNode(cp, np->left);

    if (type->hasInitializer) {
        /*
            Create the static initializer
         */
        ecEncodeOpcode(cp, EJS_OP_RETURN);
        setFunctionCode(cp, np->klass.initializer, state->staticCodeBuf);
    }

    if (type->constructor.block.pot.isFunction) {
        assure(constructorNode);
        assure(state->instanceCodeBuf);
        code = state->code = state->instanceCodeBuf;

        constructor = state->currentFunction = (EjsFunction*) type;
        assure(constructor);
        if (constructorNode->function.isDefault) {
            /*
                No constructor exists, so generate the default constructor. Append the default constructor 
                instructions after any initialization code. Will only get here if there is no required instance 
                initialization.
             */
            baseType = type->baseType;
            if (baseType && baseType->constructor.block.pot.isFunction) {
                ecEncodeOpcode(cp, EJS_OP_CALL_NEXT_CONSTRUCTOR);
                ecEncodeName(cp, baseType->qname);
                ecEncodeNum(cp, 0);
            }
            ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
            ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
            setFunctionCode(cp, (EjsFunction*) type, code);
            ecAddCStringConstant(cp, EJS_PUBLIC_NAMESPACE);
            ecAddCStringConstant(cp, EJS_CONSTRUCTOR_NAMESPACE);

        } else if (type->constructor.block.pot.isFunction) {
            injectCode(ejs, constructor, code);
        }
    }
    ecAddNameConstant(cp, np->qname);

    if (type->hasInitializer) {
        qname = ejsGetPropertyName(ejs, (EjsObj*) type, 0);
        ecAddNameConstant(cp, qname);
    }
    if (type->baseType) {
        ecAddNameConstant(cp, type->baseType->qname);
    }

    /*
        Emit any properties implemented via another class (there is no Node for these)
     */
    ecAddConstants(cp, (EjsObj*) type);
    if (type->prototype) {
        ecAddConstants(cp, (EjsObj*) type->prototype);
    }
    if (cp->ejs->flags & EJS_FLAG_DOC) {
        ecAddDocConstant(cp, "class", np->lookup.obj, np->lookup.slotNum);
    }
    LEAVE(cp);
}


static void genDassign(EcCompiler *cp, EcNode *np)
{
    EcNode      *field;
    int         next, count;

    assure(np->kind == N_DASSIGN);

    ENTER(cp);

    count = mprGetListLength(np->children);
    for (next = 0; (field = getNextNode(cp, np, &next)) != 0; ) {
        assure(field->kind == N_FIELD);
        if (next < count) {
            ecEncodeOpcode(cp, EJS_OP_DUP);
            pushStack(cp, 1);
        }
        if (np->objectLiteral.isArray) {
            ecEncodeOpcode(cp, EJS_OP_GET_OBJ_SLOT);
            ecEncodeNum(cp, field->field.index);
        } else {
            ecEncodeOpcode(cp, EJS_OP_GET_OBJ_NAME);
            ecEncodeName(cp, field->field.fieldName->qname);
        }
        assure(field->field.expr);
        processNode(cp, field->field.expr);
    }
    LEAVE(cp);
}


static void genDirectives(EcCompiler *cp, EcNode *np, bool saveResult)
{
    EcState     *lastDirectiveState;
    EcNode      *child;
    int         next, mark;

    ENTER(cp);

    lastDirectiveState = cp->directiveState;
    next = 0;
    mark = getStackCount(cp);
    while ((child = getNextNode(cp, np, &next)) && !cp->error) {
        cp->directiveState = cp->state;
        processNode(cp, child);
        if (!saveResult) {
            discardStackItems(cp, mark);
        }
    }
    if (saveResult) {
        ecEncodeOpcode(cp, EJS_OP_SAVE_RESULT);
    }
    cp->directiveState = lastDirectiveState;
    LEAVE(cp);
}


/*
    Handle property dereferencing via "." and "[". This routine generates code for bound properties where we know
    the slot offsets and also for unbound references. Return the right most node in right.
 */
static void genDot(EcCompiler *cp, EcNode *np, EcNode **rightMost)
{
    Ejs         *ejs;
    EcState     *state;
    EcNode      *left, *right;
    int         put;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;
    state->onLeft = 0;
    left = np->left;
    right = np->right;

    /*
        Process the left of the dot and leave an object reference on the stack
     */
    switch (left->kind) {
    case N_DOT:
    case N_EXPRESSIONS:
    case N_LITERAL:
    case N_THIS:
    case N_REF:
    case N_QNAME:
    case N_CALL:
    case N_SUPER:
    case N_OBJECT_LITERAL:
        state->needsValue = 1;
        processNode(cp, left);
        state->needsValue = state->next->needsValue;
        break;

    default:
        assure(0);
    }
    state->currentObjectNode = np->left;

    if (np->needThis || state->dupLeft) {
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
        np->needThis = 0;
        state->dupLeft = 0;
    }
    put = state->next->onLeft;

    /*
        Process the right
     */
    switch (right->kind) {
    case N_CALL:
        state->needsValue = state->next->needsValue;
        genCall(cp, right);
        state->needsValue = 0;
        break;

    case N_QNAME:
        state->onLeft = state->next->onLeft;
        genName(cp, right);
        break;

    case N_SUPER:
        ecEncodeOpcode(cp, EJS_OP_SUPER);
        break;

    case N_LITERAL:
    case N_OBJECT_LITERAL:
    default:
        state->currentObjectNode = 0;
        state->needsValue = 1;
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeConst(cp, ESV(empty));
        pushStack(cp, 1);
        if (right->kind == N_LITERAL) {
            genLiteral(cp, right);
        } else if (right->kind == N_OBJECT_LITERAL) {
            genObjectLiteral(cp, right);
        } else {
            processNode(cp, right);
        }
        state->onLeft = state->next->onLeft;
        ecEncodeOpcode(cp, put ? EJS_OP_PUT_OBJ_NAME_EXPR :  EJS_OP_GET_OBJ_NAME_EXPR);
        popStack(cp, (put) ? 4 : 2);
        break;
    }
    if (rightMost) {
        *rightMost = right;
    }
    LEAVE(cp);
}


static void genEndFunction(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EjsFunction     *fun;

    ENTER(cp);

    assure(np);

    ejs = cp->ejs;
    fun = cp->state->currentFunction;
    
    if (cp->lastOpcode != EJS_OP_RETURN_VALUE && cp->lastOpcode != EJS_OP_RETURN) {
        /*
            Ensure code cannot run off the end of a method.
            TODO OPT - must do a better job of basic block analysis and check if all paths out of a function have a return.
         */
        if (fun->isConstructor) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
            ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);

        } else if (fun->resultType == 0) {
            if (fun->hasReturn) {
                //  TODO - OPT. Should be able to avoid this somehow. We put it here now to ensure that all
                //  paths out of the function terminate with a return.
                ecEncodeOpcode(cp, EJS_OP_LOAD_NULL);
                ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
            } else {
                ecEncodeOpcode(cp, EJS_OP_RETURN);
            }

        } else if (fun->resultType == EST(Void)) {
            ecEncodeOpcode(cp, EJS_OP_RETURN);

        } else {
            //  TODO - OPT. Should be able to avoid this somehow.
            ecEncodeOpcode(cp, EJS_OP_LOAD_NULL);
            ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
        }
    }
    cp->lastOpcode = 0;
    LEAVE(cp);
}


static void genExpressions(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    int         next;

    assure(np->kind == N_EXPRESSIONS);

    ENTER(cp);

    next = 0;
    while ((child = getNextNode(cp, np, &next)) != 0) {
        processNode(cp, child);
    }
    LEAVE(cp);
}


/*
    This handles "do { ... } while" constructs.
  
    do {
         body
    } while (conditional)
  
    Labels:
        topOfLoop:
            body
        continueLabel:
            conditional
            bxx topOfLoop
        endLoop:
 */
static void genDo(EcCompiler *cp, EcNode *np)
{
    EcCodeGen   *outerBlock, *code;
    EcState     *state;
    int         condLen, bodyLen, len, condShortJump, continueLabel, breakLabel, mark;

    ENTER(cp);
    assure(np->kind == N_DO);

    state = cp->state;
    state->captureFinally = 0;
    state->captureBreak = 0;
    outerBlock = state->code;
    code = state->code = allocCodeBuffer(cp);

    ecStartBreakableStatement(cp, EC_JUMP_BREAK | EC_JUMP_CONTINUE);

    if (np->forLoop.body) {
        np->forLoop.bodyCode = state->code = allocCodeBuffer(cp);
        mark = getStackCount(cp);
        processNode(cp, np->forLoop.body);
        discardStackItems(cp, mark);
    }
    if (np->forLoop.cond) {
        np->forLoop.condCode = state->code = allocCodeBuffer(cp);
        state->needsValue = 1;
        processNode(cp, np->forLoop.cond);
        state->needsValue = 0;
    }
    /*
        Get the lengths of code blocks
     */
    condLen = bodyLen = 0;
    if (np->forLoop.condCode) {
        condLen = (int) mprGetBufLength(np->forLoop.condCode->buf);
    }
    if (np->forLoop.bodyCode) {
        bodyLen = (int) mprGetBufLength(np->forLoop.bodyCode->buf);
    }

    /*
        Now that we know the body length, we can calculate the jump back to the top.
     */
    condShortJump = 0;
    len = bodyLen + condLen;
    if (len > 0) {
        if (len < 0x7f && cp->optimizeLevel > 0) {
            condShortJump = 1;
            condLen += 2;
        } else {
            condLen += 5;
        }
    }

    setCodeBuffer(cp, code);
    if (np->forLoop.cond) {
        pushStack(cp, 1);
    }
    continueLabel = (int) mprGetBufLength(cp->state->code->buf);

    /*
        Add the body
     */
    if (np->forLoop.bodyCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.bodyCode);
    }

    /*
        Copy the conditional code and add condition jump to the end of the for loop, then copy the body code.
     */
    if (np->forLoop.condCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.condCode);
        len = bodyLen + condLen;
        if (condShortJump) {
            ecEncodeOpcode(cp, EJS_OP_BRANCH_TRUE_8);
            ecEncodeByte(cp, -len);
        } else {
            ecEncodeOpcode(cp, EJS_OP_BRANCH_TRUE);
            ecEncodeInt32(cp, -len);
        }
        popStack(cp, 1);
    }

    breakLabel = (int) mprGetBufLength(cp->state->code->buf);
    patchJumps(cp, EC_JUMP_BREAK, breakLabel);
    patchJumps(cp, EC_JUMP_CONTINUE, continueLabel);

    copyCodeBuffer(cp, outerBlock, state->code);
    LEAVE(cp);
}


/*
    This handles "for" and while" constructs but not "for .. in"
  
    for (initializer; conditional; perLoop) { body }
  
    Labels:
            initializer
        topOfLoop:
            conditional
            bxx endLoop
        topOfBody:
            body
        continueLabel:
            perLoop
        endIteration:
            goto topOfLoop
        endLoop:
 */
static void genFor(EcCompiler *cp, EcNode *np)
{
    EcCodeGen   *outerBlock, *code;
    EcState     *state;
    int         condLen, bodyLen, perLoopLen, len, condShortJump, perLoopShortJump, continueLabel, breakLabel, mark;
    int         startMark;

    ENTER(cp);

    assure(np->kind == N_FOR);

    state = cp->state;
    outerBlock = state->code;
    code = state->code = allocCodeBuffer(cp);
    startMark = getStackCount(cp);
    state->captureFinally = 0;
    state->captureBreak = 0;

    /*
        initializer is outside the loop
     */
    if (np->forLoop.initializer) {
        mark = getStackCount(cp);
        processNode(cp, np->forLoop.initializer);
        discardStackItems(cp, mark);
    }

    /*
        For conditional
     */
    ecStartBreakableStatement(cp, EC_JUMP_BREAK | EC_JUMP_CONTINUE);

    if (np->forLoop.cond) {
        np->forLoop.condCode = state->code = allocCodeBuffer(cp);
        state->needsValue = 1;
        processNode(cp, np->forLoop.cond);
        state->needsValue = 0;
        /* Leaves one item on the stack, but this will be cleared when compared */
        assure(state->code->stackCount >= 1);
        popStack(cp, 1);
    }

    if (np->forLoop.body) {
        mark = getStackCount(cp);
        np->forLoop.bodyCode = state->code = allocCodeBuffer(cp);
        processNode(cp, np->forLoop.body);
        discardStackItems(cp, mark);
    }

    /*
        Per loop iteration
     */
    if (np->forLoop.perLoop) {
        np->forLoop.perLoopCode = state->code = allocCodeBuffer(cp);
        mark = getStackCount(cp);
        processNode(cp, np->forLoop.perLoop);
        discardStackItems(cp, mark);
    }

    /*
        Get the lengths of code blocks
     */
    perLoopLen = condLen = bodyLen = 0;

    if (np->forLoop.condCode) {
        condLen = (int) mprGetBufLength(np->forLoop.condCode->buf);
    }
    if (np->forLoop.bodyCode) {
        bodyLen = (int) mprGetBufLength(np->forLoop.bodyCode->buf);
    }
    if (np->forLoop.perLoopCode) {
        perLoopLen = (int) mprGetBufLength(np->forLoop.perLoopCode->buf);
    }

    /*
        Now that we know the body length, we can calculate the jump at the top. This is the shorter of
        the two jumps as it does not span the conditional code, so we optimize it first incase the saving
        of 3 bytes allows us to also optimize the branch back to the top. Subtract 5 to the test with 0x7f to
        account for the worst-case jump at the bottom back to the top
     */
    condShortJump = 0;
    if (condLen > 0) {
        len = bodyLen + perLoopLen;
        if (len < (0x7f - 5) && cp->optimizeLevel > 0) {
            condShortJump = 1;
            condLen += 2;
        } else {
            condLen += 5;
        }
    }

    /*
        Calculate the jump back to the top of the loop (per-iteration jump). Subtract 5 to account for the worst case
        where the per loop jump is a long jump.
     */
    len = condLen + bodyLen + perLoopLen;
    if (len < (0x7f - 5) && cp->optimizeLevel > 0) {
        perLoopShortJump = 1;
        perLoopLen += 2;
    } else {
        perLoopShortJump = 0;
        perLoopLen += 5;
    }

    /*
        Copy the conditional code and add condition jump to the end of the for loop, then copy the body code.
     */
    setCodeBuffer(cp, code);
    if (np->forLoop.condCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.condCode);
        len = bodyLen + perLoopLen;
        if (condShortJump) {
            ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE_8);
            ecEncodeByte(cp, len);
        } else {
            ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE);
            ecEncodeInt32(cp, len);
        }
    }

    /*
        Add the body and per loop code
     */
    if (np->forLoop.bodyCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.bodyCode);
    }
    continueLabel = (int) mprGetBufLength(state->code->buf);
    if (np->forLoop.perLoopCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.perLoopCode);
    }

    /*
        Add the per-loop jump back to the top of the loop
     */
    len = condLen + bodyLen + perLoopLen;
    if (perLoopShortJump) {
        ecEncodeOpcode(cp, EJS_OP_GOTO_8);
        ecEncodeByte(cp, -len);
    } else {
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        ecEncodeInt32(cp, -len);
    }
    breakLabel = (int) mprGetBufLength(state->code->buf);
    discardStackItems(cp, startMark);

    patchJumps(cp, EC_JUMP_BREAK, breakLabel);
    patchJumps(cp, EC_JUMP_CONTINUE, continueLabel);

    copyCodeBuffer(cp, outerBlock, state->code);
    LEAVE(cp);
}


/*
    This routine is a little atypical in that it hand-crafts an exception block.
 */
static void genForIn(EcCompiler *cp, EcNode *np)
{
    Ejs         *ejs;
    EcNode      *iterVar, *iterGet;
    EcCodeGen   *outerBlock, *code;
    EcState     *state;
    int         len, breakLabel, tryStart, tryEnd, handlerStart, mark, startMark, varCount;

    ENTER(cp);

    assure(cp->state->code->stackCount >= 0);
    assure(np->kind == N_FOR_IN);

    ejs = cp->ejs;
    state = cp->state;
    outerBlock = state->code;
    code = state->code = allocCodeBuffer(cp);
    startMark = getStackCount(cp);
    state->captureFinally = 0;
    state->captureBreak = 0;
    iterVar = np->forInLoop.iterVar;
    iterGet = np->forInLoop.iterGet;
    varCount = mprGetListLength(iterVar->children);

    ecStartBreakableStatement(cp, EC_JUMP_BREAK | EC_JUMP_CONTINUE);
    processNode(cp, iterVar);

    /*
        Consider:
            for (i in obj.get())
                body
      
        Now process the obj.get()
     */
    np->forInLoop.initCode = state->code = allocCodeBuffer(cp);

    if (varCount == 2) {
        state->dupLeft = 1;
        processNode(cp, iterGet);
        state->dupLeft = 0;
    } else {
        processNode(cp, iterGet);
    }
    ecEncodeOpcode(cp, EJS_OP_PUSH_RESULT);
    pushStack(cp, 1);
    assure(state->code->stackCount >= 1);

    /*
        Process the iter.next()
     */
    np->forInLoop.bodyCode = state->code = allocCodeBuffer(cp);

    /*
        Dup the iterator reference each time round the loop as iter.next() will consume the object.
        TODO - OPT. Consider having a CALL op code that does not consume the object.
     */
    ecEncodeOpcode(cp, EJS_OP_DUP);
    pushStack(cp, 1);

    /*
        Emit code to invoke the iterator
     */
    tryStart = getCodeLength(cp, np->forInLoop.bodyCode);

    if (np->forInLoop.iterNext->lookup.bind && np->forInLoop.iterNext->lookup.slotNum >= 0) {
        assure(0);
        ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_SLOT);
        ecEncodeNum(cp, np->forInLoop.iterNext->lookup.slotNum);
    } else {
        ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_NAME);
        ecEncodeName(cp, np->forInLoop.iterNext->qname);
    }
    ecEncodeNum(cp, 0);
    popStack(cp, 1);
    
    tryEnd = getCodeLength(cp, np->forInLoop.bodyCode);

    if (varCount == 2) {
        /* Dup original object being iterated */
        ecEncodeOpcode(cp, EJS_OP_DUP_STACK);
        ecEncodeByte(cp, 1);
        pushStack(cp, 1);
        //  TODO space is not used with numericIndicies
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeConst(cp, ESV(empty));
        pushStack(cp, 1);
    }

    /*
        Save the result of the iter.next() call
     */
    ecEncodeOpcode(cp, EJS_OP_PUSH_RESULT);
    pushStack(cp, 1);

    if (varCount == 2) {
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
    }
    state->onLeft = 1;
    genName(cp, iterVar->left);
    state->onLeft = 0;

    if (iterVar->kind == N_VAR_DEFINITION && iterVar->def.varKind == KIND_LET) {
        ecAddNameConstant(cp, iterVar->left->qname);
    }
    mark = getStackCount(cp);
    if (np->forInLoop.body) {
        if (varCount == 2) {
            ecEncodeOpcode(cp, EJS_OP_GET_OBJ_NAME_EXPR);
            popStack(cp, 2);
            state->onLeft = 1;
            genName(cp, iterVar->right);
            state->onLeft = 0;
        }
        processNode(cp, np->forInLoop.body);
        discardStackItems(cp, mark);
    }
    len = getCodeLength(cp, np->forInLoop.bodyCode);
    if (len < (0x7f - 5)) {
        ecEncodeOpcode(cp, EJS_OP_GOTO_8);
        len += 2;
        ecEncodeByte(cp, -len);
    } else {
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        len += 5;
        ecEncodeInt32(cp, -len);
    }

    /*
        Create exception catch block around iter.next() to catch the StopIteration exception.
        Note: we have a zero length handler (noop)
     */
    handlerStart = ecGetCodeOffset(cp);
    addException(cp, tryStart, tryEnd, EST(StopIteration), handlerStart, handlerStart, 0, startMark,
        EJS_EX_CATCH | EJS_EX_ITERATION);

    /*
        Patch break/continue statements
     */
    discardStackItems(cp, startMark);
    breakLabel = (int) mprGetBufLength(state->code->buf);

    patchJumps(cp, EC_JUMP_BREAK, breakLabel);
    patchJumps(cp, EC_JUMP_CONTINUE, 0);

    setCodeBuffer(cp, code);
    copyCodeBuffer(cp, state->code, np->forInLoop.initCode);
    copyCodeBuffer(cp, state->code, np->forInLoop.bodyCode);
    copyCodeBuffer(cp, outerBlock, state->code);
    LEAVE(cp);
}


/*
    Generate code for default parameters. Native classes must handle this themselves. We
    generate the code for all default parameters in sequence with a computed goto at the front.
 */
static void genDefaultParameterCode(EcCompiler *cp, EcNode *np, EjsFunction *fun)
{
    Ejs             *ejs;
    EcNode          *parameters, *child;
    EcState         *state;
    EcCodeGen       **buffers, *saveCode;
    int             len, next, needLongJump, count, firstDefault;

    ejs = cp->ejs;
    state = cp->state;
    saveCode = state->code;

    parameters = np->function.parameters;
    assure(parameters);

    count = mprGetListLength(parameters->children);
    buffers = (EcCodeGen**) mprAllocZeroed(count * sizeof(EcCodeGen*));

    for (next = 0; (child = getNextNode(cp, parameters, &next)) && !cp->error; ) {
        assure(child->kind == N_VAR_DEFINITION);
        if (child->left->kind == N_ASSIGN_OP) {
            buffers[next - 1] = state->code = allocCodeBuffer(cp);
            genAssignOp(cp, child->left);
        }
    }
    if (fun->rest) {
        buffers[count - 1] = state->code = allocCodeBuffer(cp);
        ecEncodeOpcode(cp, EJS_OP_NEW_ARRAY);
        ecEncodeGlobal(cp, (EjsObj*) EST(Array), EST(Array)->qname);
        ecEncodeNum(cp, 0);
        pushStack(cp, 1);
        if (fun->numArgs < 10) {
            ecEncodeOpcode(cp, EJS_OP_PUT_LOCAL_SLOT_0 + fun->numArgs - 1);
        } else {
            ecEncodeOpcode(cp, EJS_OP_PUT_LOCAL_SLOT);
            ecEncodeNum(cp, fun->numArgs - 1);
        }
    }
    firstDefault = fun->numArgs - fun->numDefault - fun->rest;
    assure(firstDefault >= 0);
    needLongJump = cp->optimizeLevel > 0 ? 0 : 1;

    /*
        Compute the worst case jump size. Start with 4 because the table is always one larger than the
        number of default args.
     */
    len = 4;
    for (next = firstDefault; next < count; next++) {
        if (buffers[next]) {
            len = (int) mprGetBufLength(buffers[next]->buf) + 4;
            if (len >= 0x7f) {
                needLongJump = 1;
                break;
            }
        }
    }
    setCodeBuffer(cp, saveCode);

    /*
        This is a jump table where each parameter initialization segments falls through to the next one.
        We have one more entry in the table to jump over the entire computed jump section.
     */
    ecEncodeOpcode(cp, (needLongJump) ? EJS_OP_INIT_DEFAULT_ARGS: EJS_OP_INIT_DEFAULT_ARGS_8);
    ecEncodeByte(cp, fun->numDefault + fun->rest + 1);

    len = (fun->numDefault + fun->rest + 1) * ((needLongJump) ? 4 : 1);
    for (next = firstDefault; next < count; next++) {
        if (buffers[next] == 0) {
            continue;
        }
        if (needLongJump) {
            ecEncodeInt32(cp, len);
        } else {
            ecEncodeByte(cp, len);
        }
        len += (int) mprGetBufLength(buffers[next]->buf);
    }
    /*
        Add one more jump to jump over the entire jump table
     */
    if (needLongJump) {
        ecEncodeInt32(cp, len);
    } else {
        ecEncodeByte(cp, len);
    }

    /*
        Now copy all the initialization code
     */
    for (next = firstDefault; next < count; next++) {
        if (buffers[next]) {
            copyCodeBuffer(cp, state->code, buffers[next]);
        }
    }
}


static void genFunction(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EjsEx           *ex;
    EcState         *state;
    EcCodeGen       *code;
    EjsFunction     *fun;
    EjsType         *baseType;
    EjsName         qname;
    EjsTrait        *trait;
    EjsPot          *activation;
    int             i, numProp;

    ENTER(cp);

    assure(np->kind == N_FUNCTION);
    
    ejs = cp->ejs;
    state = cp->state;
    cp->lastOpcode = 0;
    assure(state);

    assure(np->function.functionVar);
    fun = np->function.functionVar;
    activation = fun->activation;
    numProp = activation ? activation->numProp: 0;

    state->inFunction = 1;
    state->inMethod = state->inMethod || np->function.isMethod;
    state->blockIsMethod = np->function.isMethod;
    state->currentFunction = fun;
    state->currentFunctionNode = np;

    /*
        Capture the scope chain by the defineFunction op code. Emit this into the existing code buffer. 
        Don't do if a method as they get scope via other means. Native methods also don't use this as an optimization.
        Native methods must handle scope explicitly.
      
        We only need to define the function if it needs full scope (unbound property access) or it is a nested function.
     */
    if (fun->fullScope) {
        ecEncodeOpcode(cp, EJS_OP_DEFINE_FUNCTION);
        ecEncodeName(cp, np->qname);
    }
    code = state->code = allocCodeBuffer(cp);

    if (!fun->isNativeProc) {
        addDebug(cp, np);
    }

    /*
        Generate code for any parameter default initialization.
        Native classes must do default parameter initialization themselves.
     */
    if (fun->numDefault > 0 && !(np->attributes & EJS_PROP_NATIVE)) {
        genDefaultParameterCode(cp, np, fun);
    }
    if (np->function.constructorSettings) {
        genDirectives(cp, np->function.constructorSettings, 0);
    }
    state->letBlock = (EjsBlock*) fun;
    state->varBlock = (EjsBlock*) fun;

    if (np->function.isConstructor) {
        /*
            Function is a constructor. Call any default constructors if required.
            Should this be before or after default variable initialization?
         */
        assure(state->currentClass);
        baseType = state->currentClass->baseType;
        if (!state->currentClass->callsSuper && baseType && baseType->constructor.block.pot.isFunction && 
                !(np->attributes & EJS_PROP_NATIVE)) {
            ecEncodeOpcode(cp, EJS_OP_CALL_NEXT_CONSTRUCTOR);
            ecEncodeName(cp, baseType->qname);
            ecEncodeNum(cp, 0);
        }
    }

    /*
        May be no body for native functions
     */
    if (np->function.body) {
        assure(np->function.body->kind == N_DIRECTIVES);
        processNode(cp, np->function.body);
    }
    if (cp->errorCount > 0) {
        LEAVE(cp);
        return;
    }
    setFunctionCode(cp, fun, code);
    ecAddNameConstant(cp, np->qname);

    for (i = 0; i < numProp; i++) {
        qname = ejsGetPropertyName(ejs, activation, i);
        ecAddNameConstant(cp, qname);
        trait = ejsGetPropertyTraits(ejs, activation, i);
        if (trait && trait->type) {
            ecAddNameConstant(cp, trait->type->qname);
        }
    }
    for (i = 0; i < fun->block.pot.numProp; i++) {
        qname = ejsGetPropertyName(ejs, (EjsObj*) fun, i);
        ecAddNameConstant(cp, qname);
        trait = ejsGetPropertyTraits(ejs, fun, i);
        if (trait && trait->type) {
            ecAddNameConstant(cp, trait->type->qname);
        }
    }
    if (fun->resultType) {
        ecAddNameConstant(cp, fun->resultType->qname);
    }
    if (fun->body.code) {
        for (i = 0; i < fun->body.code->numHandlers; i++) {
            ex = fun->body.code->handlers[i];
            if (ex && ex->catchType) {
                ecAddNameConstant(cp, ex->catchType->qname);
            }
        }
    }
    if (cp->ejs->flags & EJS_FLAG_DOC) {
        ecAddDocConstant(cp, "fun", np->lookup.obj, np->lookup.slotNum);
    }
    LEAVE(cp);
}


static void genHash(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (!np->hash.disabled) {
        processNode(cp, np->hash.body);
    }
    LEAVE(cp);
}


static void genIf(EcCompiler *cp, EcNode *np)
{
    EcCodeGen   *saveCode;
    EcState     *state;
    int         thenLen, elseLen, mark;

    ENTER(cp);

    assure(np->kind == N_IF);

    state = cp->state;
    saveCode = state->code;

    /*
        Process the conditional. Put the popStack for the branch here so the stack is correct for the "then" and 
        "else" blocks.
     */
    state->needsValue = 1;
    processNode(cp, np->tenary.cond);
    state->needsValue = 0;
    popStack(cp, 1);

    /*
        Process the "then" block.
     */
    np->tenary.thenCode = state->code = allocCodeBuffer(cp);
    mark = getStackCount(cp);
    
    //  CHANGE: Added for return (cond) ? call(): other;
    state->needsValue = state->next->needsValue;
    processNode(cp, np->tenary.thenBlock);
    if (state->next->needsValue) {
        /* Part of a tenary expression */
        if (state->code->stackCount < (mark + 1)) {
            genError(cp, np, "Then expression does not evaluate to a value. Check if operands are void");
        }
        discardStackItems(cp, mark + 1);
        if (np->tenary.elseBlock) {
            setStack(cp, mark);
        }
    } else {
        discardStackItems(cp, mark);
    }

    /*
        Else block (optional)
     */
    if (np->tenary.elseBlock) {
        np->tenary.elseCode = state->code = allocCodeBuffer(cp);
        state->needsValue = state->next->needsValue;
        processNode(cp, np->tenary.elseBlock);
        state->needsValue = 0;
        if (state->next->needsValue) {
            if (state->code->stackCount < (mark + 1)) {
                genError(cp, np, "Else expression does not evaluate to a value. Check if operands are void");
            }
            discardStackItems(cp, mark + 1);
        } else {
            discardStackItems(cp, mark);
        }
    }

    /*
        Calculate jump lengths. Then length will vary depending on if the jump at the end of the "then" block
        can jump over the "else" block with a short jump.
     */
    elseLen = (np->tenary.elseCode) ? (int) mprGetBufLength(np->tenary.elseCode->buf) : 0;
    thenLen = (int) mprGetBufLength(np->tenary.thenCode->buf);
    thenLen += (elseLen < 0x7f && cp->optimizeLevel > 0) ? 2 : 5;

    /*
        Now copy the basic blocks into the output code buffer, starting with the jump around the "then" code.
     */
    setCodeBuffer(cp, saveCode);

    if (thenLen < 0x7f && cp->optimizeLevel > 0) {
        ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE_8);
        ecEncodeByte(cp, thenLen);
    } else {
        ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE);
        ecEncodeInt32(cp, thenLen);
    }

    /*
        Copy the then code
     */
    copyCodeBuffer(cp, state->code, np->tenary.thenCode);

    /*
        Create the jump to the end of the if statement
     */
    if (elseLen < 0x7f && cp->optimizeLevel > 0) {
        ecEncodeOpcode(cp, EJS_OP_GOTO_8);
        ecEncodeByte(cp, elseLen);
    } else {
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        ecEncodeInt32(cp, elseLen);
    }
    if (np->tenary.elseCode) {
        copyCodeBuffer(cp, state->code, np->tenary.elseCode);
    }
    if (state->next->needsValue) {
        pushStack(cp, 1);
    }
    LEAVE(cp);
}


/*
    Expect data on the stack already to assign
 */
static void genLeftHandSide(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    assure(cp);
    assure(np);

    cp->state->onLeft = 1;

    switch (np->kind) {
    case N_DASSIGN:
    case N_DOT:
    case N_QNAME:
    case N_SUPER:
    case N_EXPRESSIONS:
    case N_OBJECT_LITERAL:
    case N_VAR:
        processNode(cp, np);
        break;

    case N_CALL:
    default:
        genError(cp, np, "Illegal left hand side");
    }
    LEAVE(cp);
}


static void genLiteral(EcCompiler *cp, EcNode *np)
{
    EjsNamespace    *nsp;
    EjsBoolean      *bp;
    EjsNumber       *ip;
    EjsString       *pattern, *data;
    Ejs             *ejs;
    int64           n;
    int             sid;

    ENTER(cp);
    ejs = cp->ejs;

    if (TYPE(np->literal.var) == EST(XML)) {
        ecEncodeOpcode(cp, EJS_OP_LOAD_XML);
        //  UNICODE
        data = ejsCreateString(ejs, (wchar*) mprGetBufStart(np->literal.data), 
                mprGetBufLength(np->literal.data) / sizeof(wchar));
        ecEncodeConst(cp, data);
        pushStack(cp, 1);
        LEAVE(cp);
        return;
    }

    /*
        Map Numbers to the configured real type
     */
    sid = TYPE(np->literal.var)->sid;
    switch (sid) {
    case ES_Boolean:
        bp = (EjsBoolean*) np->literal.var;
        if (bp->value) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_TRUE);
        } else {
            ecEncodeOpcode(cp, EJS_OP_LOAD_FALSE);
        }
        break;

    case ES_Number:
        /*
            These are signed values
         */
        ip = (EjsNumber*) np->literal.var;
        if (ip->value != floor(ip->value) || ip->value <= -MAXINT || ip->value >= MAXINT) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_DOUBLE);
            ecEncodeDouble(cp, ip->value);
        } else {
            n = (int64) ip->value;
            if (0 <= n && n <= 9) {
                ecEncodeOpcode(cp, EJS_OP_LOAD_0 + (int) n);
            } else {
                ecEncodeOpcode(cp, EJS_OP_LOAD_INT);
                ecEncodeNum(cp, n);
            }
        }
        break;

    case ES_Namespace:
        ecEncodeOpcode(cp, EJS_OP_LOAD_NAMESPACE);
        nsp = (EjsNamespace*) np->literal.var;
        ecEncodeConst(cp, nsp->value);
        break;

    case ES_Null:
        ecEncodeOpcode(cp, EJS_OP_LOAD_NULL);
        break;

    case ES_String:
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeConst(cp, ((EjsString*) np->literal.var));
        break;

    case ES_RegExp:
        ecEncodeOpcode(cp, EJS_OP_LOAD_REGEXP);
        pattern = (EjsString*) ejsRegExpToString(cp->ejs, (EjsRegExp*) np->literal.var);
        ecEncodeConst(cp, pattern);
        break;

    case ES_Void:
        ecEncodeOpcode(cp, EJS_OP_LOAD_UNDEFINED);
        break;

    default:
        assure(0);
        break;
    }
    pushStack(cp, 1);
    LEAVE(cp);
}


/*
    Generate code for name reference. This routine handles both loads and stores.
 */
static void genName(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    assure(np->kind == N_QNAME || np->kind == N_USE_NAMESPACE || np->kind == N_VAR);

    if (np->needThis) {
        if (np->lookup.useThis) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);

        } else if (np->lookup.obj == cp->ejs->global){
            ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);

        } else if (cp->state->currentObjectNode) {
            ecEncodeOpcode(cp, EJS_OP_DUP);

        } else {
            /*
                Unbound function
             */
            ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);
        }
        pushStack(cp, 1);
        np->needThis = 0;
    }
    if (np->lookup.bind && np->lookup.slotNum >= 0) {
        genBoundName(cp, np);
    } else {
        genUnboundName(cp, np);
    }
    LEAVE(cp);
}


static void genNew(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);
    assure(np->kind == N_NEW);
    processNode(cp, np->left);
    ecEncodeOpcode(cp, EJS_OP_NEW);
    popStack(cp, 1);
    pushStack(cp, 1);
    LEAVE(cp);
}


static void genArrayLiteral(EcCompiler *cp, EcNode *np)
{
    EcNode      *child, *typeNode;
    EjsType     *type;
    Ejs         *ejs;
    int         next, argc;

    ENTER(cp);
    ejs = cp->ejs;

    for (next = 0; (child = getNextNode(cp, np, &next)) != 0; ) {
        processNode(cp, child);
    }
    argc = next;
    ecEncodeOpcode(cp, EJS_OP_NEW_ARRAY);
    typeNode = np->objectLiteral.typeNode;
    type = (EjsType*) typeNode->lookup.ref;
    ecEncodeGlobal(cp, (EjsObj*) type, (type) ? type->qname: N(NULL, NULL));
    ecEncodeNum(cp, argc);
    pushStack(cp, 1);
    popStack(cp, argc * 2);
    LEAVE(cp);
}


static void genObjectLiteral(EcCompiler *cp, EcNode *np)
{
    EcNode      *child, *typeNode;
    EjsType     *type;
    Ejs         *ejs;
    int         next, argc;

    if (np->objectLiteral.isArray) {
        genArrayLiteral(cp, np);
        return;
    }
    ENTER(cp);
    ejs = cp->ejs;

    /*
        Push all the literal args
     */
    next = 0;
    while ((child = getNextNode(cp, np, &next)) != 0) {
        processNode(cp, child);
    }
    argc = next;
    ecEncodeOpcode(cp, EJS_OP_NEW_OBJECT);
    typeNode = np->objectLiteral.typeNode;
    type = (EjsType*) typeNode->lookup.ref;
    ecEncodeGlobal(cp, (EjsObj*) type, (type) ? type->qname: N(NULL, NULL));
    ecEncodeNum(cp, argc);
    for (next = 0; (child = getNextNode(cp, np, &next)) != 0; ) {
        ecEncodeNum(cp, child->attributes);
    }
    pushStack(cp, 1);
    popStack(cp, argc * 3);
    LEAVE(cp);
}


static void genField(EcCompiler *cp, EcNode *np)
{
    Ejs         *ejs;
    EcNode      *fieldName;

    ejs = cp->ejs;
    fieldName = np->field.fieldName;

    if (np->field.index >= 0) {
        //  TODO OPT use LOAD_INT_NN instructions
        ecEncodeOpcode(cp, EJS_OP_LOAD_INT);
        ecEncodeNum(cp, np->field.index);
        pushStack(cp, 1);

    } else if (fieldName->kind == N_QNAME) {
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeConst(cp, np->field.fieldName->qname.space);
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeConst(cp, np->field.fieldName->qname.name);
        pushStack(cp, 2);

    } else if (fieldName->kind == N_LITERAL) {
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeConst(cp, ESV(empty));
        pushStack(cp, 1);
        genLiteral(cp, fieldName);

    } else {
        //  TODO
        assure(0);
        processNode(cp, fieldName);
    }
    if (np->field.fieldKind == FIELD_KIND_VALUE || np->field.fieldKind == FIELD_KIND_FUNCTION) {
        if (np->field.expr) {
            processNode(cp, np->field.expr);
        } else {
            ecEncodeOpcode(cp, EJS_OP_LOAD_NULL);
            pushStack(cp, 1);            
        }
    }
}


static void genPostfixOp(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    /*
        Dup before inc
     */
    processNode(cp, np->left);
    ecEncodeOpcode(cp, EJS_OP_DUP);
    ecEncodeOpcode(cp, EJS_OP_INC);
    ecEncodeByte(cp, (np->tokenId == T_PLUS_PLUS) ? 1 : -1);
    genLeftHandSide(cp, np->left);
    pushStack(cp, 1);
    LEAVE(cp);
}


static void genProgram(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    int         next;

    ENTER(cp);

    next = 0;
    while ((child = getNextNode(cp, np, &next)) && !cp->error) {

        switch (child->kind) {
        case N_MODULE:
            genModule(cp, child);
            break;

        case N_DIRECTIVES:
            genDirectives(cp, child, 0);
            break;

        default:
            badNode(cp, np);
        }
    }
    LEAVE(cp);
}


static void genPragmas(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    int         next;

    next = 0;
    while ((child = getNextNode(cp, np, &next))) {
        processNode(cp, child);
    }
}


/*
    Generate code for function returns
 */
static void genReturn(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EjsFunction     *fun;

    ENTER(cp);

    ejs = cp->ejs;
    if (cp->state->captureFinally) {
        ecEncodeOpcode(cp, EJS_OP_CALL_FINALLY);
    }
    if (np->left) {
        fun = cp->state->currentFunction;
        if (fun->resultType == NULL || fun->resultType != EST(Void)) {
            cp->state->needsValue = 1;
            processNode(cp, np->left);
            cp->state->needsValue = 0;
            ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
            popStack(cp, 1);

        } else if (np->ret.blockless) {
            /*
                The return was inserted by the parser. So we must still process the statement
             */
            processNode(cp, np->left);
        }

    } else {
        /*
            return;
         */
        fun = cp->state->currentFunction;
        if (fun->isConstructor) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
            ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
        } else {
            ecEncodeOpcode(cp, EJS_OP_RETURN);
        }
    }
    LEAVE(cp);
}


/*
    Load the super pointer. Super function calls (super()) are handled via N_CALL.
 */
static void genSuper(EcCompiler *cp, EcNode *np)
{
    int         argc;

    ENTER(cp);
    assure(np->kind == N_SUPER);

    if (np->left) {
        argc = mprGetListLength(np->left->children);
        if (argc > 0) {
            processNode(cp, np->left);
        }
        ecEncodeOpcode(cp, EJS_OP_CALL_NEXT_CONSTRUCTOR);
        ecEncodeName(cp, cp->state->currentClass->baseType->qname);
        ecEncodeNum(cp, argc);        
        popStack(cp, argc);
    } else {
        ecEncodeOpcode(cp, EJS_OP_SUPER); 
        pushStack(cp, 1);
    }
    LEAVE(cp);
}


static void genSwitch(EcCompiler *cp, EcNode *np)
{
    EcNode      *caseItem, *elements;
    EcCodeGen   *code, *outerBlock;
    EcState     *state;
    int         next, len, nextCaseLen, nextCodeLen, totalLen, mark;

    ENTER(cp);

    state = cp->state;
    state->captureFinally = 0;
    state->captureBreak = 0;

    outerBlock = state->code;
    code = state->code = allocCodeBuffer(cp);

    /*
        Generate code for the switch (expression)
     */
    processNode(cp, np->left);

    ecStartBreakableStatement(cp, EC_JUMP_BREAK);

    /*
        Generate the code for each case label expression and case statements.
        next set to one to skip the switch expression.
     */
    elements = np->right;
    assure(elements->kind == N_CASE_ELEMENTS);

    next = 0;
    while ((caseItem = getNextNode(cp, elements, &next)) && !cp->error) {
        /*
            Allocate a buffer for the case expression and generate that code
         */
        mark = getStackCount(cp);
        assure(caseItem->kind == N_CASE_LABEL);
        if (caseItem->caseLabel.kind == EC_SWITCH_KIND_CASE) {
            caseItem->caseLabel.expressionCode = state->code = allocCodeBuffer(cp);
            /*
                Dup the switch expression value to preserve it for later cases.
                OPT - don't need to preserve for default cases or if this is the last case
             */
            ecEncodeOpcode(cp, EJS_OP_DUP);
            assure(caseItem->caseLabel.expression);
            processNode(cp, caseItem->caseLabel.expression);
            popStack(cp, 1);
        }

        /*
            Generate code for the case directives themselves.
         */
        caseItem->code = state->code = allocCodeBuffer(cp);
        assure(caseItem->left->kind == N_DIRECTIVES);
        processNode(cp, caseItem->left);
        setStack(cp, mark);
    }

    /*
        Calculate jump lengths. Start from the last case and work backwards.
     */
    nextCaseLen = 0;
    nextCodeLen = 0;
    totalLen = 0;

    next = -1;
    while ((caseItem = getPrevNode(cp, elements, &next)) && !cp->error) {
        if (caseItem->kind != N_CASE_LABEL) {
            break;
        }
        /*
            CODE jump
            Jump to the code block of the next case. In the last block, we just fall out the bottom.
         */
        caseItem->caseLabel.nextCaseCode = nextCodeLen;
        if (nextCodeLen > 0) {
            len = (caseItem->caseLabel.nextCaseCode < 0x7f && cp->optimizeLevel > 0) ? 2 : 5;
            nextCodeLen += len;
            nextCaseLen += len;
            totalLen += len;
        }

        /*
            CASE jump
            Jump to the next case expression evaluation.
         */
        len = getCodeLength(cp, caseItem->code);
        nextCodeLen += len;
        nextCaseLen += len;
        totalLen += len;

        caseItem->jumpLength = nextCaseLen;
        nextCodeLen = 0;

        if (caseItem->caseLabel.kind == EC_SWITCH_KIND_CASE) {
            /*
                Jump to the next case expression test. Increment the length depending on whether we are using a
                goto_8 (2 bytes) or goto (4 bytes). Add one for the CMPEQ instruction (3 vs 6)
             */
            len = (caseItem->jumpLength < 0x7f && cp->optimizeLevel > 0) ? 3 : 6;
            nextCodeLen += len;
            totalLen += len;

            if (caseItem->caseLabel.expressionCode) {
                len = getCodeLength(cp, caseItem->caseLabel.expressionCode);
                nextCodeLen += len;
                totalLen += len;
            }
        }
        nextCaseLen = 0;
    }

    /*
        Now copy the basic blocks into the output code buffer.
     */
    setCodeBuffer(cp, code);

    next = 0;
    while ((caseItem = getNextNode(cp, elements, &next)) && !cp->error) {

        if (caseItem->caseLabel.expressionCode) {
            copyCodeBuffer(cp, state->code, caseItem->caseLabel.expressionCode);
        }

        /*
            Encode the jump to the next case
         */
        if (caseItem->caseLabel.kind == EC_SWITCH_KIND_CASE) {
            ecEncodeOpcode(cp, EJS_OP_COMPARE_STRICTLY_EQ);
            if (caseItem->jumpLength < 0x7f && cp->optimizeLevel > 0) {
                ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE_8);
                ecEncodeByte(cp, caseItem->jumpLength);
            } else {
                ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE);
                ecEncodeInt32(cp, caseItem->jumpLength);
            }
        }
        assure(caseItem->code);
        copyCodeBuffer(cp, state->code, caseItem->code);

        /*
            Encode the jump to the next case's code. Last case/default block may have zero length jump.
         */
        if (caseItem->caseLabel.nextCaseCode > 0) {
            if (caseItem->caseLabel.nextCaseCode < 0x7f && cp->optimizeLevel > 0) {
                ecEncodeOpcode(cp, EJS_OP_GOTO_8);
                ecEncodeByte(cp, caseItem->caseLabel.nextCaseCode);
            } else {
                ecEncodeOpcode(cp, EJS_OP_GOTO);
                ecEncodeInt32(cp, caseItem->caseLabel.nextCaseCode);
            }
        }
    }
    popStack(cp, 1);

    totalLen = (int) mprGetBufLength(state->code->buf);
    patchJumps(cp, EC_JUMP_BREAK, totalLen);

    /*
        Pop the switch value
     */
    ecEncodeOpcode(cp, EJS_OP_POP);
    copyCodeBuffer(cp, outerBlock, state->code);
    LEAVE(cp);
}


/*
    Load the this pointer.
 */
static void genThis(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);

    state = cp->state;

    switch (np->thisNode.thisKind) {
    case EC_THIS_GENERATOR:
        //  TODO
        break;

    case EC_THIS_CALLEE:
        //  TODO
        break;

    case EC_THIS_TYPE:
        genClassName(cp, state->currentClass);
        break;

    case EC_THIS_FUNCTION:
        genName(cp, state->currentFunctionNode);
        break;

    default:
        ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
        pushStack(cp, 1);
    }
    LEAVE(cp);
}


/*
  
 */
static void genThrow(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    cp->state->needsValue = 1;
    processNode(cp, np->left);
    ecEncodeOpcode(cp, EJS_OP_THROW);
    popStack(cp, 1);
    LEAVE(cp);
}


/*
    Try, catch, finally.
 */
static void genTry(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EcNode          *child, *arg;
    EcCodeGen       *saveCode;
    EcState         *state;
    EjsType         *catchType;
    uint            tryStart, tryEnd, handlerStart, handlerEnd;
    int             next, len, numStack;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;

    /*
        Switch to a new code buffer for the try block
     */
    numStack = getStackCount(cp);
    saveCode = state->code;
    assure(saveCode);
    np->exception.tryBlock->code = state->code = allocCodeBuffer(cp);

    /*
        Process the try block. Will add a goto into either the finally block or if no finally block,
        to after the last catch.
     */
    state->captureFinally = np->exception.finallyBlock ? 1 : 0;
    processNode(cp, np->exception.tryBlock);
    state->captureFinally = 0;

    if (np->exception.catchClauses) {
        /*
            If there is a finally block it must be invoked before acting on any break/continue and return statements 
         */
        next = 0;
        state->captureFinally = np->exception.finallyBlock ? 1 : 0;
        state->captureBreak = 1;
        while ((child = getNextNode(cp, np->exception.catchClauses, &next)) && !cp->error) {
            child->code = state->code = allocCodeBuffer(cp);
            assure(child->left);
            processNode(cp, child->left);
            if (np->exception.finallyBlock == 0) {
                ecEncodeOpcode(cp, EJS_OP_END_EXCEPTION);
            }
            /* Add jumps below */
        }
        state->captureFinally = 0;
        state->captureBreak = 0;
    }
    if (np->exception.finallyBlock) {
        state->captureBreak = 1;
        np->exception.finallyBlock->code = state->code = allocCodeBuffer(cp);
        /* Finally pushes the original PC */
        pushStack(cp, 1);
        processNode(cp, np->exception.finallyBlock);
        ecEncodeOpcode(cp, EJS_OP_END_EXCEPTION);
        popStack(cp, 1);
        state->captureBreak = 0;
    }

    /*
        Calculate jump lengths for the catch block into a finally block. Start from the last catch block and work backwards.
     */
    len = 0;
    if (np->exception.catchClauses) {
        next = -1;
        while ((child = getPrevNode(cp, np->exception.catchClauses, &next)) && !cp->error) {
            child->jumpLength = len;
            if (child->jumpLength > 0 && np->exception.finallyBlock) {
                /*
                    Add jumps if there is a finally block. Otherwise, we use and end_ecception instruction
                    Increment the length depending on whether we are using a goto_8 (2 bytes) or goto (4 bytes)
                 */
                len += (child->jumpLength < 0x7f && cp->optimizeLevel > 0) ? 2 : 5;
            }
            len += getCodeLength(cp, child->code);
        }
    }

    /*
        Now copy the code. First the try block. Restore the primary code buffer and copy try/catch/finally
        code blocks into the code buffer.
     */
    setCodeBuffer(cp, saveCode);

    tryStart = ecGetCodeOffset(cp);

    /*
        Copy the try code and add a jump
     */
    copyCodeBuffer(cp, state->code, np->exception.tryBlock->code);

    if (np->exception.finallyBlock) {
        ecEncodeOpcode(cp, EJS_OP_GOTO_FINALLY);
    } else if (len < 0x7f && cp->optimizeLevel > 0) {
        ecEncodeOpcode(cp, EJS_OP_GOTO_8);
        ecEncodeByte(cp, len);
    } else {
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        ecEncodeInt32(cp, len);
    }
    tryEnd = ecGetCodeOffset(cp);

    /*
        Now the copy the catch blocks and add jumps
     */
    if (np->exception.catchClauses) {
        next = 0;
        while ((child = getNextNode(cp, np->exception.catchClauses, &next)) && !cp->error) {
            handlerStart = ecGetCodeOffset(cp);
            copyCodeBuffer(cp, state->code, child->code);
            if (child->jumpLength > 0 && np->exception.finallyBlock) {
                if (child->jumpLength < 0x7f && cp->optimizeLevel > 0) {
                    ecEncodeOpcode(cp, EJS_OP_GOTO_8);
                    ecEncodeByte(cp, child->jumpLength);
                } else {
                    ecEncodeOpcode(cp, EJS_OP_GOTO);
                    ecEncodeInt32(cp, child->jumpLength);
                }
            }
            handlerEnd = ecGetCodeOffset(cp);

            /*
                Create exception handler record
             */
            catchType = 0;
            arg = 0;
            if (child->catchBlock.arg && child->catchBlock.arg->left) {
                arg = child->catchBlock.arg->left;
            }
            if (arg && arg->typeNode && ejsIsType(cp->ejs, arg->typeNode->lookup.ref)) {
                catchType = (EjsType*) arg->typeNode->lookup.ref;
            }
            if (catchType == 0) {
                catchType = EST(Void);
            }
            ecAddNameConstant(cp, catchType->qname);
            addException(cp, tryStart, tryEnd, catchType, handlerStart, handlerEnd, np->exception.numBlocks, numStack, 
                EJS_EX_CATCH);
        }
    }

    /*
        Finally, the finally block
     */
    if (np->exception.finallyBlock) {
        handlerStart = ecGetCodeOffset(cp);
        copyCodeBuffer(cp, state->code, np->exception.finallyBlock->code);
        handlerEnd = ecGetCodeOffset(cp);
        addException(cp, tryStart, tryEnd, EST(Void), handlerStart, handlerEnd, np->exception.numBlocks, numStack, 
            EJS_EX_FINALLY);
    }
    LEAVE(cp);
}


static void genUnaryOp(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    assure(np->kind == N_UNARY_OP);
    assure(np->left);

    switch (np->tokenId) {
    case T_DELETE:
        genDelete(cp, np);
        break;

    case T_LOGICAL_NOT:
        cp->state->needsValue = 1;
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_LOGICAL_NOT);
        break;

    case T_PLUS:
        /* Just ignore the plus */
        processNode(cp, np->left);
        break;

    case T_PLUS_PLUS:
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_INC);
        ecEncodeByte(cp, 1);
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
        genLeftHandSide(cp, np->left);
        break;

    case T_MINUS:
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_NEG);
        break;

    case T_MINUS_MINUS:
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_INC);
        ecEncodeByte(cp, -1);
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
        genLeftHandSide(cp, np->left);
        break;

    case T_TILDE:
        /* Bitwise not */
        cp->state->needsValue = 1;
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_NOT);
        break;

    case T_TYPEOF:
        cp->state->needsValue = 1;
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_TYPE_OF);
        break;

    case T_VOID:
        /* Ignore the node and just push a void */
        ecEncodeOpcode(cp, EJS_OP_LOAD_UNDEFINED);
        pushStack(cp, 1);
        break;
    }
    LEAVE(cp);
}


static void genNameExpr(EcCompiler *cp, EcNode *np)
{
    EcState     *state;
    
    ENTER(cp);
    
    state = cp->state;
    state->currentObjectNode = 0;
    state->onLeft = 0;
    
    if (np->name.qualifierExpr) {
        processNode(cp, np->name.qualifierExpr);
    } else {
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeConst(cp, np->qname.space);
        pushStack(cp, 1);
    }
    if (np->name.nameExpr) {
        processNode(cp, np->name.nameExpr);
    } else {
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeConst(cp, np->qname.name);
        pushStack(cp, 1);
    }
    LEAVE(cp);
}


/*
    Generate code for an unbound name reference. We don't know the slot.
 */
static void genUnboundName(EcCompiler *cp, EcNode *np)
{
    EcState     *state;
    EjsObj      *owner;
    EjsLookup   *lookup;
    int         code;

    ENTER(cp);

    state = cp->state;
    assure(!np->lookup.bind || !cp->bind);

    lookup = &np->lookup;
    owner = lookup->obj;
    
    if (state->currentObjectNode && np->needThis) {
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
        np->needThis = 0;
    }
    if (np->name.qualifierExpr || np->name.nameExpr) {
        genNameExpr(cp, np);
        if (state->currentObjectNode) {
            code = (!cp->state->onLeft) ? EJS_OP_GET_OBJ_NAME_EXPR :  EJS_OP_PUT_OBJ_NAME_EXPR;
            popStack(cp, (cp->state->onLeft) ? 4 : 2);
        } else {
            code = (!cp->state->onLeft) ? EJS_OP_GET_SCOPED_NAME_EXPR :  EJS_OP_PUT_SCOPED_NAME_EXPR;
            popStack(cp, (cp->state->onLeft) ? 3 : 1);
        }
        ecEncodeOpcode(cp, code);
        LEAVE(cp);
        return;
    }
    if (state->currentObjectNode) {
        /*
            Property name (requires obj on stack)
            Store: -2, load: 0
         */
        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_NAME :  EJS_OP_PUT_OBJ_NAME;
        ecEncodeOpcode(cp, code);
        ecEncodeName(cp, np->qname);

        popStack(cp, 1);
        pushStack(cp, (state->onLeft) ? -1 : 1);

    } else if (lookup->useThis) {
        ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
        pushStack(cp, 1);
        if (np->needThis) {
            assure(0);
            ecEncodeOpcode(cp, EJS_OP_DUP);
            pushStack(cp, 1);
            np->needThis = 0;
        }
        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_NAME :  EJS_OP_PUT_OBJ_NAME;
        ecEncodeOpcode(cp, code);
        ecEncodeName(cp, np->qname);

        /*
            Store: -2, load: 0
         */
        popStack(cp, 1);
        pushStack(cp, (state->onLeft) ? -1 : 1);

    } else if (owner && ejsIsType(ejs, owner)) {
        SAVE_ONLEFT(cp);
        genClassName(cp, (EjsType*) owner);
        RESTORE_ONLEFT(cp);

        if (np->needThis) {
            assure(0);
            ecEncodeOpcode(cp, EJS_OP_DUP);
            pushStack(cp, 1);
            np->needThis = 0;
        }
        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_NAME :  EJS_OP_PUT_OBJ_NAME;
        ecEncodeOpcode(cp, code);
        ecEncodeName(cp, np->qname);

        /*
            Store: -2, load: 0
         */
        popStack(cp, 1);
        pushStack(cp, (state->onLeft) ? -1 : 1);

    } else {
        /*
            Unqualified name
         */
        if (np->needThis) {
            assure(0);
            ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
            pushStack(cp, 1);
            np->needThis = 0;
        }
        code = (!state->onLeft) ?  EJS_OP_GET_SCOPED_NAME :  EJS_OP_PUT_SCOPED_NAME;
        ecEncodeOpcode(cp, code);
        ecEncodeName(cp, np->qname);

        /*
            Store: -1, load: 1
         */
        pushStack(cp, (state->onLeft) ? -1 : 1);
    }
    LEAVE(cp);
}


static void genModule(EcCompiler *cp, EcNode *np)
{    
    ENTER(cp);

    assure(np->kind == N_MODULE);

    addModule(cp, np->module.ref);
    genBlock(cp, np->left);
    LEAVE(cp);
}


static void genUseModule(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    int         next;

    ENTER(cp);

    assure(np->kind == N_USE_MODULE);

    next = 0;
    while ((child = getNextNode(cp, np, &next))) {
        processNode(cp, child);
    }
    LEAVE(cp);
}


static void genUseNamespace(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    assure(np->kind == N_USE_NAMESPACE);

    /*
        Load the namespace reference. NOTE: use default space; will not add a namespace to the set of open spaces.
     */
    if (np->name.isLiteral) {
        ecEncodeOpcode(cp, EJS_OP_ADD_NAMESPACE);
        ecEncodeConst(cp, np->qname.name);
    } else {
        genName(cp, np);
        ecEncodeOpcode(cp, EJS_OP_ADD_NAMESPACE_REF);
        popStack(cp, 1);
    }
    LEAVE(cp);
}


static void genVar(EcCompiler *cp, EcNode *np)
{
    assure(np->kind == N_VAR);

    ENTER(cp);
    ecAddNameConstant(cp, np->qname);
    if (np->lookup.trait && np->lookup.trait->type) {
        ecAddStringConstant(cp, np->lookup.trait->type->qname.name);
    }
    if (cp->ejs->flags & EJS_FLAG_DOC) {
        ecAddDocConstant(cp, "var", np->lookup.obj, np->lookup.slotNum);
    }
    if (np->left) {
        processNode(cp, np->left);
    }
    LEAVE(cp);
}


static void genVarDefinition(EcCompiler *cp, EcNode *np)
{
    EcState     *state;
    EcNode      *var;
    int         next;

    assure(np->kind == N_VAR_DEFINITION);

    ENTER(cp);
    state = cp->state;

    for (next = 0; (var = getNextNode(cp, np, &next)) != 0; ) {
        if (var->kind == N_VAR) {
            if (var->left) {
                /*
                    Class level variable initializations must go into the instance code buffer.
                 */
                if (var->name.instanceVar) {
                    state->instanceCode = 1;
                    assure(state->instanceCodeBuf);
                    state->code = state->instanceCodeBuf;
                }
            }
            genVar(cp, var);
        } else {
            processNode(cp, var);
        }
    }
    LEAVE(cp);
}


static void genWith(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    processNode(cp, np->with.object);
    ecEncodeOpcode(cp, EJS_OP_OPEN_WITH);
    popStack(cp, 1);
    processNode(cp, np->with.statement);
    ecEncodeOpcode(cp, EJS_OP_CLOSE_BLOCK);
    LEAVE(cp);
}


/********************************* Support Code *******************************/
/*
    Create the module file.
 */

static MprFile *openModuleFile(EcCompiler *cp, cchar *filename)
{
    EcState     *state;

    assure(cp);
    assure(filename && *filename);

    state = cp->state;

    if (cp->noout) {
        return 0;
    }
    filename = mprJoinPath(cp->outputDir, filename);
    if ((cp->file = mprOpenFile(filename,  O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, 0664)) == 0) {
        genError(cp, 0, "Cannot create module file \"%s\"", filename);
        return 0;
    }

    /*
        Create a module header once per file instead of per-module in the file
     */
    state->code = allocCodeBuffer(cp);
    if (ecCreateModuleHeader(cp) < 0) {
        genError(cp, 0, "Cannot write module file header");
        return 0;
    }
    return cp->file;
}


static void manageCodeGen(EcCodeGen *code, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(code->buf);
        mprMark(code->jumps);
        mprMark(code->exceptions);
        if (code->debug) {
            assure(code->debug->magic == EJS_DEBUG_MAGIC);
        }
        mprMark(code->debug);
    }
}


/*
    Create a new code buffer
 */
static EcCodeGen *allocCodeBuffer(EcCompiler *cp)
{
    EcState     *state;
    EcCodeGen   *code;

    assure(cp);

    state = cp->state;
    assure(state);

    if ((code = mprAllocObj(EcCodeGen, manageCodeGen)) == 0) {
        cp->fatalError = 1;
        return 0;
    }
    if ((code->buf = mprCreateBuf(EC_CODE_BUFSIZE, 0)) == 0) {
        assure(0);
        cp->fatalError = 1;
        return 0;
    }
    if ((code->exceptions = mprCreateList(-1, 0)) == 0) {
        assure(0);
        return 0;
    }
    /*
        Jumps are fully processed before the state is freed
     */
    code->jumps = mprCreateList(-1, 0);
    if (code->jumps == 0) {
        assure(0);
        return 0;
    }

    /*
        Inherit the allowable jump kinds and stack level
     */
    if (state->code) {
        code->jumpKinds = state->code->jumpKinds;
        code->blockCount = state->code->blockCount;
        code->stackCount = state->code->stackCount;
        code->breakMark = state->code->breakMark;
        code->blockMark = state->code->blockMark;
    }
    return code;
}


static int getCodeLength(EcCompiler *cp, EcCodeGen *code)
{
    return (int) mprGetBufLength(code->buf);
}


static void copyCodeBuffer(EcCompiler *cp, EcCodeGen *dest, EcCodeGen *src)
{
    EjsEx           *exception;
    EjsDebug        *debug;
    EcJump          *jump;
    uint            baseOffset;
    int             next, len, i;

    assure(dest != src);

    len = getCodeLength(cp, src);
    if (len <= 0) {
        return;
    }
    /*
        Copy the code
     */
    baseOffset = (int) mprGetBufLength(dest->buf);
    if (mprPutBlockToBuf(dest->buf, mprGetBufStart(src->buf), len) != len) {
        assure(0);
        return;
    }
    /*
        Copy and fix the jump offset of jump patch records. jump->offset starts out being relative to the current code src.
        We add the original length of dest to make it absolute to the new dest buffer.
     */
    if (src->jumps) {
        if (src->jumps != dest->jumps) {
            next = 0;
            while ((jump = (EcJump*) mprGetNextItem(src->jumps, &next)) != 0) {
                jump->offset += baseOffset;
                mprAddItem(dest->jumps, jump);
            }
        }
    }

    /*
        Copy and fix exception target addresses
     */
    if (src->exceptions) {
        next = 0;
        while ((exception = (EjsEx*) mprGetNextItem(src->exceptions, &next)) != 0) {
            exception->tryStart += baseOffset;
            exception->tryEnd += baseOffset;
            exception->handlerStart += baseOffset;
            exception->handlerEnd += baseOffset;
            mprAddItem(dest->exceptions, exception);
        }
    }

    /*
        Copy and fix debug offsets
     */
    if (src->debug) {
        debug = src->debug;
        for (i = 0; i < debug->numLines; i++) {
            addDebugLine(cp, dest, baseOffset + debug->lines[i].offset, debug->lines[i].source);
        }
    }
}


/*
    Patch jump addresses a code buffer. Kind is the kind of jump (break | continue)
 */
static void patchJumps(EcCompiler *cp, int kind, int target)
{
    EcJump      *jump;
    EcCodeGen   *code;
    int         next, offset;

    code = cp->state->code;
    assure(code);

rescan:
    next = 0;
    while ((jump = (EcJump*) mprGetNextItem(code->jumps, &next)) != 0) {
        if (jump->kind == kind) {
            offset = target - jump->offset - 4;
            assure(-10000 < offset && offset < 10000);
            assure(jump->offset < mprGetBufLength(code->buf));
            ecEncodeInt32AtPos(cp, jump->offset, offset);
            mprRemoveItem(code->jumps, jump);
            goto rescan;
        }
    }
}


/*
    Write the module contents
 */
static int flushModule(MprFile *file, EcCodeGen *code)
{
    int         len;

    len = (int) mprGetBufLength(code->buf);
    if (len > 0) {
        if (mprWriteFile(file, mprGetBufStart(code->buf), len) != len) {
            return EJS_ERR;
        }
        mprFlushBuf(code->buf);
    }
    return 0;
}


/*
    Create the module initializer
 */
static void createInitializer(EcCompiler *cp, EjsModule *mp)
{
    EjsFunction     *fun;
    EcState         *state;
    EcCodeGen       *code;

    ENTER(cp);

    state = cp->state;
    assure(state);

    /*
        Note: if hasInitializer is false, we may still have some code in the buffer if --debug is used.
        We can safely just ignore this debug code.
     */
    if (!mp->hasInitializer) {
        LEAVE(cp);
        return;
    }
    assure((int) mprGetBufLength(mp->code->buf) > 0);

    if (cp->errorCount > 0) {
        LEAVE(cp);
        return;
    }
    state->code = mp->code;
    cp->directiveState = state;
    code = cp->state->code;
    ecEncodeOpcode(cp, EJS_OP_END_CODE);

    /*
        Extract the initialization code
     */
    fun = state->currentFunction = mp->initializer;
    if (fun) {
        setFunctionCode(cp, fun, code);
    }
    LEAVE(cp);
}


static EcNode *getNextNode(EcCompiler *cp, EcNode *np, int *next)
{
    if (cp->error) {
        return 0;
    }
    return (EcNode*) mprGetNextItem(np->children, next);
}


static EcNode *getPrevNode(EcCompiler *cp, EcNode *np, int *next)
{
    if (cp->fatalError || cp->error) {
        return 0;
    }
    return (EcNode*) mprGetPrevItem(np->children, next);
}


/*
    Map a lexical token to an op code
 */
static int mapToken(EcCompiler *cp, int tokenId)
{
    int     cond;

    cond = cp->state->conditional;

    switch (tokenId) {
    case T_BIT_AND:
        return EJS_OP_AND;

    case T_BIT_OR:
        return EJS_OP_OR;

    case T_BIT_XOR:
        return EJS_OP_XOR;

    case T_DIV:
        return EJS_OP_DIV;

    case T_EQ:
        return (cond) ? EJS_OP_BRANCH_EQ : EJS_OP_COMPARE_EQ;

    case T_NE:
        return (cond) ? EJS_OP_BRANCH_NE : EJS_OP_COMPARE_NE;

    case T_GT:
        return (cond) ? EJS_OP_BRANCH_GT : EJS_OP_COMPARE_GT;

    case T_GE:
        return (cond) ? EJS_OP_BRANCH_GE : EJS_OP_COMPARE_GE;

    case T_LT:
        return (cond) ? EJS_OP_BRANCH_LT : EJS_OP_COMPARE_LT;

    case T_LE:
        return (cond) ? EJS_OP_BRANCH_LE : EJS_OP_COMPARE_LE;

    case T_STRICT_EQ:
        return (cond) ? EJS_OP_BRANCH_STRICTLY_EQ : EJS_OP_COMPARE_STRICTLY_EQ;

    case T_STRICT_NE:
        return (cond) ? EJS_OP_BRANCH_STRICTLY_NE : EJS_OP_COMPARE_STRICTLY_NE;

    case T_LSH:
        return EJS_OP_SHL;

    case T_LOGICAL_NOT:
        return EJS_OP_NOT;

    case T_MINUS:
        return EJS_OP_SUB;

    case T_MOD:
        return EJS_OP_REM;

    case T_MUL:
        return EJS_OP_MUL;

    case T_PLUS:
        return EJS_OP_ADD;

    case T_RSH:
        return EJS_OP_SHR;

    case T_RSH_ZERO:
        return EJS_OP_USHR;

    case T_IS:
        return EJS_OP_IS_A;

    case T_INSTANCEOF:
        return EJS_OP_INST_OF;

    case T_CAST:
        return EJS_OP_CAST;

    case T_IN:
        return EJS_OP_IN;

    default:
        assure(0);
        return -1;
    }
}


static void addDebugLine(EcCompiler *cp, EcCodeGen *code, int offset, wchar *source)
{
    assure(code->debug == 0 || code->debug->magic == EJS_DEBUG_MAGIC);
    if (ejsAddDebugLine(cp->ejs, &code->debug, offset, source) < 0) {
        genError(cp, 0, "Cannot allocate memory for debug section");
        return;
    }
}


static void addDebug(EcCompiler *cp, EcNode *np)
{
    EcCodeGen   *code;
    wchar       *source;
    int         offset;

    code = cp->state->code;
    if (!cp->debug || code == 0 || np->loc.lineNumber <= code->lastLineNumber) {
        return;
    }
    source = np->loc.source;
    if (source[0] == '}' && source[1] == 0) {
        return;
    }
    offset = (int) mprGetBufLength(code->buf);
    source = (wchar*) mfmt("%s|%d|%w", np->loc.filename, np->loc.lineNumber, np->loc.source);
    addDebugLine(cp, code, offset, source);
    code->lastLineNumber = np->loc.lineNumber;
}


static void processNode(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);
    state = cp->state;

    assure(np->parent || np->kind == N_PROGRAM || np->kind == N_MODULE);

    if (np->kind != N_FUNCTION) {
        addDebug(cp, np);
    }
    switch (np->kind) {
    case N_ARGS:
        state->needsValue = 1;
        genArgs(cp, np);
        break;

    case N_ASSIGN_OP:
        genAssignOp(cp, np);
        break;

    case N_BINARY_OP:
        genBinaryOp(cp, np);
        break;

    case N_BLOCK:
        genBlock(cp, np);
        break;

    case N_BREAK:
        genBreak(cp, np);
        break;

    case N_CALL:
        genCall(cp, np);
        break;

    case N_CLASS:
        genClass(cp, np);
        break;

    case N_CATCH_ARG:
        genCatchArg(cp, np);
        break;

    case N_CONTINUE:
        genContinue(cp, np);
        break;

    case N_DASSIGN:
        genDassign(cp, np);
        break;

    case N_DIRECTIVES:
        genDirectives(cp, np, 0);
        break;

    case N_DO:
        genDo(cp, np);
        break;

    case N_DOT:
        genDot(cp, np, 0);
        break;

    case N_END_FUNCTION:
        genEndFunction(cp, np);
        break;

    case N_EXPRESSIONS:
        genExpressions(cp, np);
        break;

    case N_FOR:
        genFor(cp, np);
        break;

    case N_FOR_IN:
        genForIn(cp, np);
        break;

    case N_FUNCTION:
        genFunction(cp, np);
        break;

    case N_HASH:
        genHash(cp, np);
        break;

    case N_IF:
        genIf(cp, np);
        break;

    case N_LITERAL:
        genLiteral(cp, np);
        break;

    case N_OBJECT_LITERAL:
        genObjectLiteral(cp, np);
        break;

    case N_FIELD:
        genField(cp, np);
        break;

    case N_QNAME:
        genName(cp, np);
        break;

    case N_NEW:
        genNew(cp, np);
        break;

    case N_NOP:
        break;

    case N_POSTFIX_OP:
        genPostfixOp(cp, np);
        break;

    case N_PRAGMA:
        break;

    case N_PRAGMAS:
        genPragmas(cp, np);
        break;

    case N_PROGRAM:
        genProgram(cp, np);
        break;

    case N_REF:
        break;

    case N_RETURN:
        genReturn(cp, np);
        break;

    case N_SPREAD:
        genSpread(cp, np);
        break;

    case N_SUPER:
        genSuper(cp, np);
        break;

    case N_SWITCH:
        genSwitch(cp, np);
        break;

    case N_THIS:
        genThis(cp, np);
        break;

    case N_THROW:
        genThrow(cp, np);
        break;

    case N_TRY:
        genTry(cp, np);
        break;

    case N_UNARY_OP:
        genUnaryOp(cp, np);
        break;

    case N_USE_NAMESPACE:
        genUseNamespace(cp, np);
        break;

    case N_VAR:
        genVar(cp, np);
        break;

    case N_VAR_DEFINITION:
        genVarDefinition(cp, np);
        break;

    case N_MODULE:
        genModule(cp, np);
        break;

    case N_USE_MODULE:
        genUseModule(cp, np);
        break;

    case N_WITH:
        genWith(cp, np);
        break;

    default:
        assure(0);
        badNode(cp, np);
    }
    assure(state == cp->state);
    LEAVE(cp);
}


/*
    Oputput one module.
 */
static void processModule(EcCompiler *cp, EjsModule *mp)
{
    EcState     *state;
    EcCodeGen   *code;
    char        *path;

    ENTER(cp);

    state = cp->state;
    state->currentModule = mp;

    createInitializer(cp, mp);

    if (cp->noout) {
        return;
    }
    if (! cp->outputFile) {
        if (mp->version) {
            path = sfmt("%s-%d.%d.%d%s", mp->name, EJS_MAJOR(mp->version), EJS_MINOR(mp->version), 
                EJS_PATCH(mp->version), EJS_MODULE_EXT);
        } else {
            path = sfmt("%@%s", mp->name, EJS_MODULE_EXT);
        }
        if ((mp->file = openModuleFile(cp, path)) == 0) {
            LEAVE(cp);
            return;
        }

    } else {
        mp->file = cp->file;
    }
    assure(mp->code);
    assure(mp->file);

    code = state->code;

    if (mp->hasInitializer) {
        //  TODO -- make these standard strings in native core
        ecAddCStringConstant(cp, EJS_INITIALIZER_NAME);
        ecAddCStringConstant(cp, EJS_EJS_NAMESPACE);
        if (mp->initializer->resultType) {
            ecAddNameConstant(cp, mp->initializer->resultType->qname);
        }
    }
    if (ecCreateModuleSection(cp) < 0) {
        genError(cp, 0, "Cannot write module sections");
        LEAVE(cp);
        return;
    }
    if (flushModule(mp->file, code) < 0) {
        genError(cp, 0, "Cannot write to module file %s", mp->name);
        LEAVE(cp);
        return;
    }
    if (! cp->outputFile) {
        mprCloseFile(mp->file);
        mp->file = 0;
        mp->code = 0;
    } else {
        mp->code = 0;
    }
    mp->file = 0;
}


/*
    Keep a list of modules potentially containing generated code and declarations.
 */
static void addModule(EcCompiler *cp, EjsModule *mp)
{
    EjsModule       *module;
    Ejs             *ejs;
    EcState         *state;
    int             next;

    assure(cp);

    state = cp->state;
    ejs = cp->ejs;

    if (mp->code == 0 || cp->interactive) {
        mp->code = state->code = allocCodeBuffer(cp);
    }
    mp->loaded = 0;
    state->code = mp->code;
    assure(mp->code);
    assure(mp->code->buf);

    state->currentModule = mp;
    state->varBlock = ejs->global;
    state->letBlock = ejs->global;

    assure(mp->initializer);
    state->currentFunction = mp->initializer;

    /*
        Merge means aggregate dependent input modules with the output
     */
    if (mp->dependencies && !cp->merge) {
        for (next = 0; (module = mprGetNextItem(mp->dependencies, &next)) != 0; ) {
            ecAddStringConstant(cp, module->name);
        }
    }
}


//  TODO -- cleanup
static int level = 8;

static void pushStack(EcCompiler *cp, int count)
{
    EcCodeGen       *code;

    code = cp->state->code;

    assure(code);

    assure(code->stackCount >= 0);
    code->stackCount += count;
    assure(code->stackCount >= 0);

    mprLog(level, "Stack %d, after push %d", code->stackCount, count);
}


static void popStack(EcCompiler *cp, int count)
{
    EcCodeGen       *code;

    code = cp->state->code;

    assure(code);
    assure(code->stackCount >= 0);

    code->stackCount -= count;
    assure(code->stackCount >= 0);

    mprLog(level, "Stack %d, after pop %d", code->stackCount, count);
}


static void setStack(EcCompiler *cp, int count)
{
    EcCodeGen       *code;

    code = cp->state->code;
    assure(code);
    code->stackCount = count;
}


static int getStackCount(EcCompiler *cp)
{
    return cp->state->code->stackCount;
}


static void discardStackItems(EcCompiler *cp, int preserve)
{
    EcCodeGen       *code;
    int             count;

    code = cp->state->code;

    assure(code);
    count = code->stackCount - preserve;

    if (count <= 0) {
        return;
    }
    if (count == 1) {
        ecEncodeOpcode(cp, EJS_OP_POP);
    } else {
        ecEncodeOpcode(cp, EJS_OP_POP_ITEMS);
        ecEncodeByte(cp, count);
    }
    code->stackCount -= count;
    assure(code->stackCount >= 0);
    mprLog(level, "Stack %d, after discard\n", code->stackCount);
}


static void discardBlockItems(EcCompiler *cp, int preserve)
{
    EcCodeGen       *code;
    int             count, i;

    code = cp->state->code;
    assure(code);
    count = code->blockCount - preserve;

    for (i = 0; i < count; i++) {
        ecEncodeOpcode(cp, EJS_OP_CLOSE_BLOCK);
    }
    code->blockCount -= count;
    assure(code->blockCount >= 0);
    mprLog(level, "Block level %d, after discard\n", code->blockCount);
}


/*
    Set the default code buffer
 */
static void setCodeBuffer(EcCompiler *cp, EcCodeGen *saveCode)
{
    cp->state->code = saveCode;
    mprLog(level, "Stack %d, after restore code buffer\n", cp->state->code->stackCount);
}


static void addException(EcCompiler *cp, uint tryStart, uint tryEnd, EjsType *catchType, uint handlerStart, uint handlerEnd, 
    int numBlocks, int numStack, int flags)
{
    EcCodeGen       *code;
    EcState         *state;
    EjsEx           *exception;

    state = cp->state;
    assure(state);

    code = state->code;
    assure(code);

    if ((exception = mprAllocZeroed(sizeof(EjsEx))) == 0) {
        assure(0);
        return;
    }
    exception->tryStart = tryStart;
    exception->tryEnd = tryEnd;
    exception->catchType = catchType;
    exception->handlerStart = handlerStart;
    exception->handlerEnd = handlerEnd;
    exception->numBlocks = numBlocks;
    exception->numStack = numStack;
    exception->flags = flags;
    mprAddItem(code->exceptions, exception);
}


static void addJump(EcCompiler *cp, EcNode *np, int kind)
{
    EcJump      *jump;

    ENTER(cp);

    jump = mprAllocZeroed(sizeof(EcJump));
    assure(jump);

    jump->kind = kind;
    jump->node = np;
    jump->offset = ecGetCodeOffset(cp);

    mprAddItem(cp->state->code->jumps, jump);
    LEAVE(cp);
}


static void setFunctionCode(EcCompiler *cp, EjsFunction *fun, EcCodeGen *code)
{
    EjsEx       *ex;
    int         next, len;

    len = (int) mprGetBufLength(code->buf);
    assure(len >= 0);
    if (len > 0) {
        ejsSetFunctionCode(cp->ejs, fun, cp->state->currentModule, (uchar*) mprGetBufStart(code->buf), len, code->debug);
    }
    /*
        Define any try/catch blocks encountered
     */
    next = 0;
    while ((ex = (EjsEx*) mprGetNextItem(code->exceptions, &next)) != 0) {
        ejsAddException(cp->ejs, fun, ex->tryStart, ex->tryEnd, ex->catchType, ex->handlerStart, 
            ex->handlerEnd, ex->numBlocks, ex->numStack, ex->flags, -1);
    }
}


static void emitNamespace(EcCompiler *cp, EjsNamespace *nsp)
{
    ecEncodeOpcode(cp, EJS_OP_ADD_NAMESPACE);
    ecEncodeConst(cp, nsp->value);
}


/*
    Aggregate the allowable kinds of jumps
 */
PUBLIC void ecStartBreakableStatement(EcCompiler *cp, int kinds)
{
    EcState     *state;

    assure(cp);

    state = cp->state;
    state->code->jumpKinds |= kinds;
    state->breakState = state;
    state->code->breakMark = state->code->stackCount;
    state->code->blockMark = state->code->blockCount;
}

static void genError(EcCompiler *cp, EcNode *np, char *fmt, ...)
{
    va_list     args;
    EcLocation  *loc;

    va_start(args, fmt);

    cp->errorCount++;
    cp->error = 1;
    cp->noout = 1;
    if (np) {
        loc = &np->loc;
        ecErrorv(cp, "Error", loc, fmt, args);
    } else {
        ecErrorv(cp, "Error", NULL, fmt, args);
    }
    va_end(args);
}


static void badNode(EcCompiler *cp, EcNode *np)
{
    cp->fatalError = 1;
    cp->errorCount++;
    mprError("Unsupported language feature\nUnknown AST node kind %d", np->kind);
}


/*
    @copy   default
  
    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.
    Copyright (c) Michael O'Brien, 1993-2012. All Rights Reserved.
  
    This software is distributed under commercial and open source licenses.
    You may use the GPL open source license described below or you may acquire
    a commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details.
  
    This software is open source; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version. See the GNU General Public License for more
    details at: http://embedthis.com/downloads/gplLicense.html
  
    This program is distributed WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
    This GPL license does NOT permit incorporating this software into
    proprietary programs. If you are unable to comply with the GPL, you must
    acquire a commercial license to use this software. Commercial licenses
    for this software and support services are available from Embedthis
    Software at http://embedthis.com
  
    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */

