/*##############################################################################

HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
############################################################################## */

#ifndef HPCCFILE_HPP_
#define HPCCFILE_HPP_

#include "ws_sql.hpp"
#include "SQLColumn.hpp"

#include "ws_sql_esp.ipp"
#include "hqlerror.hpp"
#include "hqlexpr.hpp"

/* undef SOCKET definitions to avoid collision in Antlrdefs.h*/
#ifdef INVALID_SOCKET
    //#pragma message( "UNDEFINING INVALID_SOCKET - Will be redefined by ANTLRDEFS.h" )
    #undef INVALID_SOCKET
#endif
#ifdef SOCKET
    //#pragma message( "UNDEFINING SOCKET - Will be redefined by ANTLRDEFS.h" )
    #undef SOCKET
#endif

#include "HPCCSQLLexer.h"
#include "HPCCSQLParser.h"

typedef enum _HPCCFileFormat
{
    HPCCFileFormatUnknown=-1,
    HPCCFileFormatFlat,
    HPCCFileFormatCSV,
    HPCCFileFormatXML,
    HPCCFileFormatKey,
} HPCCFileFormat;

class HPCCFile : public CInterface, public IInterface
{
public:
    static HPCCFileFormat DEFAULTFORMAT;

    IMPLEMENT_IINTERFACE;
    HPCCFile();
    virtual ~HPCCFile();

    const char * getCluster() const
    {
        return cluster.str();
    }

    void setCluster(const char * cluster)
    {
        this->cluster.set(cluster);
    }

    HPCCColumnMetaData * getColumn(const char * colname);

    IArrayOf<HPCCColumnMetaData> * getColumns()
    {
        return &columns;
    }

    const char * getEcl() const
    {
        return ecl;
    }

    bool setEcl(const char * ecl)
    {
        if (setFileColumns(ecl))
            this->ecl = ecl;
        else
            return false;

        return true;
    }

    const char * getFormat() const
    {
        switch(formatEnum)
        {
            case HPCCFileFormatFlat:
                return "FLAT";
            case HPCCFileFormatCSV:
                return "CSV";
            case HPCCFileFormatXML:
                return "XML";
            case HPCCFileFormatKey:
                return "KEYED";
            case HPCCFileFormatUnknown:
            default:
                return "UNKNOWN";
        }
    }

    void setFormat(const char * format)
    {
        if (!format || !*format)
            this->formatEnum = DEFAULTFORMAT;
        else
        {
            if (stricmp(format, "FLAT")==0)
                this->formatEnum = HPCCFileFormatFlat;
            else if (stricmp(format, "utf8n")==0)
                this->formatEnum = HPCCFileFormatCSV;
            else if (stricmp(format, "CSV")==0)
                this->formatEnum = HPCCFileFormatCSV;
            else if (stricmp(format, "XML")==0)
                this->formatEnum = HPCCFileFormatXML;
            else if (stricmp(format, "KEY")==0)
                this->formatEnum = HPCCFileFormatKey;
            else
                this->formatEnum = DEFAULTFORMAT;
        }
    }

    const char * getFullname() const
    {
        return fullname.str();
    }

    void setFullname(const char * fullname)
    {
        this->fullname.set(fullname);
    }

    bool isFileKeyed() const
    {
        return iskeyfile;
    }

    void setIsKeyedFile(bool iskeyfile)
    {
        this->iskeyfile = iskeyfile;
    }

    bool isFileSuper() const
    {
        return issuperfile;
    }

    void setIsSuperfile(bool issuperfile)
    {
        this->issuperfile = issuperfile;
    }

    const char * getName() const
    {
        return name.str();
    }

    void setName(const char * name)
    {
        this->name.set(name);
    }

    static bool validateFileName(const char * fullname);
    void setKeyedColumn(const char * name);
    bool getFileRecDefwithIndexpos(HPCCColumnMetaData * fieldMetaData, StringBuffer & out, const char * structname);
    bool getFileRecDef(StringBuffer & out, const char * structname, const char * linedelimiter  = "\n", const char * recordindent  = "\t");
    int getNonKeyedColumnsCount();
    int getKeyedColumnsCount();
    void getKeyedFieldsAsDelimitedString(char delim, const char * prefix, StringBuffer & out);
    void getNonKeyedFieldsAsDelmitedString(char delim, const char * prefix, StringBuffer & out);
    const char * getIdxFilePosField()
    {
       return idxFilePosField.length() > 0 ? idxFilePosField.str() : getLastNonKeyedNumericField();
    }

