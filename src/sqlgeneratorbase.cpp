/**************************************************************************
**
** This file is part of Nut project.
** https://github.com/HamedMasafi/Nut
**
** Nut is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Nut is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with Nut.  If not, see <http://www.gnu.org/licenses/>.
**
**************************************************************************/

#include <QDate>
#include <QDateTime>
#include <QPointF>
#include <QTime>
#include <QVariant>

#include "database.h"
#include "databasemodel.h"
#include "sqlgeneratorbase_p.h"
#include "table.h"
#include "tablemodel.h"
#include "wherephrase.h"
#include "sqlvalue.h"

NUT_BEGIN_NAMESPACE

SqlGeneratorBase::SqlGeneratorBase(Database *parent)
    : QObject((QObject *)parent)
{
    if (parent)
        _database = parent;
}

SqlGeneratorBase::~SqlGeneratorBase()
{
}

QString SqlGeneratorBase::masterDatabaseName(QString databaseName)
{
    Q_UNUSED(databaseName);
    return "";
}

QString SqlGeneratorBase::saveRecord(Table *t, QString tableName)
{
    Q_ASSERT(!tableName.isEmpty() && !tableName.isNull());
    switch (t->status()) {
    case Table::Added:
        return insertRecord(t, tableName);

    case Table::Deleted:
        return deleteRecord(t, tableName);

    case Table::Modified:
        return updateRecord(t, tableName);

    case Table::NewCreated:
    case Table::FeatchedFromDB:
        Q_UNREACHABLE();
    }

    return "";
}

QString SqlGeneratorBase::fieldDeclare(FieldModel *field)
{
    return field->name + " " + fieldType(field);
}

QStringList SqlGeneratorBase::diff(DatabaseModel lastModel,
                                   DatabaseModel newModel)
{
    QStringList ret;

    QSet<QString> tableNames;
    foreach (TableModel *table, lastModel)
        tableNames.insert(table->name());

    foreach (TableModel *table, newModel)
        tableNames.insert(table->name());

    foreach (QString tableName, tableNames) {
        TableModel *oldTable = lastModel.tableByName(tableName);
        TableModel *newTable = newModel.tableByName(tableName);
        QString sql = diff(oldTable, newTable);
        ret << sql;
    }

    return ret;
}

QString SqlGeneratorBase::diff(FieldModel *oldField, FieldModel *newField)
{
    QString sql = "";
    if (oldField && newField)
        if (*oldField == *newField)
            return QString::null;

    if (!newField) {
        sql = "DROP COLUMN " + oldField->name;
    } else {
        if (oldField)
            sql = "MODIFY COLUMN ";
        else
            sql = "ADD COLUMN ";
        sql.append(fieldDeclare(newField));
    }
    return sql;
}

QString SqlGeneratorBase::diff(TableModel *oldTable, TableModel *newTable)
{
    if (oldTable && newTable)
        if (*oldTable == *newTable)
            return "";

    if (!newTable)
        return "DROP TABLE " + oldTable->name();

    QSet<QString> fieldNames;

    if (oldTable)
        foreach (FieldModel *f, oldTable->fields())
            fieldNames.insert(f->name);

    foreach (FieldModel *f, newTable->fields())
        fieldNames.insert(f->name);

    QStringList columnSql;
    foreach (QString fieldName, fieldNames) {
        FieldModel *newField = newTable->field(fieldName);
        if (oldTable) {
            FieldModel *oldField = oldTable->field(fieldName);

            QString buffer = diff(oldField, newField);
            if (!buffer.isNull())
                columnSql << buffer;
        } else {
            columnSql << fieldDeclare(newField);
        }
    }
    QString sql;
    if (oldTable) {
        sql = QString("ALTER TABLE %1 \n%2").arg(newTable->name()).arg(
            columnSql.join(",\n"));
    } else {
        if (!newTable->primaryKey().isNull())
            columnSql << QString("CONSTRAINT pk_%1 PRIMARY KEY (%2)")
                             .arg(newTable->name())
                             .arg(newTable->primaryKey());

        sql = QString("CREATE TABLE %1 \n(%2)").arg(newTable->name()).arg(
            columnSql.join(",\n"));
    }
    return sql;
}

