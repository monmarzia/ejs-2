/**
    ejsFunction.c - Function class

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/*********************************** Forwards *********************************/

static void setFunctionAttributes(EjsFunction *fun, int attributes);

/************************************* Code ***********************************/
/*
    Create a function object.
 */
static EjsFunction *createFunction(Ejs *ejs, EjsType *type, int numProp)
{
    EjsFunction     *fun;

    if ((fun = ejsCreatePot(ejs, ESV(Function), 0)) == 0) {
        return 0;
    }
    fun->block.pot.isFunction = 1;
    SET_DYNAMIC(fun, 1);
    return fun;
}


/*
    Cast the operand to the specified type

    function cast(type: Type) : Object
 */
static EjsAny *castFunction(Ejs *ejs, EjsFunction *vp, EjsType *type)
{
    switch (type->sid) {
    case S_String:
        return ejsCreateStringFromAsc(ejs, "[function Function]");

    case S_Number:
        return ESV(nan);

    case S_Boolean:
        return ESV(true);
            
    default:
        ejsThrowTypeError(ejs, "Cannot cast type \"%@\"", type->qname.name);
        return 0;
    }
    return 0;
}


PUBLIC EjsFunction *ejsCloneFunction(Ejs *ejs, EjsFunction *src, int deep)
{
    EjsFunction     *dest;

    if ((dest = (EjsFunction*) ejsCloneBlock(ejs, &src->block, deep)) == 0) {
        return 0;
    }
    dest->body.code = src->body.code;
    dest->resultType = src->resultType;
    dest->boundArgs = src->boundArgs;
    dest->boundThis = src->boundThis;
    dest->numArgs = src->numArgs;
    dest->numDefault = src->numDefault;

    /*
        OPT
     */
    dest->staticMethod = src->staticMethod;
    dest->hasReturn = src->hasReturn;
    dest->isConstructor = src->isConstructor;
    dest->isInitializer = src->isInitializer;
    dest->isNativeProc = src->isNativeProc;
    dest->moduleInitializer = src->moduleInitializer;
    dest->rest = src->rest;
    dest->fullScope = src->fullScope;
    dest->strict = src->strict;
    dest->name = src->name;

    if (src->activation) {
        dest->activation = ejsClonePot(ejs, src->activation, 0);
    }
    mprCopyName(dest, src);
    return dest;
}


/*************************************************************************************************************/
/*
    function Function(...[args], body)
 */
static EjsFunction *fun_Function(Ejs *ejs, EjsFunction *fun, int argc, void *argv)
{
#if FUTURE
    EjsArray        *args;
    EjsString       *str;
    MprBuf          *buf;
    cchar           *body, *param, *script;
    int             i, count;
    
    assure(argc > 1);
    args = (EjsArray*) argv[1];
    assure(ejsIs(ejs, args, Array));

    if (args->length <= 0) {
        ejsThrowArgError(ejs, "Missing function body");
        return 0;
    }
    str = ejsToString(ejs, args->data[args->length - 1]);
    body = ejsToMulti(ejs, str);

    buf = mprCreateBuf(ejs, -1, -1);
    mprPutStringToBuf(buf, "function(");
    count = args->length - 1;
    for (i = 0; i < count; i++) {
        str = ejsToString(ejs, args->data[i]);
        param = ejsToMulti(ejs, str);
        mprPutStringToBuf(buf, param);
        if (i < (count - 1)) {
            mprPutCharToBuf(buf, ',');
        }
        mprPutStringToBuf(buf, "\n{");
    }
    mprPutStringToBuf(buf, body);
    mprPutStringToBuf(buf, "\n}");

    script = mprGetBufStart(buf);
    if (ejsLoadScriptLiteral(ejs, script, NULL, EC_FLAGS_NO_OUT | EC_FLAGS_DEBUG | EC_FLAGS_THROW | EC_FLAGS_VISIBLE) < 0) {
        //  TODO -- what happens to compiler errors
        return 0;
    }
    fun->body.code = ;
    fun->body.codeLen
#endif
    return fun;
}

/*
    function apply(thisObj: Object, args: Array)
 */
static EjsObj *fun_applyFunction(Ejs *ejs, EjsFunction *fun, int argc, EjsObj **argv)
{
    EjsArray    *args;
    EjsObj      *save, *result, *thisObj;
    
    assure(argc > 1);
    args = (EjsArray*) argv[1];
    assure(ejsIs(ejs, args, Array));

    save = fun->boundThis;
    thisObj = argv[0];
    if (thisObj == ESV(null)) {
        thisObj = fun->boundThis ? fun->boundThis : ejs->global;
    }
    result =  ejsRunFunction(ejs, fun, thisObj, args->length, args->data);
    fun->boundThis = save;
    return result;
}


