/**
    ecParser. Parse ejscript source files.

    Parse source and create an internal abstract syntax tree of nodes representing the program.

    The Abstract Syntax Tree (AST) is comprised of a linked set of EcNodes. EjsNodes have a left and right pointer.
    Node with a list of children are represented by right hand links.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejsCompiler.h"

/********************************** Defines ***********************************/

#define peekToken(cp)   peekAheadToken(cp, 1)

/*
    State level macros. Enter/Leave manage state and inheritance of state.
 */
#undef ENTER
#define ENTER(a)        if (ecEnterState(a) < 0) { return 0; } else

#undef LEAVE
#define LEAVE(cp, np)   ecLeaveStateWithResult(cp, np)

/***************************** Forward Declarations ***************************/

static void     addAscToLiteral(EcCompiler *cp, EcNode *np, cchar *str, ssize len);
static void     addCharsToLiteral(EcCompiler *cp, EcNode *np, wchar *str, ssize len);
static void     addTokenToLiteral(EcCompiler *cp, EcNode *np);
static void     appendDocString(EcCompiler *cp, EcNode *np, EcNode *parameter, EcNode *value);
static EcNode   *appendNode(EcNode *top, EcNode *np);
static void     applyAttributes(EcCompiler *cp, EcNode *np, EcNode *attributes, EjsString *namespaceName);
static void     copyDocString(EcCompiler *cp, EcNode *np, EcNode *from);
static EcNode   *compileInput(EcCompiler *cp, cchar *path);
static EcNode   *createAssignNode(EcCompiler *cp, EcNode *lhs, EcNode *rhs, EcNode *parent);
static EcNode   *createBinaryNode(EcCompiler *cp, EcNode *lhs, EcNode *rhs, EcNode *parent);
static EcNode   *createNameNode(EcCompiler *cp, EjsName name);
static EcNode   *createNamespaceNode(EcCompiler *cp, EjsString *name, bool isDefault, bool isLiteral);
static EcNode   *createNode(EcCompiler *cp, int kind, EjsString *name);
static void     dummy(int junk);
static EcNode   *expected(EcCompiler *cp, cchar *str);
static int      getToken(EcCompiler *cp);
static EjsString *tokenString(EcCompiler *cp);
static EcNode   *insertNode(EcNode *top, EcNode *np, int pos);
static EcNode   *linkNode(EcNode *np, EcNode *node);
static EcNode   *parseAnnotatableDirective(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseArgumentList(EcCompiler *cp);
static EcNode   *parseArguments(EcCompiler *cp);
static EcNode   *parseArrayComprehension(EcCompiler *cp, EcNode *literalElement);
static EcNode   *parseArrayPattern(EcCompiler *cp);
static EcNode   *parseArrayType(EcCompiler *cp);
static EcNode   *parseAssignmentExpression(EcCompiler *cp);
static EcNode   *parseAttribute(EcCompiler *cp);
static EcNode   *parseAttributeName(EcCompiler *cp);
static EcNode   *parseBlock(EcCompiler *cp);
static EcNode   *parseBlockStatement(EcCompiler *cp);
static EcNode   *parseBrackets(EcCompiler *cp);
static EcNode   *parseBreakStatement(EcCompiler *cp);
static EcNode   *parseCaseElements(EcCompiler *cp);
static EcNode   *parseCaseLabel(EcCompiler *cp);
static EcNode   *parseCatchClause(EcCompiler *cp);
static EcNode   *parseCatchClauses(EcCompiler *cp);
static EcNode   *parseClassBody(EcCompiler *cp);
static EcNode   *parseClassDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseClassInheritance(EcCompiler *cp);
static EcNode   *parseClassName(EcCompiler *cp);
static EcNode   *parseComprehensionExpression(EcCompiler *cp, EcNode *literalElement);
static EcNode   *parseConstructorSignature(EcCompiler *cp, EcNode *np);
static EcNode   *parseConstructorInitializer(EcCompiler *cp);
static EcNode   *parseContinueStatement(EcCompiler *cp);
static EcNode   *parseDirective(EcCompiler *cp);
static EcNode   *parseDirectives(EcCompiler *cp);
static EcNode   *parseDoStatement(EcCompiler *cp);
static EcNode   *parseDirectivesPrefix(EcCompiler *cp);
static EcNode   *parseElementList(EcCompiler *cp, EcNode *newNode);
static EcNode   *parseElementListPattern(EcCompiler *cp);
static EcNode   *parseElements(EcCompiler *cp, EcNode *newNode);
static EcNode   *parseElementTypeList(EcCompiler *cp);
static EcNode   *parseFieldList(EcCompiler *cp, EcNode *np);
static EcNode   *parseEmptyStatement(EcCompiler *cp);
static EcNode   *parseError(EcCompiler *cp, cchar *fmt, ...);
static EcNode   *parseExpressionStatement(EcCompiler *cp);
static EcNode   *parseFieldListPattern(EcCompiler *cp);
static EcNode   *parseFieldPattern(EcCompiler *cp, EcNode *np);
static EcNode   *parseFieldName(EcCompiler *cp);
static EcNode   *parseForStatement(EcCompiler *cp);
static EcNode   *parseFunctionDeclaration(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseFunctionDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseFunctionBody(EcCompiler *cp, EcNode *fun);
static EcNode   *parseFunctionExpression(EcCompiler *cp);
static EcNode   *parseFunctionExpressionBody(EcCompiler *cp);
static EcNode   *parseFunctionName(EcCompiler *cp);
static EcNode   *parseFunctionSignature(EcCompiler *cp, EcNode *np);
static EcNode   *parseHashStatement(EcCompiler *cp);
static EcNode   *parseIdentifier(EcCompiler *cp);
static EcNode   *parseIfStatement(EcCompiler *cp);
static EcNode   *parseInterfaceBody(EcCompiler *cp);
static EcNode   *parseInterfaceInheritance(EcCompiler *cp);
static EcNode   *parseInitializerList(EcCompiler *cp, EcNode *np);
static EcNode   *parseInitializer(EcCompiler *cp);
static EcNode   *parseParameter(EcCompiler *cp, bool rest);
static EcNode   *parseParameterInit(EcCompiler *cp, EcNode *args);
static EcNode   *parseInterfaceDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseLabeledStatement(EcCompiler *cp);
static EcNode   *parseLeftHandSideExpression(EcCompiler *cp);
static EcNode   *parseLetBindingList(EcCompiler *cp);
static EcNode   *parseLetExpression(EcCompiler *cp);
static EcNode   *parseLetStatement(EcCompiler *cp);
static EcNode   *parseLiteralElement(EcCompiler *cp);
static EcNode   *parseLiteralField(EcCompiler *cp);
static EcNode   *parseListExpression(EcCompiler *cp);
static EcNode   *parseNamespaceAttribute(EcCompiler *cp);
static EcNode   *parseNamespaceDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseNamespaceInitialisation(EcCompiler *cp, EcNode *nameNode);
static EcNode   *parseNonemptyParameters(EcCompiler *cp, EcNode *list);
static EcNode   *parseNullableTypeExpression(EcCompiler *cp);
static EcNode   *parseObjectPattern(EcCompiler *cp);
static EcNode   *parseOptionalExpression(EcCompiler *cp);
static EcNode   *parseOverloadedOperator(EcCompiler *cp);
static EcNode   *parseParenListExpression(EcCompiler *cp);
static EcNode   *parseParameterisedTypeName(EcCompiler *cp);
static EcNode   *parseParameterKind(EcCompiler *cp);
static EcNode   *parseParameters(EcCompiler *cp, EcNode *args);
static EcNode   *parsePath(EcCompiler *cp, EcNode *lhs);
static EcNode   *parsePattern(EcCompiler *cp);
static EcNode   *parsePragmaItems(EcCompiler *cp, EcNode *np);
static EcNode   *parsePragmaItem(EcCompiler *cp);
static EcNode   *parsePragmas(EcCompiler *cp, EcNode *np);
static EcNode   *parsePrimaryExpression(EcCompiler *cp);
static EcNode   *parsePrimaryName(EcCompiler *cp);
static EcNode   *parseProgram(EcCompiler *cp, cchar *path);
static EcNode   *parsePropertyName(EcCompiler *cp);
static EcNode   *parsePropertyOperator(EcCompiler *cp);
static EcNode   *parseQualifiedNameIdentifier(EcCompiler *cp);
static EcNode   *parseRegularExpression(EcCompiler *cp);
static EcNode   *parseRequireItem(EcCompiler *cp);
static EcNode   *parseRequireItems(EcCompiler *cp, EcNode *np);
static EcNode   *parseReservedNamespace(EcCompiler *cp);
static EcNode   *parseRestArgument(EcCompiler *cp);
static EcNode   *parseRestParameter(EcCompiler *cp);
static EcNode   *parseResultType(EcCompiler *cp);
static EcNode   *parseReturnStatement(EcCompiler *cp);
static EcNode   *parseSimplePattern(EcCompiler *cp);
static EcNode   *parseSimpleQualifiedName(EcCompiler *cp);
static EcNode   *parseStatement(EcCompiler *cp);
static EcNode   *parseSubstatement(EcCompiler *cp);
static EcNode   *parseSuperInitializer(EcCompiler *cp);
static EcNode   *parseSwitchStatement(EcCompiler *cp);
static EcNode   *parseThrowStatement(EcCompiler *cp);
static EcNode   *parseTryStatement(EcCompiler *cp);
static EcNode   *parseTypeDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseTypeExpression(EcCompiler *cp);
static EcNode   *parseTypeIdentifierList(EcCompiler *cp);
static EcNode   *parseTypeInitialisation(EcCompiler *cp);
static EcNode   *parseModuleBody(EcCompiler *cp);
static EcNode   *parseModuleName(EcCompiler *cp);
static EcNode   *parseModuleDefinition(EcCompiler *cp);
static EcNode   *parseUsePragma(EcCompiler *cp, EcNode *np);
static EcNode   *parseVariableBinding(EcCompiler *cp, EcNode *np, EcNode *attributes);
static EcNode   *parseVariableBindingList(EcCompiler *cp, EcNode *list, EcNode *attributes);
static EcNode   *parseVariableDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseVariableDefinitionKind(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseVariableInitialisation(EcCompiler *cp);
static int       parseVersion(EcCompiler *cp, int parseMax);
static EcNode   *parseWhileStatement(EcCompiler *cp);
static EcNode   *parseWithStatement(EcCompiler *cp);
PUBLIC struct EcNode   *parseXMLAttribute(EcCompiler *cp, EcNode *np);
PUBLIC struct EcNode   *parseXMLAttributes(EcCompiler *cp, EcNode *np);
PUBLIC struct EcNode   *parseXMLElement(EcCompiler *cp, EcNode *np);
PUBLIC struct EcNode   *parseXMLElementContent(EcCompiler *cp, EcNode *np);
PUBLIC struct EcNode   *parseXMLTagContent(EcCompiler *cp, EcNode *np);
PUBLIC struct EcNode   *parseXMLTagName(EcCompiler *cp, EcNode *np);
static EcNode   *parseYieldExpression(EcCompiler *cp);
static int      peekAheadToken(EcCompiler *cp, int ahead);
static EcToken  *peekAheadTokenStruct(EcCompiler *cp, int ahead);
static void     putSpecificToken(EcCompiler *cp, EcToken *token);
static void     putToken(EcCompiler *cp);
static EcNode   *removeNode(EcNode *np, EcNode *child);
static void     setNodeDoc(EcCompiler *cp, EcNode *np);
static EcNode   *unexpected(EcCompiler *cp);

#if BIT_DEBUG
static void     updateDebug(EcCompiler *cp);
#else
#define         updateDebug(cp)
#endif

#if BIT_DEBUG
/*
    Just for debugging. Generated via tokens.ksh
 */
char *tokenNames[] = {
    "",
    "assign",
    "at",
    "attribute",
    "bit_and",
    "bit_and_assign",
    "bit_or",
    "bit_or_assign",
    "bit_xor",
    "bit_xor_assign",
    "break",
    "call",
    "callee",
    "case",
    "cast",
    "catch",
    "cdata_end",
    "cdata_start",
    "class",
    "colon",
    "colon_colon",
    "comma",
    "const",
    "context_reserved_id",
    "continue",
    "debugger",
    "decrement",
    "default",
    "delete",
    "div",
    "div_assign",
    "do",
    "dot",
    "dot_dot",
    "dot_less",
    "double",
    "dynamic",
    "each",
    "elipsis",
    "else",
    "enumerable",
    "eof",
    "eq",
    "err",
    "extends",
    "false",
    "final",
    "finally",
    "float",
    "for",
    "function",
    "ge",
    "generator",
    "get",
    "goto",
    "gt",
    "has",
    "hash",
    "id",
    "if",
    "implements",
    "in",
    "include",
    "increment",
    "instanceof",
    "int",
    "interface",
    "internal",
    "intrinsic",
    "is",
    "lbrace",
    "lbracket",
    "le",
    "let",
    "logical_and",
    "logical_and_assign",
    "logical_not",
    "logical_or",
    "logical_or_assign",
    "logical_xor",
    "logical_xor_assign",
    "lparen",
    "lsh",
    "lsh_assign",
    "lt",
    "lt_slash",
    "minus",
    "minus_assign",
    "minus_minus",
    "mod",
    "module",
    "mod_assign",
    "mul",
    "mul_assign",
    "namespace",
    "native",
    "ne",
    "new",
    "nop",
    "null",
    "number",
    "number_word",
    "override",
    "plus",
    "plus_assign",
    "plus_plus",
    "private",
    "protected",
    "prototype",
    "public",
    "query",
    "rbrace",
    "rbracket",
    "regexp",
    "require",
    "reserved_namespace",
    "return",
    "rparen",
    "rsh",
    "rsh_assign",
    "rsh_zero",
    "rsh_zero_assign",
    "semicolon",
    "set",
    "slash_gt",
    "standard",
    "static",
    "strict",
    "strict_eq",
    "strict_ne",
    "string",
    "super",
    "switch",
    "this",
    "throw",
    "tilde",
    "to",
    "true",
    "try",
    "type",
    "typeof",
    "uint",
    "undefined",
    "use",
    "var",
    "void",
    "while",
    "with",
    "xml_comment_end",
    "xml_comment_start",
    "xml_pi_end",
    "xml_pi_start",
    "yield",
    0,
};


/*
    Just for debugging. Generated via tokens.ksh
 */
char *nodes[] = {
    "",
    "n_args",
    "n_assign_op",
    "n_attributes",
    "n_binary_op",
    "n_binary_type_op",
    "n_block",
    "n_break",
    "n_call",
    "n_case_elements",
    "n_case_label",
    "n_catch",
    "n_catch_arg",
    "n_catch_clauses",
    "n_class",
    "n_continue",
    "n_dassign",
    "n_directives",
    "n_do",
    "n_dot",
    "n_end_function",
    "n_expressions",
    "n_field",
    "n_for",
    "n_for_in",
    "n_function",
    "n_goto",
    "n_hash",
    "n_if",
    "n_literal",
    "n_module",
    "n_new",
    "n_nop",
    "n_object_literal",
    "n_parameter",
    "n_postfix_op",
    "n_pragma",
    "n_pragmas",
    "n_program",
    "n_qname",
    "n_ref",
    "n_return",
    "n_spread",
    "n_super",
    "n_switch",
    "n_this",
    "n_throw",
    "n_try",
    "n_type_identifiers",
    "n_unary_op",
    "n_use_module",
    "n_use_namespace",
    "n_value",
    "n_var",
    "n_var_definition",
    "n_void",
    "n_with",
    0,
};

#endif  /* BIT_DEBUG */

/************************************ Code ************************************/
/*
    Compile the input stream and parse all directives into the given nodes reference.
 */
static EcNode *compileInput(EcCompiler *cp, cchar *path)
{
    EcNode      *np;

    assure(cp);

    if (ecEnterState(cp) < 0) {
        return 0;
    }
    cp->fileState = cp->state;
    cp->fileState->strict = cp->strict;
    cp->blockState = cp->state;

    if (cp->shbang) {
        if (getToken(cp) == T_HASH && peekToken(cp) == T_LOGICAL_NOT) {
            while (cp->token->loc.lineNumber <= 1 && cp->token->tokenId != T_EOF && cp->token->tokenId != T_NOP) {
                getToken(cp);
            }
        }
        putToken(cp);
    }
    np = parseProgram(cp, path);
    assure(np || cp->error);
    np = ecLeaveStateWithResult(cp, np);
    cp->fileState = 0;

    if (cp->errorCount > 0) {
        return 0;
    }
    return np;
}


/*
    Compile a source file and parse all directives into the given nodes reference.
    This may be called with the input stream already setup to parse a script.
 */
PUBLIC EcNode *ecParseFile(EcCompiler *cp, char *path)
{
    EcNode  *np;
    int     opened;

    assure(path);

    opened = 0;
    path = mprNormalizePath(path);
    if (cp->stream == 0) {
        if (ecOpenFileStream(cp, path) < 0) {
            parseError(cp, "Cannot open %s", path);
            return 0;
        }
        opened = 1;
    }
    np = compileInput(cp, path);
    if (opened) {
        ecCloseStream(cp);
    }
    return np;
}


/*
    Lookup a module of the right version
    If max is <= 0, then accept any version from min upwards.
    This allows the caller to provide -1, -1 to match all versions.
    If both are equal, then only that version is acceptable.
 */
PUBLIC EjsModule *ecLookupModule(EcCompiler *cp, EjsString *name, int minVersion, int maxVersion)
{
    EjsModule   *mp, *best;
    int         next;

    if (maxVersion <= 0) {
        maxVersion = MAXINT;
    }
    best = 0;
    for (next = 0; (mp = (EjsModule*) mprGetNextItem(cp->modules, &next)) != 0; ) {
        if (minVersion <= mp->version && mp->version <= maxVersion) {
            if (ejsCompareString(cp->ejs, mp->name, name) == 0) {
                if (best == 0 || best->version < mp->version) {
                    best = mp;
                }
            }
        }
    }
    return best;
}


PUBLIC int ecAddModule(EcCompiler *cp, EjsModule *mp)
{
    assure(cp->modules);
    return mprAddItem(cp->modules, mp);
}


PUBLIC int ecRemoveModule(EcCompiler *cp, EjsModule *mp)
{
    assure(cp->modules);
    return mprRemoveItem(cp->modules, mp);
}


PUBLIC int ecResetModuleList(EcCompiler *cp)
{
    cp->modules = mprCreateList(-1, 0);
    if (cp->modules == 0) {
        return EJS_ERR;
    }
    return 0;
}


PUBLIC void ecResetParser(EcCompiler *cp)
{
    cp->token = 0;
}


/************************************* Parser Productions *************************/
/*
    XMLComment (ECMA-357)

    Input Sequences
        <!-- XMLCommentCharacters -->

    AST
 */
static EcNode *parseXMLComment(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);

    np = 0;
    if (getToken(cp) == T_XML_COMMENT_START) { }
    if (getToken(cp) != T_XML_COMMENT_END) {
        return expected(cp, "-->");
    }
    return LEAVE(cp, np);
}


/*
    XMLCdata (ECMA-357)

    Input Sequences
        <![CDATA[ XMLCDataCharacters ]]>

    AST
 */
static EcNode *parseXMLCdata(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = 0;
    if (getToken(cp) == T_CDATA_START) { }
    if (getToken(cp) != T_CDATA_END) {
        return expected(cp, "]]>");
    }
    return LEAVE(cp, np);
}


/*
    XMLPI (ECMA-357)

    Input Sequences
        <? .... ?>

    AST
 */
static EcNode *parseXMLPI(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = 0;
    if (getToken(cp) == T_XML_PI_START) { }
    if (getToken(cp) != T_XML_PI_END) {
        return expected(cp, "?>");
    }
    return LEAVE(cp, np);
}


/*
    XMLMarkup (ECMA-357)
        XMLComment
        XMLCDATA
        XMLPI

    Input Sequences
        <!--
        [CDATA
        <?

    AST
 */
static EcNode *parseXMLMarkup(EcCompiler *cp, EcNode *np)
{
    switch (peekToken(cp)) {
    case T_XML_COMMENT_START:
        return parseXMLComment(cp);

    case T_CDATA_START:
        return parseXMLCdata(cp);

    case T_XML_PI_START:
        return parseXMLPI(cp);

    default:
        getToken(cp);
        return LEAVE(cp, unexpected(cp));
    }
}


/*
    XMLText (ECMA-357)

    Input Sequences
        SourceCharacters but no { or <

    AST
 */
static EcNode *parseXMLText(EcCompiler *cp, EcNode *np)
{
    wchar   *p;
    int     count;

    //  TODO This is discarding text white space. Need a low level getXmlToken routine
    //  or need lexer to preserved inter-token white space somewhere.
  
    peekToken(cp);
    for (count = 0; np; count++) {
        for (p = cp->peekToken->text; p && *p; p++) {
            if (*p == '{' || *p == '<') {
                if (cp->peekToken->text < p) {
                    addCharsToLiteral(cp, np, cp->token->text, (p - cp->token->text));
                    if (getToken(cp) == T_EOF || cp->token->tokenId == T_ERR || cp->token->tokenId == T_NOP) {
                        return 0;
                    }
                }
                return np;
            }
        }
        if (getToken(cp) == T_EOF || cp->token->tokenId == T_ERR || cp->token->tokenId == T_NOP) {
            return 0;
        }
        if (isalnum((uchar) cp->token->text[0]) && count > 0) {
            addAscToLiteral(cp, np, " ", 1);
        }
        addTokenToLiteral(cp, np);
        peekToken(cp);
    }
    return np;
}


/*
    XMLName (ECMA-357)
        XMLNameStart
        XMLName XMLNamePart

    Input Sequences
        UnicodeLetter
        _       underscore
        :       colon

    AST
 */
static EcNode *parseXMLName(EcCompiler *cp, EcNode *np)
{
    int         c;

    ENTER(cp);

    getToken(cp);
    if (cp->token == 0 || cp->token->text == 0) {
        return LEAVE(cp, unexpected(cp));
    }
    c = cp->token->text[0];
    if (isalpha((uchar) c) || c == '_' || c == ':') {
        addTokenToLiteral(cp, np);
    } else {
        np = parseError(cp, "Not an XML Name \"%@\"", cp->token->text);
    }
    return LEAVE(cp, np);
}


/*
    XMLAttributeValue (ECMA-357)
        XMLDoubleStringCharacters
        XMLSingleStringCharacters

    Input Sequences
        "
        '
    AST
        Add data to literal.data buffer
 */
static EcNode *parseXMLAttributeValue(EcCompiler *cp, EcNode *np)
{
    //  TODO - should be preserving whether the input was ' or ""
    if (getToken(cp) != T_STRING) {
        return expected(cp, "quoted string");
    }
    addAscToLiteral(cp, np, "\"", 1);
    addTokenToLiteral(cp, np);
    addAscToLiteral(cp, np, "\"", 1);
    return np;
}


/*
    Identifier (1)
        ID |
        ContextuallyReservedIdentifier

    Input Sequences
        ID
        ContextuallyReservedIdentifier

    AST
        N_QNAME
            name
                id
 */
static EcNode *parseIdentifier(EcCompiler *cp)
{
    EcNode      *np;
    int         tid;

    ENTER(cp);

    tid = getToken(cp);
    if (cp->token->groupMask & G_CONREV) {
        tid = T_ID;
    }
    switch (tid) {
    case T_MUL:
    case T_ID:
        np = createNode(cp, N_QNAME, tokenString(cp));
        break;

    default:
        np = parseError(cp, "Not an identifier \"%s\"", cp->token->text);
    }
    return LEAVE(cp, np);
}


/*
    Qualifier (3)
        *
        Identifier
        ReservedNamespace
        "StringLiteral"

    Input Sequences:
        *
        ID

    AST
        N_ATTRIBUTES
            namespace
 */

static EcNode *parseQualifier(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_ID:
        np = parseIdentifier(cp);
        break;

    case T_STRING:
        getToken(cp);
        np = createNode(cp, N_QNAME, NULL);
        np->qname.space = tokenString(cp);
        np->literalNamespace = 1;
        break;

    case T_MUL:
        getToken(cp);
        np = createNode(cp, N_ATTRIBUTES, NULL);
        np->qname.space = tokenString(cp);
        break;

    case T_RESERVED_NAMESPACE:
        np = parseReservedNamespace(cp);
        break;

    default:
        getToken(cp);
        np = unexpected(cp);
    }
    return LEAVE(cp, np);
}


/*
    ReservedNamespace (6)
        internal
        intrinsic
        private
        protected
        public

    Input
        internal
        intrinsic
        private
        protected
        public

    AST
        N_ATTRIBUTES
            namespace
            attributes
 */
static EcNode *parseReservedNamespace(EcCompiler *cp)
{
    EcNode      *np;
    int         attributes;

    ENTER(cp);

    if (getToken(cp) != T_RESERVED_NAMESPACE) {
        return LEAVE(cp, expected(cp, "reserved namespace"));
    }
    np = createNode(cp, N_ATTRIBUTES, NULL);

    attributes = 0;

    switch (cp->token->subId) {
    case T_INTRINSIC:
        break;

    case T_INTERNAL:
    case T_PRIVATE:
    case T_PROTECTED:
    case T_PUBLIC:
        np->specialNamespace = 1;
        break;

    default:
        return LEAVE(cp, parseError(cp, "Unknown reserved namespace %s", cp->token->text));
    }
    np->attributes = attributes;
    np->qname.space = tokenString(cp);
    return LEAVE(cp, np);
}


/*
    QualifiedNameIdentifier (11)
        Identifier
        ReservedIdentifier
        StringLiteral
        NumberLiteral
        Brackets
        OverloadedOperator

    Notes:
        Can be used to the right of a namespace qualifier. Eg. public::QualfiedNameIdentifier

    Input
        Identifier
        ReservedIdentifier
        Number
        "String"
        [
        Overloaded Operator

    AST
        N_QNAME
            name:
                namespace
                id
            left: N_EXPRESSIONS
 */
static EcNode *parseQualifiedNameIdentifier(EcCompiler *cp)
{
    EcNode      *np;
    EjsObj      *vp;
    int         tid, reservedWord;

    ENTER(cp);

    tid = peekToken(cp);
    reservedWord = (cp->peekToken->groupMask & G_RESERVED);

    if (reservedWord) {
        np = createNode(cp, N_QNAME, tokenString(cp));

    } else switch (tid) {
        case T_ID:
        case T_TYPE:
            np = parseIdentifier(cp);
            break;

        case T_NUMBER:
            getToken(cp);
            np = createNode(cp, N_QNAME, NULL);
            vp = ejsParse(cp->ejs, cp->token->text, -1);
            //  cant set literal.var in a QNAME. clashes with "name" union structure. Not marked.
            assure(0);
            np->literal.var = vp;
            break;

        case T_STRING:
            getToken(cp);
            np = createNode(cp, N_QNAME, NULL);
            vp = (EjsObj*) tokenString(cp);
            //  cant set literal.var in a QNAME. clashes with "name" union structure. Not marked.
            np->literal.var = vp;
            break;

        case T_LBRACKET:
            np = parseBrackets(cp);
            break;

        default:
            if (cp->token->groupMask == G_OPERATOR) {
                np = parseOverloadedOperator(cp);
            } else {
                getToken(cp);
                np = unexpected(cp);
            }
            break;
    }
    return LEAVE(cp, np);
}


/*
    SimpleQualifiedName (17)
        Identifier
        Qualifier :: QualifiedNameIdentifier

    Notes:
        Optionally namespace qualified name

    Input
        Identifier
        *

    AST
        N_QNAME
            name
                id
            qualifier: N_ATTRIBUTES
 */
static EcNode *parseSimpleQualifiedName(EcCompiler *cp)
{
    EcNode      *np, *name, *qualifier;

    ENTER(cp);

    if (peekToken(cp) == T_MUL || cp->peekToken->tokenId == T_STRING) {
        if (peekAheadToken(cp, 2) == T_COLON_COLON) {
            qualifier = parseQualifier(cp);
            getToken(cp);
            np = parseQualifiedNameIdentifier(cp);
            if (np->kind == N_EXPRESSIONS) {
                name = np;
                np = createNode(cp, N_QNAME, NULL);
                np->name.nameExpr = linkNode(np, name);
            } else {
                np->literalNamespace = 1;
            }
            np->qname.space = qualifier->qname.space;

        } else {
            np = parseIdentifier(cp);
        }

    } else {
        np = parseIdentifier(cp);
        if (peekToken(cp) == T_COLON_COLON) {
            getToken(cp);
            qualifier = np;
            np = parseQualifiedNameIdentifier(cp);
            if (np) {
                if (np->kind == N_EXPRESSIONS) {
                    name = np;
                    np = createNode(cp, N_QNAME, NULL);
                    np->name.nameExpr = linkNode(np, name);
                }
                np->name.qualifierExpr = linkNode(np, qualifier);
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    ExpressionQualifiedName (15)
        ParenListExpression :: QualifiedNameIdentifier

    Input
        ( ListExpression ) :: *
        ( ListExpression ) :: Identifier
        ( ListExpression ) :: ReservedIdentifier
        ( ListExpression ) :: Number
        ( ListExpression ) :: String
        ( ListExpression ) :: [ ... ]
        ( ListExpression ) :: OverloadedOperator

    AST
        N_QNAME
            left: N_EXPRESSIONS
            qualifier: N_ATTRIBUTES
 */
static EcNode *parseExpressionQualifiedName(EcCompiler *cp)
{
    EcNode      *np, *qualifier;

    ENTER(cp);

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, unexpected(cp));
    }
    qualifier = parseListExpression(cp);
    if (getToken(cp) != T_RPAREN) {
        return LEAVE(cp, expected(cp, ")"));
    }
    if (getToken(cp) == T_COLON_COLON) {
        np = parseQualifiedNameIdentifier(cp);
        np->name.qualifierExpr = linkNode(np, qualifier);
    } else {
        np = expected(cp, "\"::\"");
    }
    return LEAVE(cp, np);
}


/*
    PropertyName (20)
        SimpleQualifiedName         |
        ExpressionQualifiedName

    Input
        Identifier
        *
        internal, intrinsic, private, protected, public
        (

    AST
        N_QNAME
            name
                namespace
                id
            left: N_EXPRESSIONS
            right: N_EXPRESSIONS
 */
static EcNode *parsePropertyName(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) == T_LPAREN) {
        np = parseExpressionQualifiedName(cp);
    } else {
        np = parseSimpleQualifiedName(cp);
    }
    return LEAVE(cp, np);
}


/*
    AttributeName (22)
        @ Brackets
        @ PropertyName

    Input
        @ [ ... ]
        @ PropertyName
            @ *
            @ ID
            @ Qualifier :: *
            @ Qualifier :: ID
            @ Qualifier :: ReservedIdentifier
            @ Qualifier :: Brackets
            @ ( ListExpression ) :: *
            @ ( ListExpression ) :: ID
            @ ( ListExpression ) :: ReservedIdentifier
            @ ( ListExpression ) :: [ ... ]

    AST
        N_QNAME
            name
                id
                namespace
                isAttribute
            left: N_EXPRESSIONS
 */
static EcNode *parseAttributeName(EcCompiler *cp)
{
    EcNode      *np;
    char        *attribute;

    ENTER(cp);

    if (getToken(cp) != T_AT) {
        return LEAVE(cp, expected(cp, "@ prefix"));
    }
    if (peekToken(cp) == T_LBRACKET) {
        np = createNode(cp, N_QNAME, tokenString(cp));
        np = appendNode(np, parseBrackets(cp));
    } else {
        np = parsePropertyName(cp);
    }
    if (np && np->kind == N_QNAME) {
        //  OPT. Better to allow lexer to keep @ in the id name and return T_AT with the entire attribute name.
        np->name.isAttribute = 1;
        attribute = sjoin("@", np->qname.name->value, NULL);
        np->qname.name = ejsCreateStringFromAsc(cp->ejs, attribute);
    }
    return LEAVE(cp, np);
}


/*
    QualifiedName (24)
        AttributeName
        PropertyName

    Input
        @ ...
        Identifier
        *
        internal, intrinsic, private, protected, public
        (

    AST
        N_QNAME
            name
                id
                namespace
                isAttribute
            left: listExpression
            right: bracketExpression
 */
static EcNode *parseQualifiedName(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) == T_AT) {
        np = parseAttributeName(cp);
    } else {
        np = parsePropertyName(cp);
    }
    return LEAVE(cp, np);
}