QString SqlGeneratorBase::insertRecord(Table *t, QString tableName)
{
    QString sql = "";
    QString key = t->primaryKey();
    QStringList values;

    foreach (QString f, t->changedProperties())
        if (f != key)
            values.append(QString("'%1'")
                          .arg(_database->serializeSqlValue(t->property(f.toLatin1().data()))));

    QString changedPropertiesText = "";
    QSet<QString> props = t->changedProperties();
    foreach (QString s, props) {
        if (changedPropertiesText != "")
            changedPropertiesText.append(", ");
        changedPropertiesText.append(s);
    }
    sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
              .arg(tableName)
              .arg(changedPropertiesText)
              .arg(values.join(", "));

    replaceTableNames(sql);

    return sql;
}

QString SqlGeneratorBase::updateRecord(Table *t, QString tableName)
{
    QString sql = "";
    QString key = t->primaryKey();
    QStringList values;

    foreach (QString f, t->changedProperties())
        if (f != key)
            values.append(QString("%1 ='%2'")
                          .arg(f)
                          .arg(_database->serializeSqlValue(t->property(f.toLatin1().data()))));

    sql = QString("UPDATE %1 SET %2 WHERE %3=%4")
              .arg(tableName)
              .arg(values.join(", "))
              .arg(key)
              .arg(t->primaryValue().toString());

    replaceTableNames(sql);

    return sql;
}

QString SqlGeneratorBase::deleteRecord(Table *t, QString tableName)
{
    QString sql = QString("DELETE FROM %1 WHERE %2='%3'")
        .arg(tableName)
        .arg(t->primaryKey())
        .arg(t->primaryValue().toString());
    replaceTableNames(sql);
    return sql;
}

QString SqlGeneratorBase::agregateText(const AgregateType &t,
                                       const QString &arg) const
{
    switch (t) {
    case SelectAll:
        return "*";
        break;

    case Min:
        return "MIN(" + arg + ")";
        break;

    case Max:
        return "MAX(" + arg + ")";
        break;

    case Average:
        return "AVERAGE(" + arg + ")";
        break;

    case Count:
        return "COUNT(" + arg + ")";
        break;

    case SignleField:
        return arg;
        break;

    default:
        return QString::null;
    }
}

QString SqlGeneratorBase::fromTableText(const QString &tableName,
                                        QString &joinClassName,
                                        QString &orderBy) const
{
    QString tableNameText = tableName;
    if (!joinClassName.isNull()) {
        QString joinTableName = _database->tableName(joinClassName);
        RelationModel *rel
            = _database->model().relationByTableNames(tableName, joinTableName);
        if (rel) {
            QString pk = _database->model().tableByName(tableName)->primaryKey();
            tableNameText = QString("%1 INNER JOIN %2 ON (%1.%3 = %2.%4)")
                                .arg(tableName)
                                .arg(joinTableName)
                                .arg(pk)
                                .arg(rel->localColumn);
            orderBy = tableName + "." + pk;
        } else {
            qWarning(
                QString(
                    "Relation between table %1 and class %2 (%3) not exists!")
                    .arg(tableName)
                    .arg(joinClassName)
                    .arg(joinTableName.isNull() ? "NULL" : joinTableName)
                    .toLatin1()
                    .data());
            joinClassName = QString::null;
        }
    }

    return tableNameText;
}

QString SqlGeneratorBase::deleteRecords(QString tableName, QString where)
{
    QString sql = "";
    if (where.isEmpty() || where.isNull())
        sql = "DELETE FROM " + tableName;
    else
        sql = "DELETE FROM " + tableName + " WHERE " + where;

    replaceTableNames(sql);

    return sql;
}