    bool hasValidIdxFilePosField()
    {
        const char * posfield = getIdxFilePosField();
        return (posfield && *posfield);
    }

    static HPCCFile * createHPCCFile();

    bool containsField(SQLColumn * field, bool verifyEclType) const;

    const char * getOwner() const
    {
        return owner.str();
    }

    void setOwner(const char * owner)
    {
        this->owner.set(owner);
    }

    const char* getDescription() const
    {
        return description.str();
    }

    bool static inline isBlankChar(const char * thechar)
    {
        if (thechar && *thechar)
        {
            switch (*thechar)
            {
                case ' ':
                case '\r':
                case '\n':
                case '\t':
                    return true;
                default:
                    return false;
            }
        }
        return false;
    }

    void setDescription(const char* description)
    {
        if (description && *description)
        {
            this->description.set(description);
            const char * pos = strstr(description, "XDBC:RelIndexes");
            if (pos)
            {
                pos += 15;//advance to end of "XDBC:RelIndexes"
                while(pos && *pos) //find the = char
                {
                    if (isBlankChar(pos))
                        pos++;
                    else if (*pos == '=' )
                    {
                        pos++;
                        break;
                    }
                    else
                        return;//perhaps log?
                }

                while(pos && *pos) //find the beginning bracket
                {
                    if (isBlankChar(pos))
                        pos++;
                    else if (*pos == '[' )
                    {
                        pos++;
                        break;
                    }
                    else
                        return;//perhaps log?
                }

                if ( pos && *pos) //found keyword
                    setRelatedIndexes(pos);
            }
        }
    }

    void getRelatedIndexes(StringArray & indexes)
    {
        ForEachItemIn(c, relIndexFiles)
        {
            indexes.append(relIndexFiles.item(c));
        }
    }

    const char * getRelatedIndex(int relindexpos)
    {
        if (relindexpos > -1 && relindexpos < relIndexFiles.length())
            return relIndexFiles.item(relindexpos);
        else
            return NULL;
    }

    int getRelatedIndexCount()
    {
        return relIndexFiles.length();
    }
  /*
    tutorial::yn::peoplebyzipindex;
    Tutorial.IDX_PeopleByName;
    Tutorial.IDX_PeopleByPhonetic]
*/
    void setRelatedIndexes(const char * str)
    {
        StringBuffer index;
        while (str && *str)
        {
            if (!isBlankChar(str))
            {
                if  (*str == ';' || *str == ']')
                {
                    relIndexFiles.append(index.str());
                    if (*str == ']')
                        break;
                    else
                    index.clear();
                }
                else
                    index.append(*str);
            }
            str++;
        }
    }

    void setIdxFilePosField(const char* idxfileposfieldname)
    {
        idxFilePosField.set(idxfileposfieldname);
    }

    bool containsNestedColumns() const
    {
        return hasNestedColumns;
    }

    void setHasNestedColumns(bool hasNestedColumns)
    {
        this->hasNestedColumns = hasNestedColumns;
    }

private:
    void getFieldsAsDelmitedString(char delim, const char* prefix, StringBuffer& out, bool onlykeyed);
    bool setFileColumns(const char* eclString);
    void setKeyCounts();

    const char* getLastNonKeyedNumericField()
    {
        for (int i = columns.length() - 1;i >= 0;i--)
        {
            if (!columns.item(i).isKeyedField())
                return columns.item(i).getColumnName();
        }
        return NULL;
    }

private:
    HPCCFileFormat formatEnum;
    StringBuffer name;
    StringBuffer fullname;
    StringBuffer cluster;
    StringBuffer idxFilePosField;
    bool iskeyfile;
    bool issuperfile;
    StringBuffer ecl;
    IArrayOf<HPCCColumnMetaData> columns;
    int keyedCount;
    int nonKeyedCount;
    bool hasNestedColumns;
    StringBuffer description;
    StringBuffer owner;
    StringArray relIndexFiles;
}
;

#endif /* HPCCFILE_HPP_ */