/*
    PrimaryName (26)
        Path . PropertyName
        PropertyName

    Input
        *
        internal, intrinsic, private, protected, public
        Identifier
        (

    AST
        N_QNAME
        N_DOT
 */
static EcNode *parsePrimaryName(EcCompiler *cp)
{
    EcNode      *np, *path;
    int         tid, id;

    ENTER(cp);

    np = 0;
    tid = peekToken(cp);
    if (cp->peekToken->groupMask & G_CONREV) {
        tid = T_ID;
    }
    if (tid == T_ID && peekAheadToken(cp, 2) == T_DOT) {
        EcToken     *tok;
        tok = peekAheadTokenStruct(cp, 3);
        id = tok->tokenId;
        if (tok->groupMask & G_CONREV) {
            id = T_ID;
        }
        if (id == T_ID || id == T_MUL || id == T_RESERVED_NAMESPACE || id == T_LPAREN) {
            path = parsePath(cp, 0);
            np = createNode(cp, N_DOT, NULL);
            np = appendNode(np, path);
            getToken(cp);
        }
    }
    if (np) {
        np = appendNode(np, parsePropertyName(cp));
    } else {
        np = parsePropertyName(cp);
    }
    return LEAVE(cp, np);
}


/*
    Path (28)
        Identifier |
        Path . Identifier

    Input
        ID
        ID. ... .ID

    AST
        N_QNAME
        N_DOT

    "dontConsumeLast" will be set if parsePath should not consume the last Identifier.
 */
static EcNode *parsePath(EcCompiler *cp, EcNode *lhs)
{
    EcNode      *np;
    EcToken     *tok;
    int         tid;

    ENTER(cp);

    if (lhs) {
        np = appendNode(createNode(cp, N_DOT, NULL), lhs);
        np = appendNode(np,  parseIdentifier(cp));
    } else {
        np = parseIdentifier(cp);
    }

    /*
        parsePath is called only from parsePrimaryName which requires that a ".PropertyName" be preserved.
        TODO - OPT. Perhaps hoist back into parsePrimaryName.
     */
    if (peekToken(cp) == T_DOT && peekAheadToken(cp, 2) == T_ID) {
        if (peekAheadToken(cp, 3) == T_DOT) {
            tok = peekAheadTokenStruct(cp, 4);
            tid = tok->tokenId;
            if (tok->groupMask & G_CONREV) {
                tid = T_ID;
            }
            if (tid == T_ID || tid == T_MUL || tid == T_RESERVED_NAMESPACE || tid == T_LPAREN) {
                getToken(cp);
                np = parsePath(cp, np);
            }
        }
    }
    return LEAVE(cp, np);
}


#if UNUSED
/*
    ParenExpression (30)
        ( AssignmentExpression )
        ( )                                 # EJS FIX

    Input
        (

    AST
        N_EXPR
        N_NOP
 */
static EcNode *parseParenExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, unexpected(cp));
    }
    if (peekToken(cp) != T_RPAREN) {
        np = parseAssignmentExpression(cp);
    } else {
        np = createNode(cp, N_NOP, NULL);
    }
    if (getToken(cp) != T_RPAREN) {
        np = expected(cp, ")");
    }
    return LEAVE(cp, np);
}
#endif


/*
    ParenListExpression (31)
        ( ListExpression )

    Input
        (

    AST
        N_EXPRESSIONS
 */
static EcNode *parseParenListExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, expected(cp, "("));
    }
    np = parseListExpression(cp);
    if (np && getToken(cp) != T_RPAREN) {
        np = expected(cp, ")");
    }
    return LEAVE(cp, np);
}


/*
    FunctionExpression (32)
        function Identifier FunctionSignature FunctionBody
        function FunctionSignature FunctionBody

    Input
        function id ( args ) { body }
        function ( args ) { body }

    AST
        N_FUNCTION
 */
static EcNode *parseFunctionExpression(EcCompiler *cp)
{
    Ejs         *ejs;
    EcState     *state;
    EcNode      *np, *funRef;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;

    if (getToken(cp) != T_FUNCTION) {
        return LEAVE(cp, unexpected(cp));
    }
    np = createNode(cp, N_FUNCTION, NULL);
    np->function.isExpression = 1;

    if (peekToken(cp) == T_ID) {
        getToken(cp);
        np->qname.name = tokenString(cp);
    }
    if (np->qname.name == 0) {
        np->qname.name = ejsSprintf(cp->ejs, "--fun_%d-%d--", np->seqno, (int) mprGetTime());
    }
    np->qname.space = state->inFunction ? ESV(empty): cp->fileState->nspace;

    np = parseFunctionSignature(cp, np);
    if (np == 0) {
        return LEAVE(cp, np);
    }
    if (STRICT_MODE(cp)) {
        if (np->function.resultType == 0) {
            return LEAVE(cp, parseError(cp,
                "Function has not defined a return type. Fuctions must be typed when declared in strict mode"));
        }
    }
    cp->state->currentFunctionNode = np;
    np->function.body = linkNode(np, parseFunctionExpressionBody(cp));

    if (np->function.body == 0) {
        return LEAVE(cp, 0);
    }
    /*
        The function must get linked into the top var block. It must not get processed inline at this point in the AST tree.
     */
    assure(cp->state->topVarBlockNode);
    appendNode(cp->state->topVarBlockNode, np);

    /*
        Create a name node to reference the function. This is the value of this function expression.
        The funRef->name will be filled in by the AST processing for the function node.
     */
    funRef = createNode(cp, N_QNAME, NULL);
    funRef->qname = np->qname;
    return LEAVE(cp, funRef);
}


/*
    FunctionExpressionBody (34)
        Block
        AssignmentExpression

    Input
        {
    AST
 */
static EcNode *parseFunctionExpressionBody(EcCompiler *cp)
{
    EcNode      *np, *ret;

    ENTER(cp);

    if (peekToken(cp) == T_LBRACE) {
        np = parseBlock(cp);
        if (np) {
            np = np->left;
        }
    } else {
        np = createNode(cp, N_DIRECTIVES, NULL);
        ret = createNode(cp, N_RETURN, NULL);
        ret->ret.blockless = 1;
        ret = appendNode(ret, parseAssignmentExpression(cp));
        np = appendNode(np, ret);
    }
    if (np) {
        assure(np->kind == N_DIRECTIVES);
    }
    np = appendNode(np, createNode(cp, N_END_FUNCTION, NULL));
    return LEAVE(cp, np);
}


/*
    ObjectLiteral (36)
        { FieldList }
        { FieldList } : NullableTypeExpression

    Input
        { LiteralField , ... }

    AST
        N_EXPRESSIONS
 */
static EcNode *parseObjectLiteral(EcCompiler *cp)
{
    Ejs     *ejs;
    EcNode  *typeNode, *np;

    ENTER(cp);

    ejs = cp->ejs;
    np = createNode(cp, N_OBJECT_LITERAL, NULL);
    if (getToken(cp) != T_LBRACE) {
        return LEAVE(cp, unexpected(cp));
    }
    if ((np = parseFieldList(cp, np)) == 0) {
        return LEAVE(cp, 0);
    }
    if (peekToken(cp) == T_COLON) {
        getToken(cp);
        typeNode = parseNullableTypeExpression(cp);
    } else {
        typeNode = createNode(cp, N_QNAME, EST(Object)->qname.name);
    }
    np->objectLiteral.typeNode = linkNode(np, typeNode);
    np->objectLiteral.isArray = 0;

    if (getToken(cp) != T_RBRACE) {
        return LEAVE(cp, unexpected(cp));
    }
    return LEAVE(cp, np);
}


/*
    FieldList (41)
        EMPTY
        LiteralField
        LiteralField , LiteralField

    Input
        LiteralField , ...

    AST
 */
static EcNode *parseFieldList(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);
    while (peekToken(cp) != T_RBRACE && !cp->error) {
        if (peekToken(cp) != T_COMMA) {
            np = appendNode(np, parseLiteralField(cp));
        } else {
            getToken(cp);
        }
    }
    return LEAVE(cp, np);
}


/*
    LiteralField (42)
        FieldKind FieldName : AssignmentExpression
        get Identifier FunctionSignature FunctionBody
        set Identifier FunctionSignature FunctionBody

    Input

    AST
 */
static EcNode *parseLiteralField(EcCompiler *cp)
{
    EcNode  *fp, *np, *id, *funRef, *fieldName;

    ENTER(cp);
    np = 0;
    
    getToken(cp);
    if ((cp->token->tokenId == T_GET || cp->token->tokenId == T_SET) && peekToken(cp) != T_COLON) {
        /*
            get NAME() {}
         */
        fp = createNode(cp, N_FUNCTION, NULL);
        if (cp->token->tokenId == T_GET) {
            fp->function.getter = 1;
            fp->attributes |= EJS_TRAIT_GETTER;
        } else {
            fp->function.setter = 1;
            fp->attributes |= EJS_TRAIT_SETTER;
        }
        id = parseIdentifier(cp);
        if (id == 0) {
            return LEAVE(cp, 0);
        }
        cp->state->currentFunctionNode = fp;
        fp = parseFunctionSignature(cp, fp);
        fp->function.body = linkNode(fp, parseFunctionBody(cp, fp));
        if (fp->function.body == 0) {
            return LEAVE(cp, 0);
        }

        np = createNode(cp, N_FIELD, NULL);
        np->field.fieldKind = FIELD_KIND_FUNCTION;
        np->field.index = -1;
        np->attributes = fp->attributes;
        /*
            The function must get linked into the current var block. It must not get processed inline at
            this point in the AST tree because it must not use the block scope. Create a name based on the
            object literal seqno. This permits setters and getters to share the same name and thus when
            ejsDefineProperty is called -- they will get cross-linked.
         */
        fp->qname.name = ejsSprintf(cp->ejs, "--fun_%d-%d--", fp->seqno, (int) mprGetTime());
        fp->qname.space = cp->fileState->nspace;
        assure(cp->state->topVarBlockNode);
        appendNode(cp->state->topVarBlockNode, fp);
        /*
            Must clear the getter|setter attributes so it can be loaded without invoking the accessor.
            The NEW_OBJECT opcode will call ejsDefineProperty which will restore the attributes.
         */
        fp->attributes &= ~(EJS_TRAIT_GETTER | EJS_TRAIT_SETTER);

        fieldName = createNode(cp, N_QNAME, id->qname.name);
        np->field.fieldName = linkNode(np, fieldName);

        funRef = createNode(cp, N_QNAME, fp->qname.name);
        np->field.expr = linkNode(np, funRef);

    } else {
        if (cp->token->tokenId == T_CONST) {
            np = createNode(cp, N_FIELD, NULL);
            np->field.varKind = KIND_CONST;
            np->attributes |= EJS_TRAIT_READONLY;
        } else {
            putToken(cp);
            np = createNode(cp, N_FIELD, NULL);
        }
        np->field.index = -1;
        np->field.fieldKind = FIELD_KIND_VALUE;
        if ((np->field.fieldName = linkNode(np, parseFieldName(cp))) == 0) {
            np = 0;
        } else {
            if (peekToken(cp) == T_COLON) {
                getToken(cp);
                np->field.expr = linkNode(np, parseAssignmentExpression(cp));
                
            } else if (np->field.fieldName->kind == N_QNAME) {
                np->field.expr = linkNode(np, createNode(cp, N_QNAME, NULL));
                np->field.expr->qname.name = np->field.fieldName->qname.name;

            } else if (np->field.fieldName->kind != N_LITERAL) {
                np = expected(cp, ": value");
            }
        }
    }
    return LEAVE(cp, np);
}


#if ROLLED_UP
/*
    FieldKind (45)
        EMPTY
        const

    Input

    AST
 */
static EcNode *parseFieldKind(EcCompiler *cp, EcNode *np)
{
    EcNode  *np;

    ENTER(cp);

    if (peekToken(cp) == T_CONST) {
        getToken(cp);
        np->def.varKind = KIND_CONST;
    }
    return LEAVE(cp, np);
}
#endif


/*
    FieldName (47)
        PropertyName
        StringLiteral
        NumberLiteral
        ReservedIdentifier

    Input

    AST
 */
static EcNode *parseFieldName(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_ID:
    case T_NUMBER:
    case T_STRING:
        np = parsePrimaryExpression(cp);
        break;

    default:
        np = parsePropertyName(cp);
        break;
    }
    return LEAVE(cp, np);
}


/*
    ArrayLiteral (51)
        [ Elements ]

    ArrayLiteral (52)
        [ Elements ] : NullableTypeExpression

    Input sequence
        [

    AST
        N_EXPRESSIONS
            N_NEW
                N_QNAME
            N_EXPRESSIONS
                N_ASSIGN_OP
                    N_DOT
                        N_PARENT
                        N_LITERAL
                    ANY
 */
static EcNode *parseArrayLiteral(EcCompiler *cp)
{
    Ejs         *ejs;
    EcNode      *np, *typeNode;

    ENTER(cp);

    ejs = cp->ejs;
    np = createNode(cp, N_OBJECT_LITERAL, NULL);
    np->objectLiteral.isArray = 1;

    if (getToken(cp) != T_LBRACKET) {
        np = parseError(cp, "Expecting \"[\"");
    } else {
        np = parseElements(cp, np);
        typeNode = 0;
        if (getToken(cp) != T_RBRACKET) {
            np = parseError(cp, "Expecting \"[\"");
        } else {
            if (peekToken(cp) == T_COLON) {
                typeNode = parseArrayType(cp);
                if (typeNode == 0) {
                    return LEAVE(cp, 0);
                }
            }
        }
        if (np) {
            if (typeNode == 0) {
                typeNode = createNode(cp, N_QNAME, EST(Array)->qname.name);
            }
            np->objectLiteral.typeNode = linkNode(np, typeNode);
        }
    }
    return LEAVE(cp, np);
}


/*
    Elements (54)
        ElementList
        ArrayComprehension

    Input sequence

    AST
        N_EXPRESSIONS
            N_ASSIGN_OP
                N_DOT
                    N_REF
                    N_LITERAL
                ANY
 */
static EcNode *parseElements(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (peekToken(cp) == T_FOR || cp->peekToken->tokenId == T_LET || cp->peekToken->tokenId == T_IF) {
        np = parseArrayComprehension(cp, np);
    } else {
        np = parseElementList(cp, np);
    }
    return LEAVE(cp, np);
}


/*
    ElementList (56)
        EMPTY
        LiteralElement
        , ElementList
        LiteralElement , ElementList

    Input sequence

    AST
        N_EXPRESSIONS
            N_ASSIGN_OP
                N_DOT
                    N_REF
                    N_LITERAL
                ANY
 */
static EcNode *parseElementList(EcCompiler *cp, EcNode *np)
{
    EcNode      *elt;
    int         index;

    ENTER(cp);

    for (index = 0; np; ) {
        if (peekToken(cp) == T_COMMA) {
            getToken(cp);
            index++;
        } else if (cp->peekToken->tokenId == T_RBRACKET) {
            break;
        } else {
            if ((elt = parseLiteralElement(cp)) != 0) {
                assure(elt->kind == N_FIELD);
                elt->field.index = index;
                if (peekToken(cp) != T_COMMA && cp->peekToken->tokenId != T_RBRACKET) {
                    getToken(cp);
                    return LEAVE(cp, unexpected(cp));
                }
            }
            np = appendNode(np, elt);
        }
    }
    return LEAVE(cp, np);
}


/*
    LiteralElement (60)
        AssignmentExpression -noList, allowin-

    Input sequence

    AST
        N_ASSIGN_OP
            N_DOT
                N_REF
                N_LITERAL (empty - caller must set node->var)
            ANY
 */
static EcNode *parseLiteralElement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = createNode(cp, N_FIELD, NULL);
    np->field.varKind = KIND_CONST;
    np->attributes |= EJS_TRAIT_READONLY;
    np->field.fieldKind = FIELD_KIND_VALUE;
    np->field.expr = linkNode(np, parseAssignmentExpression(cp));
    return LEAVE(cp, np);
}


/*
    ArrayComprehension (42)
        AssignmentExpression ComprehensionExpression
 */
static EcNode *parseArrayComprehension(EcCompiler *cp, EcNode *literalElement)
{
    EcNode      *np;

    ENTER(cp);

    np = parseAssignmentExpression(cp);
    np = parseComprehensionExpression(cp, np);
    return LEAVE(cp, np);
}


/*
    ComprehensionExpression (43)
        for (TypedPattern in CommaExpression) ComprehensionClause
        for each (TypedPattern in CommaExpression) ComprehensionClause
        let ParenExpression ComprehensionClause
        if ParenExpression ComprehensionClause
 */
static EcNode *parseComprehensionExpression(EcCompiler *cp, EcNode *literalElement)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    return LEAVE(cp, np);
}


#if UNUSED
/*
    ForInExpressionList (62)
        ForExpression
        ForExpressionList ForExpression

    Input sequence

    AST
 */
static EcNode *parseForInExpressionList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    ForInExpression (64)
        for ( ForInBinding in ListExpression -allowin- )
        for each ( ForInBinding in ListExpression -allowin- )
        ForExpressionList ForExpression

    Input sequence

    AST
 */
static EcNode *parseForInExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    OptionalIfCondition (66)
        EMPTY
        if ParenListExpression

    Input
        this

    AST
 */


/*
    XMLInitializer (68)
        XMLMarkup
        XMLElement
        < > XMLElementContent </ >

    Input
        <!--
        [CDATA
        <?
        <

    AST
        Add data to literal.data buffer
 */
PUBLIC struct EcNode *parseXMLInitializer(EcCompiler *cp)
{
    Ejs         *ejs;
    EcNode      *np;

    ENTER(cp);
    ejs = cp->ejs;

    np = createNode(cp, N_LITERAL, NULL);
    np->literal.data = mprCreateBuf(0, 0);

    if (EST(XML) == 0) {
        return LEAVE(cp, parseError(cp, "No XML support configured"));
    }
    np->literal.var = (EjsObj*) ejsCreateObj(ejs, EST(XML), 0);

    switch (peekToken(cp)) {
    case T_XML_COMMENT_START:
    case T_CDATA_START:
    case T_XML_PI_START:
        return parseXMLMarkup(cp, np);

    case T_LT:
        if (peekAheadToken(cp, 2) == T_GT) {
            getToken(cp);
            getToken(cp);
            addAscToLiteral(cp, np, "<>", 2);
            np = parseXMLElementContent(cp, np);
            if (getToken(cp) != T_LT_SLASH) {
                return LEAVE(cp, expected(cp, "</"));
            }
            if (getToken(cp) != T_GT) {
                return LEAVE(cp, expected(cp, ">"));
            }
            addAscToLiteral(cp, np, "</>", 3);
        } else {
            return parseXMLElement(cp, np);
        }
        break;

    default:
        getToken(cp);
        return LEAVE(cp, unexpected(cp));
    }
    np = 0;
    return LEAVE(cp, np);
}


/*
    XMLElementContent (71)
        { ListExpression } XMLElementContent
        XMLMarkup XMLElementContent
        XMLText XMLElementContent
        XMLElement XMLElementContent
        EMPTY
    Input
        {
        <!--
        [CDATA
        <?
        <
        Text
    AST
 */
PUBLIC struct EcNode *parseXMLElementContent(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (np == 0) {
        return LEAVE(cp, np);
    }
    
    switch (peekToken(cp)) {
    case T_LBRACE:
        getToken(cp);
        np = parseListExpression(cp);
        if (getToken(cp) != T_RBRACE) {
            return LEAVE(cp, expected(cp, "}"));
        }
        np = parseXMLElementContent(cp, np);
        break;

    case T_XML_COMMENT_START:
    case T_CDATA_START:
    case T_XML_PI_START:
        np = parseXMLMarkup(cp, np);
        break;

    case T_LT:
        np = parseXMLElement(cp, np);
        np = parseXMLElementContent(cp, np);
        break;

    case T_LT_SLASH:
        break;

    case T_EOF:
    case T_ERR:
    case T_NOP:
        return LEAVE(cp, 0);

    default:
        np = parseXMLText(cp, np);
        np = parseXMLElementContent(cp, np);
    }
    return LEAVE(cp, np);
}


/*
    XMLElement (76)
        < XMLTagContent XMLWhitespace />
        < XMLTagContent XMLWhitespace > XMLElementContent </ XMLTagName XMLWhitespace >

    Input
        <

    AST
        Add data to literal.data buffer
 */
PUBLIC struct EcNode *parseXMLElement(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (getToken(cp) != T_LT) {
        return LEAVE(cp, expected(cp, "<"));
    }
    addTokenToLiteral(cp, np);

    np = parseXMLTagContent(cp, np);
    if (np == 0) {
        return LEAVE(cp, np);
    }
    if (getToken(cp) == T_SLASH_GT) {
        addTokenToLiteral(cp, np);
        return LEAVE(cp, np);

    } else if (cp->token->tokenId != T_GT) {
        return LEAVE(cp, unexpected(cp));
    }

    addTokenToLiteral(cp, np);

    np = parseXMLElementContent(cp, np);
    if (getToken(cp) != T_LT_SLASH) {
        return LEAVE(cp, expected(cp, "</"));
    }
    addTokenToLiteral(cp, np);

    np = parseXMLTagName(cp, np);
    if (getToken(cp) != T_GT) {
        return LEAVE(cp, expected(cp, ">"));
    }
    addTokenToLiteral(cp, np);
    return LEAVE(cp, np);
}


/*
    XMLTagContent (79)
        XMLTagName XMLAttributes

    Input
        {
        UnicodeLetter
        _       underscore
        :       colon

    AST
        Add data to literal.data buffer
 */
PUBLIC struct EcNode *parseXMLTagContent(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    np = parseXMLTagName(cp, np);
    if (np) {
        np = parseXMLAttributes(cp, np);
    }
    return LEAVE(cp, np);
}


/*
    XMLTagName (80)
        { ListExpression }
        XMLName

    Input
        {
        UnicodeLetter
        _       underscore
        :       colon

    AST
        Add data to literal.data buffer
 */
PUBLIC struct EcNode *parseXMLTagName(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (np == 0) {
        return LEAVE(cp, np);
    }
    if (peekToken(cp) == T_LBRACE) {
        getToken(cp);
        np = parseListExpression(cp);
        if (getToken(cp) != T_RBRACE) {
            return LEAVE(cp, expected(cp, "}"));
        }
    } else {
        np = parseXMLName(cp, np);
    }
    return LEAVE(cp, np);
}


/*
    XMLAttributes (82)
        XMLWhitespace { ListExpression }
        XMLAttribute XMLAttributes
        EMPTY
    Input

    AST
        Add data to literal.data buffer
 */
PUBLIC struct EcNode *parseXMLAttributes(EcCompiler *cp, EcNode *np)
{
    int         tid;

    ENTER(cp);

    tid = peekToken(cp);
    if (tid == T_LBRACE) {
        parseListExpression(cp);
        if (peekToken(cp) == T_RBRACE) {
            return LEAVE(cp, expected(cp, "}"));
        }
    } else {
        while (tid != T_GT && tid != T_SLASH_GT) {
            if ((np = parseXMLAttribute(cp, np)) == 0) {
                break;
            }
            tid = peekToken(cp);
        }
    }
    return LEAVE(cp, np);
}