/*
    function bind(thisObj: Object, ...args): Void
 */
static EjsObj *fun_bindFunction(Ejs *ejs, EjsFunction *fun, int argc, EjsObj **argv)
{
    EjsAny      *thisObj;

    assure(argc >= 1);

    thisObj = argv[0];
    fun->boundThis = ejsIsDefined(ejs, thisObj) ? thisObj : 0;
    if (argc == 2) {
        fun->boundArgs = (EjsArray*) argv[1];
        assure(ejsIs(ejs, fun->boundArgs, Array));
    }
    return 0;
}


/*
    function get bound(): Object
 */
static EjsAny *fun_bound(Ejs *ejs, EjsFunction *fun, int argc, EjsObj **argv)
{
    return fun->boundThis ? fun->boundThis : ESV(undefined);
}


/*
    function call(thisObj, ...args)
 */
static EjsObj *fun_call(Ejs *ejs, EjsFunction *fun, int argc, EjsObj **argv)
{
    assure(argc > 1);
    return fun_applyFunction(ejs, fun, argc, argv);
}


/*
    Return the number of required args.

    function get length(): Number
 */
static EjsNumber *fun_length(Ejs *ejs, EjsFunction *fun, int argc, EjsObj **argv)
{
    return ejsCreateNumber(ejs, fun->numArgs);
}


/*
    function get name(): String
 */
static EjsString *fun_name(Ejs *ejs, EjsFunction *fun, int argc, EjsObj **argv)
{
    if (fun->name && fun->name->value[0] == '-') {
        return ESV(empty);
    }
    return fun->name;
}


/*
    function setScope(obj): Void
 */
static EjsObj *fun_setScope(Ejs *ejs, EjsFunction *fun, int argc, EjsObj **argv)
{
    EjsBlock    *scope;

    scope = (EjsBlock*) argv[0];
    if (!ejsIsBlock(ejs, scope)) {
        scope = (EjsBlock*) TYPE(scope);
        if (!ejsIsBlock(ejs, scope)) {
            ejsThrowArgError(ejs, "Scope object must be a class or function");
            return 0;
        }
    }
    fun->block.scope = scope;
    return 0;
}


/*************************************************************************************************************/

PUBLIC void ejsRemoveConstructor(Ejs *ejs, EjsType *type)
{
    EjsFunction     *fun;

    fun = (EjsFunction*) type;
    fun->block.pot.isFunction = 0;
    fun->isConstructor = 0;
    fun->isInitializer = 0;
    fun->activation = 0;
}


static void setFunctionAttributes(EjsFunction *fun, int attributes)
{
    if (attributes & EJS_FUN_CONSTRUCTOR) {
        fun->isConstructor = 1;
    }
    if (attributes & EJS_FUN_INITIALIZER) {
        fun->isInitializer = 1;
    }
    if (attributes & EJS_PROP_NATIVE) {
        fun->isNativeProc = 1;
    }
    if (attributes & EJS_FUN_MODULE_INITIALIZER) {
        fun->moduleInitializer = 1;
    }
    if (attributes & EJS_FUN_REST_ARGS) {
        fun->rest = 1;
    }
    if (attributes & EJS_PROP_STATIC) {
        fun->staticMethod = 1;
    }
    if (attributes & EJS_FUN_FULL_SCOPE) {
        fun->fullScope = 1;
    }
    if (attributes & EJS_FUN_HAS_RETURN) {
        fun->hasReturn = 1;
    }
    if (attributes & EJS_TRAIT_CAST_NULLS) {
        fun->castNulls = 1;
    }
    if (attributes & EJS_TRAIT_THROW_NULLS) {
        fun->throwNulls = 1;
    }
}


PUBLIC void ejsSetFunctionName(Ejs *ejs, EjsFunction *fun, EjsString *name)
{
    fun->name = name;
}


