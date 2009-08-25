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

#include "qtdocparser.h"
#include "reporthandler.h"
#include <QtXmlPatterns/QXmlQuery>
#include <QUrl>

Documentation QtDocParser::retrieveModuleDocumentation()
{
    // TODO: This method of acquiring the module name supposes that the target language uses
    // dots as module separators in package names. Improve this.
    QString moduleName = QString(packageName()).remove(0, packageName().lastIndexOf('.') + 1);
    QString sourceFile = documentationDataDirectory() + '/' + moduleName.toLower() + ".xml";

    if (!QFile::exists(sourceFile)) {
        ReportHandler::warning("Can't find qdoc3 file for module "
                               + packageName() + ", tried: "
                               + sourceFile);
        return Documentation();
    }

    QXmlQuery xquery;
    xquery.setFocus(QUrl(sourceFile));

    // Module documentation
    QString query = "/WebXML/document/page[@name=\"" + moduleName + "\"]/description";
    return Documentation(getDocumentation(xquery, query, DocModificationList()));
}

void QtDocParser::fillDocumentation(AbstractMetaClass* metaClass)
{
    if (!metaClass)
        return;

    QString sourceFile = documentationDataDirectory() + '/' + metaClass->name().toLower() + ".xml";
    if (metaClass->enclosingClass())
        sourceFile.replace("::", "-");

    if (!QFile::exists(sourceFile)) {
        ReportHandler::warning("Can't find qdoc3 file for class "
                               + metaClass->name() + ", tried: "
                               + sourceFile);
        return;
    }

    QXmlQuery xquery;
    xquery.setFocus(QUrl(sourceFile));

    QString className = metaClass->name();
    QString nameType = metaClass->enclosingClass() ? "fullname" : "name";

    // Class/Namespace documentation
    QString type = metaClass->isNamespace() ? QLatin1String("namespace") : QLatin1String("class");
    QString query = "/WebXML/document/" + type + "[@" + nameType + "=\"" + className + "\"]/description";

    DocModificationList signedModifs, classModifs;
    foreach (DocModification docModif, metaClass->typeEntry()->docModifications()) {
        if (docModif.signature().isEmpty())
            classModifs.append(docModif);
        else
            signedModifs.append(docModif);
    }

    Documentation doc(getDocumentation(xquery, query, classModifs));
    metaClass->setDocumentation(doc);


    //Functions Documentation
    AbstractMetaFunctionList funcs = metaClass->functionsInTargetLang();
    foreach (AbstractMetaFunction *func, funcs) {
        if (!func || func->isPrivate())
            continue;

        QString query = "/WebXML/document/" + type + "[@" + nameType + "=\"" + className + "\"]";
        // properties
        if (func->isPropertyReader() || func->isPropertyWriter() || func->isPropertyResetter()) {
            query += "/property[@name=\"" + func->propertySpec()->name() + "\"]";
        } else { // normal methods
            QString isConst = func->isConstant() ? "true" : "false";
            query += "/function[@name=\"" + func->originalName()
                     + "\" and count(parameter)="
                     + QString::number(func->arguments().count())
                     + " and @const=\"" + isConst + "\"]";

            int i = 1;
            foreach (AbstractMetaArgument* arg, func->arguments()) {
                QString type = arg->type()->name();
                /* XXX Hack to get arguments working inside namespaces types */
                type = type.split("::").last();

                if (arg->type()->isConstant())
                    type.prepend("const ");

                if (arg->type()->isReference()) {
                    type += " &";
                } if (arg->type()->indirections()) {
                    type += ' ';
                    for (int j = 0, max = arg->type()->indirections(); j < max; ++j)
                        type += '*';
                }
                query += "/parameter[" + QString::number(i) + "][@left=\"" + type + "\"]/..";
                ++i;
            }
        }
        query += "/description";
        DocModificationList funcModifs;
        foreach (DocModification funcModif, signedModifs) {
            if (funcModif.signature() == func->minimalSignature())
                funcModifs.append(funcModif);
        }
        doc.setValue(getDocumentation(xquery, query, funcModifs));
        func->setDocumentation(doc);
    }
#if 0
    // Fields
    AbstractMetaFieldList fields = metaClass->fields();
    foreach (AbstractMetaField *field, fields) {
        if (field->isPrivate())
            return;

        QString query = "/doxygen/compounddef/sectiondef/memberdef/name[text()=\"" + field->name() + "\"]/..";
        Documentation doc = getDocumentation(DocModificationList(), xquery, query);
        field->setDocumentation(doc);
    }
#endif
    // Enums
    AbstractMetaEnumList enums = metaClass->enums();
    foreach (AbstractMetaEnum *meta_enum, enums) {
        QString query = "/WebXML/document/" + type + "[@" + nameType + "=\""
                        + className + "\"]/enum[@name=\""
                        + meta_enum->name() + "\"]/description";
        doc.setValue(getDocumentation(xquery, query, DocModificationList()));
        meta_enum->setDocumentation(doc);
    }
}