/*
    XMLAttributes (85)
        XMLWhitespace XMLName XMLWhitespace = XMLWhitepace { ListExpression (allowIn) }
        XMLWhitespace XMLName XMLWhitespace = XMLAttributeValue

    Input
        UnicodeLetter
        _       underscore
        :       colon

    AST
        Add data to literal.data buffer
 */
PUBLIC struct EcNode *parseXMLAttribute(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    addAscToLiteral(cp, np, " ", 1);
    np = parseXMLName(cp, np);

    if (getToken(cp) != T_ASSIGN) {
        return LEAVE(cp, expected(cp, "="));
    }
    addAscToLiteral(cp, np, "=", 1);

    if (peekToken(cp) == T_LBRACE) {
        np = parseListExpression(cp);
        if (getToken(cp) != T_RBRACE) {
            return LEAVE(cp, expected(cp, "}"));
        }
    } else {
        np = parseXMLAttributeValue(cp, np);
    }
    return LEAVE(cp, np);
}


/*
    ThisExpression (87)
        this
        this callee
        this generator
        this function
    Input
        this

    AST
 */
static EcNode *parseThisExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_THIS) {
        return LEAVE(cp, unexpected(cp));
    }
    np = createNode(cp, N_THIS, NULL);
    peekToken(cp);
    
    if (cp->token->loc.lineNumber == cp->peekToken->loc.lineNumber) {
        switch (peekToken(cp)) {
        case T_TYPE:
            getToken(cp);
            np->thisNode.thisKind = EC_THIS_TYPE;
            break;

        case T_FUNCTION:
            getToken(cp);
            np->thisNode.thisKind = EC_THIS_FUNCTION;
            break;

        case T_CALLEE:
            getToken(cp);
            np->thisNode.thisKind = EC_THIS_CALLEE;
            break;

        case T_GENERATOR:
            getToken(cp);
            np->thisNode.thisKind = EC_THIS_GENERATOR;
            break;
        }
    }
    return LEAVE(cp, np);
}


/*
    PrimaryExpression (90)
        null
        true
        false
        NumberLiteral
        StringLiteral
        RegularExpression
        ThisExpression
        XMLInitializer
        ParenListExpression
        ArrayLiteral
        ObjectLiteral
        FunctionExpression
        AttributeName
        PrimaryName

    Input sequence
        null
        true
        false
        this
        function
        Identifier
        ContextuallyReservedIdentifier
        {
        [
        (
        @
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <

    AST
        N_FUNCTION
        N_NEW           (array / object literals)
        N_LITERAL
        N_QNAME
        N_THIS
 */
static EcNode *parsePrimaryExpression(EcCompiler *cp)
{
    Ejs         *ejs;
    EcNode      *np, *qualifier, *name;
    EjsObj      *vp;
    int         tid;

    ENTER(cp);

    ejs = cp->ejs;
    tid = peekToken(cp);
    if (cp->peekToken->groupMask & G_CONREV) {
        tid = T_ID;
    }
    np = 0;
    switch (tid) {
    case T_STRING:
        if (peekAheadToken(cp, 2) == T_COLON_COLON) {
            np = parsePrimaryName(cp);
        } else {
            getToken(cp);
            vp = (EjsObj*) tokenString(cp);
            np = createNode(cp, N_LITERAL, tokenString(cp));
            np->literal.var = vp;
        }
        break;

    case T_ID:
        np = parsePrimaryName(cp);
        break;

    case T_AT:
        np = appendNode(np, parseAttributeName(cp));
        break;

    case T_NUMBER:
        getToken(cp);
        vp = ejsParse(cp->ejs, cp->token->text, -1);
        np = createNode(cp, N_LITERAL, tokenString(cp));
        np->literal.var = vp;
        break;

    case T_NULL:
        getToken(cp);
        if (!ejs->initialized) {
            np = createNode(cp, N_QNAME, tokenString(cp));
        } else {
            np = createNode(cp, N_LITERAL, tokenString(cp));
            np->literal.var = ESV(null);
        }
        break;

    case T_UNDEFINED:
        getToken(cp);
        if (!ejs->initialized) {
            np = createNode(cp, N_QNAME, tokenString(cp));
        } else {
            np = createNode(cp, N_LITERAL, tokenString(cp));
            np->literal.var = ESV(undefined);
        }
        break;

    case T_TRUE:
        getToken(cp);
        if (!ejs->initialized) {
            np = createNode(cp, N_QNAME, tokenString(cp));
        } else {
            np = createNode(cp, N_LITERAL, tokenString(cp));
            vp = (EjsObj*) ejsCreateBoolean(cp->ejs, 1);
            np->literal.var = vp;
        }
        break;

    case T_FALSE:
        getToken(cp);
        if (!ejs->initialized) {
            np = createNode(cp, N_QNAME, tokenString(cp));
        } else {
            np = createNode(cp, N_LITERAL, tokenString(cp));
            vp = (EjsObj*) ejsCreateBoolean(cp->ejs, 0);
            np->literal.var = vp;
        }
        break;

    case T_THIS:
        np = parseThisExpression(cp);
        break;

    case T_LPAREN:
        np = parseParenListExpression(cp);
        if (peekToken(cp) == T_COLON_COLON) {
            getToken(cp);
            qualifier = np;
            if ((np = parseQualifiedNameIdentifier(cp)) != 0) {
                if (np->kind == N_EXPRESSIONS) {
                    name = np;
                    np = createNode(cp, N_QNAME, tokenString(cp));
                    np->name.nameExpr = linkNode(np, name);
                }
                np->name.qualifierExpr = linkNode(np, qualifier);
            }
        }
        break;

    case T_LBRACKET:
        np = parseArrayLiteral(cp);
        break;

    case T_LBRACE:
        np = parseObjectLiteral(cp);
        break;

    case T_FUNCTION:
        np = parseFunctionExpression(cp);
        break;

    case T_VOID:
    case T_NAMESPACE:
    case T_TYPEOF:
        getToken(cp);
        if (!ejs->initialized) {
            np = createNode(cp, N_QNAME, tokenString(cp));
        } else {
            np = unexpected(cp);
        }
        break;

    case T_LT:
    case T_XML_COMMENT_START:
    case T_CDATA_START:
    case T_XML_PI_START:
        np = parseXMLInitializer(cp);
        break;

    case T_DIV:
    case T_SLASH_GT:
        np = parseRegularExpression(cp);
        break;

    case T_ERR:
    default:
        getToken(cp);
        np = unexpected(cp);
        break;
    }
    return LEAVE(cp, np);
}


static EcNode *parseRegularExpression(EcCompiler *cp)
{
    EcNode      *np;
    EjsObj      *vp;
    wchar       *prefix;
    int         id;

    ENTER(cp);

    /* Flush peek ahead buffer */
    while (cp->putback) {
        getToken(cp);
    }
    prefix = wclone(cp->token->text);
    id = ecGetRegExpToken(cp, prefix);
    cp->peekToken = 0;
    updateDebug(cp);

    if (id != T_REGEXP) {
        return LEAVE(cp, parseError(cp, "Cannot parse regular expression"));
    }
    if ((vp = (EjsObj*) ejsCreateRegExp(cp->ejs, tokenString(cp))) == NULL) {
        return LEAVE(cp, parseError(cp, "Cannot compile regular expression"));
    }
    np = createNode(cp, N_LITERAL, NULL);
    np->literal.var = vp;
    return LEAVE(cp, np);
}


/*
    SuperExpression (104)
        super
        super ParenExpression

    Input
        super

    AST
        N_SUPER

    NOTES:
        Using Arguments instead of ParenExpression so we can have multiple args.
 */
static EcNode *parseSuperExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_SUPER) {
        np = unexpected(cp);

    } else {
        if (peekToken(cp) == T_LPAREN) {
            np = createNode(cp, N_SUPER, NULL);
            np = appendNode(np, parseArguments(cp));
        } else {
            np = createNode(cp, N_SUPER, NULL);
        }
    }
    return LEAVE(cp, np);
}


/*
    Arguments (106)
        ( )
        ( ArgumentList )

    Input
        (

    AST
        N_ARGS
 */
static EcNode *parseArguments(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_LPAREN) {
        np = parseError(cp, "Expecting \"(\"");

    } else if (peekToken(cp) == T_RPAREN) {
        getToken(cp);
        np = createNode(cp, N_ARGS, NULL);

    } else {
        np = parseArgumentList(cp);
        if (np && getToken(cp) != T_RPAREN) {
            np = parseError(cp, "Expecting \")\"");
        }
    }
    return LEAVE(cp, np);
}


/*
    ArgumentList (118)
        AssignmentExpression
        ArgumentList , AssignmentExpression

    Input

    AST N_ARGS
        children: arguments
 */
static EcNode *parseArgumentList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_ARGS, NULL);

    if (np && peekToken(cp) == T_ELIPSIS) {
        np = appendNode(np, parseRestArgument(cp));
    } else {
        np = appendNode(np, parseAssignmentExpression(cp));
    }
    while (np && peekToken(cp) == T_COMMA) {
        getToken(cp);
        if (np && peekToken(cp) == T_ELIPSIS) {
            np = appendNode(np, parseRestArgument(cp));
        } else {
            np = appendNode(np, parseAssignmentExpression(cp));
        }
    }
    return LEAVE(cp, np);
}


/*
    RestArgument (NEW)
        ...
        ... Parameter

    Input

    AST
 */
static EcNode *parseRestArgument(EcCompiler *cp)
{
    EcNode      *np, *spreadArg;

    ENTER(cp);

    if (getToken(cp) == T_ELIPSIS) {
        spreadArg = createNode(cp, N_SPREAD, NULL);
        np = appendNode(spreadArg, parseAssignmentExpression(cp));
        if (peekToken(cp) == T_COMMA) {
            np = unexpected(cp);
        }
    } else {
        np = unexpected(cp);
    }
    return LEAVE(cp, np);
}


/*
    PropertyOperator (110)
        . ReservedIdentifier
        . PropertyName
        . AttributeName
        .. QualifiedName
        . ParenListExpression
        . ParenListExpression :: QualifiedNameIdentifier
        Brackets
        TypeApplication

    Input
        .
        ..
        [

    AST
        N_DOT
 */
static EcNode *parsePropertyOperator(EcCompiler *cp)
{
    EcNode      *np, *qualifier, *name;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_DOT:
        np = createNode(cp, N_DOT, NULL);
        getToken(cp);
        switch (peekToken(cp)) {
        case T_LPAREN:
            name = parseParenListExpression(cp);
            if (peekToken(cp) == T_COLON_COLON) {
                qualifier = name;
                getToken(cp);
                name = parseQualifiedNameIdentifier(cp);
                name->name.qualifierExpr = linkNode(name, qualifier);
            }
            np = appendNode(np, name);
            break;

        /* TODO - should handle all contextually reserved identifiers here */
        case T_TYPE:
        case T_ID:
        case T_GET:
        case T_SET:
        case T_STRING:
        case T_REQUIRE:
        case T_RESERVED_NAMESPACE:
        case T_MUL:
            if (cp->token->groupMask & G_RESERVED) {
                np = appendNode(np, parseIdentifier(cp));
            } else {
                np = appendNode(np, parsePropertyName(cp));
            }
            break;

        case T_AT:
            np = appendNode(np, parseAttributeName(cp));
            break;

        case T_SUPER:
            getToken(cp);
            np = appendNode(np, createNode(cp, N_SUPER, NULL));
            break;

        default:
            if (cp->token->groupMask & G_RESERVED) {
                np = appendNode(np, parseIdentifier(cp));
            } else {
                np = appendNode(np, parsePropertyName(cp));
            }
            break;
        }
        break;

    case T_LBRACKET:
        np = createNode(cp, N_DOT, NULL);
        np = appendNode(np, parseBrackets(cp));
        break;

    case T_DOT_DOT:
        getToken(cp);
        np = createNode(cp, N_DOT, NULL);
        name = parseQualifiedName(cp);
        if (name && name->kind == N_QNAME) {
            name->qname.name = ejsSprintf(cp->ejs, ".%@", name->qname.name);
        }
        np = appendNode(np, name);
        break;

    default:
        getToken(cp);
        np = parseError(cp, "Expecting property operator . .. or [");
        break;
    }
    return LEAVE(cp, np);
}


/*
    Brackets (125)
        [ ListExpression ]
        [ SliceExpression ]

    Input
        [

    AST
        N_EXPRESSIONS
 */
static EcNode *parseBrackets(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    //  TODO - not yet implementing SliceExpression
    if (getToken(cp) != T_LBRACKET) {
        np = parseError(cp, "Expecting \"[\"");
    }

    if (peekToken(cp) == T_COLON) {
        /*
            Slice expression
         */
        /* First optional expression in a slice expression is empty */
        np = parseOptionalExpression(cp);
        if (getToken(cp) != T_COLON) {
            np = parseError(cp, "Expecting \":\"");
        }
        np = appendNode(np, parseOptionalExpression(cp));

    } else {

        np = parseListExpression(cp);

        if (peekToken(cp) == T_COLON) {
            /*
                Slice expression
             */
            np = appendNode(np, parseOptionalExpression(cp));
            if (peekToken(cp) != T_COLON) {
                getToken(cp);
                np = parseError(cp, "Expecting \":\"");
            }
            np = appendNode(np, parseOptionalExpression(cp));
        }
    }

    if (getToken(cp) != T_RBRACKET) {
        np = parseError(cp, "Expecting \"[\"");
    }
    return LEAVE(cp, np);
}


/*
    TypeApplication (120)
        .< TypeExpressionList >

    Input
        .<

    AST
 */
#if FUTURE
/*
    SliceExpression (121)
        OptionalExpression : OptionalExpression
        OptionalExpression : OptionalExpression : OptionalExpression

    Input

    AST
 */
static EcNode *parseSliceExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    OptionalExpression (123)
        ListExpression -allowin-
        EMPTY

    Input

    AST
 */
static EcNode *parseOptionalExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = parseListExpression(cp);
    return LEAVE(cp, np);
}


/*
    MemberExpression (125) -a,b-
        PrimaryExpression -a,b-
        new MemberExpression Arguments
        SuperExpression PropertyOperator
        MemberExpression PropertyOperator

    Input
        null
        true
        false
        this
        function
        Identifier
        {
        [
        (
        @
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new

    AST
        N_FUNCTION
        N_DOT
            left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
            right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
        N_LITERAL
        N_NEW           (array / object literals)
        N_QNAME
        N_SUPER
        N_THIS
 */
static EcNode *parseMemberExpression(EcCompiler *cp)
{
    EcNode      *np, *newNode;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_SUPER:
        np = parseSuperExpression(cp);
        if (peekToken(cp) == T_DOT || cp->peekToken->tokenId == T_DOT_DOT ||
                cp->peekToken->tokenId == T_LBRACKET) {
            np = insertNode(parsePropertyOperator(cp), np, 0);
        }
        break;

    case T_NEW:
        getToken(cp);
        newNode = createNode(cp, N_NEW, NULL);
        newNode = appendNode(newNode, parseMemberExpression(cp));
        np = createNode(cp, N_NEW, NULL);
        np = appendNode(np, newNode);
        np = appendNode(np, parseArguments(cp));
        break;

    default:
        np = parsePrimaryExpression(cp);
        break;
    }
    while (np && (peekToken(cp) == T_DOT || cp->peekToken->tokenId == T_DOT_DOT ||
            cp->peekToken->tokenId == T_LBRACKET)) {
#if 1
        if (np->loc.lineNumber == cp->peekToken->loc.lineNumber) {
            np = insertNode(parsePropertyOperator(cp), np, 0);
        } else {
            break;
        }
#else
        np = insertNode(parsePropertyOperator(cp), np, 0);
#endif
    }
    return LEAVE(cp, np);
}


/*
    CallExpression (129) -a,b-
        MemberExpression Arguments
        CallExpression Arguments
        CallExpression PropertyOperator

    Input

    AST
        N_CALL

    "me" is to to an already parsed member expression
 */
static EcNode *parseCallExpression(EcCompiler *cp, EcNode *me)
{
    EcNode      *np;

    ENTER(cp);

    np = 0;

    while (1) {
        peekToken(cp);
        if (me && me->loc.lineNumber != cp->peekToken->loc.lineNumber) {
            return LEAVE(cp, np);
        }
        switch (peekToken(cp)) {
        case T_LPAREN:
            np = createNode(cp, N_CALL, NULL);
            np = appendNode(np, me);
            np = appendNode(np, parseArguments(cp));
            if (np && cp->token) {
                np->loc.lineNumber = cp->token->loc.lineNumber;
            }
            break;

        case T_DOT:
        case T_DOT_DOT:
        case T_LBRACKET:
            if (me == 0) {
                getToken(cp);
                return LEAVE(cp, unexpected(cp));
            }
            np = insertNode(parsePropertyOperator(cp), me, 0);
            if (np && cp->token) {
                np->loc.lineNumber = cp->token->loc.lineNumber;
            }
            break;

        default:
            if (np == 0) {
                getToken(cp);
                return LEAVE(cp, unexpected(cp));
            }
            return LEAVE(cp, np);
        }
        if (np == 0) {
            return LEAVE(cp, np);
        }
        me = np;
    }
    return LEAVE(cp, np);
}


/*
    NewExpression (132)
        MemberExpression
        new NewExpression

    Input
        null
        true
        false
        this
        function
        Identifier
        {
        [
        (
        @
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new

    AST
        N_FUNCTION
        N_DOT
            left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
            right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
        N_LITERAL
        N_NEW           (array / object literals)
        N_QNAME
        N_SUPER
        N_THIS
 */
static EcNode *parseNewExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) == T_NEW) {
        getToken(cp);
        np = createNode(cp, N_NEW, NULL);
        np = appendNode(np, parseNewExpression(cp));

    } else {
        np = parseMemberExpression(cp);
    }
    return LEAVE(cp, np);
}


/*
    LeftHandSideExpression (134) -a,b-
        NewExpression
        CallExpression

    Where NewExpression is:
        MemberExpression
        new NewExpression

    Where CallExpression is:
        MemberExpression Arguments
        CallExpression Arguments
        CallExpression PropertyOperator

    Where MemberExpression is:
        PrimaryExpression -a,b-
        new MemberExpression Arguments
        SuperExpression PropertyOperator
        MemberExpression PropertyOperator

    So look ahead problem on MemberExpression. We don't know if it is a newExpression or a CallExpression. This requires
    large lookahead. So, refactored to be:

    LeftHandSideExpression (136) -a,b-
        new MemberExpression LeftHandSidePrime
        MemberExpression LeftHandSidePrime

    and where LeftHandSidePrime is:
        Arguments LeftHandSidePrime
        PropertyOperator LeftHandSidePrime
        EMPTY

    Input
        null
        true
        false
        this
        function
        Identifier
        {
        [
        (
        @
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new

    Also:
        new MemberExpression
        MemberExpression
        MemberExpression (
        MemberExpression .
        MemberExpression ..
        MemberExpression [

    AST
        N_CALL
        N_FUNCTION
        N_DOT
            left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
            right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
        N_LITERAL
        N_NEW           (array / object literals)
        N_QNAME
        N_SUPER
        N_THIS
 */
static EcNode *parseLeftHandSideExpression(EcCompiler *cp)
{
    EcNode      *np, *callNode;

    ENTER(cp);

    if (peekToken(cp) == T_NEW) {
        np = parseNewExpression(cp);
    } else {
        np = parseMemberExpression(cp);
    }
    if (np) {
        /*
            Refactored CallExpression processing
         */
        switch (peekToken(cp)) {
        case T_LPAREN:
        case T_DOT:
        case T_DOT_DOT:
        case T_LBRACKET:
            if (cp->token->loc.lineNumber == cp->peekToken->loc.lineNumber) {
                /*
                    May have multiline function expression: x = (function ..... multiple lines)() 
                 */
                np->loc.lineNumber = cp->token->loc.lineNumber;
                np = parseCallExpression(cp, np);
            }
            break;

        default:
            if (np->kind == N_NEW) {
                /*
                    Create a dummy call to the constructor
                 */
                callNode = createNode(cp, N_CALL, NULL);
                np = appendNode(callNode, np);
                np = appendNode(np, createNode(cp, N_ARGS, NULL));
            }
            break;
        }
    }
    return LEAVE(cp, np);
}


#if UNUSED
/*
    LeftHandSidePrime (psudo) -a,b-
        Arguments LeftHandSidePrime
        PropertyOperator LeftHandSidePrime
        EMPTY

    Input
        (
        .
        ..
        null
        true
        false
        this
        function
        Identifier
        {
        [
        (
        @
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new

    AST
        N_CALL
        N_FUNCTION
        N_DOT
            left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
            right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
        N_LITERAL
        N_NEW           (array / object literals)
        N_QNAME
        N_SUPER
        N_THIS
 */
static EcNode *parseLeftHandSidePrime(EcCompiler *cp, EcNode *np)
{
    EcNode      *callNode;

    ENTER(cp);

    do {
        switch (peekToken(cp)) {
        case T_LPAREN:
#if UNUSED
            callNode = createNode(cp, N_CALL, NULL);
            np = appendNode(callNode, np);
            np = appendNode(np, parseArguments(cp));
#endif
            break;

        case T_DOT:
        case T_DOT_DOT:
        case T_LBRACKET:
            np = appendNode(np, parsePropertyOperator(cp));
            break;

        default:
            return LEAVE(cp, np);
        }

    } while (np);

    return LEAVE(cp, np);
}
#endif


/*
    UnaryTypeExpression (136)
        LeftHandSideExpression
        type NullableTypeExpression

    Input
        type

    AST
 */
static EcNode *parseUnaryTypeExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    if (peekToken(cp) == T_TYPE) {
        getToken(cp);
        np = parseNullableTypeExpression(cp);
    } else {
        np = parseLeftHandSideExpression(cp);
    }
    return LEAVE(cp, np);
}


/*
    PostfixExpression (138)
        UnaryTypeExpression
        LeftHandSideExpression [no line break] ++
        LeftHandSideExpression [no line break] --

    Input

    AST
 */
static EcNode *parsePostfixExpression(EcCompiler *cp)
{
    EcNode      *parent, *np;

    ENTER(cp);

    if (peekToken(cp) == T_TYPE) {
        np = parseUnaryTypeExpression(cp);
    } else {
        np = parseLeftHandSideExpression(cp);
        if (np) {
            if (peekToken(cp) == T_PLUS_PLUS || cp->peekToken->tokenId == T_MINUS_MINUS) {
                getToken(cp);
                parent = createNode(cp, N_POSTFIX_OP, NULL);
                np = appendNode(parent, np);
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    UnaryExpression (141)
        PostfixExpression
        delete PostfixExpression
        void UnaryExpression
        typeof UnaryExpression
        ++ PostfixExpression
        -- PostfixExpression
        + UnaryExpression
        - UnaryExpression
        ~ UnaryExpression           (bitwise not)
        ! UnaryExpression

    Input

    AST
 */
static EcNode *parseUnaryExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_DELETE:
    case T_LOGICAL_NOT:
    case T_PLUS:
    case T_PLUS_PLUS:
    case T_MINUS:
    case T_MINUS_MINUS:
    case T_TILDE:
    case T_TYPEOF:
    case T_VOID:
        getToken(cp);
        np = createNode(cp, N_UNARY_OP, NULL);
        np = appendNode(np, parseUnaryExpression(cp));
        break;

    default:
        np = parsePostfixExpression(cp);
    }
    return LEAVE(cp, np);
}


/*
    MultiplicativeExpression (152) -a,b-
        UnaryExpression
        MultiplicativeExpression * UnaryExpression
        MultiplicativeExpression / UnaryExpression
        MultiplicativeExpression % UnaryExpression

    Input

    AST
 */
static EcNode *parseMultiplicativeExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);
    
    np = parseUnaryExpression(cp);
    while (np) {
        switch (peekToken(cp)) {
        case T_MUL:
        case T_DIV:
        case T_MOD:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseUnaryExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    assure(cp > 0);
    return LEAVE(cp, np);
}


/*
    AdditiveExpression (156)
        MultiplicativeExpression
        AdditiveExpression + MultiplicativeExpression
        AdditiveExpression - MultiplicativeExpression

    Input

    AST
 */
static EcNode *parseAdditiveExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;
    
    ENTER(cp);

    np = parseMultiplicativeExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_PLUS:
        case T_MINUS:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseMultiplicativeExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
    ShiftExpression (159) -a,b-
        AdditiveExpression
        ShiftExpression << AdditiveExpression
        ShiftExpression >> AdditiveExpression
        ShiftExpression >>> AdditiveExpression

    Input

    AST
 */
static EcNode *parseShiftExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);

    np = parseAdditiveExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_LSH:
        case T_RSH:
        case T_RSH_ZERO:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseAdditiveExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
    RelationalExpression (163) -allowin-
        ShiftExpression
        RelationalExpression < ShiftExpression
        RelationalExpression > ShiftExpression
        RelationalExpression <= ShiftExpression
        RelationalExpression >= ShiftExpression
        RelationalExpression [in] ShiftExpression
        RelationalExpression instanceOf ShiftExpression
        RelationalExpression cast ShiftExpression
        RelationalExpression to ShiftExpression
        RelationalExpression is ShiftExpression
        RelationalExpression like ShiftExpression

    Input

    AST
 */
static EcNode *parseRelationalExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);

    np = parseShiftExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_IN:
            if (cp->state->noin) {
                return LEAVE(cp, np);
            }
            /* Fall through */

        case T_LT:
        case T_LE:
        case T_GT:
        case T_GE:
        case T_INSTANCEOF:
        case T_IS:
        case T_CAST:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseShiftExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
    EqualityExpression (182)
        RelationalExpression
        EqualityExpression == RelationalExpression
        EqualityExpression != RelationalExpression
        EqualityExpression === RelationalExpression
        EqualityExpression !== RelationalExpression

    Input

    AST
 */
static EcNode *parseEqualityExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);

    np = parseRelationalExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_EQ:
        case T_NE:
        case T_STRICT_EQ:
        case T_STRICT_NE:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseRelationalExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
    BitwiseAndExpression (187)
        EqualityExpression
        BitwiseAndExpression & EqualityExpression

    Input

    AST
 */
static EcNode *parseBitwiseAndExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);

    np = parseEqualityExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_BIT_AND:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseEqualityExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
    BitwiseXorExpression (189)
        BitwiseAndExpression
        BitwiseXorExpression ^ BitwiseAndExpression

    Input

    AST
 */
static EcNode *parseBitwiseXorExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);

    np = parseBitwiseAndExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_BIT_XOR:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseBitwiseAndExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
    BitwiseOrExpression (191)
        BitwiseXorExpression
        BitwiseOrExpression | BitwiseXorExpression

    Input

    AST
 */
static EcNode *parseBitwiseOrExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);

    np = parseBitwiseXorExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_BIT_OR:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseBitwiseXorExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
    LogicalAndExpression (193)
        BitwiseOrExpression
        LogicalAndExpression && BitwiseOrExpression

    Input

    AST
 */
static EcNode *parseLogicalAndExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);

    np = parseBitwiseOrExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_LOGICAL_AND:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseBitwiseOrExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
    LogicalOrExpression (195)
        LogicalAndExpression
        LogicalOrExpression || LogicalOrExpression

    Input

    AST
 */