QString SqlGeneratorBase::selectCommand(SqlGeneratorBase::AgregateType t,
                                        QString agregateArg,
                                        QList<WherePhrase> &wheres,
                                        QList<WherePhrase> &orders,
                                        QString tableName,
                                        QString joinClassName, int skip, int take)
{
    QString select = agregateText(t, agregateArg);
    QString where = createWhere(wheres);
    QString order = "";
    QString from = fromTableText(tableName, joinClassName, order);

    foreach (WherePhrase p, orders) {
        if (order != "")
            order.append(", ");
        order.append(phraseOrder(p.data()));
    }

    QString sql = "SELECT " + select + " FROM " + from;

    if (where != "")
        sql.append(" WHERE " + where);

    if (order != "")
        sql.append(" ORDER BY " + order);

    for (int i = 0; i < _database->model().count(); i++)
        sql = sql.replace(_database->model().at(i)->className() + ".",
                          _database->model().at(i)->name() + ".");

    replaceTableNames(sql);

    return sql;
}

QString SqlGeneratorBase::createWhere(QList<WherePhrase> &wheres)
{
    QString whereText = "";

    foreach (WherePhrase w, wheres) {
        if (whereText != "")
            whereText.append(" AND ");

        whereText.append(phrase(w.data()));
    }

    return whereText;
}

void SqlGeneratorBase::replaceTableNames(QString &command)
{
    foreach (TableModel *m, TableModel::allModels())
        command = command.replace("[" + m->className() + "].", m->name() + ".");
}

QString SqlGeneratorBase::deleteCommand(QList<WherePhrase> &wheres,
                                        QString tableName)
{
    QString command = "DELETE FROM " + tableName;
    QString where = createWhere(wheres);

    if (where != "")
        command.append(" WHERE " + where);

    for (int i = 0; i < _database->model().count(); i++)
        command = command.replace(_database->model().at(i)->className() + ".",
                                  _database->model().at(i)->name() + ".");

    replaceTableNames(command);

    return command;
}

QString SqlGeneratorBase::updateCommand(WherePhrase &phrase,
                                        QList<WherePhrase> &wheres,
                                        QString tableName)
{
    QString p = this->phrase(phrase.data());
    QString where = createWhere(wheres);

    QString sql = "UPDATE " + tableName + " SET " + p;

    if (where != "")
        sql.append(" WHERE " + where);

    for (int i = 0; i < _database->model().count(); i++)
        sql = sql.replace(_database->model().at(i)->className() + ".",
                          _database->model().at(i)->name() + ".");

    replaceTableNames(sql);

    return sql;
}

QString SqlGeneratorBase::escapeValue(const QVariant &v) const
{
    switch (v.type()) {
    case QVariant::Bool:
        return v.toBool() ? "1" : "0";
        break;

    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::ULongLong:
    case QVariant::LongLong:
    case QVariant::Double:
        return v.toString();
        break;

    case QVariant::Char:
    case QVariant::String:
        return "'" + v.toString() + "'";

    case QVariant::DateTime:
        return "'" + v.toDateTime().toString() + "'";

    case QVariant::Date:
        return "'" + v.toDate().toString() + "'";

    case QVariant::Time:
        return "'" + v.toTime().toString() + "'";

    case QVariant::StringList:
    case QVariant::List:
        return "('" + v.toStringList().join("', '") + "')";

    case QVariant::Point: {
        QPoint pt = v.toPoint();
        return QString("POINT(%1 %2)").arg(pt.x()).arg(pt.y());
    }

    case QVariant::PointF: {
        QPointF pt = v.toPointF();
        return QString("POINT(%1 %2)").arg(pt.x()).arg(pt.y());
    }

    case QVariant::Invalid:
        qFatal("Invalud field value");
        return "<FAIL>";
    }
    return "";
}

