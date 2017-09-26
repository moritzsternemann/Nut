QT += core sql

INCLUDEPATH += $$PWD/include

HEADERS += \
    $$PWD/src/tableset.h \
    $$PWD/src/defines_p.h \
    $$PWD/src/defines.h \
    $$PWD/src/query.h \
    $$PWD/src/databasemodel.h \
    $$PWD/src/sqlgeneratorbase_p.h \
    $$PWD/src/postgresqlgenerator.h \
    $$PWD/src/changelogtable.h \
    $$PWD/src/tablesetbase_p.h \
    $$PWD/src/querybase_p.h \
    $$PWD/src/mysqlgenerator.h \
    $$PWD/src/sqlitegenerator.h \
    $$PWD/src/tablemodel.h \
    $$PWD/src/sqlservergenerator.h \
    $$PWD/src/wherephrase.h \
    $$PWD/src/query_p.h \
    $$PWD/src/table.h \
    $$PWD/src/database.h \
    $$PWD/src/database_p.h \
    $$PWD/src/dbgeography.h \
    $$PWD/src/sqlvalue.h

SOURCES += \
    $$PWD/src/tableset.cpp \
    $$PWD/src/query.cpp \
    $$PWD/src/databasemodel.cpp \
    $$PWD/src/tablesetbase.cpp \
    $$PWD/src/sqlgeneratorbase.cpp \
    $$PWD/src/postgresqlgenerator.cpp \
    $$PWD/src/changelogtable.cpp \
    $$PWD/src/querybase.cpp \
    $$PWD/src/mysqlgenerator.cpp \
    $$PWD/src/sqlitegenerator.cpp \
    $$PWD/src/tablemodel.cpp \
    $$PWD/src/sqlservergenerator.cpp \
    $$PWD/src/wherephrase.cpp \
    $$PWD/src/table.cpp \
    $$PWD/src/database.cpp \
    $$PWD/src/dbgeography.cpp \
    $$PWD/src/sqlvalue.cpp
