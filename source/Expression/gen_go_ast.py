import StringIO

def addNodes():
    addNode("ArrayType", "Expr", "len", "Expr", "elt", "Expr")
    addNode("AssignStmt", "Stmt", "lhs", "[]Expr", "rhs", "[]Expr", "define", "bool")
    addNode("BadDecl", "Decl")
    addNode("BadExpr", "Expr")
    addNode("BadStmt", "Stmt")
    addNode("BasicLit", "Expr", "value", "Token")
    addNode("BinaryExpr", "Expr", "x", "Expr", "y", "Expr", "op", "TokenType")
    addNode("BlockStmt", "Stmt", "list", "[]Stmt")
    addNode("Ident", "Expr", "name", "Token")
    addNode("BranchStmt", "Stmt", "label", "Ident", "tok", "TokenType")
    addNode("CallExpr", "Expr", "fun", "Expr", "args", "[]Expr", "ellipsis", "bool")
    addNode("CaseClause", "Stmt", "list", "[]Expr", "body", "[]Stmt")
    addNode("ChanType", "Expr", "dir", "ChanDir", "value", "Expr")
    addNode("CommClause", "Stmt", "comm", "Stmt", "body", "[]Stmt")
    addNode("CompositeLit", "Expr", "type", "Expr", "elts", "[]Expr")
    addNode("DeclStmt", "Stmt", "decl", "Decl")
    addNode("DeferStmt", "Stmt", "call", "CallExpr")
    addNode("Ellipsis", "Expr", "elt", "Expr")
    addNode("EmptyStmt", "Stmt")
    addNode("ExprStmt", "Stmt", "x", "Expr")
    addNode("Field", "Node", "names", "[]Ident", "type", "Expr", "tag", "BasicLit")
    addNode("FieldList", "Node", "list", "[]Field")
    addNode("ForStmt", "Stmt", "init", "Stmt", "cond", "Expr", "post", "Stmt", "body", "BlockStmt")
    addNode("FuncType", "Expr", "params", "FieldList", "results", "FieldList")
    addNode("FuncDecl", "Decl", "recv", "FieldList", "name", "Ident", "type", "FuncType", "body", "BlockStmt")
    addNode("FuncLit", "Expr", "type", "FuncType", "body", "BlockStmt")
    addNode("GenDecl", "Decl", "tok", "TokenType", "specs", "[]Spec")
    addNode("GoStmt", "Stmt", "call", "CallExpr")
    addNode("IfStmt", "Stmt", "init", "Stmt", "cond", "Expr", "body", "BlockStmt", "els", "Stmt")
    addNode("ImportSpec", "Spec", "name", "Ident", "path", "BasicLit")
    addNode("IncDecStmt", "Stmt", "x", "Expr", "tok", "TokenType")
    addNode("IndexExpr", "Expr", "x", "Expr", "index", "Expr")
    addNode("InterfaceType", "Expr", "methods", "FieldList")
    addNode("KeyValueExpr", "Expr", "key", "Expr", "value", "Expr")
    addNode("LabeledStmt", "Stmt", "label", "Ident", "stmt", "Stmt")
    addNode("MapType", "Expr", "key", "Expr", "value", "Expr")
    addNode("ParenExpr", "Expr", "x", "Expr")
    addNode("RangeStmt", "Stmt", "key", "Expr", "value", "Expr", "define", "bool", "x", "Expr", "body", "BlockStmt")
    addNode("ReturnStmt", "Stmt", "results", "[]Expr")
    addNode("SelectStmt", "Stmt", "body", "BlockStmt")
    addNode("SelectorExpr", "Expr", "x", "Expr", "sel", "Ident")
    addNode("SendStmt", "Stmt", "chan", "Expr", "value", "Expr")
    addNode("SliceExpr", "Expr", "x", "Expr", "low", "Expr", "high", "Expr", "max", "Expr", "slice3", "bool")
    addNode("StarExpr", "Expr", "x", "Expr")
    addNode("StructType", "Expr", "fields", "FieldList")
    addNode("SwitchStmt", "Stmt", "init", "Stmt", "tag", "Expr", "body", "BlockStmt")
    addNode("TypeAssertExpr", "Expr", "x", "Expr", "type", "Expr")
    addNode("TypeSpec", "Spec", "name", "Ident", "type", "Expr")
    addNode("TypeSwitchStmt", "Stmt", "init", "Stmt", "assign", "Stmt", "body", "BlockStmt")
    addNode("UnaryExpr", "Expr", "op", "TokenType", "x", "Expr")
    addNode("ValueSpec", "Spec", "names", "[]Ident", "type", "Expr", "values", "[]Expr")
    addParent("Decl", "Node")
    addParent("Expr", "Node")
    addParent("Spec", "Node")
    addParent("Stmt", "Node")


