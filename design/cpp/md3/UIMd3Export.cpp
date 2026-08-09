/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Export class implementation.
 */

/*
 * Copyright (C) 2026 Material Virtual Machine contributors.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */


/* Qt includes: */
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

/* GUI includes: */
#include "UIMd3Export.h"

QStringList UIMd3Export::formats()
{
    return QStringList() << "md" << "txt" << "json" << "jsonl" << "yaml" << "toml" << "xml"
                         << "csv" << "tsv" << "html" << "sql" << "ts" << "py" << "go" << "rs"
                         << "proto" << "schema.json";
}

static QString escapeXml(const QString &str)
{
    QString out = str;
    out.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;").replace('"', "&quot;");
    return out;
}

static QString escapeCsv(const QString &str)
{
    if (!str.contains(',') && !str.contains('"') && !str.contains('\n'))
        return str;
    QString out = str;
    out.replace('"', "\"\"");
    return '"' + out + '"';
}

static QString identifier(const QString &str)
{
    QString out;
    foreach (const QChar &ch, str)
        out += ch.isLetterOrNumber() ? ch : QChar('_');
    if (!out.isEmpty() && out.at(0).isDigit())
        out.prepend('_');
    return out;
}

QByteArray UIMd3Export::serialize(const QVector<UIMd3ExportRow> &rows, const QString &strFormat, const QString &strTitle)
{
    QStringList sections;
    foreach (const UIMd3ExportRow &row, rows)
        if (!sections.contains(row.strSection))
            sections << row.strSection;

    QString out;

    if (strFormat == "md")
    {
        out += QString("# %1\n\n").arg(strTitle);
        foreach (const QString &strSection, sections)
        {
            out += QString("## %1\n\n").arg(strSection);
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    out += QString("- **%1**: %2\n").arg(row.strKey, row.strValue);
            out += "\n";
        }
    }
    else if (strFormat == "txt")
    {
        out += strTitle + "\n\n";
        foreach (const UIMd3ExportRow &row, rows)
            out += QString("%1 / %2: %3\n").arg(row.strSection, row.strKey, row.strValue);
    }
    else if (strFormat == "json" || strFormat == "schema.json")
    {
        QJsonObject root;
        foreach (const QString &strSection, sections)
        {
            QJsonObject section;
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    section.insert(row.strKey, row.strValue);
            root.insert(strSection, section);
        }
        if (strFormat == "schema.json")
        {
            QJsonObject schema;
            schema.insert("$schema", "https://json-schema.org/draft/2020-12/schema");
            schema.insert("title", strTitle);
            schema.insert("type", "object");
            QJsonObject properties;
            foreach (const QString &strSection, sections)
            {
                QJsonObject sectionSchema;
                sectionSchema.insert("type", "object");
                properties.insert(strSection, sectionSchema);
            }
            schema.insert("properties", properties);
            return QJsonDocument(schema).toJson(QJsonDocument::Indented);
        }
        return QJsonDocument(root).toJson(QJsonDocument::Indented);
    }
    else if (strFormat == "jsonl")
    {
        foreach (const UIMd3ExportRow &row, rows)
        {
            QJsonObject entry;
            entry.insert("section", row.strSection);
            entry.insert("key", row.strKey);
            entry.insert("value", row.strValue);
            out += QString::fromUtf8(QJsonDocument(entry).toJson(QJsonDocument::Compact)) + "\n";
        }
    }
    else if (strFormat == "yaml")
    {
        out += QString("title: %1\n").arg(strTitle);
        foreach (const QString &strSection, sections)
        {
            out += QString("%1:\n").arg(identifier(strSection));
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    out += QString("  %1: \"%2\"\n").arg(identifier(row.strKey), row.strValue);
        }
    }
    else if (strFormat == "toml")
    {
        out += QString("title = \"%1\"\n\n").arg(strTitle);
        foreach (const QString &strSection, sections)
        {
            out += QString("[%1]\n").arg(identifier(strSection));
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    out += QString("%1 = \"%2\"\n").arg(identifier(row.strKey), row.strValue);
            out += "\n";
        }
    }
    else if (strFormat == "xml")
    {
        out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out += QString("<view title=\"%1\">\n").arg(escapeXml(strTitle));
        foreach (const QString &strSection, sections)
        {
            out += QString("  <section name=\"%1\">\n").arg(escapeXml(strSection));
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    out += QString("    <item key=\"%1\">%2</item>\n").arg(escapeXml(row.strKey), escapeXml(row.strValue));
            out += "  </section>\n";
        }
        out += "</view>\n";
    }
    else if (strFormat == "csv" || strFormat == "tsv")
    {
        const QString strSep = strFormat == "csv" ? "," : "\t";
        out += QStringList({ "section", "key", "value" }).join(strSep) + "\n";
        foreach (const UIMd3ExportRow &row, rows)
            out += QStringList({ escapeCsv(row.strSection), escapeCsv(row.strKey), escapeCsv(row.strValue) }).join(strSep) + "\n";
    }
    else if (strFormat == "html")
    {
        out += QString("<!doctype html><meta charset=\"utf-8\"><title>%1</title>\n<h1>%1</h1>\n").arg(escapeXml(strTitle));
        foreach (const QString &strSection, sections)
        {
            out += QString("<h2>%1</h2>\n<table>\n").arg(escapeXml(strSection));
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    out += QString("<tr><th>%1</th><td>%2</td></tr>\n").arg(escapeXml(row.strKey), escapeXml(row.strValue));
            out += "</table>\n";
        }
    }
    else if (strFormat == "sql")
    {
        out += "CREATE TABLE IF NOT EXISTS view_export (section TEXT, key TEXT, value TEXT);\n";
        foreach (const UIMd3ExportRow &row, rows)
        {
            QString v = row.strValue; v.replace('\'', "''");
            QString k = row.strKey;   k.replace('\'', "''");
            QString s = row.strSection; s.replace('\'', "''");
            out += QString("INSERT INTO view_export VALUES ('%1', '%2', '%3');\n").arg(s, k, v);
        }
    }
    else if (strFormat == "ts")
    {
        out += QString("export const view = {\n  title: %1,\n").arg(QJsonDocument(QJsonObject{{"t", strTitle}}).object().value("t").toString().prepend('"').append('"'));
        foreach (const QString &strSection, sections)
        {
            out += QString("  %1: {\n").arg(identifier(strSection));
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    out += QString("    %1: \"%2\",\n").arg(identifier(row.strKey), row.strValue);
            out += "  },\n";
        }
        out += "} as const;\n";
    }
    else if (strFormat == "py")
    {
        out += QString("VIEW = {\n    \"title\": \"%1\",\n").arg(strTitle);
        foreach (const QString &strSection, sections)
        {
            out += QString("    \"%1\": {\n").arg(strSection);
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    out += QString("        \"%1\": \"%2\",\n").arg(row.strKey, row.strValue);
            out += "    },\n";
        }
        out += "}\n";
    }
    else if (strFormat == "go")
    {
        out += "package view\n\nvar View = map[string]map[string]string{\n";
        foreach (const QString &strSection, sections)
        {
            out += QString("\t\"%1\": {\n").arg(strSection);
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    out += QString("\t\t\"%1\": \"%2\",\n").arg(row.strKey, row.strValue);
            out += "\t},\n";
        }
        out += "}\n";
    }
    else if (strFormat == "rs")
    {
        out += "pub const VIEW: &[(&str, &str, &str)] = &[\n";
        foreach (const UIMd3ExportRow &row, rows)
            out += QString("    (\"%1\", \"%2\", \"%3\"),\n").arg(row.strSection, row.strKey, row.strValue);
        out += "];\n";
    }
    else if (strFormat == "proto")
    {
        out += "syntax = \"proto3\";\n\n";
        foreach (const QString &strSection, sections)
        {
            out += QString("message %1 {\n").arg(identifier(strSection));
            int iField = 1;
            foreach (const UIMd3ExportRow &row, rows)
                if (row.strSection == strSection)
                    out += QString("  string %1 = %2;\n").arg(identifier(row.strKey)).arg(iField++);
            out += "}\n\n";
        }
    }
    else
        out = strTitle;

    return out.toUtf8();
}

bool UIMd3Export::writeToFile(const QVector<UIMd3ExportRow> &rows, const QString &strPath, const QString &strTitle, QString &strError)
{
    const QString strFormat = QFileInfo(strPath).completeSuffix();
    QFile file(strPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        strError = file.errorString();
        return false;
    }
    file.write(serialize(rows, strFormat, strTitle));
    file.close();
    return true;
}

void UIMd3Export::copyToClipboard(const QVector<UIMd3ExportRow> &rows, const QString &strFormat, const QString &strTitle)
{
    QApplication::clipboard()->setText(QString::fromUtf8(serialize(rows, strFormat, strTitle)));
}