static EcNode *parseLogicalOrExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);

    np = parseLogicalAndExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_LOGICAL_OR:
            getToken(cp);
            parent = createNode(cp, N_BINARY_OP, NULL);
            np = createBinaryNode(cp, np, parseLogicalOrExpression(cp), parent);
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
    ConditionalExpression (197) -allowList,b-
        LetExpression -b-
        YieldExpression -b-
        LogicalOrExpression -a,b-
        LogicalOrExpression -allowList,b-
            ? AssignmentExpression : AssignmentExpression

    ConditionalExpression (197) -noList,b-
        SimpleYieldExpression
        LogicalOrExpression -a,b-
        LogicalOrExpression -allowList,b-
            ? AssignmentExpression : AssignmentExpression

    Input
        let
        yield
        (
        .
        ..
        null
        true
        false
        this
        function
        Identifier
        delete
        {
        [
        (
        @
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new

    AST
        N_CALL
        N_EXPRESSIONS
        N_FUNCTION
        N_DOT
            left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
            right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
        N_LITERAL
        N_NEW           (array / object literals)
        N_QNAME
        N_SUPER
        N_THIS
 */
static EcNode *parseConditionalExpression(EcCompiler *cp)
{
    EcNode      *np, *cond;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_LET:
        np = parseLetExpression(cp);
        break;

    case T_YIELD:
        np = parseYieldExpression(cp);
        break;

    default:
        np = parseLogicalOrExpression(cp);
        if (np) {
            if (peekToken(cp) == T_QUERY) {
                getToken(cp);
                cond = np;
                np = createNode(cp, N_IF, NULL);
                np->tenary.cond = linkNode(np, cond);
                np->tenary.thenBlock = linkNode(np, parseAssignmentExpression(cp));
                if (getToken(cp) != T_COLON) {
                    np = parseError(cp, "Expecting \":\"");
                } else {
                    np->tenary.elseBlock = linkNode(np, parseAssignmentExpression(cp));
                }
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    NonAssignmentExpression -allowList,b- (199)
        LetExpression
        YieldExpression
        LogicalOrExpression -a,b-
        LogicalOrExpression -allowList,b-
            ? AssignmentExpression : AssignmentExpression

    NonAssignmentExpression -noList,b-
        SimpleYieldExpression
        LogicalOrExpression -a,b-
        LogicalOrExpression -allowList,b-
            ? AssignmentExpression : AssignmentExpression

    Input

    AST
 */
static EcNode *parseNonAssignmentExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_LET:
        np = parseLetExpression(cp);
        break;

    case T_YIELD:
        np = parseYieldExpression(cp);
        break;

    default:
        np = parseLogicalOrExpression(cp);
        if (np) {
            if (peekToken(cp) == T_QUERY) {
                getToken(cp);
                np = parseAssignmentExpression(cp);
                if (getToken(cp) != T_COLON) {
                    np = parseError(cp, "Expecting \":\"");
                } else {
                    np = parseAssignmentExpression(cp);
                }
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    LetExpression (204)
        let ( LetBindingList ) ListExpression

    Input
        let

    AST
 */
static EcNode *parseLetExpression(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    
    if (getToken(cp) != T_LET) {
        return LEAVE(cp, expected(cp, "let"));
    }
    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, expected(cp, "("));
    }
    np = parseLetBindingList(cp);
    if (getToken(cp) != T_RPAREN) {
        return LEAVE(cp, expected(cp, ")"));
    }
    //  MOB - was return 0
    return np;
}


/*
    LetBindingList (205)
        EMPTY
        NonemptyLetBindingList -allowList-

    Input

    AST
 */
static EcNode *parseLetBindingList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


#if UNUSED
/*
    NonemptyLetBindingList (207) -a-
        VariableBinding -a,allowin-
        VariableBinding -noList,allowin- , NonemptyLetBindingList -a-

    Input

    AST
 */
static EcNode *parseNonemptyLetBindingList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    YieldExpression (209)
        yield
        yield [no line break] ListExpression

    Input

    AST
 */
static EcNode *parseYieldExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    Rewrite compound assignment. Eg. given x += 3  rewrite as
        x = x + 3;
 */
static EcNode *rewriteCompoundAssignment(EcCompiler *cp, int subId, EcNode *lhs, EcNode *rhs)
{
    EcNode      *np, *parent;

    ENTER(cp);

    /*
        Map the operator token to its non-assignment counterpart
     */
    np = createNode(cp, N_BINARY_OP, NULL);
    np->tokenId = subId - 1;

    np = appendNode(np, lhs);
    np = appendNode(np, rhs);
    parent = createNode(cp, N_ASSIGN_OP, NULL);
    np = createAssignNode(cp, lhs, np, parent);

    return LEAVE(cp, np);
}


static void fixDassign(EcCompiler *cp, EcNode *np)
{
    EcNode      *elt;
    int         next;
    
    for (next = 0; (elt = mprGetNextItem(np->children, &next)) != 0 && !cp->error; ) {
        fixDassign(cp, elt);
    }
    if (np->kind == N_OBJECT_LITERAL) {
        np->kind = N_DASSIGN;
        np->kindName = "n_dassign";
    } else if (np->kind == N_FIELD) {
        if (np->field.expr) {
            fixDassign(cp, np->field.expr);
        }
    }
}


/*
    AssignmentExpression (211)
        ConditionalExpression
        Pattern -a,b-allowin- = AssignmentExpression -a,b-
        SimplePattern -a,b-allowExpr- CompoundAssignmentOperator
                AssignmentExpression -a,b-

    Where
        SimplePattern is:
            LeftHandSideExpression -a,b-
            Identifier
        ConditionalExpression is:
            LetExpression -b-
            YieldExpression -b-
            LogicalOrExpression -a,b-

    Input
        (
        .
        ..
        null
        true
        false
        this
        function
        Identifier
        {
        [
        (
        @
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new
        let
        yield

    AST
        N_CALL
        N_EXPRESSIONS
        N_FUNCTION
        N_DOT
            left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
            right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
        N_LITERAL
        N_NEW           (array / object literals)
        N_QNAME
        N_SUPER
        N_THIS
        N_DELETE
 */

static EcNode *parseAssignmentExpression(EcCompiler *cp)
{
    EcNode      *np, *parent;
    int         subId;

    ENTER(cp);

    np = parseConditionalExpression(cp);
    if (np) {
        if (peekToken(cp) == T_ASSIGN) {
            getToken(cp);
            if (np->kind == N_OBJECT_LITERAL || np->kind == N_EXPRESSIONS) {
                fixDassign(cp, np );
            }
            subId = cp->token->subId;
            if (cp->token->groupMask & G_COMPOUND_ASSIGN) {
                np = rewriteCompoundAssignment(cp, subId, np, parseAssignmentExpression(cp));

            } else {
                parent = createNode(cp, N_ASSIGN_OP, NULL);
                np = createAssignNode(cp, np, parseAssignmentExpression(cp), parent);
            }
        }
    }
    return LEAVE(cp, np);
}


#if UNUSED
/*
    TODO - refactored
    CompoundAssignmentOperator (227)
        *=
        /=
        %=
        +=
        -=
        <<=
        >>=
        >>>=
        &=
        ^=
        |=
        &&=
        ||=

    Input (see above)

    AST
 */
static EcNode *parseCompoundAssignmentOperator(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    ListExpression (227)
        AssignmentExpression -allowList,b-
        ListExpression -b- , AssignmentExpression -allowList,b-

    Input
        x = ...

    AST
        N_EXPRESSIONS
 */
static EcNode *parseListExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_EXPRESSIONS, NULL);
    assure(np);
    do {
        np = appendNode(np, parseAssignmentExpression(cp));
    } while (np && getToken(cp) == T_COMMA);
    if (np) {
        putToken(cp);
    }
    return LEAVE(cp, np);
}


/*
    Pattern -a,b,g- (231)
        SimplePattern -a,b-g-
        ObjectPattern
        ArrayPattern

    Input
        Identifier
        {
        [

    AST
 */
static EcNode *parsePattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_LBRACKET:
        np = parseArrayPattern(cp);
        break;

    case T_LBRACE:
        np = parseObjectPattern(cp);
        break;

    default:
        np = parseSimplePattern(cp);
        break;
    }
    return LEAVE(cp, np);
}


/*
    SimplePattern -a,b,noExpr- (232)
        Identifier

    SimplePattern -a,b,noExpr- (233)
        LeftHandSideExpression -a,b-

    Input

    AST
        N_QNAME
        N_LIST_EXPRESSION
 */
static EcNode *parseSimplePattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = parseLeftHandSideExpression(cp);
    if (np == 0 && peekToken(cp) == T_ID) {
        np = parseIdentifier(cp);
    }
    return LEAVE(cp, np);
}


/*
    ObjectPattern -g- (234)
        { FieldListPattern }

    Input

    AST
 */
static EcNode *parseObjectPattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    if (getToken(cp) != T_LBRACE) {
        return LEAVE(cp, expected(cp, "{"));
    }
    np = parseFieldListPattern(cp);
    if (getToken(cp) != T_LBRACE) {
        return LEAVE(cp, expected(cp, "}"));
    }
    return LEAVE(cp, np);
}


/*
    FieldListPattern -g- (248)
        EMPTY
        FieldPattern
        FieldPattern , FieldPattern

    Input

    AST
 */
static EcNode *parseFieldListPattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_DASSIGN, NULL);
    while (1) {
        np = appendNode(np, parseFieldPattern(cp, np));
        if (peekToken(cp) == T_COMMA) {
            getToken(cp);
        } else {
            break;
        }
    }
    return LEAVE(cp, np);
}


/*
    FieldPattern -g- (251)
        FieldName
        FieldName : Pattern -noList,allowin,g-

    Input

    AST
 */
static EcNode *parseFieldPattern(EcCompiler *cp, EcNode *np)
{
    EcNode      *typeNode, *elt;

    ENTER(cp);
    elt = parseFieldName(cp);
    np = appendNode(np, elt);
    if (peekToken(cp) == ':') {
        getToken(cp);
        typeNode = parsePattern(cp);
        if (typeNode) {
            elt->typeNode = linkNode(np, typeNode);
        }
    }
    return LEAVE(cp, np);
}


/*
    ArrayPattern (240)
        [ ElementListPattern ]

    Input

    AST
 */
static EcNode *parseArrayPattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    if (getToken(cp) != T_LBRACKET) {
        return LEAVE(cp, expected(cp, "["));
    }
    np = parseElementListPattern(cp);
    if (getToken(cp) != T_RBRACKET) {
        return LEAVE(cp, expected(cp, "]"));
    }
    return LEAVE(cp, np);
}


/*
    ElementListPattern -g- (253)
        EMPTY
        ElementPattern
        , ElementListPattern
        ElementPattern , ElementListPattern

    Input

    AST
 */
static EcNode *parseElementListPattern(EcCompiler *cp)
{
    EcNode      *np, *elt;
    int         index;

    ENTER(cp);
    
    np = createNode(cp, N_DASSIGN, NULL);
    np->objectLiteral.isArray = 1;
    
    for (index = 0; np; index++) {
        if (peekToken(cp) != T_COMMA) {
            elt = createNode(cp, N_FIELD, NULL);
            elt->attributes |= EJS_TRAIT_READONLY;
            elt->field.varKind = KIND_CONST;
            elt->field.fieldKind = FIELD_KIND_VALUE;
            elt->field.expr = linkNode(np, parsePattern(cp));
            elt->qname.name = elt->field.expr->qname.name;
            elt->field.index = index;
            np = appendNode(np, elt);
        }
        if (peekToken(cp) == T_COMMA) {
            getToken(cp);
        } else {
            break;
        }
    }
    return LEAVE(cp, np);
}


/*
    TypedIdentifier (258)
        SimplePattern -noList,noin,noExpr-
        SimplePattern -a,b,noExpr- : TypeExpression

    Input

    AST
 */
static EcNode *parseTypedIdentifier(EcCompiler *cp)
{
    EcNode      *np, *typeNode;

    ENTER(cp);

    np = parseSimplePattern(cp);

    if (peekToken(cp) == T_COLON) {
        getToken(cp);
        typeNode = parseNullableTypeExpression(cp);
        if (typeNode) {
            np->typeNode = linkNode(np, typeNode);
            /* Accumulate EJS_TRAIT_MATCH | EJS_TRAIT_NULLABLE */
            np->attributes |= typeNode->attributes;
        } else {
            np = parseError(cp, "Expecting type");
        }
    }
    return LEAVE(cp, np);
}


/*
    TypedPattern (248)
        SimplePattern -a,b,noExpr-
        SimplePattern -a,b,noExpr- : NullableTypeExpression
        ObjectPattern -noExpr-
        ObjectPattern -noExpr- : TypeExpression
        ArrayPattern -noExpr-
        ArrayPattern -noExpr- : TypeExpression

    Input

    AST
 */
static EcNode *parseTypedPattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseSimplePattern(cp);
    if (peekToken(cp) == T_COLON) {
        getToken(cp);
        np->typeNode = linkNode(np, parseNullableTypeExpression(cp));
    }
    if (np && np->kind != N_QNAME) {
        return LEAVE(cp, unexpected(cp));
    }
    return LEAVE(cp, np);
}


/*
    NullableTypeExpression (266)
        null
        undefined
        TypeExpression
        TypeExpression ?    # Nullable
        TypeExpression !    # Non-Nullable (throws)
        TypeExpression ~    # Cast nulls

    Input

    AST
 */
static EcNode *parseNullableTypeExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_NULL:
    case T_UNDEFINED:
        np = createNode(cp, N_QNAME, tokenString(cp));
        np->name.isType = 1;
        break;

    default:
        np = parseTypeExpression(cp);
        if (peekToken(cp) == T_QUERY) {
            /* Allow Nulls */
            getToken(cp);
        } else if (cp->peekToken->tokenId == T_LOGICAL_NOT) {
            /* Don't allow nulls */
            getToken(cp);
            np->attributes |= EJS_TRAIT_THROW_NULLS;
        } else if (cp->peekToken->tokenId == T_TILDE) {
            /* Cast nulls to type */
            getToken(cp);
            np->attributes |= EJS_TRAIT_CAST_NULLS;
        } else {
            /* Default is same as Type! */
            np->attributes |= EJS_TRAIT_THROW_NULLS;
        }
        break;
    }
    return LEAVE(cp, np);
}


/*
    TypeExpression (271)
        FunctionType
        UnionType
        RecordType
        ArrayType
        PrimaryName

    Input
        function
        (
        {
        [
        Identifier

    AST
        N_QNAME
        N_DOT
 */
static EcNode *parseTypeExpression(EcCompiler *cp)
{
    Ejs         *ejs;
    EcNode      *np;

    ENTER(cp);

    ejs = cp->ejs;

    switch (peekToken(cp)) {
#if UNUSED
    case T_FUNCTION:
        np = appendNode(np, parseFunctionType(cp));
        break;

    case T_LPAREN:
        np = appendNode(np, parseUnionType(cp));
        break;

    case T_LBRACE:
        np = appendNode(np, parseRecordType(cp));
        break;

    case T_LBRACKET:
        appendNode(np, parseFunctionType(cp));
        break;
#endif

    case T_MUL:
        getToken(cp);
        np = createNode(cp, N_QNAME, EST(Object)->qname.name);
        np->name.isType = 1;
        break;

    case T_STRING:
    case T_ID:
        np = parsePrimaryName(cp);
        if (np) {
            np->name.isType = 1;
        }
        break;

    default:
        getToken(cp);
        np = unexpected(cp);
    }
    return LEAVE(cp, np);
}


#if UNUSED
/*
    FunctionType (276)
        function FunctionSignatureType

    Input sequnces
        function ...

    AST
 */
static EcNode *parseFunctionType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    FunctionSignatureType (277)
        TypeParameters ( ParametersType ) ResultType
        TypeParameters ( this : PrimaryName ) ResultType
        TypeParameters ( this : PrimaryName , NonemptyParameters )
                ResultType

    Input sequnces
        function ...

    AST
 */
static EcNode *parseFunctionSignatureType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    ParametersType (280)
        EMPTY
        NonemptyParametersType

    Input

    AST
 */
static EcNode *parseParametersType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    NonemptyParametersType (282)
        ParameterInitType
        ParameterInitType , NonemptyParametersType
        RestParameterType

    Input

    AST
 */
static EcNode *parseNonemptyParametersType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    ParameterInitType (285)
        ParameterType
        ParameterType =

    Input

    AST
 */
static EcNode *parseParameterInitType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    RestParameterType (288)
        ...
        ... ParameterType

    Input
        ...

    AST
 */
static EcNode *parseRestParameterType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    UnionType (290)
        ( TypeExpressionList )

    Input
        (

    AST
 */
static EcNode *parseUnionType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    RecordType (291)
        { FieldTypeList }

    Input
        {

    AST
 */
static EcNode *parseRecordType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    FieldTypeList (292)
        EMPTY
        NonemptyFieldTypeList

    Input

    AST
 */
static EcNode *parseFieldTypeList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    NonemptyFieldTypeList (294)
        FieldType
        FieldType , NonemptyFieldTypeList

    Input

    AST
 */
static EcNode *parseNonemptyFieldTypeList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    FieldType (296)
        FieldName : NullableTypeExpression

    Input

    AST
 */
static EcNode *parseFieldType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    ArrayType (297)
        [ ElementTypeList ]

    Input

    AST
 */
static EcNode *parseArrayType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_LBRACKET) {
        np = expected(cp, "[");
    } else {
        np = parseElementTypeList(cp);
        if (np) {
            if (getToken(cp) != T_LBRACKET) {
                np = expected(cp, "[");
            }
        }
    }

    return LEAVE(cp, np);
}


/*
    ElementTypeList (298)
        EMPTY
        NullableTypeExpression
        , ElementTypeList
        NullableTypeExpression , ElementTypeList

    Input

    AST
 */
static EcNode *parseElementTypeList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


#if FUTURE
/*
    TypeExpressionList (302)
        NullableTypeExpression
        TypeExpressionList , NullableTypeExpression

    Input

    AST
 */