PUBLIC EjsEx *ejsAddException(Ejs *ejs, EjsFunction *fun, uint tryStart, uint tryEnd, EjsType *catchType, uint handlerStart,
        uint handlerEnd, int numBlocks, int numStack, int flags, int preferredIndex)
{
    EjsEx           *exception;
    EjsCode         *code;
    int             size;

    assure(fun);

    /* Managed by manageCode */
    if ((exception = mprAllocZeroed(sizeof(EjsEx))) == 0) {
        assure(0);
        return 0;
    }
    exception->flags = flags;
    exception->tryStart = tryStart;
    exception->tryEnd = tryEnd;
    exception->catchType = catchType;
    exception->handlerStart = handlerStart;
    exception->handlerEnd = handlerEnd;
    exception->numBlocks = numBlocks;
    exception->numStack = numStack;

    code = fun->body.code;
    if (preferredIndex < 0) {
        preferredIndex = code->numHandlers++;
    }
    if (preferredIndex >= code->sizeHandlers) {
        size = code->sizeHandlers + EJS_EX_INC;
        code->handlers = mprRealloc(code->handlers, size * sizeof(EjsEx));
        if (code->handlers == 0) {
            assure(0);
            return 0;
        }
        memset(&code->handlers[code->sizeHandlers], 0, EJS_EX_INC * sizeof(EjsEx)); 
        code->sizeHandlers = size;
    }
    code->handlers[preferredIndex] = exception;
    return exception;
}


#if UNUSED
PUBLIC void ejsOffsetExceptions(EjsFunction *fun, int offset)
{
    EjsEx           *ex;
    int             i;

    assure(fun);

    for (i = 0; i < fun->body.code->numHandlers; i++) {
        ex = fun->body.code->handlers[i];
        ex->tryStart += offset;
        ex->tryEnd += offset;
        ex->handlerStart += offset;
        ex->handlerEnd += offset;
    }
}
#endif


static void manageCode(EjsCode *code, int flags)
{
    int     i;

    assure(code->magic == EJS_CODE_MAGIC);
    if (flags & MPR_MANAGE_MARK) {
        assure(code->debug == 0 || code->debug->magic == EJS_DEBUG_MAGIC);        
        mprMark(code->module);
        mprMark(code->debug);
        if (code->handlers) {
            mprMark(code->handlers);
            for (i = 0; i < code->numHandlers; i++) {
                /* Manage EjsEx */
                mprMark(code->handlers[i]);
            }
        }
    }
}


PUBLIC EjsCode *ejsCreateCode(Ejs *ejs, EjsFunction *fun, EjsModule *module, cuchar *byteCode, ssize len, 
    EjsDebug *debug)
{
    EjsCode     *code;

    assure(fun);
    assure(module);
    assure(byteCode);
    assure(len >= 0);
    assure(debug == 0 || debug->magic == EJS_DEBUG_MAGIC);

    if ((code = mprAllocBlock(sizeof(EjsCode) + len, MPR_ALLOC_ZERO | MPR_ALLOC_MANAGER)) == 0) {
        return NULL;
    }
    mprSetManager(code, (MprManager) manageCode);
    code->codeLen = (int) len;
    code->module = module;
    code->debug = debug;
    code->magic = EJS_CODE_MAGIC;
    memcpy(code->byteCode, byteCode, len);
    return code;
}


/*
    Set the byte code for a script function
 */
PUBLIC int ejsSetFunctionCode(Ejs *ejs, EjsFunction *fun, EjsModule *module, cuchar *byteCode, ssize len, EjsDebug *debug)
{
    assure(fun);
    assure(byteCode);
    assure(len >= 0);
    assure(debug == 0 || debug->magic == EJS_DEBUG_MAGIC);

    fun->body.code = ejsCreateCode(ejs, fun, module, byteCode, len, debug);
    return 0;
}


static EjsObj *nopFunction(Ejs *ejs, EjsObj *obj, int argc, EjsObj **argv)
{
    return ESV(undefined);
}


#if UNUSED
PUBLIC void ejsUseActivation(Ejs *ejs, EjsFunction *fun)
{
    EjsPot  *activation;
    int     numProp;

    if ((activation = fun->activation) == 0) {
        return;
    }
    numProp = activation->numProp;
    if (numProp > 0) {
        ejsGrowPot(ejs, (EjsPot*) fun, numProp);
        ejsCopySlots(ejs, (EjsPot*) fun, 0, fun->activation, 0, numProp);
        fun->block.pot.numProp = numProp;
    }
}
#endif


PUBLIC EjsPot *ejsCreateActivation(Ejs *ejs, EjsFunction *fun, int numProp)
{
    EjsPot  *activation;

    activation = ejsCreatePot(ejs, ESV(Object), numProp);
    mprCopyName(activation, fun);
    return activation;
}

/********************************** Factory **********************************/

