/*
* This file is part of the API Extractor project.
*
* Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
*
* Contact: PySide team <contact@pyside.org>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*/

#include "testaddfunction.h"
#include <QtTest/QTest>
#include "testutil.h"


void TestAddFunction::testParsingFuncNameAndConstness()
{
    // generic test...
    const char sig1[] = "func(type1, const type2, const type3* const)";
    AddedFunction f1(sig1, "void");
    QCOMPARE(f1.name(), QString("func"));
    QCOMPARE(f1.arguments().count(), 3);
    AddedFunction::TypeInfo retval = f1.returnType();
    QCOMPARE(retval.name, QString("void"));
    QCOMPARE(retval.indirections, 0);
    QCOMPARE(retval.isConstant, false);
    QCOMPARE(retval.isReference, false);

    // test with a ugly template as argument and other ugly stuff
    const char sig2[] = "    _fu__nc_       (  type1, const type2, const Abc<int& , C<char*> *   >  * *, const type3* const    )   const ";
    AddedFunction f2(sig2, "const Abc<int& , C<char*> *   >  * *");
    QCOMPARE(f2.name(), QString("_fu__nc_"));
    QList< AddedFunction::TypeInfo > args = f2.arguments();
    QCOMPARE(args.count(), 4);
    retval = f2.returnType();
    QCOMPARE(retval.name, QString("Abc<int& , C<char*> *   >"));
    QCOMPARE(retval.indirections, 2);
    QCOMPARE(retval.isConstant, true);
    QCOMPARE(retval.isReference, false);
    retval = args[2];
    QCOMPARE(retval.name, QString("Abc<int& , C<char*> *   >"));
    QCOMPARE(retval.indirections, 2);
    QCOMPARE(retval.isConstant, true);
    QCOMPARE(retval.isReference, false);

    // function with no args.
    const char sig3[] = "func()";
    AddedFunction f3(sig3, "void");
    QCOMPARE(f3.name(), QString("func"));
    QCOMPARE(f3.arguments().count(), 0);
}

void TestAddFunction::testAddFunction()
{
    const char cppCode[] = "struct B {}; struct A { void a(int); };";
    const char xmlCode[] = "\
    <typesystem package=\"Foo\">\
        <primitive-type name='int' />\
        <primitive-type name='float' />\
        <value-type name='B' />\
        <value-type name='A'>\
            <add-function signature='b(int, float = 4.6, const B&amp;)' return-type='int' access='protected'>\
            </add-function>\
        </value-type>\
    </typesystem>";
    TestUtil t(cppCode, xmlCode);
    TypeDatabase* typeDb = TypeDatabase::instance();
    AbstractMetaClassList classes = t.builder()->classes();
    AbstractMetaClass* classA = classes.findClass("A");
    QVERIFY(classA);
    QCOMPARE(classA->functions().count(), 3); // default ctor, func a() and the added function

    AbstractMetaFunction* addedFunc = classA->functions().last();
    QCOMPARE(addedFunc->visibility(), uint(AbstractMetaFunction::Protected));
    QCOMPARE(addedFunc->functionType(), AbstractMetaFunction::UserAddedFunction);
    QCOMPARE(addedFunc->ownerClass(), classA);
    QCOMPARE(addedFunc->implementingClass(), classA);
    QCOMPARE(addedFunc->declaringClass(), classA);
    QVERIFY(!addedFunc->isVirtual());
    QVERIFY(!addedFunc->isSignal());
    QVERIFY(!addedFunc->isSlot());
    QVERIFY(!addedFunc->isStatic());

    AbstractMetaType* returnType = addedFunc->type();
    QCOMPARE(returnType->typeEntry(), typeDb->findPrimitiveType("int"));
    AbstractMetaArgumentList args = addedFunc->arguments();
    QCOMPARE(args.count(), 3);
    QCOMPARE(args[0]->type()->typeEntry(), returnType->typeEntry());
    QCOMPARE(args[1]->defaultValueExpression(), QString("4.6"));
    QCOMPARE(args[2]->type()->typeEntry(), typeDb->findType("B"));
}

void TestAddFunction::testAddFunctionTagDefaultValues()
{
    const char cppCode[] = "struct A {};";
    const char xmlCode[] = "\
    <typesystem package=\"Foo\">\
        <value-type name='A'>\
            <add-function signature='func()' />\
        </value-type>\
    </typesystem>";
    TestUtil t(cppCode, xmlCode, false);
    AbstractMetaClassList classes = t.builder()->classes();
    AbstractMetaClass* classA = classes.findClass("A");
    QVERIFY(classA);
    QCOMPARE(classA->functions().count(), 2); // default ctor and the added function
    AbstractMetaFunction* addedFunc = classA->functions().last();
    QCOMPARE(addedFunc->visibility(), uint(AbstractMetaFunction::Public));
    QCOMPARE(addedFunc->functionType(), AbstractMetaFunction::UserAddedFunction);
    QVERIFY(!addedFunc->type());
}


void TestAddFunction::testAddFunctionCodeSnippets()
{
    const char cppCode[] = "struct A {};";
    const char xmlCode[] = "\
    <typesystem package=\"Foo\">\
        <value-type name='A'>\
            <add-function signature='func()'>\
                <inject-code class='target' position='end'>Hi!, I am the code.</inject-code>\
            </add-function>\
        </value-type>\
    </typesystem>";

    TestUtil t(cppCode, xmlCode, false);
    AbstractMetaClassList classes = t.builder()->classes();
    AbstractMetaClass* classA = classes.findClass("A");
    QVERIFY(classA);
    AbstractMetaFunction* addedFunc = classA->functions().last();
    QVERIFY(addedFunc->hasInjectedCode());
}


QTEST_APPLESS_MAIN(TestAddFunction)

#include "testaddfunction.moc"