static EcNode *parseTypeExpressionList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    Statement (289) -t,o-
        Block -t-
        BreakStatement Semicolon -o-
        ContinueStatement Semicolon -o-
        DefaultXMLNamespaceStatement Semicolon -o-
        DoStatement Semicolon -o-
        ExpresssionStatement Semicolon -o-
        ForStatement -o-
        IfStatement -o-
        LabeledStatement -o-
        LetStatement -o-
        ReturnStatement Semicolon -o-
        SwitchStatement
        SwitchTypeStatement
        ThrowStatement Semicolon -o-
        TryStatement
        WhileStatement -o-
        WithStatement -o-

    Input
        EMPTY
        {
        (
        .
        ..
        [
        (
        @
        break
        continue
        ?? DefaultXML
        do
        for
        if
        let
        return
        switch
        throw
        try
        while
        with
        null
        true
        false
        this
        function
        Identifier
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new

    AST
        N_BLOCK
        N_CONTINUE
        N_BREAK
        N_FOR
        N_FOR_IN
        N_HASH
        N_IF
        N_SWITCH
        N_THROW
        N_TRY
        N_WHILE

 */
static EcNode *parseStatement(EcCompiler *cp)
{
    EcNode      *np;
    int         expectSemi, tid;

    ENTER(cp);

    expectSemi = 0;
    np = 0;

    switch ((tid = peekToken(cp))) {
    case T_AT:
    case T_DELETE:
    case T_DIV:
    case T_DOT:
    case T_DOT_DOT:
    case T_FALSE:
    case T_FUNCTION:
    case T_LBRACKET:
    case T_LOGICAL_NOT:
    case T_LPAREN:
    case T_MINUS_MINUS:
    case T_NEW:
    case T_NUMBER:
    case T_NULL:
    case T_PLUS_PLUS:
    case T_STRING:
    case T_SUPER:
    case T_THIS:
    case T_TRUE:
    case T_TYPEOF:
    case T_RESERVED_NAMESPACE:
    case T_REQUIRE:                             /* require used as an identifier */
        np = parseExpressionStatement(cp);
        expectSemi++;
        break;

    case T_BREAK:
        np = parseBreakStatement(cp);
        expectSemi++;
        break;

    case T_CONTINUE:
        np = parseContinueStatement(cp);
        expectSemi++;
        break;

    case T_DO:
        np = parseDoStatement(cp);
        expectSemi++;
        break;

    case T_FOR:
        np = parseForStatement(cp);
        break;

    case T_HASH:
        np = parseHashStatement(cp);
        break;

    case T_ID:
        if (tid == T_ID && peekAheadToken(cp, 2) == T_COLON) {
            np = parseLabeledStatement(cp);
        } else {
            np = parseExpressionStatement(cp);
            expectSemi++;
        }
        break;

    case T_IF:
        np = parseIfStatement(cp);
        break;

    case T_LBRACE:
        np = parseBlockStatement(cp);
        break;

    case T_LET:
        np = parseLetStatement(cp);
        break;

    case T_RETURN:
        np = parseReturnStatement(cp);
        expectSemi++;
        break;

    case T_SWITCH:
        np = parseSwitchStatement(cp);
        break;

    case T_THROW:
        np = parseThrowStatement(cp);
        expectSemi++;
        break;

    case T_TRY:
        np = parseTryStatement(cp);
        break;

    case T_WHILE:
        np = parseWhileStatement(cp);
        break;

    case T_WITH:
        np = parseWithStatement(cp);
        break;

    default:
        getToken(cp);
        np = unexpected(cp);
    }

    if (np && expectSemi) {
        if (getToken(cp) != T_SEMICOLON) {
            if (np->loc.lineNumber < cp->token->loc.lineNumber || cp->token->tokenId == T_EOF || 
                    cp->token->tokenId == T_NOP || cp->token->tokenId == T_RBRACE) {
                putToken(cp);
            } else {
                np = unexpected(cp);
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    Substatement -o- (320)
        EmptyStatement
        Statement -o-

    Statement:
        Block -t-
        BreakStatement Semicolon -o-
        ContinueStatement Semicolon -o-
        DefaultXMLNamespaceStatement Semicolon -o-
        DoStatement Semicolon -o-
        ExpresssionStatement Semicolon -o-
        ForStatement -o-
        IfStatement -o-
        LabeledStatement -o-
        LetStatement -o-
        ReturnStatement Semicolon -o-
        SwitchStatement
        SwitchTypeStatement
        ThrowStatement Semicolon -o-
        TryStatement
        WhileStatement -o-
        WithStatement -o-

    Input
        EMPTY
        {
        (
        .
        ..
        [
        (
        @
        break
        continue
        ?? DefaultXML
        do
        for
        if
        let
        return
        switch
        throw
        try
        while
        with
        null
        true
        false
        this
        function
        Identifier
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new

    AST
        N_NOP
        N_BLOCK
        N_CONTINUE
        N_BREAK
        N_FOR
        N_FOR_IN
        N_IF
        N_SWITCH
        N_THROW
        N_TRY
        N_WHILE
 */

static EcNode *parseSubstatement(EcCompiler *cp)
{
    EcNode      *np;
    int         tid;

    ENTER(cp);

    np = 0;

    /*
        TODO: Missing: DefaultXML
     */
    switch ((tid = peekToken(cp))) {
    case T_AT:
    case T_BREAK:
    case T_CONTINUE:
    case T_DELETE:
    case T_DO:
    case T_DOT:
    case T_DOT_DOT:
    case T_FALSE:
    case T_FOR:
    case T_FUNCTION:
    case T_IF:
    case T_LBRACE:
    case T_LBRACKET:
    case T_LET:
    case T_LPAREN:
    case T_NEW:
    case T_NUMBER:
    case T_NULL:
    case T_RETURN:
    case T_STRING:
    case T_SUPER:
    case T_SWITCH:
    case T_THIS:
    case T_THROW:
    case T_TRUE:
    case T_TRY:
    case T_WHILE:
    case T_WITH:
        np = parseStatement(cp);
        break;

    case T_ID:
        if (peekAheadToken(cp, 2) == T_COLON) {
            /* Labeled expression */
            np = parseStatement(cp);
        } else {
            np = parseStatement(cp);
        }
        break;

    default:
        np = createNode(cp, N_NOP, NULL);
    }
    return LEAVE(cp, np);
}


#if UNUSED
/*
    Semicolon -abbrev- (322)
        ;
        VirtualSemicolon
        EMPTY

    Semicolon -noshortif- (325)
        ;
        VirtualSemicolon
        EMPTY

    Input

    AST
 */
static EcNode *parseSemicolon(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    EmptyStatement (33)
        ;

    Input
        EMPTY

    AST
        N_NOP
 */
static EcNode *parseEmptyStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = createNode(cp, N_NOP, NULL);
    return LEAVE(cp, np);
}


/*
    ExpressionStatement (331)
        [lookahead !function,{}] ListExpression -allowin-

    Input
        (
        .
        ..
        null
        true
        false
        this
        function
        delete
        Identifier
        [
        (
        @
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new

    AST
        N_CALL
        N_DELETE
        N_DOT
            left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
            right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
        N_EXPRESSIONS
        N_LITERAL
        N_NEW           (array / object literals)
        N_QNAME
        N_SUPER
        N_THIS
 */
static EcNode *parseExpressionStatement(EcCompiler *cp)
{
    EcNode      *np;
    int         tid;

    ENTER(cp);
    tid = peekToken(cp);
    if (tid == T_FUNCTION || tid == T_LBRACE) {
        np = createNode(cp, 0, NULL);
    } else {
        np = parseListExpression(cp);
    }
    return LEAVE(cp, np);
}


/*
    BlockStatement (318)
        Block

    Input
        {

    AST
        N_BLOCK
 */
static EcNode *parseBlockStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = parseBlock(cp);
    return LEAVE(cp, np);
}


/*
    LabeledStatement -o- (319)
        Identifier : Substatement

    Input

    AST
 */
static EcNode *parseLabeledStatement(EcCompiler *cp)
{
    getToken(cp);
    return parseError(cp, "Labeled statements are not yet implemented");
}


/*
    IfStatement -abbrev- (320)
        if ParenListExpression Substatement
        if ParenListExpression Substatement else Substatement

    IfStatement -full- (322)
        if ParenListExpression Substatement
        if ParenListExpression Substatement else Substatement

    IfStatement -noShortif- (324)
        if ParenListExpression Substatement else Substatement

    Input
        if ...

    AST
 */
static EcNode *parseIfStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_IF) {
        return LEAVE(cp, parseError(cp, "Expecting \"if\""));
    }
    if (peekToken(cp) != T_LPAREN) {
        getToken(cp);
        return LEAVE(cp, parseError(cp, "Expecting \"(\""));
    }
    np = createNode(cp, N_IF, NULL);
    if ((np->tenary.cond = linkNode(np, parseParenListExpression(cp))) == 0) {
        return LEAVE(cp, 0);
    }
    if ((np->tenary.thenBlock = linkNode(np, parseSubstatement(cp))) == 0) {
        return LEAVE(cp, 0);
    }
    if (peekToken(cp) == T_ELSE) {
        getToken(cp);
        np->tenary.elseBlock = linkNode(np, parseSubstatement(cp));
    }
    return LEAVE(cp, np);
}


/*
    SwitchStatement (328)
        switch ParenListExpression { CaseElements }
        switch type ( ListExpression -allowList,allowin- : TypeExpression )
                { TypeCaseElements }

    Input
        switch ...

    AST
        N_SWITCH
            N_EXPRESSIONS           ( ListExpression )
            N_CASE_ELEMENTS
 */
static EcNode *parseSwitchStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_SWITCH, NULL);

    if (getToken(cp) != T_SWITCH) {
        np = unexpected(cp);
    } else {
        if (peekToken(cp) != T_TYPE) {
            np = appendNode(np, parseParenListExpression(cp));
            if (getToken(cp) != T_LBRACE) {
                np = parseError(cp, "Expecting \"{\"");
            } else {
                np = appendNode(np, parseCaseElements(cp));
                if (getToken(cp) != T_RBRACE) {
                    np = parseError(cp, "Expecting \"{\"");
                }
            }

        } else {
#if FUTURE
            //  switch type
            getToken(cp);
            if (getToken(cp) != T_LPAREN) {
                np = parseError(cp, "Expecting \"(\"");
            } else {
                x = parseListExpression(cp);
                if (getToken(cp) != T_COLON) {
                    np = parseError(cp, "Expecting \":\"");
                } else {
                    x = parseTypeExpression(cp);
                    if (getToken(cp) != T_RPAREN) {
                        np = parseError(cp, "Expecting \")\"");
                    } else  if (getToken(cp) != T_LBRACE) {
                        np = parseError(cp, "Expecting \"{\"");
                    } else {
                        x = parseTypeCaseElements(cp);
                        if (getToken(cp) != T_RBRACE) {
                            np = parseError(cp, "Expecting \"}\"");
                        }
                    }
                }
                parseListExpression(cp);
            }
#endif
        }
    }
    return LEAVE(cp, np);
}


/*
    CaseElements (342)
        EMPTY
        CaseLabel
        CaseLabel CaseElementsPrefix CaseLabel
        CaseLabel CaseElementsPrefix Directive -abbrev-

    Refactored as:
        EMPTY
        CaseLable Directives
        CaseElements

    Input
        case
        default

    AST
        N_CASE_ELEMENTS
            N_CASE_LABEL: kind, expression
 */
static EcNode *parseCaseElements(EcCompiler *cp)
{
    EcNode      *np, *caseLabel, *directives;

    ENTER(cp);

    np = createNode(cp, N_CASE_ELEMENTS, NULL);

    while (np && (peekToken(cp) == T_CASE || cp->peekToken->tokenId == T_DEFAULT)) {
        caseLabel = parseCaseLabel(cp);
        directives = createNode(cp, N_DIRECTIVES, NULL);
        caseLabel = appendNode(caseLabel, directives);

        while (caseLabel && directives && peekToken(cp) != T_CASE && cp->peekToken->tokenId != T_DEFAULT) {
            if (cp->peekToken->tokenId == T_RBRACE) {
                break;
            }
            directives = appendNode(directives, parseDirective(cp));
        }
        np = appendNode(np, caseLabel);
    }
    return LEAVE(cp, np);
}


#if UNUSED && NOT_REQUIRED
/*
    CaseElementsPrefix (346)
        EMPTY
        CaseElementsPrefix CaseLabel
        CaseElementsPrefix Directive -full-

    Input

    AST
 */
static EcNode *parseCaseElementsPrefix(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    CaseLabel (349)
        case ListExpression -allowin- :
        default :

    Input
        case .. :
        default :

    AST
        N_CASE_LABEL  kind, expression
 */
static EcNode *parseCaseLabel(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;

    if (peekToken(cp) == T_CASE) {
        getToken(cp);
        np = createNode(cp, N_CASE_LABEL, NULL);
        np->caseLabel.kind = EC_SWITCH_KIND_CASE;
        if ((np->caseLabel.expression = linkNode(np, parseListExpression(cp))) == 0) {
            return LEAVE(cp, np);
        }
    } else if (cp->peekToken->tokenId == T_DEFAULT) {
        getToken(cp);
        np = createNode(cp, N_CASE_LABEL, NULL);
        np->caseLabel.kind = EC_SWITCH_KIND_DEFAULT;
    }
    if (getToken(cp) != T_COLON) {
        np = expected(cp, ":");
    }
    return LEAVE(cp, np);
}


#if UNUSED
/*
    TypeCaseElements (351)
        TypeCaseElement
        TypeCaseElement TypeCaseElement

    Input

    AST
 */
static EcNode *parseTypeCaseElements(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    TypeCaseElement (353)
        case ( TypedPattern -noList,noIn- ) Block -local-
        default Block -local-

    Input

    AST
 */
static EcNode *parseTypeCaseElement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    DoStatement (355)
        do Substatement -abbrev- while ParenListExpresison

    Input
        do

    AST
        N_FOR
 */
static EcNode *parseDoStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_DO, NULL);

    if (getToken(cp) != T_DO) {
        return LEAVE(cp, unexpected(cp));
    }
    np->forLoop.body = linkNode(np, parseSubstatement(cp));

    if (getToken(cp) != T_WHILE) {
        np = expected(cp, "while");
    } else {
        np->forLoop.cond = linkNode(np, parseParenListExpression(cp));
    }
    return LEAVE(cp, np);
}


/*
    WhileStatement (356)
        while ParenListExpresison Substatement -o-

    Input
        while

    AST
        N_FOR
 */
static EcNode *parseWhileStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_WHILE) {
        return LEAVE(cp, parseError(cp, "Expecting \"while\""));
    }
    /*
        Convert into a "for" AST
     */
    np = createNode(cp, N_FOR, NULL);
    np->forLoop.cond = linkNode(np, parseParenListExpression(cp));
    np->forLoop.body = linkNode(np, parseSubstatement(cp));
    return LEAVE(cp, np);
}


/*
    ForStatement -o- (357)
        for ( ForInitializer ; OptionalExpression ; OptionalExpression )
                Substatement
        for ( ForInBinding in ListExpression -allowin- ) Substatement
        for each ( ForInBinding in ListExpression -allowin- ) Substatement

    Where:

    ForIntializer (360)
        EMPTY
        ListExpression -noin-
        VariableDefinition -noin-

    ForInBinding (363)
        Pattern -allowList,noIn,allowExpr-
        VariableDefinitionKind VariableBinding -allowList,noIn-

    VariableDefinition -b- (429)
        VariableDefinitionKind VariableBindingList -allowList,b-

    VariableDefinitionKind (430)
        const
        let
        let const
        var

    MemberExpression Input tokens
        null
        true
        false
        this
        function
        Identifier
        {
        [
        (
        @
        NumberLiteral
        StringLiteral
        RegularExpression
        XMLInitializer: <!--, [CDATA, <?, <
        super
        new

    Input
        for ( 'const|let|let const|var' '[|{'

    AST
        N_FOR
        N_FOR_IN
 */
static EcNode *parseForStatement(EcCompiler *cp)
{
    EcNode      *np, *initializer, *body, *iterGet, *block, *callGet, *dot;
    Ejs         *ejs;
    int         each, forIn;

    ENTER(cp);

    ejs = cp->ejs;
    initializer = 0;
    np = 0;
    forIn = 0;
    each = 0;

    if (getToken(cp) != T_FOR) {
        return LEAVE(cp, parseError(cp, "Expecting \"for\""));
    }
    if (peekToken(cp) == T_EACH) {
        each++;
        getToken(cp);
    }
    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \"(\""));
    }
    if (peekToken(cp) == T_ID && peekAheadToken(cp, 2) == T_IN) {
        /*
            For in forces the variable to be a let scoped var
         */
        initializer = createNode(cp, N_VAR_DEFINITION, NULL);
        if (initializer) {
            initializer->def.varKind = KIND_LET;
            initializer = parseVariableBindingList(cp, initializer, 0);
        }

    } else if (peekToken(cp) == T_CONST || cp->peekToken->tokenId == T_LET || cp->peekToken->tokenId == T_VAR) {
        initializer = parseVariableDefinition(cp, 0);

    } else if (cp->peekToken->tokenId != T_SEMICOLON) {
        cp->state->noin = 1;
        initializer = parseListExpression(cp);
    }
    if (initializer == 0 && cp->error) {
        return LEAVE(cp, 0);
    }
    if (initializer && mprGetListLength(initializer->children) > 2) {
        return LEAVE(cp, parseError(cp, "Too many iteration variables"));
    }
    if (getToken(cp) == T_SEMICOLON) {
        forIn = 0;
        np = createNode(cp, N_FOR, NULL);
        np->forLoop.initializer = linkNode(np, initializer);
        if (peekToken(cp) != T_SEMICOLON) {
            np->forLoop.cond = linkNode(np, parseOptionalExpression(cp));
        }
        if (getToken(cp) != T_SEMICOLON) {
            np = parseError(cp, "Expecting \";\"");
        } else if (peekToken(cp) != T_RPAREN) {
            np->forLoop.perLoop = linkNode(np, parseOptionalExpression(cp));
        }

    } else if (cp->token->tokenId == T_IN) {
        forIn = 1;
        np = createNode(cp, N_FOR_IN, NULL);
        np->forInLoop.iterVar = linkNode(np, initializer);

        /*
            Create a "listExpression.get/values" node
         */
        dot = createNode(cp, N_DOT, NULL);
        iterGet = appendNode(dot, parseListExpression(cp));
        iterGet = appendNode(iterGet, createNameNode(cp, N(EJS_ITERATOR_NAMESPACE, (each) ? "getValues" : "get")));

        /*
            Create a call node for "get"
         */
        callGet = createNode(cp, N_CALL, NULL);
        callGet = appendNode(callGet, iterGet);
        callGet = appendNode(callGet, createNode(cp, N_ARGS, NULL));
        np->forInLoop.iterGet = linkNode(np, callGet);

        np->forInLoop.iterNext = linkNode(np, createNode(cp, N_NOP, NULL));

        if (np->forInLoop.iterVar == 0 || np->forInLoop.iterGet == 0) {
            return LEAVE(cp, 0);
        }

    } else {
        return LEAVE(cp, unexpected(cp));
    }
    if (getToken(cp) != T_RPAREN) {
        np = parseError(cp, "Expecting \")\"");
    }
    body = linkNode(np, parseSubstatement(cp));
    if (body == 0) {
        return LEAVE(cp, body);
    }

    /*
        Fixup the body block and move it outside the entire for loop.
     */
    if (body->kind == N_BLOCK) {
        block = body;
        body = removeNode(block, block->left);
    } else {
        block = createNode(cp, N_BLOCK, NULL);
    }
    if (forIn) {
        np->forInLoop.body = linkNode(np, body);
        np->forInLoop.each = each;
    } else {
        if (each) {
            return LEAVE(cp, parseError(cp, "\"for each\" can only be used with \"for .. in\""));
        }
        assure(np != body);
        np->forLoop.body = linkNode(np, body);
    }

    /*
        Now make the for loop node a child of the outer block. Block will initially be a child of np, so must re-parent first
     */
    assure(block != np);
    np = appendNode(block, np);
    return LEAVE(cp, np);
}


#if UNUSED
/*
    ForIntializer (360)
        EMPTY
        ListExpression -noin-
        VariableDefinition -noin-

    Input

    AST
 */
static EcNode *parseForInitializer(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    ForInBinding (363)
        Pattern -allowList,noIn,allowExpr-
        VariableDefinitionKind VariableBinding -allowList,noIn-

    Input

    AST
 */
static EcNode *parseForInBinding(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    HashStatement (EJS)
        # ListExpression

    Input
        # expression directive

    AST
        N_HASH
 */
static EcNode *parseHashStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_HASH) {
        return LEAVE(cp, parseError(cp, "Expecting \"#\""));
    }
    np = createNode(cp, N_HASH, NULL);
    np->hash.expr = linkNode(np, parseListExpression(cp));
    np->hash.body = linkNode(np, parseDirective(cp));
    return LEAVE(cp, np);
}


/*
    LetStatement (367)
        let ( LetBindingList ) Substatement -o-

    Input

    AST
 */
static EcNode *parseLetStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    WithStatement -o- (368)
        with ( ListExpression -allowin- ) Substatement -o-
        with ( ListExpression -allowin- : TypeExpression ) Substatement -o-

    Input

    AST
 */
static EcNode *parseWithStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_WITH) {
        return LEAVE(cp, expected(cp, "with"));
    }
    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, expected(cp, "("));
    }
    np = createNode(cp, N_WITH, NULL);
    np->with.object = linkNode(np, parseListExpression(cp));

    if (getToken(cp) != T_RPAREN) {
        return LEAVE(cp, expected(cp, ")"));
    }
    np->with.statement = linkNode(np, parseSubstatement(cp));
    return LEAVE(cp, np);
}


/*
    ContinueStatement (370)
        continue
        continue [no line break] Identifier

    Input
        continue

    AST
        N_CONTINUE
 */
static EcNode *parseContinueStatement(EcCompiler *cp)
{
    EcNode      *np;
    int         lineNumber;

    ENTER(cp);

    if (getToken(cp) != T_CONTINUE) {
        np = expected(cp, "continue");
    } else {
        np = createNode(cp, N_CONTINUE, NULL);
        lineNumber = cp->token->loc.lineNumber;
        if (peekToken(cp) == T_ID && lineNumber == cp->peekToken->loc.lineNumber) {
            np = appendNode(np, parseIdentifier(cp));
        }
    }
    return LEAVE(cp, np);
}


/*
    BreakStatement (372)
        break
        break [no line break] Identifier

    Input
        break

    AST
        N_BREAK
 */
static EcNode *parseBreakStatement(EcCompiler *cp)
{
    EcNode      *np;
    int         lineNumber;

    ENTER(cp);

    if (getToken(cp) != T_BREAK) {
        np = expected(cp, "break");
    } else {
        np = createNode(cp, N_BREAK, NULL);
        lineNumber = cp->token->loc.lineNumber;
        if (peekToken(cp) == T_ID && lineNumber == cp->peekToken->loc.lineNumber) {
            np = appendNode(np, parseIdentifier(cp));
        }
    }
    return LEAVE(cp, np);
}


/*
    ReturnStatement (374)
        return
        return [no line break] ListExpression -allowin-

    Input
        return ...

    AST
        N_RETURN
 */
static EcNode *parseReturnStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_RETURN) {
        np = unexpected(cp);
    } else {
        if (cp->state->currentFunctionNode == 0) {
            np = parseError(cp, "Return statement outside function");
        } else {
            np = createNode(cp, N_RETURN, NULL);
            if (peekToken(cp) != T_SEMICOLON && np->loc.lineNumber == cp->peekToken->loc.lineNumber) {
                np = appendNode(np, parseListExpression(cp));
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    ThrowStatement (376)
        throw ListExpression -allowin-

    Input
        throw ...

    AST
 */
static EcNode *parseThrowStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_THROW) {
        return LEAVE(cp, unexpected(cp));
    }
    np = createNode(cp, N_THROW, NULL);
    np = appendNode(np, parseListExpression(cp));
    return LEAVE(cp, np);
}


/*
    TryStatement (377)
        try Block -local- CatchClauses
        try Block -local- CatchClauses finally Block -local-
        try Block -local- finally Block -local-

    Input
        try ...

    AST
 */
static EcNode *parseTryStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    /*
        Just ignore try / catch for now
     */
    if (getToken(cp) != T_TRY) {
        return LEAVE(cp, unexpected(cp));
    }
    np = createNode(cp, N_TRY, NULL);
    if (np) {
        np->exception.tryBlock = linkNode(np, parseBlock(cp));
        if (peekToken(cp) == T_CATCH) {
            np->exception.catchClauses = linkNode(np, parseCatchClauses(cp));
        }
        if (peekToken(cp) == T_FINALLY) {
            getToken(cp);
            np->exception.finallyBlock = linkNode(np, parseBlock(cp));
        }
    }
    return LEAVE(cp, np);
}


/*
    CatchClauses (380)
        CatchClause
        CatchClauses CatchClause

    Input
        catch
 */
static EcNode *parseCatchClauses(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) != T_CATCH) {
        getToken(cp);
        return LEAVE(cp, unexpected(cp));
    }
    np = createNode(cp, N_CATCH_CLAUSES, NULL);
    do {
        np = appendNode(np, parseCatchClause(cp));
    } while (peekToken(cp) == T_CATCH);
    return LEAVE(cp, np);
}


/*
    CatchClause (382)
        catch ( Parameter ) Block -local-

    Input
        catch

    AST
        T_CATCH
 */
static EcNode *parseCatchClause(EcCompiler *cp)
{
    EcNode      *np, *block, *arg, *varDef, *assign, *name;

    ENTER(cp);


    if (getToken(cp) != T_CATCH) {
        return LEAVE(cp, unexpected(cp));
    }

    np = createNode(cp, N_CATCH, NULL);

    /*
        EJS enhancement: allow no (Parameter)
     */
    varDef = 0;
    arg = 0;
    if (peekToken(cp) == T_LPAREN) {
        getToken(cp);
        varDef = parseParameter(cp, 0);
        if (getToken(cp) != T_RPAREN) {
            return LEAVE(cp, unexpected(cp));
        }
        if (varDef) {
            assure(varDef->kind == N_VAR_DEFINITION);
            varDef->def.varKind = KIND_LET;
            arg = varDef->left;
            removeNode(varDef, arg);
            assure(arg->kind == N_QNAME);
            arg->kind = N_VAR;
            arg->name.varKind = KIND_LET;
            arg->kindName = "n_var";
            arg->qname.space = cp->state->nspace;

            /* Create assignment node */
            name = createNode(cp, N_QNAME, arg->qname.name);
            assign = appendNode(createNode(cp, N_ASSIGN_OP, NULL), name);
            assign = appendNode(assign, createNode(cp, N_CATCH_ARG, NULL));
            arg = appendNode(arg, assign);
            varDef = appendNode(varDef, arg);
#if UNUSED
            parent = createNode(cp, N_ASSIGN_OP, NULL);
            arg = createAssignNode(cp, arg, createNode(cp, N_CATCH_ARG, NULL), parent);
            varDef = appendNode(varDef, arg);
#endif
        }
    }
    np->catchBlock.arg = varDef;

    block = parseBlock(cp);
    if (block) {
        if (varDef) {
            block = insertNode(block, varDef, 0);
        }
    }
    np = appendNode(np, block);
    return LEAVE(cp, np);
}


/* -t- == global, class, interface, local */

/*
    Directives -t- (367)
        EMPTY
        DirectivesPrefix Directive -t,abbrev-

    Input
        #
        import
        use
        {
        (
        break
        continue
        ?? DefaultXML
        do
        ?? ExpressionS
        for
        if
        label :
        let
        new
        return
        switch
        throw
        try
        while
        with
        internal, intrinsic, private, protected, public
        final, native, override, prototype, static
        [ attribute AssignmentExpression
        Identifier
        const
        let
        var
        function
        interface
        namespace
        type
        module

    AST
        N_DIRECTIVES
 */
static EcNode *parseDirectives(EcCompiler *cp)
{
    EcNode      *np;
    EcState     *saveState;
    EcState     *state;

    ENTER(cp);

    np = createNode(cp, N_DIRECTIVES, NULL);
    state = cp->state;
    state->topVarBlockNode = np;

    saveState = cp->directiveState;
    cp->directiveState = state;
    state->blockNestCount++;

    do {
        switch (peekToken(cp)) {
        case T_ERR:
            cp->directiveState = saveState;
            getToken(cp);
            return LEAVE(cp, unexpected(cp));

        case T_EOF:
            cp->directiveState = saveState;
            return LEAVE(cp, np);

        case T_REQUIRE:
            if (peekAheadToken(cp, 2) != T_ID && peekAheadToken(cp, 2) != T_STRING) {
                np = appendNode(np, parseDirective(cp));
            } else {
                np = appendNode(np, parseDirectivesPrefix(cp));
            }
            break;

        case T_USE:
            np = appendNode(np, parseDirectivesPrefix(cp));
            break;

        case T_RBRACE:
            if (state->blockNestCount == 1) {
                getToken(cp);
            }
            cp->directiveState = saveState;
            return LEAVE(cp, np);

        case T_SEMICOLON:
            getToken(cp);
            break;

        case T_ATTRIBUTE:
        case T_BREAK:
        case T_CLASS:
        case T_CONST:
        case T_CONTINUE:
        case T_DELETE:
        case T_DIV:
        case T_DO:
        case T_DOT:
        case T_FALSE:
        case T_FOR:
        case T_FINAL:
        case T_FUNCTION:
        case T_HASH:
        case T_ID:
        case T_IF:
        case T_INTERFACE:
        case T_MINUS_MINUS:
        case T_LBRACKET:
        case T_LBRACE:
        case T_LPAREN:
        case T_LET:
        case T_NAMESPACE:
        case T_NATIVE:
        case T_NEW:
        case T_LOGICAL_NOT:
        case T_NUMBER:
        case T_RESERVED_NAMESPACE:
        case T_RETURN:
        case T_PLUS_PLUS:
        case T_STRING:
        case T_SUPER:
        case T_SWITCH:
        case T_THIS:
        case T_THROW:
        case T_TRUE:
        case T_TRY:
        case T_TYPEOF:
        case T_VAR:
        case T_WHILE:
        case T_MODULE:
        case T_WITH:
            np = appendNode(np, parseDirective(cp));
            break;

        case T_NOP:
            if (state->blockNestCount == 1) {
                getToken(cp);
                break;
            } else {
                /*
                    NOP tokens are injected when reading from the console. If nested, eat all input and continue.
                 */
                ecResetInput(cp);
            }
            break;

        default:
            getToken(cp);
            np = unexpected(cp);
            cp->directiveState = saveState;
            return LEAVE(cp, np);
        }

        if (cp->error && !cp->fatalError) {
            np = ecResetError(cp, np, 1);
        }
    } while (np && (!cp->interactive || state->blockNestCount > 1));

    cp->directiveState = saveState;
    return LEAVE(cp, np);
}


/*
    DirectivesPrefix -t- (369)
        EMPTY
        Pragmas
        DirectivesPrefix Directive -t,full-

    Rewritten as:
        DirectivesPrefix

    Input
        use
        import

    AST
        N_PRAGMAS
 */
static EcNode *parseDirectivesPrefix(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_PRAGMAS, NULL);
    do {
        switch (peekToken(cp)) {
        case T_ERR:
            return LEAVE(cp, unexpected(cp));

        case T_EOF:
            return LEAVE(cp, np);

        case T_USE:
        case T_REQUIRE:
            np = parsePragmas(cp, np);
            break;

        default:
            return LEAVE(cp, np);
        }
        if (!(peekToken(cp) == T_REQUIRE && (peekAheadToken(cp, 2) == T_ID || peekAheadToken(cp, 2) == T_STRING))) {
            break;
        }
    } while (np);
    return LEAVE(cp, np);
}


/*
    Scan ahead and see if this is an annotatable directive
 */
static int isAttribute(EcCompiler *cp)
{
    int     i, tid;

    /*
        Assume we have just seen an ID. Handle the following patterns:
            nspace var
            nspace function
            nspace class
            nspace interface
            nspace let
            nspace const
            nspace type
            nspace namespace
            a.nspace namespace
            a.b.c.nspace::nspace namespace
     */
    for (i = 2; i < EC_MAX_LOOK_AHEAD + 2; i++) {
        peekAheadToken(cp, i);
        tid = cp->peekToken->tokenId;
        switch (tid) {
        case T_ATTRIBUTE:
        case T_CLASS:
        case T_CONST:
        case T_FUNCTION:
        case T_INTERFACE:
        case T_LET:
        case T_MUL:
        case T_NAMESPACE:
        case T_RESERVED_NAMESPACE:
        case T_TYPE:
        case T_VAR:
            return 1;

        case T_COLON_COLON:
        case T_DOT:
            break;

        default:
            return 0;
        }

        /*
            Just saw a "." or "::".  Make sure this is part of a PropertyName
         */
        tid = peekAheadToken(cp, ++i);
        if (tid != T_ID && tid != T_RESERVED_NAMESPACE && tid == T_MUL && tid != T_STRING && tid != T_NUMBER &&
                tid != T_LBRACKET && !(cp->peekToken->groupMask & G_RESERVED)) {
            return 0;
        }
    }
    return 0;
}


/*
    Directive -t,o- (372)
        EmptyStatement
        Statement
        AnnotatableDirective -t,o-

    Input
        #
        {
        break
        continue
        ?? DefaultXML
        do
        ?? Expressions
        for
        if
        label :
        let
        return
        switch
        throw
        try
        while
        with
        *
        internal, intrinsic, private, protected, public
        final, native, override, prototype, static
        [ attribute AssignmentExpression
        Identifier
        const
        let
        var
        function
        interface
        namespace
        type

    AST
        N_BLOCK
        N_CONTINUE
        N_BREAK
        N_FOR
        N_FOR_IN
        N_HASH
        N_IF
        N_SWITCH
        N_THROW
        N_TRY
        N_WHILE
 */
static EcNode *parseDirective(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_EOF:
        getToken(cp);
        return LEAVE(cp, 0);

    case T_ERR:
        getToken(cp);
        return LEAVE(cp, unexpected(cp));

    /* EmptyStatement */
    case T_SEMICOLON:
        getToken(cp);
        np = parseEmptyStatement(cp);
        break;

    /* Statement */
    /*
        TBD -- missing:
            - DefaultXMLNamespaceStatement
            - ExpressionStatement
            - LabeledStatement
     */
    case T_LBRACE:
    case T_BREAK:
    case T_CONTINUE:
    case T_DELETE:
    case T_DO:
    case T_FOR:
    case T_HASH:
    case T_IF:
    case T_RETURN:
    case T_SUPER:
    case T_SWITCH:
    case T_THROW:
    case T_TRY:
    case T_WHILE:
    case T_WITH:
        np = parseStatement(cp);
        break;

    case T_ID:
        if (isAttribute(cp)) {
            np = parseAnnotatableDirective(cp, 0);
        } else {
            np = parseStatement(cp);
        }
        break;

    case T_RESERVED_NAMESPACE:
        if (peekAheadToken(cp, 2) == T_COLON_COLON) {
            np = parseStatement(cp);
            break;
        }
        /* Fall through */
                
    /* AnnotatableDirective */
    case T_ATTRIBUTE:
    case T_CLASS:
    case T_CONST:
    case T_FUNCTION:
    case T_INTERFACE:
    case T_LET:
    case T_MUL:
    case T_NAMESPACE:
    case T_TYPE:
    case T_MODULE:
    case T_VAR:
        np = parseAnnotatableDirective(cp, 0);
        break;

    case T_STRING:
        //  TDOO - should we test let ...?
        if (peekAheadToken(cp, 2) == T_VAR || peekAheadToken(cp, 2) == T_CLASS || peekAheadToken(cp, 2) == T_FUNCTION) {
            np = parseAnnotatableDirective(cp, 0);
        } else {
            np = parseStatement(cp);
        }
        break;

#if FUTURE
    /* IncludeDirective */
    case T_INCLUDE:
        np = parseIncludeStatement(cp);
        break;
#endif

    default:
        np = parseStatement(cp);
    }
    return LEAVE(cp, np);
}


/*
    AnnotatableDirective -global,o- (375)
        Attributes [no line break] AnnotatableDirective -t,o-
        VariableDefinition -allowin- Semicolon -o-
        FunctionDefinition -global-
        ClassDefinition
        InterfaceDefintion
        NamespaceDefinition Semicolon -o-
        TypeDefinition Semicolon
        PackageDefinition
        ModuleDefinition

    AnnotatableDirective -interface,o- (384)
        Attributes [no line break] AnnotatableDirective -t,o-
        FunctionDeclaration Semicolon -o-
        TypeDefinition Semicolon -o-

    AnnotatableDirective -t,o- (387)
        Attributes [no line break] AnnotatableDirective -t,o-
        VariableDefinition -allowin- Semicolon -o-
        FunctionDeclaration -t-
        NamespaceDefintion Semicolon -o-
        TypeDefinition Semicolon -o-

    Input
        internal, intrinsic, private, protected, public
        final, native, override, prototype, static
        [ attribute AssignmentExpression
        Identifier
        const
        let
        var
        function
        interface
        namespace
        type
        module
        package

    AST
        N_CLASS
        N_FUNCTION
        N_QNAME
        N_NAMESPACE
        N_VAR_DEFINITION
        N_MODULE??
 */