PUBLIC EjsFunction *ejsCreateBareFunction(Ejs *ejs, EjsString *name, int attributes)
{
    EjsFunction     *fun;

    if ((fun = ejsCreateObj(ejs, ESV(Function), 0)) == NULL) {
        return 0;
    }
    fun->name = name;
    fun->block.pot.isBlock = 1;
    fun->block.pot.isFunction = 1;
    mprCopyName(fun, fun->name);
    setFunctionAttributes(fun, attributes);
    return fun;
}


/*
    Create a script function. This defines the method traits. It does not create a  method slot. ResultType may
    be null to indicate untyped. NOTE: untyped functions may return a result at their descretion.
 */
PUBLIC EjsFunction *ejsCreateFunction(Ejs *ejs, EjsString *name, cuchar *byteCode, int codeLen, int numArgs, int numDefault, 
    int numExceptions, EjsType *resultType, int attributes, EjsModule *module, EjsBlock *scope, int strict)
{
    EjsFunction     *fun;

    if ((fun = ejsCreateBareFunction(ejs, name, attributes)) == 0) {
        return 0;
    }
    ejsInitFunction(ejs, fun, name, byteCode, codeLen, numArgs, numDefault, numExceptions, resultType, attributes, 
        module, scope, strict);
    return fun;
}


/*
    Init function to initialize constructors inside types
 */
PUBLIC int ejsInitFunction(Ejs *ejs, EjsFunction *fun, EjsString *name, cuchar *byteCode, int codeLen, int numArgs, 
    int numDefault, int numExceptions, EjsType *resultType, int attributes, EjsModule *module, EjsBlock *scope, int strict)
{
    if (scope) {
        fun->block.scope = scope;
    }
    fun->block.pot.isBlock = 1;
    fun->block.pot.isFunction = 1;
    fun->numArgs = numArgs;
    fun->numDefault = numDefault;
    fun->resultType = resultType;
    fun->strict = strict;

    if (codeLen > 0) {
        fun->body.code = ejsCreateCode(ejs, fun, module, byteCode, codeLen, NULL);
        fun->body.code->numHandlers = numExceptions;
    }
    fun->name = name;
    setFunctionAttributes(fun, attributes);
    return 0;
}


PUBLIC void ejsManageFunction(EjsFunction *fun, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        ejsManageBlock((EjsBlock*) fun, flags);
        mprMark(fun->activation);
        mprMark(fun->name);
        mprMark(fun->setter);
        mprMark(fun->archetype);
        if (!fun->isNativeProc) {
            mprMark(fun->body.code);
        }
        mprMark(fun->boundArgs);
        mprMark(fun->boundThis);
        mprMark(fun->resultType);
    }
}


PUBLIC void ejsCreateFunctionType(Ejs *ejs)
{
    EjsType         *type;
    EjsHelpers      *helpers;
    EjsFunction     *nop;

    type = ejsCreateCoreType(ejs, N("ejs", "Function"), sizeof(EjsFunction), S_Function, ES_Function_NUM_CLASS_PROP,
        ejsManageFunction, EJS_TYPE_POT | EJS_TYPE_IMMUTABLE_INSTANCES | EJS_TYPE_DYNAMIC_INSTANCES);
    helpers = &type->helpers;
    helpers->create = (EjsCreateHelper) createFunction;
    helpers->cast   = (EjsCastHelper) castFunction;
    helpers->clone  = (EjsCloneHelper) ejsCloneFunction;

    nop = ejsCreateFunction(ejs, ejsCreateStringFromAsc(ejs, "nop"), NULL, 0, -1, 0, 0, NULL, EJS_PROP_NATIVE, NULL, NULL,0);
    ejsAddImmutable(ejs, S_nop, EN("nop"), nop);
    nop->body.proc = (EjsProc) nopFunction;
    nop->isNativeProc = 1;
}


PUBLIC void ejsConfigureFunctionType(Ejs *ejs)
{
    EjsType     *type;
    EjsPot      *prototype;

    if ((type = ejsFinalizeCoreType(ejs, N("ejs", "Function"))) == 0) {
        return;
    }
    prototype = type->prototype;
    ejsBindConstructor(ejs, type, fun_Function);
    ejsBindMethod(ejs, prototype, ES_Function_apply, fun_applyFunction);
    ejsBindMethod(ejs, prototype, ES_Function_bind, fun_bindFunction);
    ejsBindMethod(ejs, prototype, ES_Function_bound, fun_bound);
    ejsBindMethod(ejs, prototype, ES_Function_call, fun_call);
    ejsBindMethod(ejs, prototype, ES_Function_length, fun_length);
    ejsBindMethod(ejs, prototype, ES_Function_name, fun_name);
    ejsBindMethod(ejs, prototype, ES_Function_setScope, fun_setScope);
}


/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