QVariant SqlGeneratorBase::readValue(const QVariant::Type &type,
                                     const QVariant &dbValue)
{
    return dbValue;
}

QString SqlGeneratorBase::phraseOrder(const PhraseData *d) const
{

    QString ret = "";

    switch (d->type) {
    case PhraseData::Field:
        if (d->operatorCond == PhraseData::Not)
            ret = d->text + " DESC";
        else
            ret = d->text;
        break;

    case PhraseData::WithOther:
        if (d->operatorCond != PhraseData::Append)
            qFatal("Order phease can only have & operator");

        ret = phraseOrder(d->left) + ", " + phraseOrder(d->right);
        break;

    case PhraseData::WithoutOperand:
    case PhraseData::WithVariant:
        break;
    }

    return ret;
}

QString SqlGeneratorBase::phrase(const PhraseData *d) const
{
    QString ret = "";

    switch (d->type) {
    case PhraseData::Field:
        ret = d->text;
        break;

    case PhraseData::WithVariant:
        ret = phrase(d->left) + " " + operatorString(d->operatorCond) + " "
              + escapeValue(d->operand);
        break;

    case PhraseData::WithOther:
        ret = phrase(d->left) + " " + operatorString(d->operatorCond) + " "
              + phrase(d->right);
        break;

    case PhraseData::WithoutOperand:
        ret = phrase(d->left) + " " + operatorString(d->operatorCond);
        break;

    default:
        ret = "<FAIL>";
    }

    if (d->operatorCond == PhraseData::And || d->operatorCond == PhraseData::Or)
        ret = "(" + ret + ")";

    return ret;
}

QString SqlGeneratorBase::phraseUpdate(const PhraseData *d) const
{
    QString ret = "";

    if (d->operatorCond != PhraseData::And
        && d->operatorCond != PhraseData::Equal)
        qFatal("Update command does not accept any phrase else &, =");

    switch (d->type) {
    case PhraseData::Field:
        ret = d->text;
        break;

    case PhraseData::WithVariant:
        ret = phrase(d->left) + " " + operatorString(d->operatorCond) + " "
              + escapeValue(d->operand);
        break;

    case PhraseData::WithOther:
        ret = phrase(d->left) + " " + operatorString(d->operatorCond) + " "
              + phrase(d->right);
        break;

    case PhraseData::WithoutOperand:
        ret = phrase(d->left) + " " + operatorString(d->operatorCond);
        break;

    default:
        ret = "<FAIL>";
    }

    if (d->operatorCond == PhraseData::And || d->operatorCond == PhraseData::Or)
        ret = "(" + ret + ")";

    return ret;
}

QString
SqlGeneratorBase::operatorString(const PhraseData::Condition &cond) const
{
    switch (cond) {
    case PhraseData::Equal:
        return "=";
    case PhraseData::NotEqual:
        return "<>";
    case PhraseData::Less:
        return "<";
    case PhraseData::Greater:
        return ">";
    case PhraseData::LessEqual:
        return "<=";
    case PhraseData::GreaterEqual:
        return ">=";
    case PhraseData::Null:
        return "IS NULL";

    case PhraseData::NotNull:
        return "IS NOT NULL";

    case PhraseData::In:
        return "IN";

    case PhraseData::NotIn:
        return "NOT IN";

    case PhraseData::And:
        return "AND";
    case PhraseData::Or:
        return "OR";

    case PhraseData::Like:
        return "LIKE";
    case PhraseData::NotLike:
        return "NOT LIKE";

    case PhraseData::Add:
        return "+";
    case PhraseData::Minus:
        return "-";
    case PhraseData::Multiple:
        return "*";
    case PhraseData::Divide:
        return "/";

    case PhraseData::Set:
        return "=";

    case PhraseData::Append:
        return ",";
    }

    return QString("<FAIL>");
}

NUT_END_NAMESPACE