static EcNode *parseAnnotatableDirective(EcCompiler *cp, EcNode *attributes)
{
    EcState     *state;
    EcNode      *nextAttribute, *np;
    int         expectSemi;

    ENTER(cp);

    np = 0;
    expectSemi = 0;
    state = cp->state;

    switch (peekToken(cp)) {

    /* Attributes AnnotatableDirective */
    case T_STRING:
    case T_ATTRIBUTE:
    case T_RESERVED_NAMESPACE:
    case T_ID:
        nextAttribute = parseAttribute(cp);
        if (nextAttribute) {
            getToken(cp);
            if (nextAttribute->loc.lineNumber < cp->token->loc.lineNumber) {
                /* Must be no line break after the attribute */
                return LEAVE(cp, unexpected(cp));
            }
            putToken(cp);
            /*
                Aggregate the attributes and pass in. Must do this to allow "private static var a, b, c"
             */
            if (attributes) {
                nextAttribute->attributes |= attributes->attributes;
                if (attributes->qname.space && nextAttribute->qname.space) {
                    return LEAVE(cp, parseError(cp, "Cannot define multiple namespaces for directive"));
                }
                if (attributes->qname.space) {
                    nextAttribute->qname.space = attributes->qname.space;
                }
            }
            np = parseAnnotatableDirective(cp, nextAttribute);
        }
        break;

    case T_CONST:
    case T_LET:
    case T_VAR:
        np = parseVariableDefinition(cp, attributes);
        expectSemi++;
        break;

    case T_FUNCTION:
        if (state->inInterface) {
            np = parseFunctionDeclaration(cp, attributes);
        } else {
            np = parseFunctionDefinition(cp, attributes);
        }
        break;

    case T_CLASS:
#if OLD && UNUSED
        if (state->inClass == 0) {
            /* Nested classes are not supported */
            np = parseClassDefinition(cp, attributes);
        } else {
            getToken(cp);
            np = unexpected(cp);
        }
#else
            np = parseClassDefinition(cp, attributes);
#endif
        break;

    case T_INTERFACE:
        if (state->inClass == 0) {
            np = parseInterfaceDefinition(cp, attributes);
        } else {
            np = unexpected(cp);
        }
        break;

    case T_NAMESPACE:
        np = parseNamespaceDefinition(cp, attributes);
        expectSemi++;
        break;

    case T_TYPE:
        np = parseTypeDefinition(cp, attributes);
        expectSemi++;
        break;

    case T_MODULE:
        np = parseModuleDefinition(cp);
        break;

    default:
        getToken(cp);
        np = parseError(cp, "Unknown directive \"%s\"", cp->token->text);
    }
    if (np && expectSemi) {
        if (getToken(cp) != T_SEMICOLON) {
            if (np->loc.lineNumber < cp->token->loc.lineNumber || cp->token->tokenId == T_EOF) {
                putToken(cp);
            } else if (cp->token->tokenId != T_NOP) {
                np = unexpected(cp);
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    Attribute -global- (391)
        NamespaceAttribute
        dynamic
        final
        native
        [ AssignmentExpression allowList,allowIn]

    Attribute -class- (396)
        NamespaceAttribute
        final
        native
        override
        prototype
        static
        [ AssignmentExpression allowList,allowIn]

    Attribute -interface- (419)
        NamespaceAttribute

    Attribute -local- (420)
        NamespaceAttribute

    Input -common-
        NamespaceAttribute
            Path . Identifier
            Identifier
            public
            internal
        final
        native
        [

    Input -global-
        NamespaceAttribute -global-
            intrinsic
        dynamic

    Input -class-
        NamespaceAttribute -class-
            intrinsic
            private
            protected
        override
        prototype
        static

    AST
        N_ATTRIBUTES
            attribute
                flags
            attributes
 */
static EcNode *parseAttribute(EcCompiler *cp)
{
    EcNode      *np;
    EcState     *state;
    int         inClass, inInterface, subId;

    ENTER(cp);

    state = cp->state;
    inClass =state->inClass ? 1 : 0;
    inInterface = state->inInterface ? 1 : 0;
    np = 0;

    if (state->inFunction) {
        np = parseNamespaceAttribute(cp);
        return LEAVE(cp, np);
    }

    peekToken(cp);
    subId = cp->peekToken->subId;
    switch (cp->peekToken->tokenId) {
    case T_ID:
    case T_RESERVED_NAMESPACE:
    case T_STRING:
        if (!inClass && (subId == T_PRIVATE || subId ==  T_PROTECTED)) {
            getToken(cp);
            return LEAVE(cp, parseError(cp, "Cannot use private or protected in this context"));
        }
        np = parseNamespaceAttribute(cp);
        break;

    case T_ATTRIBUTE:
        getToken(cp);
        np = createNode(cp, N_ATTRIBUTES, NULL);
        switch (cp->token->subId) {
        case T_DYNAMIC:
            if (inClass || inInterface) {
                np = unexpected(cp);
            } else {
                np->attributes |= EJS_TYPE_DYNAMIC_INSTANCES;
            }
            break;

        case T_FINAL:
            np->attributes |= EJS_TYPE_FINAL;
            break;

        case T_NATIVE:
            np->attributes |= EJS_PROP_NATIVE;
            break;

#if UNUSED
        case T_ORPHAN:
            if (inClass || inInterface) {
                np = unexpected(cp);
            } else {
                np->attributes |= EJS_TYPE_ORPHAN;
            }
            break;
#endif
        case T_OVERRIDE:
            if (inClass || inInterface) {
                np->attributes |= EJS_FUN_OVERRIDE;
            } else {
                np = unexpected(cp);
            }
            break;

#if UNUSED
        case T_SHARED:
            np->attributes |= EJS_PROP_SHARED;
            break;
#endif

        case T_STATIC:
            if (inClass || inInterface) {
                np->attributes |= EJS_PROP_STATIC;
            } else {
                np = unexpected(cp);
            }
            break;

        case T_ENUMERABLE:
            np->attributes |= EJS_PROP_ENUMERABLE;
            break;

        default:
            np = parseError(cp, "Unknown or invalid attribute in this context %s", cp->token->text);
        }
        break;

    case T_LBRACKET:
        np = appendNode(np, parseAssignmentExpression(cp));
        break;

    default:
        np = parseError(cp, "Unknown or invalid attribute in this context %s", cp->token->text);
        break;
    }
    return LEAVE(cp, np);
}


/*
    NamespaceAttribute -global- (405)
        public
        internal
        intrinsic
        PrimaryName

    NamespaceAttribute -class- (409)
        ReservedNamespace
        PrimaryName

    Input -common-
        Identifier
        internal
        public

    Input -global-
        intrinsic

    Input -class-
        intrinsic
        private
        protected

    AST
        N_ATTRIBUTES
            attribute
                flags
            left: N_QNAME | N_DOT
 */
static EcNode *parseNamespaceAttribute(EcCompiler *cp)
{
    EcNode      *np, *qualifier;
    int         inClass, subId;

    ENTER(cp);

    inClass = (cp->state->inClass) ? 1 : 0;
    peekToken(cp);
    subId = cp->peekToken->subId;

    np = createNode(cp, N_ATTRIBUTES, NULL);
    np->loc.lineNumber = cp->peekToken->loc.lineNumber;

    switch (cp->peekToken->tokenId) {
    case T_RESERVED_NAMESPACE:
        if (!inClass && (subId == T_PRIVATE || subId ==  T_PROTECTED)) {
            getToken(cp);
            return LEAVE(cp, unexpected(cp));
        }
        qualifier = parseReservedNamespace(cp);
        np->attributes = qualifier->attributes;
        np->specialNamespace = qualifier->specialNamespace;
        np->qname.space = qualifier->qname.space;
        break;

    case T_ID:
        qualifier = parsePrimaryName(cp);
        if (qualifier->kind == N_QNAME) {
            np->attributes = qualifier->attributes;
            np->qname.space = qualifier->qname.name;
        } else {
            /*
                This is a N_DOT expression compile-time constant expression.
             */
            assure(0);
#if UNUSED
            np->qualifierNode = linkNode(np, qualifier);
#endif
        }
        break;

    case T_STRING:
        getToken(cp);
        np->qname.space = tokenString(cp);
        np->literalNamespace = 1;
        break;

    case T_ATTRIBUTE:
        getToken(cp);
        np = parseError(cp, "Attribute \"%s\" not supported on local variables", cp->token->text);
        break;

    default:
        getToken(cp);
        np = unexpected(cp);
        break;
    }
    return LEAVE(cp, np);
}


/*
    VariableDefinition -b- (411)
        VariableDefinitionKind VariableBindingList -allowList,b-

    Input
        const
        let
        let const
        var

    AST
        N_VAR_DEFINITION
            def: varKind
 */
static EcNode *parseVariableDefinition(EcCompiler *cp, EcNode *attributes)
{
    EcNode      *np;

    ENTER(cp);
    np = parseVariableDefinitionKind(cp, attributes);
    np = parseVariableBindingList(cp, np, attributes);
    return LEAVE(cp, np);
}


/*
    VariableDefinitionKind (412)
        const
        let
        let const
        var

    Input

    AST
        N_VAR_DEFINITION
            def: varKind
 */
static EcNode *parseVariableDefinitionKind(EcCompiler *cp, EcNode *attributes)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_VAR_DEFINITION, NULL);
    setNodeDoc(cp, np);

    switch (getToken(cp)) {
    case T_CONST:
        np->def.varKind = KIND_CONST;
        np->attributes |= EJS_TRAIT_READONLY;
        break;

    case T_LET:
        if (attributes && attributes->attributes & EJS_PROP_STATIC) {
            np = parseError(cp, "Static and let are not a valid combination. Use var instead.");

        } else {
            np->def.varKind = KIND_LET;
            if (peekToken(cp) == T_CONST) {
                np->def.varKind |= KIND_CONST;
                np->attributes |= EJS_TRAIT_READONLY;
            }
        }
        break;

    case T_VAR:
        np->def.varKind = KIND_VAR;
        break;

    default:
        np = parseError(cp, "Bad variable definition kind");
    }
    return LEAVE(cp, np);
}


/*
    VariableBindingList -a,b- (416)
        VariableBinding
        VariableBindingList -noList,b- , VariableBinding -a,b-

    Input

    AST
 */
static EcNode *parseVariableBindingList(EcCompiler *cp, EcNode *varList, EcNode *attributes)
{
    ENTER(cp);

    varList = parseVariableBinding(cp, varList, attributes);
    while (varList && peekToken(cp) == T_COMMA) {
        getToken(cp);
        varList = parseVariableBinding(cp, varList, attributes);
    }
    return LEAVE(cp, varList);
}


/*
    VariableBinding -a,b- (418)
        TypedIdentifier (258)
        TypedPattern (260) -noList,noIn- VariableInitialisation -a,b-

    TypedIdentifier (258)
        SimplePattern -noList,noin,noExpr-
        SimplePattern -a,b,noExpr- : TypeExpression

    TypedPattern (260)
        SimplePattern -a,b,noExpr-
        SimplePattern -a,b,noExpr- : NullableTypeExpression
        ObjectPattern -noExpr-
        ObjectPattern -noExpr- : TypeExpression
        ArrayPattern -noExpr-
        ArrayPattern -noExpr- : TypeExpression

    Input

    AST
        N_QNAME variableId
        N_ASSIGN
            left: N_QNAME variableId
            right: N_LITERAL

 */
static EcNode *parseVariableBinding(EcCompiler *cp, EcNode *np, EcNode *attributes)
{
    EcNode      *var, *assign, *elt, *initialize, *name;
    int         next;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_LBRACKET:
    case T_LBRACE:
        initialize = parsePattern(cp);
        for (next = 0; (elt = mprGetNextItem(initialize->children, &next)) != 0 && !cp->error; ) {
            assure(elt->kind == N_FIELD);
            if (elt->field.expr->kind != N_QNAME) {
                return LEAVE(cp, parseError(cp, "Bad destructuring variable declaration"));
            }
            var = createNode(cp, N_VAR, NULL);
            var->qname = elt->field.expr->qname;
            var->name.varKind = np->def.varKind;
            var->attributes |= np->attributes;
            applyAttributes(cp, var, attributes, 0); 
            copyDocString(cp, var, np);
            np = appendNode(np, var);
        }
        if (peekToken(cp) == T_ASSIGN) {
            getToken(cp);
            assign = createNode(cp, N_ASSIGN_OP, NULL);
            assign = createAssignNode(cp, initialize, parseAssignmentExpression(cp), assign);
            np = appendNode(np, assign);
        }
        break;

    default:
        if ((var = parseTypedIdentifier(cp)) == 0) {
            return LEAVE(cp, var);
        }
        assure(var->qname.name);
        if (var->kind != N_QNAME) {
            return LEAVE(cp, parseError(cp, "Bad variable name"));
        }
        if (STRICT_MODE(cp)) {
            if (var->typeNode == 0) {
                parseError(cp, "Variable untyped. Variables must be typed when declared in strict mode");
                var = ecResetError(cp, var, 0);
                /* Keep parsing */
            }
        }
        var->kind = N_VAR;
        var->kindName = "n_var";
        var->name.varKind = np->def.varKind;
        var->attributes |= np->attributes;
        applyAttributes(cp, var, attributes, 0);
        copyDocString(cp, var, np);

        if (peekToken(cp) == T_ASSIGN) {
            name = createNode(cp, N_QNAME, tokenString(cp));
            name->qname = var->qname;
            assign = appendNode(createNode(cp, N_ASSIGN_OP, NULL), name);
            assign = appendNode(assign, parseVariableInitialisation(cp));
            var = appendNode(var, assign);
        }
        np = appendNode(np, var);
        break;
    }
    return LEAVE(cp, np);
}


/*
    VariableInitialisation -a,b- (426)
        = AssignmentExpression -a,b-

    Input
        =

    AST
        N_EXPRESSIONS
 */
static EcNode *parseVariableInitialisation(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) == T_ASSIGN) {
        np = parseAssignmentExpression(cp);
    } else {
        np = unexpected(cp);
    }
    return LEAVE(cp, np);
}


/*
    FunctionDeclaration (421)                                   # For interfaces
        function FunctionName FunctionSignature

    Notes:
        This is for function declarations in interfaces only.

    Input
        function

    AST
        N_FUNCTION
            function: name, getter, setter, block,
                children: parameters
 */
static EcNode *parseFunctionDeclaration(EcCompiler *cp, EcNode *attributes)
{
    EcNode      *np;
    int         tid;

    ENTER(cp);

    cp->state->defaultNamespace = NULL;

    if (getToken(cp) != T_FUNCTION) {
        return LEAVE(cp, parseError(cp, "Expecting \"function\""));
    }
    tid = peekToken(cp);
    if (tid != T_ID && tid != T_GET && tid != T_SET) {
        getToken(cp);
        return LEAVE(cp, parseError(cp, "Expecting function or class name"));
    }
    np = parseFunctionName(cp);
    if (np) {
        setNodeDoc(cp, np);
        applyAttributes(cp, np, attributes, 0);
        np = parseFunctionSignature(cp, np);
        if (np) {
            np->function.isMethod = 1;
            if (STRICT_MODE(cp)) {
                if (np->function.resultType == 0) {
                    np = parseError(cp, 
                        "Function has not defined a return type. Fuctions must be typed when declared in strict mode");
                }
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    FunctionDefinition -class- (424)
        function ClassName ConstructorSignature FunctionBody -allowin-
        function FunctionName FunctionSignature FunctionBody -allowin-

    FunctionDeclaration -t- (442)
        function FunctionName FunctionSignature FunctionBody -allowin-
        let function FunctionName FunctionSignature FunctionBody -allowin-
        const function FunctionName FunctionSignature FunctionBody -allowin-

    Input
        function
        let
        const

    AST N_FUNCTION
        function: name, getter, setter, block,
            children: parameters
 */
static EcNode *parseFunctionDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcNode      *np, *className;
    EcState     *state;

    ENTER(cp);

    state = cp->state;
    state->defaultNamespace = NULL;

    if (getToken(cp) != T_FUNCTION) {
        return LEAVE(cp, parseError(cp, "Expecting \"function\""));
    }
    if (getToken(cp) != T_ID && !(cp->token->groupMask & (G_CONREV | G_OPERATOR))) {
        return LEAVE(cp, parseError(cp, "Expecting function or class name"));
    }
    if (cp->state->currentClassName.name && 
            ejsCompareWide(cp->ejs, cp->state->currentClassName.name, cp->token->text, cp->token->length) == 0) {
        /*
            Constructor
         */
        np = createNode(cp, N_FUNCTION, NULL);
        putToken(cp);
        setNodeDoc(cp, np);
        applyAttributes(cp, np, attributeNode, ejsCreateStringFromAsc(cp->ejs, EJS_PUBLIC_NAMESPACE));
        className = parseClassName(cp);

        if (className) {
            np->qname.name = className->qname.name;
            np->function.isConstructor = 1;
            cp->state->currentClassNode->klass.constructor = np;
        }
        if (np) {
            np = parseConstructorSignature(cp, np);
            if (np) {
                cp->state->currentFunctionNode = np;
                if (!(np->attributes & EJS_PROP_NATIVE)) {
                    np->function.body = linkNode(np, parseFunctionBody(cp, np));
                    if (np->function.body == 0) {
                        return LEAVE(cp, 0);
                    }
                }
                np->function.isMethod = 1;
            }
        }

    } else {
        putToken(cp);
        np = parseFunctionName(cp);
        if (np) {
            setNodeDoc(cp, np);
            applyAttributes(cp, np, attributeNode, 0);
            np = parseFunctionSignature(cp, np);
            if (np) {
                cp->state->currentFunctionNode = np;
                if (attributeNode && (attributeNode->attributes & EJS_PROP_NATIVE)) {
                    if (peekToken(cp) == T_LBRACE) {
                        return LEAVE(cp, parseError(cp, "Native functions declarations must not have bodies"));
                    }

                } else {
#if UNUSED
                    if (peekToken(cp) != T_LBRACE) {
                        np->function.noBlock = 1;
                    }
#endif
                    np->function.body = linkNode(np, parseFunctionBody(cp, np));
                    if (np->function.body == 0) {
                        return LEAVE(cp, 0);
                    }
                }
                if (state->inClass && !state->inFunction && state->classState->blockNestCount == (state->blockNestCount - 1)) {
                    np->function.isMethod = 1;
                }
            }

            if (STRICT_MODE(cp)) {
                if (np->function.resultType == 0) {
                    parseError(cp, "Function has not defined a return type. Functions must be typed in strict mode");
                    np = ecResetError(cp, np, 0);
                    /* Keep parsing */
                }
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    FunctionName (427)
        Identifier
        OverloadedOperator
        get Identifier
        set Identifier

    Input
        Identifier
        get
        set
        + - ~ * / % < > <= >= == << >> >>> & | === != !==

    AST N_FUNCTION
        function: name, getter, setter
 */
static EcNode *parseFunctionName(EcCompiler *cp)
{
    EcNode      *name, *np;
    int         accessorId, tid;

    ENTER(cp);

    tid = peekToken(cp);
    if (tid != T_GET && tid != T_SET) {
        if (cp->peekToken->groupMask & G_CONREV) {
            tid = T_ID;
        }
    }

    switch (tid) {
    case T_GET:
    case T_SET:
    case T_DELETE:
        getToken(cp);
        accessorId = cp->token->tokenId;

        tid = peekToken(cp);
        if (cp->peekToken->groupMask & G_CONREV) {
            tid = T_ID;
        }
        if (tid == T_LPAREN) {
            /*
                Function is called get() or set(). So put back the name and fall through to T_ID
             */
            putToken(cp);

        } else {
            if (tid != T_ID) {
                getToken(cp);
                return LEAVE(cp, parseError(cp, "Expecting identifier"));
            }
            name = parseIdentifier(cp);
            np = createNode(cp, N_FUNCTION, NULL);
            if (accessorId == T_GET) {
                np->function.getter = 1;
                np->attributes |= EJS_TRAIT_GETTER;
            } else {
                np->function.setter = 1;
                np->attributes |= EJS_TRAIT_SETTER;
            }
            np->qname.name = name->qname.name;
            break;
        }
        /* Fall through */

    case T_ID:
        name = parseIdentifier(cp);
        np = createNode(cp, N_FUNCTION, NULL);
        np->qname.name = name->qname.name;
        break;

    default:
        getToken(cp);
        if (cp->token->groupMask == G_OPERATOR) {
            putToken(cp);
            np = parseOverloadedOperator(cp);
        } else {
            np = unexpected(cp);
        }
    }
    return LEAVE(cp, np);
}


/*
    OverloadedOperator (431)
        + - ~ * / % < > <= >= == << >> >>> & | === != !==

    Input
        + - ~ * / % < > <= >= == << >> >>> & | === != !==
        [ . (  =        EJS exceptions

    AST
        N_QNAME
 */
static EcNode *parseOverloadedOperator(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (getToken(cp)) {
    /*
        EJS extensions
     */
    case T_LBRACKET:
    case T_LPAREN:
    case T_DOT:
    case T_ASSIGN:
        /* Fall through */

    case T_PLUS:
    case T_MINUS:
    case T_TILDE:
    case T_MUL:
    case T_DIV:
    case T_MOD:
    case T_LT:
    case T_GT:
    case T_LE:
    case T_GE:
    case T_EQ:
    case T_LSH:
    case T_RSH:
    case T_RSH_ZERO:
    case T_BIT_AND:
    case T_BIT_OR:
    case T_STRICT_EQ:
    case T_NE:
    case T_STRICT_NE:
        /* Node holds the token */
        np = createNode(cp, N_FUNCTION, NULL);
        np->qname.name = tokenString(cp);
        break;

    default:
        np = unexpected(cp);
        break;
    }

    return LEAVE(cp, np);
}


/*
    FunctionSignature (450) (See also FunctionSignatureType)
        TypeParameters ( Parameters ) ResultType
        TypeParameters ( this : PrimaryName ) ResultType
        TypeParameters ( this : PrimaryName , NonemptyParameters )
                ResultType

    Input
        .< TypeParameterList >

    AST
 */
static EcNode *parseFunctionSignature(EcCompiler *cp, EcNode *np)
{
    EcNode      *parameters;

    if (np == 0) {
        return np;
    }
    ENTER(cp);
    assure(np->kind == N_FUNCTION);

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \"(\""));
    }
    np->function.parameters = linkNode(np, createNode(cp, N_ARGS, NULL));

    //  TODO - should accep
    if (peekToken(cp) == T_ID || cp->peekToken->tokenId == T_ELIPSIS || cp->peekToken->groupMask & G_CONREV) {
        if (mcmp(cp->peekToken->text, "this") == 0) {
            ;
        } else {
            parameters = parseParameters(cp, np->function.parameters);
            if (parameters == 0) {
                while (getToken(cp) != T_RPAREN && cp->token->tokenId != T_EOF);
                return LEAVE(cp, 0);
            }
            np->function.parameters = linkNode(np, parameters);
        }
    }
    if (getToken(cp) != T_RPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \")\""));
    }
    if (np) {
        if (peekToken(cp) == T_COLON) {
            np->function.resultType = linkNode(np, parseResultType(cp));
        }
    }
    return LEAVE(cp, np);
}


#if UNUSED
/*
    TypeParameters (453)
        EMPTY
        .< TypeParameterList >
    Input

    AST
 */
static EcNode *parseTypeParameters(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    TypeParametersList (455)
        Identifier
        Identifier , TypeParameterList

    Input

    AST
 */
static EcNode *parseTypeParameterList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    Parameters (457)
        EMPTY
        NonemptyParameters

    Input

    AST
 */
static EcNode *parseParameters(EcCompiler *cp, EcNode *args)
{
    ENTER(cp);

    if (peekToken(cp) != T_RPAREN) {
        args = parseNonemptyParameters(cp, args);
    }
    return LEAVE(cp, args);
}


/*
    NonemptyParameters (459)
        ParameterInit
        ParameterInit , NonemptyParameters
        RestParameter

    Input
        Identifier
        ...

    AST
        N_ARGS
            N_VAR_DEFN
                N_QNAME
                N_ASSIGN_OP
                    N_QNAME, N_LITERAL

 */
static EcNode *parseNonemptyParameters(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (peekToken(cp) == T_ELIPSIS) {
        np = appendNode(np, parseRestParameter(cp));

    } else {
        np = appendNode(np, parseParameterInit(cp, np));
        if (np) {
            if (peekToken(cp) == T_COMMA) {
                getToken(cp);
                np = parseNonemptyParameters(cp, np);
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    ParameterInit (462)
        Parameter
        Parameter = NonAssignmentExpression -noList,allowIn-

    Input

    AST
 */
static EcNode *parseParameterInit(EcCompiler *cp, EcNode *args)
{
    EcNode      *np, *assignOp, *lastArg;

    ENTER(cp);

    np = parseParameter(cp, 0);

    if (peekToken(cp) == T_ASSIGN) {
        getToken(cp);
        /*
            Insert a N_ASSIGN_OP node under the VAR_DEFN
         */
        assignOp = createNode(cp, N_ASSIGN_OP, NULL);
        assignOp = appendNode(assignOp, np->left);
        assure(mprGetListLength(np->children) == 1);

        mprRemoveItemAtPos(np->children, 0);
        assignOp = appendNode(assignOp, parseNonAssignmentExpression(cp));
        np = appendNode(np, assignOp);

        if (assignOp) {
            appendDocString(cp, args->parent, assignOp->left, assignOp->right);
        }

    } else if (args->children) {
        lastArg = (EcNode*) mprGetLastItem(args->children);
        if (lastArg && lastArg->left->kind == N_ASSIGN_OP) {
            np = parseError(cp, "Cannot have required parameters after parameters with initializers");
        }
    }
    return LEAVE(cp, np);
}


/*
    Parameter (464)
        ParameterKind TypedPattern -noList,noIn-

    Input

    AST
 */
static EcNode *parseParameter(EcCompiler *cp, bool rest)
{
    Ejs         *ejs;
    EcNode      *np, *parameter;

    ENTER(cp);

    ejs = cp->ejs;
    np = parseParameterKind(cp);
    parameter = parseTypedPattern(cp);
    if (parameter) {
        parameter->qname.space = ESV(empty);
    }
    np = appendNode(np, parameter);
    if (parameter) {
        if (STRICT_MODE(cp)) {
            if (parameter->typeNode == 0 && !rest) {
                parseError(cp, "Parameter untyped. Parameters must be typed when declared in strict mode.");
                np = ecResetError(cp, np, 0);
                /* Keep parsing */
            }
        }
    }
    return LEAVE(cp, np);
}


/*
    ParameterKind (465)
        EMPTY
        const

    Input

    AST
 */
static EcNode *parseParameterKind(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_VAR_DEFINITION, NULL);

    if (peekToken(cp) == T_CONST) {
        getToken(cp);
        np->def.varKind = KIND_CONST;
        np->attributes |= EJS_TRAIT_READONLY;
    }
    return LEAVE(cp, np);
}


/*
    RestParameter (467)
        ...
        ... Parameter

    Input

    AST
 */
static EcNode *parseRestParameter(EcCompiler *cp)
{
    EcNode      *np, *varNode;

    ENTER(cp);

    if (getToken(cp) == T_ELIPSIS) {
        np = parseParameter(cp, 1);
        if (np && np->left) {
            if (np->left->kind == N_QNAME) {
                varNode = np->left;
            } else if (np->left->kind == N_ASSIGN_OP) {
                varNode = np->left->left;
            } else {
                varNode = 0;
                assure(0);
            }
            if (varNode) {
                assure(varNode->kind == N_QNAME);
                varNode->name.isRest = 1;
            }
        }

    } else {
        np = unexpected(cp);
    }
    return LEAVE(cp, np);
}


/*
    ResultType (469)
        EMPTY
        : void
        : NullableTypeExpression

    Input

    AST
        N_DOT
        N_QNAME
        N_VOID

    NOTE: we do not handle EMPTY here. Caller must handle.
 */
static EcNode *parseResultType(EcCompiler *cp)
{
    Ejs         *ejs;
    EcNode      *np;

    ENTER(cp);

    ejs = cp->ejs;

    if (peekToken(cp) == T_COLON) {
        getToken(cp);
        if (peekToken(cp) == T_VOID) {
            getToken(cp);
            np = createNode(cp, N_QNAME, EST(Void)->qname.name);
            np->name.isType = 1;

        } else {
            np = parseNullableTypeExpression(cp);
        }

    } else {
        /*  Don't handle EMPTY here */
        getToken(cp);
        np = unexpected(cp);;
    }
    return LEAVE(cp, np);
}


/*
    ConstructorSignature (472)
        TypeParameters ( Parameters )
        TypeParameters ( Parameters ) : ConstructorInitializer

    Input

    AST
 */
static EcNode *parseConstructorSignature(EcCompiler *cp, EcNode *np)
{
    if (np == 0) {
        return np;
    }
    ENTER(cp);

    assure(np->kind == N_FUNCTION);

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \"(\""));
    }
    np->function.parameters = linkNode(np, createNode(cp, N_ARGS, NULL));
    np->function.parameters = linkNode(np, parseParameters(cp, np->function.parameters));

    if (getToken(cp) != T_RPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \")\""));
    }
    if (np) {
        if (peekToken(cp) == T_COLON) {
            getToken(cp);
            np->function.constructorSettings = linkNode(np, parseConstructorInitializer(cp));
        }
    }
    return LEAVE(cp, np);
}


/*
    ConstructorInitializer (462)
        InitializerList
        InitializerList SuperInitializer
        SuperInitializer

    Input
        TDB
        super

    AST
        N_DIRECTIVES
 */
static EcNode *parseConstructorInitializer(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_DIRECTIVES, NULL);

    if (peekToken(cp) != T_SUPER) {
        np = parseInitializerList(cp, np);
    }
    if (peekToken(cp) == T_SUPER) {
        np = appendNode(np, parseSuperInitializer(cp));
    }
    return LEAVE(cp, np);
}