kinds = {}
parentClasses = StringIO.StringIO()
childClasses = StringIO.StringIO()
walker = StringIO.StringIO()

def startClass(name, parent, out):
    out.write("""
class GoAST%s : public GoAST%s
{
public:
""" % (name, parent))

def endClass(name, out):
    out.write("""
    %(name)s(const %(name)s&) = delete;
    const %(name)s& operator=(const %(name)s&) = delete;
};
""" % {'name': 'GoAST' + name})

def addNode(name, parent, *children):
    startClass(name, parent, childClasses)
    l = kinds.setdefault(parent, [])
    l.append(name)
    addConstructor(name, parent, children)
    childClasses.write("""
    const char* GetKindName() const { return "%(name)s"; }

    static bool
    classof(const GoASTNode* n) { return n->GetKind() == e%(name)s; }
    """ % {'name':name})
    addChildren(name, children)
    endClass(name, childClasses)

def isValueType(typename):
    if typename[0].isupper():
        return typename.startswith('Token') or typename == 'ChanDir'
    return True

def addConstructor(name, parent, children):
    childNames = [children[i] for i in xrange(0, len(children), 2)]
    childTypes = [children[i] for i in xrange(1, len(children), 2)]
    if len(childTypes) != len(childNames):
        raise Exception("Invalid children for %s: %s" % (name, children))
    for x in childTypes:
        if x.startswith("[]"):
            childNames = []
            childTypes = []
            break
    childClasses.write('    ')
    if len(childNames) == 1:
        childClasses.write('explicit ')
    childClasses.write('GoAST%s(' % name)
    for i in xrange(len(childNames)):
        if i > 0:
            childClasses.write(', ')
        t = childTypes[i]
        if isValueType(t):
            childClasses.write(t)
        else:
            t = "GoAST" + t
            childClasses.write('%s*' % t)
        childClasses.write(' ')
        childClasses.write(childNames[i])
    childClasses.write(') : GoAST%s(e%s)' % (parent, name))
    for n in childNames:
        childClasses.write(', ')
        childClasses.write('m_%(name)s(%(name)s)' %{'name':n})
    childClasses.write(""" {}
    virtual ~GoAST%s() { }
""" % name)

    
def addChildren(name, children):
    if len(children) == 0:
        return
    childNames = [children[i] for i in xrange(0, len(children), 2)]
    childTypes = [children[i] for i in xrange(1, len(children), 2)]
    varTypes = []
    walker.write("""
    case e%(n)s:
        {
            GoAST%(n)s* n = llvm::cast<GoAST%(n)s>(this);""" % {'n':name})
    for i in xrange(len(childNames)):
        t = childTypes[i]
        vname = childNames[i]
        tname = vname.title()

        if t.startswith('[]'):
            t = t[2:]
            varTypes.append('std::vector<std::unique_ptr<GoAST%s> >' % t)
            childClasses.write("""
    size_t Num%(title)s() const { return m_%(var)s.size(); }
    const %(type)s* Get%(title)s(int i) const { return m_%(var)s[i].get(); }
    void Add%(title)s(%(type)s* %(var)s) { m_%(var)s.push_back(std::unique_ptr<%(type)s>(%(var)s)); }
""" % {'type':"GoAST%s" % t, 'title': tname, 'var': vname})
            walker.write("""
            for (auto& e : n->m_%s) { v(e.get()); }""" % vname)
        else:
            const = ''
            get = ''
            set = ''
            if isValueType(t):
                varTypes.append(t)
                set = 'm_%(n)s = %(n)s' % {'n': vname}
            else:
                t = 'GoAST' + t
                varTypes.append('std::unique_ptr<%s>' % t)
                t = t + '*'
                const = 'const '
                get = '.get()'
                set = 'm_%(n)s.reset(%(n)s)' % {'n': vname}
                walker.write("""
            v(n->m_%s.get());""" % vname)
            childClasses.write("""
    %(const)s%(type)s Get%(title)s() const { return m_%(var)s%(get)s; }
    void Set%(title)s(%(type)s %(var)s) { %(set)s; }
""" % {'const':const, 'title': tname, 'var': vname, 'get': get, 'set': set, 'type': t})
    childClasses.write('\nprivate:\n    friend class GoASTNode;\n')
    walker.write("""
            return;
        }""")
    for i in xrange(len(childNames)):
        t = varTypes[i]
        name = childNames[i]
        childClasses.write('    %s m_%s;\n' %(t, name))
    

