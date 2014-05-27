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

#include "HPCCFileCache.hpp"


HPCCFileCache * HPCCFileCache::createFileCache(const char * username, const char * passwd)
{
    return new HPCCFileCache(username,passwd);
}

bool HPCCFileCache::populateTablesResponse(IEspGetDBMetaDataResponse & tablesrespstruct, const char * filterby)
{
    bool success = false;

    cacheAllHpccFiles(filterby);
    IArrayOf<IEspHPCCTable> tables;

    HashIterator iterHash(cache);
    ForEach(iterHash)
    {
       const char* key = (const char*)iterHash.query().getKey();
       HPCCFilePtr file = dynamic_cast<HPCCFile *>(*cache.getValue(key));
       if ( file )
       {
           Owned<IEspHPCCTable> pTable = createHPCCTable();
           pTable->setName(file->getFullname());
           pTable->setFormat(file->getFormat());
           const char * ecl = file->getEcl();

           if (!ecl || !*ecl)
               continue;
           else
           {
               pTable->setIsKeyed(file->isFileKeyed());
               pTable->setIsSuper(file->isFileSuper());

               IArrayOf<IEspHPCCColumn> pColumns;

               IArrayOf<HPCCColumnMetaData> * cols = file->getColumns();
               for (int i = 0; i < cols->length(); i++)
               {
                   Owned<IEspHPCCColumn> pCol = createHPCCColumn();
                   HPCCColumnMetaData currcol = cols->item(i);
                   pCol->setName(currcol.getColumnName());
                   pCol->setType(currcol.getColumnType());
                   pColumns.append(*pCol.getLink());
               }
               pTable->setColumns(pColumns);
               tables.append(*pTable.getLink());
           }
        }
     }

     tablesrespstruct.setTables(tables);
     return success;
 }

bool HPCCFileCache::fetchHpccFilesByTableName(IArrayOf<SQLTable> * sqltables, HpccFiles * hpccfilecache)
{
    bool allFound = true;

    ForEachItemIn(tableindex, *sqltables)
    {
       SQLTable table = sqltables->item(tableindex);
       allFound &= cacheHpccFileByName(table.getName());
    }

    return allFound;
}

bool HPCCFileCache::cacheAllHpccFiles(const char * filterby)
{
    bool success = false;
    StringBuffer filter;
    if(filterby && *filterby)
        filter.append(filterby);
    else
        filter.append("*");

    Owned<IDFAttributesIterator> fi = queryDistributedFileDirectory().getDFAttributesIterator(filter, userdesc.get(), true, false, NULL);
    if(!fi)
        throw MakeStringException(-1,"Cannot get information from file system.");


    success = true;
    ForEach(*fi)
    {
       IPropertyTree &attr=fi->query();

#if defined(_DEBUG)
    StringBuffer toxml;
    toXML(&attr, toxml);
    fprintf(stderr, "%s", toxml.str());
#endif

       StringBuffer name(attr.queryProp("@name"));

       if (name.length()>0)
           success &= cacheHpccFileByName(name.str());
    }

    return success;
}

bool HPCCFileCache::cacheHpccFileByName(const char * filename)
{
    if(cache.getValue(filename))
        return true;

    Owned<HPCCFile> file;

    try
    {
        IDistributedFileDirectory & dfd = queryDistributedFileDirectory();
        Owned<IDistributedFile> df = dfd.lookup(filename, userdesc);

        if(!df)
            throw MakeStringException(-1,"Cannot find file %s.",filename);

        file.setown(HPCCFile::createHPCCFile());
        const char* lname=df->queryLogicalName(), *fname=strrchr(lname,':');
        file->setFullname(lname);
        file->setName(fname ? fname+1 : lname);

        //Do we care about the clusters??
        //StringArray clusters;
        //if (cluster && *cluster)
        //{
        //df->getClusterNames(clusters);
        //if(!FindInStringArray(clusters, cluster))
            //throw MakeStringException(ECLWATCH_FILE_NOT_EXIST,"Cannot find file %s.",fname);
        //}

    #if defined(_DEBUG)
        StringBuffer atttree;
        toXML(&df->queryAttributes(), atttree, 0);
        fprintf(stderr, "%s", atttree.str());
    #endif

        IPropertyTree & properties = df->queryAttributes();

        if(properties.hasProp("ECL"))
            file->setEcl(properties.queryProp("ECL"));

        //unfortunately @format sometimes holds the file format, sometimes @kind does
        const char * kind = properties.queryProp("@kind");
        if (kind)
        {
            file->setFormat(kind);
            if ((stricmp(kind, "key") == 0))
            {
                file->setIsKeyedFile(true);

                ISecManager *secmgr = NULL;
                ISecUser *secuser = NULL;
                StringBuffer username;
                StringBuffer passwd;
                userdesc->getUserName(username);
                userdesc->getPassword(passwd);

                Owned<IResultSetFactory> resultSetFactory = getSecResultSetFactory(secmgr, secuser, username.str(), passwd.str());
                Owned<INewResultSet> result;
                try
                {
                    result.setown(resultSetFactory->createNewFileResultSet(filename, NULL));

                    if (result)
                    {
                        Owned<IResultSetCursor> cursor = result->createCursor();
                        const IResultSetMetaData & meta = cursor->queryResultSet()->getMetaData();
                        int columnCount = meta.getColumnCount();
                        int keyedColumnCount = meta.getNumKeyedColumns();

                        DBGLOG("Keyed file %s column count: %d, keyedcolumncount: %d", file->getFullname(), columnCount, keyedColumnCount);
                        for (int i = 0; i < keyedColumnCount; i++)
                        {
                            SCMStringBuffer columnLabel;
                            if (meta.hasSetTranslation(i))
                            {
                                meta.getNaturalColumnLabel(columnLabel, i);
                            }

                            if (columnLabel.length() < 1)
                            {
                                meta.getColumnLabel(columnLabel, i);
                            }

                            file->setKeyedColumn(columnLabel.str());
                        }
                    }
                }
                catch (IException * se)
                {
                    StringBuffer s;
                    se->errorMessage(s);
                    DBGLOG("Error fetching keyed file %s info: %s", fname, s.str());
                    se->Release();
                    return false;
                }
            }
        }
        else
        {
            //@format - what format the file is (if not fixed width)
            const char* format = properties.queryProp("@format");
            file->setFormat(format);
        }
    }
    catch (IException * se)
    {
        StringBuffer s;
        se->errorMessage(s);
        DBGLOG("Error fetching HPCC file %s info: %s", filename, s.str());
        se->Release();
        return false;
    }

    if (file)
        cache.setValue(file->getFullname(), file.getLink());
    else
        cache.setValue(filename, NULL); //avoid attempting to fetch next time

    return true;
}

HPCCFilePtr HPCCFileCache::getHpccFileByName(const char * filename)
{
    return isHpccFileCached(filename) ? *(cache.getValue(filename)) : NULL;
}

bool HPCCFileCache::isHpccFileCached(const char * filename)
{
    return cache.getValue(filename) != NULL;
}