/*
    InitializerList (465)
        Initializer
        InitializerList , Initializer

    Input
        TBD

    AST
        N_DIRECTIVES
 */
static EcNode *parseInitializerList(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    assure(np && np->kind == N_DIRECTIVES);

    while (1) {
        np = appendNode(np, parseInitializer(cp));
        if (peekToken(cp) == T_COMMA) {
            getToken(cp);
        } else {
            break;
        }
    }
    return LEAVE(cp, np);
}


/*
    Initializer (467)
        Pattern -noList,noIn,noExpr- VariableInitialisation -nolist,allowIn-

    Input
        TBD

    AST
        N_ASSIGN
 */
static EcNode *parseInitializer(EcCompiler *cp)
{
    EcNode      *np, *parent;

    ENTER(cp);

    np = parsePattern(cp);

    if (peekToken(cp) != T_ASSIGN) {
        return LEAVE(cp, expected(cp, "="));
    }
    parent = createNode(cp, N_ASSIGN_OP, NULL);
    np = createAssignNode(cp, np, parseVariableInitialisation(cp), parent);
    return LEAVE(cp, np);
}


/*
    SuperInitializer (481)
        super Arguments

    Input
        super

    AST
        N_SUPER
 */
static EcNode *parseSuperInitializer(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_SUPER) {
        return LEAVE(cp, expected(cp, "super"));
    }
    np = createNode(cp, N_SUPER, NULL);
    np = appendNode(np, parseArguments(cp));
    return LEAVE(cp, np);
}


/*
    FunctionBody -b- (469)
        Block -local-
        AssignmentExpression -b-

    Input
        {
        (

    AST
 */
static EcNode *parseFunctionBody(EcCompiler *cp, EcNode *fun)
{
    Ejs         *ejs;
    EcNode      *np, *end, *ret;

    ENTER(cp);

    ejs = cp->ejs;
    cp->state->inFunction = 1;
    cp->state->nspace = ESV(empty);

    if (peekToken(cp) == T_LBRACE) {
        np = parseBlock(cp);
        if (np) {
            np = np->left;
        }

    } else {
        /*
            Create a return for block-less functions
         */
        np = createNode(cp, N_DIRECTIVES, NULL);
        ret = createNode(cp, N_RETURN, NULL);
        ret->ret.blockless = 1;
        ret = appendNode(ret, parseAssignmentExpression(cp));
        np = appendNode(np, ret);
    }
    if (np) {
        end = createNode(cp, N_END_FUNCTION, NULL);
        np = appendNode(np, end);
    }
    return LEAVE(cp, np);
}


/*
    ClassDefinition (484)
        class ClassName ClassInheritance ClassBody

    Input
        class id ...

    AST
        N_CLASS
            name
                id
            extends: id

 */
static EcNode *parseClassDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcState     *state;
    EcNode      *np, *classNameNode, *inheritance, *constructor;
    int         tid;

    ENTER(cp);

    state = cp->state;

    if (getToken(cp) != T_CLASS) {
        return LEAVE(cp, expected(cp, "class"));
    }
    if (peekToken(cp) != T_ID) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "identifier"));
    }
    np = createNode(cp, N_CLASS, NULL);
    state->currentClassNode = np;
    state->topVarBlockNode = np;
    state->classState = state;
    state->defaultNamespace = NULL;

    classNameNode = parseClassName(cp);
    if (classNameNode == 0) {
        return LEAVE(cp, 0);
    }
    applyAttributes(cp, np, attributeNode, 0);
    setNodeDoc(cp, np);

    np->qname.name = classNameNode->qname.name;
    state->currentClassName = np->qname;
    state->inClass = 1;

    tid = peekToken(cp);
    if (tid == T_EXTENDS || tid == T_IMPLEMENTS) {
        inheritance = parseClassInheritance(cp);
        if (inheritance->klass.extends) {
            np->klass.extends = inheritance->klass.extends;
        }
        if (inheritance->klass.implements) {
            np->klass.implements = inheritance->klass.implements;
        }
    }
    if (peekToken(cp) != T_LBRACE) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "{"));
    }

    np = appendNode(np, parseClassBody(cp));

    if (np && np->klass.constructor == 0) {
        /*
            Create a default constructor because the user did not supply a constructor. We always
            create a constructor node even if one is not required (or generated). This makes binding easier later.
         */
        constructor = createNode(cp, N_FUNCTION, NULL);
        np->klass.constructor = linkNode(np, constructor);
        constructor->qname.name = np->qname.name;
        applyAttributes(cp, constructor, 0, ejsCreateStringFromAsc(cp->ejs, EJS_PUBLIC_NAMESPACE));
        constructor->function.isMethod = 1;
        constructor->function.isConstructor = 1;
        constructor->function.isDefault = 1;
    }
    return LEAVE(cp, np);
}


/*
    ClassName (485)
        ParameterisedTypeName
        ParameterisedTypeName !
    Input

    AST
 */
static EcNode *parseClassName(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseParameterisedTypeName(cp);
    if (peekToken(cp) == T_LOGICAL_NOT) {
        getToken(cp);
    }
    return LEAVE(cp, np);
}


/*
    ParameterisedTypeName (487)
        Identifier
        Identifier TypeParameters

    Input

    AST
 */
static EcNode *parseParameterisedTypeName(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) != T_ID) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "identifier"));
    }
    np = parseIdentifier(cp);

#if FUTURE
    if (peekToken(cp) == T_DOT_LESS) {
        np = parseTypeParameters(cp);
    }
#endif
    return LEAVE(cp, np);
}


/*
    ClassInheritance (489)
        EMPTY
        extends PrimaryName
        implements TypeIdentifierList
        extends PrimaryName implements TypeIdentifierList

    Input

    AST N_CLASS
        extends: id
        left: implements list if ids
 */
static EcNode *parseClassInheritance(EcCompiler *cp)
{
    EcNode      *np, *id;

    ENTER(cp);

    np = createNode(cp, N_CLASS, NULL);

    switch (getToken(cp)) {
    case T_EXTENDS:
        id = parsePrimaryName(cp);
        if (id) {
            np->klass.extends = id->qname.name;
        }
        if (peekToken(cp) == T_IMPLEMENTS) {
            getToken(cp);
            np->klass.implements = linkNode(np, parseTypeIdentifierList(cp));
        }
        break;

    case T_IMPLEMENTS:
        np->klass.implements = linkNode(np, parseTypeIdentifierList(cp));
        break;

    default:
        putToken(cp);
        break;
    }
    return LEAVE(cp, np);
}


/*
    TypeIdentifierList (493)
        PrimaryName
        PrimaryName , TypeIdentifierList

    Input

    AST
 */
static EcNode *parseTypeIdentifierList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_TYPE_IDENTIFIERS, NULL);
    while (peekToken(cp) == T_ID) {
        np = appendNode(np, parsePrimaryName(cp));
        if (peekToken(cp) != T_COMMA) {
            break;
        }
        getToken(cp);
    }

    /*
        Discard the first NOP node
     */
    return LEAVE(cp, np);
}


/*
    ClassBody (495)
        Block -class-

    Input

    AST
 */
static EcNode *parseClassBody(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    cp->state->inFunction = 0;

    if (peekToken(cp) != T_LBRACE) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "class body { }"));
    }
    np = parseBlock(cp);
    if (np) {
        np = np->left;
        assure(np->kind == N_DIRECTIVES);
    }
    return LEAVE(cp, np);
}


/*
    InterfaceDefinition (496)
        interface ClassName InterfaceInheritance InterfaceBody

    Input

    AST
 */
static EcNode *parseInterfaceDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcState     *state;
    EcNode      *np, *classNameNode, *inheritance;

    ENTER(cp);
    
    state = cp->state;

    np = 0;
    if (getToken(cp) != T_INTERFACE) {
        return LEAVE(cp, expected(cp, "interface"));
    }
    
    if (peekToken(cp) != T_ID) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "identifier"));
    }

    np = createNode(cp, N_CLASS, NULL);
    state->currentClassNode = np;
    state->topVarBlockNode = np;
    state->classState = state;
    state->defaultNamespace = NULL;
    
    classNameNode = parseClassName(cp);
    if (classNameNode == 0) {
        return LEAVE(cp, 0);
    }

    applyAttributes(cp, np, attributeNode, 0);
    setNodeDoc(cp, np);
    
    np->qname.name = classNameNode->qname.name;
    np->klass.isInterface = 1;
    state->currentClassName.name = np->qname.name;
    state->inInterface = 1;
    
    if (peekToken(cp) == T_EXTENDS) {
        inheritance = parseInterfaceInheritance(cp);
        if (inheritance->klass.extends) {
            np->klass.extends = inheritance->klass.extends;
        }
    }

    if (peekToken(cp) != T_LBRACE) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "{"));
    }

    np = appendNode(np, parseInterfaceBody(cp));

    return LEAVE(cp, np);
}


/*
    InterfaceInheritance (497)
        EMPTY
        extends TypeIdentifierList

    Input

    AST
 */
static EcNode *parseInterfaceInheritance(EcCompiler *cp)
{
    EcNode      *np, *id;

    ENTER(cp);

    np = createNode(cp, N_CLASS, NULL);

    if (peekToken(cp) == T_EXTENDS) {
        id = parseTypeIdentifierList(cp);
        if (id) {
            np->klass.extends = id->qname.name;
        }
    }
    return LEAVE(cp, np);
}


/*
    InterfaceBody (499)
        Block -interface-

    Input

    AST
 */
static EcNode *parseInterfaceBody(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) != T_LBRACE) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "interface body { }"));
    }

    np = parseBlock(cp);
    if (np) {
        np = np->left;
        assure(np->kind == N_DIRECTIVES);
    }

    return LEAVE(cp, np);
}


/*
    NamespaceDefinition (500)
        namespace Identifier NamespaceInitialisation

    Input
        namespace

    AST
        N_NAMESPACE
 */
static EcNode *parseNamespaceDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcState     *state;
    EcNode      *varDefNode, *assignNode, *varNode, *typeNode, *namespaceNode, *nameNode;
    EjsObj      *vp;

    ENTER(cp);
    state = cp->state;
    if (state->inClass || state->inFunction) {
        getToken(cp);
        return LEAVE(cp, parseError(cp, "Namespace definitions are not permitted inside classes or functions"));
    }
    if (getToken(cp) != T_NAMESPACE) {
        return LEAVE(cp, unexpected(cp));
    }

    /*
        Handle namespace definitions like:
            let NAME : Namespace = NAMESPACE_LITERAL
     */
    if ((varNode = parseIdentifier(cp)) == 0) {
        return LEAVE(cp, varNode);
    }
    assure(varNode->kind == N_QNAME);
    varNode->kind = N_VAR;
    varNode->kindName = "n_var";
    varNode->name.varKind = KIND_VAR;
    varNode->name.isNamespace = 1;
    setNodeDoc(cp, varNode);

    /*
        Hand-craft a "Namespace" type node
     */
    typeNode = createNode(cp, N_QNAME, tokenString(cp));
    typeNode->qname.name = ejsCreateStringFromAsc(cp->ejs, "Namespace");
    varNode->typeNode = linkNode(varNode, typeNode);
    applyAttributes(cp, varNode, attributeNode, 0);

    if (peekToken(cp) == T_ASSIGN) {
        namespaceNode = parseNamespaceInitialisation(cp, varNode);
    } else {
        /*
            Create a namespace literal node from which to assign.
         */
        namespaceNode = createNode(cp, N_LITERAL, NULL);
        vp = (EjsObj*) ejsCreateNamespace(cp->ejs, varNode->qname.name);
        namespaceNode->literal.var = vp;
        varNode->name.nsvalue = vp;
    }
    /*
        Create an assignment node
     */
    nameNode = createNode(cp, N_QNAME, tokenString(cp));
    nameNode->qname = varNode->qname;
    assignNode = appendNode(createNode(cp, N_ASSIGN_OP, NULL), nameNode);
    assignNode = appendNode(assignNode, namespaceNode);
    varNode = appendNode(varNode, assignNode);

    varDefNode = createNode(cp, N_VAR_DEFINITION, NULL);
    varDefNode->def.varKind = KIND_VAR;
    varDefNode = appendNode(varDefNode, varNode);

    return LEAVE(cp, varDefNode);
}


/*
    NamespaceInitialisation (501)
        EMPTY
        = StringLiteral
        = not supported SimpleQualifiedName

    AST
        N_LITERAL
        N_QNAME
        N_DOT
 */
static EcNode *parseNamespaceInitialisation(EcCompiler *cp, EcNode *nameNode)
{
    EcNode      *np;
    EjsObj      *vp;

    ENTER(cp);

    if (getToken(cp) != T_ASSIGN) {
        return LEAVE(cp, unexpected(cp));
    }
    if (peekToken(cp) != T_STRING) {
        return LEAVE(cp, expected(cp, "Namespace initializers must be literal strings"));
    }
    getToken(cp);
    np = createNode(cp, N_LITERAL, tokenString(cp));
    vp = (EjsObj*) ejsCreateNamespace(cp->ejs, nameNode->qname.name);
    np->literal.var = vp;

    if (peekToken(cp) != T_SEMICOLON && cp->peekToken->tokenId != T_EOF && 
            cp->peekToken->loc.lineNumber == cp->token->loc.lineNumber) {
        return LEAVE(cp, expected(cp, "Namespace initializers must be simple literal strings"));
    }
#if UNUSED
    } else {
        np = parsePrimaryName(cp);
    }
#endif
    return LEAVE(cp, np);
}


/*
    TypeDefinition (504)
        type ParameterisedTypeName TypeInitialisation

    Input

    AST
 */
static EcNode *parseTypeDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcNode      *np;

    ENTER(cp);
    np = parseTypeInitialisation(cp);
    return LEAVE(cp, np);
}


/*
    TypeInitialisation (505)
        = NullableTypeExpression

    Input

    AST
 */
static EcNode *parseTypeInitialisation(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


/*
    ModuleDefinition (493)
        module ModuleBody
        module ModuleName ModuleBody

    Input
        module ...

    AST
        N_MODULE
 */
static EcNode *parseModuleDefinition(EcCompiler *cp)
{
    EcNode      *np, *moduleName, *body;
    EjsString   *nspace;
    cchar       *name;
    int         next, isDefault, pos, version;

    ENTER(cp);
    version = 0;

    if (getToken(cp) != T_MODULE) {
        return LEAVE(cp, unexpected(cp));
    }
    np = createNode(cp, N_MODULE, NULL);

    if (peekToken(cp) == T_ID) {
        isDefault = 0;
        moduleName = parseModuleName(cp);
        if (moduleName == 0) {
            return LEAVE(cp, 0);
        }
        version = 0;
        if (peekToken(cp) == T_STRING || cp->peekToken->tokenId == T_NUMBER) {
            if ((version = parseVersion(cp, 0)) < 0) {
                expected(cp, "A version number NUM[.NUM[.NUM]]");
                return 0;
            }
        }
        np->module.name = moduleName->qname.name;
        if (version) {
            nspace = ejsSprintf(cp->ejs, "%@-%d", moduleName->qname.name, version);
        } else if (cp->modver) { 
            nspace = ejsSprintf(cp->ejs, "%@-%d", moduleName->qname.name, cp->modver);
            version = cp->modver;
        } else {
            nspace = moduleName->qname.name;
        }
    } else {
        isDefault = 1;
        nspace = cp->fileState->nspace;
    }
    assure(nspace);
    
    if (isDefault) {
        /*
            No module name. Set the namespace to the unique internal namespace name.
         */
        np->module.name = ejsCreateStringFromAsc(cp->ejs, EJS_DEFAULT_MODULE);
    }
    np->qname.name = np->module.name;
    np->module.version = version;
    cp->state->currentModule = ejsCreateModule(cp->ejs, np->qname.name, np->module.version, NULL);
    cp->state->defaultNamespace = nspace;

    body = parseModuleBody(cp);
    if (body == 0) {
        return LEAVE(cp, 0);
    }
    /* 
        Append the module namespace and also modules provided via ec/ejs --require switch
     */
    pos = 0;
    if (!isDefault) {
        body = insertNode(body, createNamespaceNode(cp, cp->fileState->nspace, 0, 1), pos++);
    }
    body = insertNode(body, createNamespaceNode(cp, nspace, 1, 1), pos++);
    for (next = 0; (name = mprGetNextItem(cp->require, &next)) != 0; ) {
        body = insertNode(body, createNamespaceNode(cp, ejsCreateStringFromAsc(cp->ejs, name), 0, 1), pos++);
    }
    assure(body->kind == N_BLOCK);
    np = appendNode(np, body);
    return LEAVE(cp, np);
}


/*
    ModuleName (494)
        Identifier
        ModuleName . Identifier

    Input
        ID
        ID. ... .ID

    AST
        N_QNAME
            name: name
 */
static EcNode *parseModuleName(EcCompiler *cp)
{
    EcNode      *np, *idp;
    EjsString   *name;

    ENTER(cp);

    np = parseIdentifier(cp);
    if (np == 0) {
        return LEAVE(cp, np);
    }
    name = np->qname.name;

    while (np && getToken(cp) == T_DOT) {
        /*
            Stop if not "identifier"
         */
        if (peekAheadToken(cp, 1) != T_ID) {
            break;
        }
        idp = parseIdentifier(cp);
        if (idp == 0) {
            return LEAVE(cp, idp);
        }
        np = appendNode(np, idp);
        name = ejsSprintf(cp->ejs, "%@.%@", name, idp->qname.name);
    }
    putToken(cp);
    np->qname.name = name;
    return LEAVE(cp, np);
}


/*
    ModuleBody (496)
        Block -global-

    Input
        {

    AST
        N_BLOCK
 */
static EcNode *parseModuleBody(EcCompiler *cp)
{
    return parseBlock(cp);
}


/*
    Pragma (505)
        UsePragma Semicolon |
        ImportPragma Semicolon

    Input
        use ...
        import ...

    AST
        N_IMPORT
        N_PRAGMAS
 */
static EcNode *parsePragma(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    switch (peekToken(cp)) {
    case T_USE:
    case T_REQUIRE:
        np = parseUsePragma(cp, np);
        break;

    default:
        getToken(cp);
        np = unexpected(cp);
        break;
    }
    return LEAVE(cp, np);
}


/*
    Pragmas (497)
        Pragma
        Pragmas Pragma

    Input
        use ...
        require
        import

    AST
        N_PRAGMAS
            N_IMPORT
            N_PRAGMA
 */
static EcNode *parsePragmas(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

#if OLD
    while (peekToken(cp) == T_USE || cp->peekToken->tokenId == T_REQUIRE) {
        np = parsePragma(cp, np);
        if (np == 0) {
            break;
        }
    }
    return LEAVE(cp, np);
#else
    //  MOB - does this allow multiple pragmas?
    return LEAVE(cp, parsePragma(cp, np));
#endif
}


/*
    UsePragma (501)
        use PragmaItems
        require

    Input
        use ...

    AST
        N_PRAGMAS
 */
static EcNode *parseUsePragma(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (peekToken(cp) == T_REQUIRE) {
        getToken(cp);
        np = parseRequireItems(cp, np);
    } else if (peekToken(cp) == T_USE) {
        getToken(cp);
        np = parsePragmaItems(cp, np);
    } else{
        getToken(cp);
        np = parseError(cp, "Expecting \"use\" or \"require\"");
    }
    return LEAVE(cp, np);
}


static EcNode *parseRequireItems(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);
    do {
        np = appendNode(np, parseRequireItem(cp));
    } while (peekToken(cp) == T_COMMA);
    return LEAVE(cp, np);
}


//  TODO - should return an EcNode
/*
    Parse "NUM[.NUM[.NUM]]" and return a version number. Return < 0 on parse errors.
 */
static int parseVersion(EcCompiler *cp, int parseMax)
{
    wchar       *str, *p, *next;
    int         major, minor, patch;

    if (parseMax) {
        major = minor = patch = EJS_VERSION_FACTOR - 1;
    } else {
        major = minor = patch = 0;
    }
    if (getToken(cp) != T_STRING && cp->token->tokenId != T_NUMBER) {
        return MPR_ERR_BAD_VALUE;
    }
    str = wclone(cp->token->text);
    if ((p = mtok(str, ".", &next)) != 0) {
        major = (int) wtoi(p);
    }
    if ((p = mtok(next, ".", &next)) != 0) {
        minor = (int) wtoi(p);
    }
    if ((p = mtok(next, ".", &next)) != 0) {
        patch = (int) wtoi(p);
    }
    return EJS_MAKE_VERSION(major, minor, patch);
}


//  TODO - should return an EcNode
/*
    Parse [version:version]. Valid forms include:
        [version]
        [:version]
        [version:version]
 */
static int parseVersions(EcCompiler *cp, int *minVersion, int *maxVersion)
{
    *minVersion = 0;
    *maxVersion = EJS_MAX_VERSION;

    getToken(cp);
    if (peekToken(cp) != T_COLON) {
        if ((*minVersion= parseVersion(cp, 0)) < 0) {
            expected(cp, "A version number NUM[.NUM[.NUM]]");
            return MPR_ERR_BAD_VALUE;
        }
    }
    if (peekToken(cp) == T_COLON) {
        getToken(cp);
        if ((*maxVersion = parseVersion(cp, 1)) < 0) {
            expected(cp, "A version number NUM[.NUM[.NUM]]");
            return MPR_ERR_BAD_VALUE;
        }
    }
    if (getToken(cp) != T_RBRACKET) {
        expected(cp, "]");
    }
    return 0;
}


static EcNode *parseRequireItem(EcCompiler *cp)
{
    EcNode      *np, *ns, *moduleName;
    int         minVersion, maxVersion;

    ENTER(cp);
    
    np = createNode(cp, N_USE_MODULE, NULL);
    ns = createNode(cp, N_USE_NAMESPACE, NULL);
    np->useModule.minVersion = 0;
    np->useModule.maxVersion = EJS_MAX_VERSION;

    if ((moduleName = parseModuleName(cp)) != 0) {
        np->qname.name = moduleName->qname.name;
    }
    /*
        Optional [version:version]
     */
    if (peekToken(cp) == T_LBRACKET) {
        if (parseVersions(cp, &minVersion, &maxVersion) < 0) {
            return 0;
        }
        np->useModule.minVersion = minVersion;
        np->useModule.maxVersion = maxVersion;
    }

    ns->qname.name = np->qname.name;
    ns->name.isLiteral = 1;
    np = appendNode(np, ns);
    return LEAVE(cp, np);
}


/*
    PragmaItems (502)
        PragmaItem
        PragmaItems , PragmaItem

    Input
        decimal
        default
        namespace
        standard
        strict
        module

    AST
        N_PRAGMAS
        N_MODULE
 */
static EcNode *parsePragmaItems(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    do {
        np = appendNode(np, parsePragmaItem(cp));
    } while (peekToken(cp) == T_COMMA);
    return LEAVE(cp, np);
}


/*
    PragmaItem (504)
        decimal LeftHandSideExpression
        default namespace PrimaryName
        // default number [decimal | default | double | int | long | uint | ulong]
        namespace PrimaryName
        standard
        strict
        module ModuleName OptionalStringLiteral

    Input
        See above

    AST
        N_PRAGMA
        N_MODULE
 */
static EcNode *parsePragmaItem(EcCompiler *cp)
{
    EcNode      *np, *ns;
    EcState     *upper;
    EjsModule   *module;

    ENTER(cp);

    np = createNode(cp, N_PRAGMA, NULL);
    np->pragma.strict = cp->fileState->strict;
    module = 0;

    /*
        PragmaIdentifiers (737)
     */
    switch (getToken(cp)) {
#if UNUSED
    case T_DECIMAL:
        np->pragma.decimalContext = linkNode(np, parseLeftHandSideExpression(cp));
        break;
#endif
    case T_MODULE:
        /* TODO DEPRECATE */
        np = parseRequireItem(cp);
        break;

    case T_DEFAULT:
        getToken(cp);
        if (cp->token->tokenId == T_NAMESPACE) {
            if (peekToken(cp) == T_MODULE) {
                getToken(cp);
                module = cp->state->currentModule;
                if (module == 0) {
                    np = parseError(cp, "No open module");
                    break;
                }
                np = createNode(cp, N_USE_NAMESPACE, NULL);
                if (module->version) {
                    np->qname.name = ejsSprintf(cp->ejs, "%@-%d", module->name, module->version);
                    np->name.isLiteral = 1;
                } else {
                    np->qname.name = module->name;
                    np->name.isLiteral = 1;
                }
                np->name.isLiteral = 1;

            } else if (peekToken(cp) == T_STRING) {
                getToken(cp);
                np = createNode(cp, N_USE_NAMESPACE, tokenString(cp));
                np->name.isLiteral = 1;

            } else {
                ns = parsePrimaryName(cp);
                if (ns) {
                    assure(ns->qname.name);
                    np = createNode(cp, N_USE_NAMESPACE, ns->qname.name);
                }
            }
            if (np) {
                /*
                    Must apply this default namespace upwards to all blocks below the blockState. It will define the
                    new namespace value. Note that functions and classes null this so it does not propagate into classes or
                    functions.
                 */
                for (upper = cp->state->next; upper; upper = upper->next) {
                    upper->defaultNamespace = np->qname.name;
                    if (upper == cp->blockState) {
                        break;
                    }
                }
                cp->blockState->nspace = np->qname.name;
                np->name.isDefault = 1;
            }
        }
        break;

    case T_STANDARD:
        cp->fileState->strict = np->pragma.strict = 0;
        break;

    case T_STRICT:
        cp->fileState->strict = np->pragma.strict = 1;
        break;

    case T_NAMESPACE:
        if (peekToken(cp) == T_STRING) {
            getToken(cp);
            np = createNode(cp, N_USE_NAMESPACE, tokenString(cp));
            np->name.isLiteral = 1;

        } else {
            np = createNode(cp, N_USE_NAMESPACE, NULL);
            ns = parsePrimaryName(cp);
            if (ns) {
                np = appendNode(np, ns);
                if (ns->kind == N_DOT) {
                    np->qname.name = ns->right->qname.name;
                } else {
                    np->qname.name = ns->qname.name;
                }
            }
        }
        break;

    default:
        np = parseError(cp, "Unknown pragma identifier");
    }
    return LEAVE(cp, np);
}


/*
    Block -t- (514)
        { Directives }

    Input
        {

    AST
        N_BLOCK
 */