def addParent(name, parent):
    startClass(name, parent, parentClasses)
    l = kinds[name]
    minName = l[0]
    maxName = l[-1]
    parentClasses.write("""    template <typename R, typename V>
    R Visit(V* v) const;

    static bool
    classof(const GoASTNode* n) { return n->GetKind() >= e%s && n->GetKind() <= e%s; }

protected:
    explicit GoAST%s(NodeKind kind) : GoASTNode(kind) { }
private:
""" % (minName, maxName, name))
    endClass(name, parentClasses)
    
addNodes()

print """//===-- GoAST.h -------------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// DO NOT EDIT.
// Generated by gen_go_ast.py

#ifndef liblldb_GoAST_h
#define liblldb_GoAST_h

#include "lldb/lldb-forward.h"
#include "lldb/lldb-private.h"
#include "lldb/Expression/GoLexer.h"
#include "llvm/Support/Casting.h"

namespace lldb_private {

class GoASTNode {
public:
    typedef GoLexer::TokenType TokenType;
    typedef GoLexer::Token Token;
    enum ChanDir {
       eChanBidir,
       eChanSend,
       eChanRecv,
    };
    enum NodeKind
    {"""
for l in kinds.itervalues():
    for x in l:
        print "        e%s," % x
print """    };

    virtual ~GoASTNode() { }

    NodeKind GetKind() const { return m_kind; }

    virtual const char* GetKindName() const = 0;

    template <typename V>
    void WalkChildren(V v, GoASTNode*);

protected:
    explicit GoASTNode(NodeKind kind) : m_kind(kind) { }

private:
    const NodeKind m_kind;
    
    GoASTNode(const GoASTNode&) = delete;
    const GoASTNode& operator=(const GoASTNode&) = delete;
};
"""


print parentClasses.getvalue()
print childClasses.getvalue()

for k, l in kinds.iteritems():
    if k == 'Node':
        continue
    print """
template <typename R, typename V>
R GoAST%s::Visit(V* v) const
{
    switch(GetKind())
    {""" % k
    for subtype in l:
        print """    case e%(n)s:
        return v->Visit%(n)s(llvm::cast<const GoAST%(n)s>(this));""" % {'n':subtype}

    print """    default:
        assert(false && "Invalid kind");
    }
}"""

print """
template <typename V>
void GoASTNode::WalkChildren(V v, GoASTNode*)
{
    switch (m_kind)
    {
"""
print walker.getvalue()
print"""
    default:
        break;
    }
}

}  // namespace lldb_private

#endif
"""