static EcNode *parseBlock(EcCompiler *cp)
{
    EcNode      *np;
    EcState     *state, *saveState;

    ENTER(cp);

    state = cp->state;
    saveState = cp->blockState;
    cp->blockState = state;

    if (getToken(cp) != T_LBRACE) {
        // putToken(cp);
        np = parseError(cp, "Expecting \"{\"");

    } else {
        np = createNode(cp, N_BLOCK, NULL);
        np = appendNode(np, parseDirectives(cp));
        if (np) {
            if (getToken(cp) != T_RBRACE) {
                // putToken(cp);
                np = parseError(cp, "Expecting \"}\"");
            }
        }
    }
    cp->blockState = saveState;
    return LEAVE(cp, np);
}


/*
    Program (515)
        Directives -global-

    Input
        AnyDirective

    AST N_PROGRAM
        N_DIRECTIVES ...

 */
static EcNode *parseProgram(EcCompiler *cp, cchar *path)
{
    Ejs         *ejs;
    EcState     *state;
    EcNode      *np, *module, *block, *require, *namespace;
    EjsString   *name;
    cchar       *requireName;
    char        *md5, *apath;
    int         next;

    ENTER(cp);
    
    ejs = cp->ejs;
    state = cp->state;
    state->strict = cp->strict;

    np = createNode(cp, N_PROGRAM, ejsCreateStringFromAsc(cp->ejs, EJS_PUBLIC_NAMESPACE));

    if (cp->visibleGlobals && ejs->state->internal) {
        np->qname.name = ejs->state->internal->value;
    } else if (path) {
        apath = mprGetAbsPath(path);
        md5 = mprGetMD5(apath);
        np->qname.name = ejsSprintf(cp->ejs, "%s-%s-%d", EJS_INTERNAL_NAMESPACE, md5, cp->uid++);
    } else {
        np->qname.name = ejsCreateStringFromAsc(cp->ejs, EJS_INTERNAL_NAMESPACE);
    }
    state->nspace = np->qname.name;
    assure(state->nspace);
        
    cp->fileState->nspace = state->nspace;

    /*
        Create the default module node
     */
    module = createNode(cp, N_MODULE, NULL);
    module->qname.name = ejsCreateStringFromAsc(cp->ejs, EJS_DEFAULT_MODULE);

    /*
        Create a block to hold the namespaces. Add a require node for the default module and add modules specified 
        via --require switch
     */
    block = createNode(cp, N_BLOCK, NULL);
    namespace = createNamespaceNode(cp, cp->fileState->nspace, 0, 1);
    namespace->name.isInternal = 1;
    block = appendNode(block, namespace);
    for (next = 0; (requireName = mprGetNextItem(cp->require, &next)) != 0; ) {
        name = ejsCreateStringFromAsc(cp->ejs, requireName);
        require = createNode(cp, N_USE_MODULE, NULL);
        require->qname.name = name;
        require->useModule.minVersion = 0;
        require->useModule.maxVersion = EJS_MAX_VERSION;
        require = appendNode(require, createNamespaceNode(cp, name, 0, 1));
        block = appendNode(block, require);
    }
    block = appendNode(block, parseDirectives(cp));
    module = appendNode(module, block);
    np = appendNode(np, module);

    if (!cp->interactive && peekToken(cp) != T_EOF) {
        if (np) {
            np = unexpected(cp);
        }
        return LEAVE(cp, np);
    }

    /*
        Reset the line number to prevent debug source lines preceeding these elements
     */
    if (np) {
        np->loc.lineNumber = 0;
    }
    if (module) {
        module->loc.lineNumber = 0;
    }
    if (block) {
        block->loc.lineNumber = 0;
    }
    return LEAVE(cp, np);
}


#if UNUSED
static EcNode *parseBreak(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseContinue(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseDo(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseFor(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseIf(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseLet(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseReturn(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseSwitch(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseThrow(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    return LEAVE(cp, np);
}


static EcNode *parseTry(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseWhile(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseWith(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseVarDefinition(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseFunctionDefinition(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}


static EcNode *parseInclude(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


#if NOT_USED_IN_GRAMMAR
static EcNode *parseSuper(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    assure(0);
    return LEAVE(cp, np);
}
#endif


/*
    Recover from a parse error to allow parsing to continue.
 */
PUBLIC EcNode *ecResetError(EcCompiler *cp, EcNode *np, bool eatInput)
{
    int     tid;

    assure(cp->error);

    if (cp->error) {
        if (!cp->fatalError && cp->errorCount < EC_MAX_ERRORS) {
            cp->error = 0;
            np = createNode(cp, N_DIRECTIVES, NULL);
        }
    }

    /*
        Try to resync by eating input up to the next statement / directive
     */
    while (!cp->interactive) {
        tid = peekToken(cp);
        if (tid == T_SEMICOLON || tid == T_RBRACE || tid == T_RBRACKET || tid == T_RPAREN || tid == T_ERR || tid == T_EOF)  {
            break;
        }
        if (np && np->loc.lineNumber < cp->peekToken->loc.lineNumber) {
            /* Virtual semicolon */
            break;
        }
        getToken(cp);
    }
    return np;
}


#if FUTURE
/*
    Returns an allocated buffer. Caller must free.
 */
static char *detab(EcCompiler *cp, char *src)
{
    char    *p, *dest;
    int     tabCount;

    tabCount = 0;

    for (p = src; *p; p++) {
        if (*p == '\t') {
            tabCount++;
        }
    }

    dest = ejsAlloc(cp->ejs, strlen(src) + 1 + (tabCount * cp->tabWidth));
    if (dest == 0) {
        assure(dest);
        return src;
    }
    for (p = dest; *src; src++) {
        if (*src== '\t') {
            *p++ = ' ';
            *p++ = ' ';
            *p++ = ' ';
            *p++ = ' ';
        } else {
            *p++ = *src;
        }
    }
    *p = '\0';

    return dest;
}
#endif


#if BIT_DEBUG
static void updateDebug(EcCompiler *cp)
{
    assure(cp);

    if (cp->token) {
        cp->token->name = tokenNames[cp->token->tokenId];
    }
    if (cp->peekToken) {
        cp->peekToken->name = tokenNames[cp->peekToken->tokenId];
    }
}
#endif


/*
    Get the next input token. May have been previous obtained and putback.
 */
static int getToken(EcCompiler *cp)
{
    int         id;

    if (cp->fatalError) {
        return T_ERR;
    }
    id = ecGetToken(cp);
    cp->peekToken = 0;
    return id;
}


/*
    Peek ahead (K) tokens and return the token id
 */
static int peekAheadToken(EcCompiler *cp, int ahead)
{
    EcToken     *token;

    token = peekAheadTokenStruct(cp, ahead);
    if (token == 0) {
        return EJS_ERR;
    }
    return token->tokenId;
}


PUBLIC int ecPeekToken(EcCompiler *cp)
{
    return peekAheadToken(cp, 1);
}


/*
    Peek ahead (K) tokens and return the token.
 */
static EcToken *peekAheadTokenStruct(EcCompiler *cp, int ahead)
{
    EcToken     *token, *currentToken, *tokens[EC_MAX_LOOK_AHEAD];
    int         i;

    assure(ahead > 0 && ahead <= EC_MAX_LOOK_AHEAD);
    if (ahead == 1) {
        /* Fast look ahead of one token.  */
        if (cp->putback) {
            cp->peekToken = cp->putback;
            return cp->putback;
        }
    }
    /*
        takeToken will take the current token and remove it from the input
        We must preserve the current token throughout.
     */
    currentToken = ecTakeToken(cp);
    for (i = 0; i < ahead; i++) {
        if (ecGetToken(cp) < 0) {
            assure(0);
            return 0;
        }
        tokens[i] = ecTakeToken(cp);
    }
    /*
        Peek at the token of interest
     */
    token = tokens[i - 1];
    for (i = ahead - 1; i >= 0; i--) {
        putSpecificToken(cp, tokens[i]);
    }
    if (currentToken) {
        ecPutSpecificToken(cp, currentToken);
        ecGetToken(cp);
    }
    cp->peekToken = token;
    updateDebug(cp);
    return token;
}


static void putToken(EcCompiler *cp)
{
    ecPutToken(cp);
    updateDebug(cp);
}


static void putSpecificToken(EcCompiler *cp, EcToken *token)
{
    ecPutSpecificToken(cp, token);
}


static void setNodeDoc(EcCompiler *cp, EcNode *np)
{
    Ejs     *ejs;

    ejs = cp->ejs;

    if (ejs->flags & EJS_FLAG_DOC && cp->doc) {
        np->doc = ejsCreateStringFromAsc(ejs, cp->docToken);
        cp->docToken = 0;
    }
}


static void appendDocString(EcCompiler *cp, EcNode *np, EcNode *parameter, EcNode *value)
{
    EjsString   *defaultValue;
    Ejs         *ejs;
    char        arg[MPR_MAX_STRING];
    int         found;
    
    ejs = cp->ejs;

    if (!(ejs->flags & EJS_FLAG_DOC)) {
        return;
    }
    if (np == 0 || parameter == 0 || parameter->kind != N_QNAME || value == 0) {
        return;
    }
    defaultValue = 0;
    if (value->kind == N_QNAME) {
        defaultValue = value->qname.name;
    } else if (value->kind == N_UNARY_OP) {
        if (value->left->kind == N_LITERAL) {
            if (value->tokenId == T_MINUS) {
                defaultValue = ejsSprintf(ejs, "-%@", ejsToString(ejs, value->left->literal.var));
            }
        }
    } else if (value->kind == N_LITERAL) {
        defaultValue = ejsToString(ejs, value->literal.var);
    }
    if (defaultValue == 0) {
        defaultValue = ejsCreateStringFromAsc(ejs, "expression");
    }
    if (np->doc) {
        found = 0;
        fmt(arg, sizeof(arg), "@param %@ ", parameter->qname.name);
        if (ejsContainsAsc(ejs, np->doc, arg) >= 0 || ejsContainsAsc(ejs, np->doc, "@duplicate") >= 0) {
            found++;
        } else {
            fmt(arg, sizeof(arg), "@params %@ ", parameter->qname.name);
            if (ejsContainsAsc(ejs, np->doc, arg) >= 0) {
                found++;
            }
        }
        if (found) {
            np->doc = ejsSprintf(ejs, "%@\n@default %@ %@", np->doc, parameter->qname.name, defaultValue);
        } else {
            np->doc = ejsSprintf(ejs, "%@\n@param %@\n@default %@ %@", np->doc, parameter->qname.name,
                parameter->qname.name, defaultValue);
        }
    }
}


static void copyDocString(EcCompiler *cp, EcNode *np, EcNode *from)
{
    Ejs     *ejs;

    ejs = cp->ejs;

    if (ejs->flags & EJS_FLAG_DOC && from->doc) {
        np->doc = from->doc;
        from->doc = 0;
    }
}


/*
    This is used outside the parser. It must reset the line number as the
    node will not correspond to any actual source code line;
 */
PUBLIC EcNode *ecCreateNode(EcCompiler *cp, int kind)
{
    EcNode  *node;

    node = createNode(cp, kind, NULL);
    if (node) {
        node->loc.lineNumber = -1;
        node->loc.source = 0;
    }
    return node;
}


static EcNode *createNameNode(EcCompiler *cp, EjsName qname)
{
    EcNode      *np;

    if ((np = createNode(cp, N_QNAME, NULL)) != NULL) {
        np->qname = qname;
    }
    return np;
}


static EcNode *createNamespaceNode(EcCompiler *cp, EjsString *name, bool isDefault, bool isLiteral)
{
    EcNode      *np;
    
    np = createNode(cp, N_USE_NAMESPACE, name);
    np->name.isDefault = isDefault;
    np->name.isLiteral = isLiteral;
    return np;
}


/*
    This is used outside the parser.
 */
PUBLIC EcNode *ecLinkNode(EcNode *np, EcNode *child)
{
    return linkNode(np, child);
}


/*
    Create a binary tree node.
 */
static EcNode *createBinaryNode(EcCompiler *cp, EcNode *lhs, EcNode *rhs, EcNode *parent)
{
    assure(cp);
    assure(lhs);
    assure(parent);

    /*
        appendNode will return the parent if no error
     */
    parent = appendNode(parent, lhs);
    parent = appendNode(parent, rhs);

    return parent;
}


static EcNode *createAssignNode(EcCompiler *cp, EcNode *lhs, EcNode *rhs, EcNode *parent)
{
    assure(cp);
    assure(lhs);
    assure(parent);

    return appendNode(appendNode(parent, lhs), rhs);
}


/*
    Add a child node. If an allocation error, return 0, otherwise return the
    parent node.
 */
static EcNode *appendNode(EcNode *np, EcNode *child)
{
    MprList         *list;
    int             index;

    if (child == 0 || np == 0) {
        return 0;
    }
    assure(np != child);
    list = np->children;

    if ((index = mprAddItem(np->children, child)) < 0) {
        return 0;
    }
    if (index == 0) {
        np->left = list->items[index];
    } else if (index == 1) {
        np->right = list->items[index];
    }
    child->parent = np;
    return np;
}


PUBLIC EcNode *ecAppendNode(EcNode *np, EcNode *child)
{
    return appendNode(np, child);
}


PUBLIC EcNode *ecChangeNode(EcCompiler *cp, EcNode *np, EcNode *oldNode, EcNode *newNode)
{
    EcNode      *child;
    int         next;

    for (next = 0; (child = mprGetNextItem(np->children, &next)) != 0; ) {
        if (child == oldNode) {
            mprSetItem(np->children, next - 1, newNode);
            if (next == 1) {
                np->left = mprGetItem(np->children, next - 1);
            } else if (next == 2) {
                np->right = mprGetItem(np->children, next - 1);
            }
            newNode->parent = np;
            return np;
        }
    }
    assure(0);
    return 0;
}


/*
    Link a node. This only steals the node.
 */
static EcNode *linkNode(EcNode *np, EcNode *node)
{
    if (node == 0 || np == 0) {
        return 0;
    }
    node->parent = np;
    return node;
}


/*
    Insert a child node. If an allocation error, return 0, otherwise return the parent node.
 */
static EcNode *insertNode(EcNode *np, EcNode *child, int pos)
{
    MprList     *list;
    int         index, len;

    if (child == 0 || np == 0) {
        return 0;
    }
    list = np->children;

    index = mprInsertItemAtPos(list, pos, child);
    if (index < 0) {
        return 0;
    }
    len = mprGetListLength(list);
    if (len > 0) {
        np->left = (EcNode*) list->items[0];
    }
    if (len > 1) {
        np->right = (EcNode*) list->items[1];
    }
    child->parent = np;
    return np;
}


/*
    Remove a child node and return it.
 */
static EcNode *removeNode(EcNode *np, EcNode *child)
{
    int         index;

    if (child == 0 || np == 0) {
        return 0;
    }
    index = mprRemoveItem(np->children, child);
    assure(index >= 0);

    if (index == 0) {
        np->left = np->right;
    } else if (index == 1) {
        np->right = 0;
    }
    child->parent = 0;
    return child;
}


static EcNode *unexpected(EcCompiler *cp)
{
    int     junk = 0;

    /*
        This is just to avoid a Vxworks 5.4 linker bug. The link crashes when this function has no local vars.
     */
    dummy(junk);
    return parseError(cp, "Unexpected input \"%s\"", cp->token->text);
}


static EcNode *expected(EcCompiler *cp, cchar *str)
{
    return parseError(cp, "Expected input \"%s\"", str);
}


static void applyAttributes(EcCompiler *cp, EcNode *np, EcNode *attributeNode, EjsString *overrideNamespace)
{
    EcState     *state;
    EjsString   *nspace;
    int         attributes;

    state = cp->state;

    attributes = 0;
    nspace = 0;

    if (attributeNode) {
        /*
            Attribute node passed in.
         */
        attributes = attributeNode->attributes;
        if (attributeNode->qname.space) {
            nspace = attributeNode->qname.space;
        }
        if (attributeNode->literalNamespace) {
            np->literalNamespace = 1;
        }
    } else {
        /*
            "space"::var
         */
        if (np->qname.space) {
            nspace = np->qname.space;
        }
    }
    if (nspace == 0) {
        if (overrideNamespace) {
            nspace = overrideNamespace;
        } else if (cp->blockState->defaultNamespace) {
            nspace = cp->blockState->defaultNamespace;
        } else {
            nspace = cp->blockState->nspace;
        }
    }
    assure(nspace);

    if (state->inFunction) {
        ;
    } else if (state->inClass) {
        if (ejsCompareAsc(cp->ejs, nspace, EJS_INTERNAL_NAMESPACE) == 0) {
            nspace = cp->fileState->nspace;
        } else if (ejsCompareAsc(cp->ejs, nspace, EJS_PRIVATE_NAMESPACE) == 0 || 
                   ejsCompareAsc(cp->ejs, nspace, EJS_PROTECTED_NAMESPACE) == 0) {
            nspace = ejsFormatReservedNamespace(cp->ejs, &state->currentClassName, nspace);
        }
    } else {
        if (cp->visibleGlobals && !(attributeNode && attributeNode->qname.space)) {
            nspace = ejsCreateStringFromAsc(cp->ejs, EJS_EMPTY_NAMESPACE);
        } else if (ejsCompareAsc(cp->ejs, nspace, EJS_INTERNAL_NAMESPACE) == 0) {
            nspace = cp->fileState->nspace;
        }
    }
    assure(nspace);
    np->qname.space = nspace;

    mprLog(7, "Parser apply attributes namespace = \"%@\", current line %w", nspace, np->loc.source);
    assure(np->qname.space);
    np->attributes |= attributes;
}


static void addTokenToLiteral(EcCompiler *cp, EcNode *np)
{
    MprBuf      *buf;

    if (np) {
        buf = np->literal.data;
        mprPutBlockToBuf(buf, (char*) cp->token->text, cp->token->length * sizeof(wchar));
        mprAddNullToBuf(buf);
    }
}


static void addCharsToLiteral(EcCompiler *cp, EcNode *np, wchar *str, ssize count)
{
    MprBuf      *buf;

    if (np) {
        buf = np->literal.data;
        mprPutBlockToBuf(buf, (char*) str, count * sizeof(wchar));
        mprAddNullToBuf(buf);
    }
}


static void addAscToLiteral(EcCompiler *cp, EcNode *np, cchar *str, ssize count)
{
    MprBuf      *buf;
    wchar       c;
    int         i;

    if (np) {
        buf = np->literal.data;
        for (i = 0; i < count; i++) {
            c = (uchar) str[i];
            mprPutBlockToBuf(buf, (char*) &c, sizeof(wchar));
        }
        mprAddNullToBuf(buf);
    }
}


/*
    Reset the input. Eat all tokens, clear errors, exceptions and the result value. Used by ejs for console input.
 */
PUBLIC void ecResetInput(EcCompiler *cp)
{
    Ejs         *ejs;
    EcToken     *tp;

    ejs = cp->ejs;
    cp->stream->flags &= ~EC_STREAM_EOL;
    cp->error = 0;
    cp->ejs->exception = 0;
    cp->ejs->result = ESV(undefined);
    while ((tp = cp->putback) != 0 && (tp->tokenId == T_EOF || tp->tokenId == T_NOP)) {
        ecGetToken(cp);
    }
}


PUBLIC void ecSetOptimizeLevel(EcCompiler *cp, int level)
{
    cp->optimizeLevel = level;
}


PUBLIC void ecSetWarnLevel(EcCompiler *cp, int level)
{
    cp->warnLevel = level;
}


PUBLIC void ecSetStrictMode(EcCompiler *cp, int enabled)
{
    cp->strict = enabled;
}


PUBLIC void ecSetTabWidth(EcCompiler *cp, int width)
{
    cp->tabWidth = width;
}


PUBLIC void ecSetOutputFile(EcCompiler *cp, cchar *outputFile)
{
    if (outputFile) {
        //  UNICODE
        cp->outputFile = sclone(outputFile);
    }
}


PUBLIC void ecSetOutputDir(EcCompiler *cp, cchar *outputDir)
{
    if (outputDir) {
        //  UNICODE
        cp->outputDir = sclone(outputDir);
    }
}


PUBLIC void ecSetCertFile(EcCompiler *cp, cchar *certFile)
{
    //  UNICODE
    cp->certFile = sclone(certFile);
}


static EjsString *tokenString(EcCompiler *cp)
{
    Ejs     *ejs;

    ejs = cp->ejs;
    if (cp->token) {
        return ejsCreateString(cp->ejs, cp->token->text, cp->token->length);
    }
    return ESV(empty);
}


PUBLIC void ecMarkLocation(EcLocation *loc)
{
    mprMark(loc->source);
    mprMark(loc->filename);
}


static void manageNode(EcNode *node, int flags) 
{
    if (flags & MPR_MANAGE_MARK) {
        ejsMarkName(&node->qname);
        ecMarkLocation(&node->loc);
        mprMark(node->blockRef);
        mprMark(node->namespaceRef);
        mprMark(node->typeNode);
        mprMark(node->children);
        mprMark(node->namespaces);
        mprMark(node->code);
        mprMark(node->doc);

        switch (node->kind) {
        case N_ARGS:
            break;

        case N_ASSIGN_OP:
            break;

        case N_BINARY_OP:
            break;

        case N_BLOCK:
            break;

        case N_BREAK:
            break;

        case N_CALL:
            break;

        case N_CLASS:
            mprMark(node->klass.implements);
            mprMark(node->klass.constructor);
            mprMark(node->klass.staticProperties);
            mprMark(node->klass.instanceProperties);
            mprMark(node->klass.classMethods);
            mprMark(node->klass.methods);
            mprMark(node->klass.ref);
            mprMark(node->klass.initializer);
            mprMark(node->klass.publicSpace);
            mprMark(node->klass.internalSpace);
            mprMark(node->klass.extends);
            break;

        case N_CASE_LABEL:
            mprMark(node->caseLabel.expression);
            mprMark(node->caseLabel.expressionCode);
            break;

        case N_CASE_ELEMENTS:
            break;

        case N_CATCH:
            mprMark(node->catchBlock.arg);
            break;

        case N_CATCH_ARG:
            break;

        case N_CATCH_CLAUSES:
            break;
            
        case N_CONTINUE:
            break;

        case N_DASSIGN:
            mprMark(node->objectLiteral.typeNode)
            break;

        case N_DIRECTIVES:
            break;

        case N_DO:
            break;

        case N_DOT:
            break;

        case N_END_FUNCTION:
            break;

        case N_EXPRESSIONS:
            break;

        case N_FIELD:
            mprMark(node->field.expr);
            mprMark(node->field.fieldName);
            break;

        case N_FOR:
            mprMark(node->forLoop.body);
            mprMark(node->forLoop.cond);
            mprMark(node->forLoop.initializer);
            mprMark(node->forLoop.perLoop);
            mprMark(node->forLoop.condCode);
            mprMark(node->forLoop.bodyCode);
            mprMark(node->forLoop.perLoopCode);
            break;

        case N_FOR_IN:
            mprMark(node->forInLoop.iterVar);
            mprMark(node->forInLoop.iterGet);
            mprMark(node->forInLoop.iterNext);
            mprMark(node->forInLoop.body);
            mprMark(node->forInLoop.initCode);
            mprMark(node->forInLoop.bodyCode);
            break;

        case N_FUNCTION:
            mprMark(node->function.resultType);
            mprMark(node->function.body);
            mprMark(node->function.parameters);
            mprMark(node->function.constructorSettings);
#if UNUSED
            mprMark(node->function.expressionRef);
#endif
            mprMark(node->function.functionVar);
            break;

        case N_HASH:
            mprMark(node->hash.expr);
            mprMark(node->hash.body);
            break;

        case N_IF:
            mprMark(node->tenary.cond);
            mprMark(node->tenary.thenBlock);
            mprMark(node->tenary.elseBlock);
            mprMark(node->tenary.thenCode);
            mprMark(node->tenary.elseCode);
            break;

        case N_LITERAL:
            mprMark(node->literal.var);
            mprMark(node->literal.data);
            break;

        case N_MODULE:
            mprMark(node->module.ref);
            mprMark(node->module.filename);
            mprMark(node->module.name);
            break;
                
        case N_NEW:
            break;
                
        case N_NOP:
            break;

        case N_OBJECT_LITERAL:
            mprMark(node->objectLiteral.typeNode);
            break;

        case N_QNAME:
            mprMark(node->name.nameExpr);
            mprMark(node->name.qualifierExpr);
            mprMark(node->name.nsvalue);
            break;

        case N_POSTFIX_OP:
            break;

        case N_PRAGMA:
            break;

        case N_PRAGMAS:
            break;

        case N_PROGRAM:
            mprMark(node->program.dependencies);
            break;

        case N_REF:
            mprMark(node->ref.node);
            break;

        case N_RETURN:
            break;

        case N_SPREAD:
            break;

        case N_SUPER:
            break;

        case N_SWITCH:
            break;

        case N_THIS:
            break;

        case N_THROW:
            break;

        case N_TRY:
            mprMark(node->exception.tryBlock);
            mprMark(node->exception.catchClauses);
            mprMark(node->exception.finallyBlock);
            break;

        case N_TYPE_IDENTIFIERS:
            break;
                
        case N_UNARY_OP:
            break;

        case N_USE_NAMESPACE:
            break;

        case N_VAR:
            break;

        case N_VAR_DEFINITION:
            break;

        case N_USE_MODULE:
            break;

        case N_WITH:
            mprMark(node->with.object);
            mprMark(node->with.statement);
            break;

        default:
            assure(0);
        }
    }
}


/*
    Create a new node. This will be automatically freed when returning from a non-terminal production (ie. the state
    is destroyed). Returning results are preserved by stealing the node from the state memory context.

    NOTE: we are using a tree based memory allocator with destructors.
 */
static EcNode *createNode(EcCompiler *cp, int kind, EjsString *name)
{
    EcNode      *np;
    EcToken     *token;

    assure(cp->state);

    if ((np = mprAllocObj(EcNode, manageNode)) == 0) {
        return 0;
    }
    np->seqno = cp->nextSeqno++;
    np->qname.name = name;
    np->kind = kind;
    np->cp = cp;
    np->slotNum = -1;
    np->lookup.slotNum = -1;

    /*
        Remember the current input token. Don't do for initial program and module nodes.
     */
    if (cp->token == 0 && cp->state->blockNestCount > 0) {
        getToken(cp);
        putToken(cp);
        peekToken(cp);
    }
    token = cp->token;

    if (token) {
        np->tokenId = token->tokenId;
        np->groupMask = token->groupMask;
        np->subId = token->subId;
    }
    np->children = mprCreateList(-1, 0);
    if (token && token->loc.source) {
        np->loc = token->loc;
    }
#if BIT_DEBUG
    np->kindName = nodes[kind];
    if (token && token->tokenId >= 0) {
        np->tokenName = tokenNames[token->tokenId];
    }
#endif
    return np;
}


/*
    Report an error. Return a null EcNode so callers can report an error and return the null in one statement.
 */
static EcNode *parseError(EcCompiler *cp, cchar *fmt, ...)
{
    EcToken     *tp;
    va_list     args;

    cp->errorCount++;
    cp->error = 1;
    tp = cp->token;
    va_start(args, fmt);
    if (tp) {
        ecErrorv(cp, "Error", &tp->loc, fmt, args);
    } else {
        ecErrorv(cp, "Error", NULL, fmt, args);
    }
    va_end(args);
    return 0;
}


#if UNUSED
PUBLIC EcNode *ecParseWarning(EcCompiler *cp, cchar *fmt, ...)
{
    EcToken     *tp;
    va_list     args;

    va_start(arg, fmt);
    cp->warningCount++;
    tp = cp->token;
    ecError(cp, "Warning", &tp->loc, fmt, args);
    va_end(args);
    return 0;
}
#endif


/*
    Just part of a VxWorks 5.4 compiler bug to avoid a linker crash
 */
static void dummy(int junk) { }

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
