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

#include "ECLEngine.hpp"
#include <limits>       // std::numeric_limits

const char * ECLEngine::SELECTOUTPUTNAME = "WSSQLSelectQueryResult";

ECLEngine::ECLEngine(){}

ECLEngine::~ECLEngine(){}

void ECLEngine::generateECL(HPCCSQLTreeWalker * sqlobj, StringBuffer & out)
{
    if (sqlobj)
    {
        switch (sqlobj->getSqlType())
        {
            case SQLTypeSelect:
                generateSelectECL(sqlobj, out);
                break;
            case SQLTypeCall:
                break;
            case SQLTypeUnknown:
            default:
                break;
        }
    }
}

void ECLEngine::generateIndexSetupAndFetch(HPCCFilePtr file, SQLTable * table, int tableindex, HPCCSQLTreeWalker * selectsqlobj, IProperties* eclEntities)
{
    bool isPayloadIndex = false;
    bool avoidindex = false;
    StringBuffer indexname;

    if (!table)
        return;

    const char * tname = table->getName();
    StringBuffer indexHintFromSQL;
    if (table->hasIndexHint())
    {
        indexHintFromSQL.set(table->getIndexhint());
        if (strncmp(indexHintFromSQL.trim().str(), "0", 1)==0)
        {
            avoidindex = true;
            WARNLOG("Will not use any index.");
            return;
        }
        else
            WARNLOG("Empty index hint found!");
    }

    findAppropriateIndex(file, indexHintFromSQL.str(), selectsqlobj, indexname);
    if (indexHintFromSQL.length() > 0 && indexname.length() == 0)
        WARNLOG("Unusable index hint detected.");

    if (indexname.length()>0)
    {
        HPCCFilePtr indexfile = dynamic_cast<HPCCFile *>(selectsqlobj->queryHPCCFileCache()->getHpccFileByName(indexname));
        if (indexfile && file)
        {
            StringBuffer idxsetupstr;
            StringBuffer idxrecdefname;

            idxrecdefname.set("TblDS").append(tableindex).append("RecDef");

            StringBuffer indexPosField;
            indexPosField.set(indexfile->getIdxFilePosField());
            HPCCColumnMetaData * poscol = indexfile->getColumn(indexPosField);

            file->getFileRecDefwithIndexpos(poscol, idxsetupstr, idxrecdefname.str());
            eclEntities->appendProp("INDEXFILERECDEF", idxsetupstr.str());

            StringBuffer keyedAndWild;
            isPayloadIndex = processIndex(indexfile, keyedAndWild, selectsqlobj);

            eclEntities->appendProp("KEYEDWILD", keyedAndWild.str());

            if (isPayloadIndex)
                eclEntities->appendProp("PAYLOADINDEX", "true");

            idxsetupstr.clear();
            idxsetupstr.appendf("Idx%d := INDEX(TblDS%d, {", tableindex, tableindex);
            indexfile->getKeyedFieldsAsDelimitedString(',', "", idxsetupstr);
            idxsetupstr.append("}");

            if (indexfile->getNonKeyedColumnsCount() > 0)
            {
                idxsetupstr.append(",{ ");
                indexfile->getNonKeyedFieldsAsDelmitedString(',', "", idxsetupstr);
                idxsetupstr.append(" }");
            }

            //Note, currently '~' is not valid char, if it is ever allowed, we'd have verify that the file name does not lead off with ~
            idxsetupstr.appendf(",\'~%s\');\n",indexfile->getFullname());

            eclEntities->appendProp("IndexDef", idxsetupstr.str());

            idxsetupstr.clear();

            if (isPayloadIndex)
            {
                WARNLOG(" as PAYLOAD");
                idxsetupstr.appendf("IdxDS%d := Idx%d(%s", tableindex, tableindex, keyedAndWild.str());
            }
            else
            {
                WARNLOG(" Not as PAYLOAD");
                idxsetupstr.appendf("IdxDS%d := FETCH(TblDS%d, Idx%d( %s ), RIGHT.%s", tableindex, tableindex, tableindex, keyedAndWild.str(), indexfile->getIdxFilePosField());
            }
            idxsetupstr.append(");\n");

            eclEntities->appendProp("IndexRead", idxsetupstr.str());
        }
        else
            WARNLOG("NOT USING INDEX!");
    }
}

void ECLEngine::generateSelectECL(HPCCSQLTreeWalker * selectsqlobj, StringBuffer & out)
{
    StringBuffer latestDS = "TblDS0";

    Owned<IProperties> eclEntities = createProperties(true);
    Owned<IProperties> eclDSSourceMapping = createProperties(true);
    Owned<IProperties> translator = createProperties(true);

    out.clear();
    out.append("import std;\n"); /* ALL Generated ECL will import std, even if std lib not used */

    //Prepared statement parameters are handled by ECL STORED service workflow statements
    if (selectsqlobj->hasWhereClause())
    {
        selectsqlobj->getWhereClause()->eclDeclarePlaceHolders(out, 0,0);
    }

    const IArrayOf<SQLTable> * tables = selectsqlobj->getTableList();

    ForEachItemIn(tableidx, *tables)
    {
        SQLTable table = tables->item(tableidx);
        const char * tname = table.getName();

        HPCCFilePtr file = dynamic_cast<HPCCFile *>(selectsqlobj->queryHPCCFileCache()->getHpccFileByName(tname));
        if (file)
        {
            translator->setProp(tname, "LEFT");

            StringBuffer currntTblDS("TblDS");
            currntTblDS.append(tableidx);

            StringBuffer currntTblRecDef(currntTblDS);
            currntTblRecDef.append("RecDef");

            StringBuffer currntJoin("JndDS");
            currntJoin.append(tableidx);

            out.append("\n");
            if (tableidx == 0)
            {
                //Currently only utilizing index fetch/read for single table queries
                if ((table.hasIndexHint() || file->getRelatedIndexCount()) && tables->length() == 1 && !file->isFileKeyed())
                    generateIndexSetupAndFetch(file, &table, tableidx, selectsqlobj, eclEntities);

                if (eclEntities->hasProp("INDEXFILERECDEF"))
                {
                    eclDSSourceMapping->appendProp(tname, "IdxDS0");
                    eclEntities->getProp("INDEXFILERECDEF", out);
                }
                else
                    file->getFileRecDef(out, currntTblRecDef);
            }
            else
                file->getFileRecDef(out, currntTblRecDef);
            out.append("\n");

            if (!file->isFileKeyed())
            {
                out.appendf("%s := DATASET(\'~%s\', %s, %s);\n", currntTblDS.str(), file->getFullname(), currntTblRecDef.str(), file->getFormat());
            }
            else
            {
                out.appendf("%s := INDEX( {", currntTblDS.str());
                file->getKeyedFieldsAsDelimitedString(',', currntTblRecDef.str(), out);
                out.append("},{");
                file->getNonKeyedFieldsAsDelmitedString(',', currntTblRecDef.str(), out);
                //Note, currently '~' is not valid char, if it is ever allowed, we'd have verify that the file name does not lead off with ~
                out.appendf("},\'~%s\');\n",file->getFullname());
            }

            if (tableidx > 0)
            {
                out.append("\n").append(currntJoin).append(" := JOIN( ");
                translator->setProp(tname, "RIGHT");

                if (tableidx == 1)
                {
                    //First Join, previous DS is TblDS0
                    out.append("TblDS0");
                    latestDS.set("JndDS1");
                }
                else
                {
                    //Nth Join, previous DS is JndDS(N-1)
                    out.appendf("JndDS%d",tableidx-1);
                    latestDS.appendf("JndDS%d",tableidx);
                }

                StringBuffer translatedAndFilteredOnClause;
                SQLJoin * tablejoin = table.getJoin();
                if (tablejoin && tablejoin->doesHaveOnclause())
                {
                    tablejoin->getOnClause()->toECLStringTranslateSource(translatedAndFilteredOnClause, translator,true, false, false, false);
                    if (selectsqlobj->hasWhereClause())
                    {
                        translatedAndFilteredOnClause.append(" AND ");
                        selectsqlobj->getWhereClause()->toECLStringTranslateSource(translatedAndFilteredOnClause, translator, true, true, false, false);
                    }
                }
                else if ( tablejoin && tablejoin->getType() == SQLJoinTypeImplicit && selectsqlobj->hasWhereClause())
                {
                    if (translatedAndFilteredOnClause.length() > 0)
                        translatedAndFilteredOnClause.append(" AND ");
                    selectsqlobj->getWhereClause()->toECLStringTranslateSource(translatedAndFilteredOnClause, translator, true, true, false, false);
                }
                else
                    throw MakeStringException(-1,"No join condition between tables %s, and earlier table", tname);

                if (translatedAndFilteredOnClause.length() <= 0)
                    throw MakeStringException(-1,"Join condition does not contain proper join condition between tables %s, and earlier table", tname);

                out.appendf(", %s, %s, ", currntTblDS.str(), translatedAndFilteredOnClause.length() > 0 ? translatedAndFilteredOnClause.str() : "TRUE");
                tablejoin->getECLTypeStr(out);

                if (tablejoin->getOnClause() != NULL && !tablejoin->getOnClause()->containsEqualityCondition(translator, "LEFT", "RIGHT"))
                {
                    WARNLOG("Warning: No Join EQUALITY CONDITION detected!, using ECL ALL option");
                    out.append(", ALL");
                }

                out.append(" );\n");

                //move this file to LEFT for possible next iteration
                translator->setProp(tname, "LEFT");
            }
            eclEntities->setProp("JoinQuery", "1");
        }
    }

    int limit=selectsqlobj->getLimit();
    int offset=selectsqlobj->getOffset();

    if (!eclEntities->hasProp("IndexDef"))
    {
        //Create filtered DS if there's a where clause, and no join clause,
        //because filtering is applied while performing join.
        //if (sqlParser.getWhereClause() != null && !eclEntities.containsKey("JoinQuery"))
        if (selectsqlobj->hasWhereClause())
        {
            out.appendf("%sFiltered := %s",latestDS.str(), latestDS.str());
            addFilterClause(selectsqlobj, out);
            out.append(";\n");
            latestDS.append("Filtered");
        }

        generateSelectStruct(selectsqlobj, eclEntities.getLink(), *selectsqlobj->getSelectList(),latestDS.str());
        out.append(eclEntities->queryProp("SELECTSTRUCT"));

        if (tables->length() > 0)
        {
            out.append(latestDS).append("Table").append(" := TABLE( ");
            out.append(latestDS);

            out.append(", SelectStruct ");

            if (selectsqlobj->hasGroupByColumns() && !selectsqlobj->hasHavingClause())
            {
                out.append(", ");
                selectsqlobj->getGroupByString(out);
            }

            out.append(");\n");

            latestDS.append("Table");
        }
        else
        {
            generateConstSelectDataset(selectsqlobj, eclEntities.getLink(), *selectsqlobj->getSelectList(),latestDS.str());
            out.append(latestDS).append(" := ").append(eclEntities->queryProp("CONSTDATASETSTRUCT")).append(";\n");
        }

    }
    else //PROCESSING FOR INDEX BASED FETCH
    {
        //Not creating a filtered DS because filtering is applied while
        //performing index read/fetch.
        eclEntities->getProp("IndexDef",out);
        eclEntities->getProp("IndexRead",out);
        StringBuffer latestDS = "IdxDS0";

        //if (eclEntities.containsKey("COUNTDEDUP"))
        //    eclCode.append(eclEntities.get("COUNTDEDUP"));

        if (eclEntities->hasProp("SCALAROUTNAME"))
        {
            out.append("OUTPUT(ScalarOut ,NAMED(\'");
            eclEntities->getProp("SCALAROUTNAME", out);
            out.append("\'));\n");
        }
        else
        {
            // If group by contains HAVING clause, use ECL 'HAVING' function,
            // otherwise group can be done implicitly in table step.
            // since the implicit approach has better performance.
            if (eclEntities->hasProp("GROUPBY") && selectsqlobj->hasHavingClause())
            {
                out.append(latestDS).append("Grouped").append(" := GROUP( ");
                out.append(latestDS);
                out.append(", ");
                eclEntities->getProp("GROUPBY", out);
                out.append(", ALL);\n");

                latestDS.append("Grouped");

                if (appendTranslatedHavingClause(selectsqlobj, out, latestDS.str()))
                    latestDS.append("Having");
            }

            generateSelectStruct(selectsqlobj, eclEntities.get(), *selectsqlobj->getSelectList(),latestDS.str());

            const char *selectstr = eclEntities->queryProp("SELECTSTRUCT");
            out.append(selectstr);
            out.appendf("%sTable := TABLE(%s, SelectStruct ", latestDS.str(), latestDS.str());

            if (eclEntities->hasProp("GROUPBY") && !selectsqlobj->hasHavingClause())
            {
                out.append(", ");
                eclEntities->getProp("GROUPBY", out);
            }
            out.append(");\n");
            latestDS.append("Table");
        }
    }

    if (selectsqlobj->isSelectDistinct())
    {
        out.appendf("%sDeduped := Dedup( %s, HASH);\n",latestDS.str(),latestDS.str());
        latestDS.append("Deduped");
    }

    out.append("OUTPUT(");
    if (limit>0 || (!eclEntities->hasProp("NONSCALAREXPECTED") && !selectsqlobj->hasGroupByColumns()))
        out.append("CHOOSEN(");

    if (selectsqlobj->hasOrderByColumns())
        out.append("SORT(");

    out.append(latestDS);
    if (selectsqlobj->hasOrderByColumns())
    {
        out.append(",");
        selectsqlobj->getOrderByString(out);
        out.append(")");
    }

    if (!eclEntities->hasProp("NONSCALAREXPECTED") && !selectsqlobj->hasGroupByColumns())
    {
        out.append(", 1)");
    }
    else if (limit>0)
    {
        out.append(",");
        out.append(limit);
        if (offset>0)
        {
            out.append(",");
            out.append(offset);
        }
        out.append(")");
    }

    out.appendf(",NAMED(\'%s\'),THOR);", SELECTOUTPUTNAME); //THOR= WU results written to file
}

void ECLEngine::generateConstSelectDataset(HPCCSQLTreeWalker * selectsqlobj, IProperties* eclEntities,  const IArrayOf<ISQLExpression> & expectedcolumns, const char * datasource)
{
    StringBuffer datasetStructSB = "DATASET([{ ";

    ForEachItemIn(i, expectedcolumns)
    {
        ISQLExpression * col = &expectedcolumns.item(i);
        col->toString(datasetStructSB, true);
        if (i < expectedcolumns.length()-1)
            datasetStructSB.append(", ");
    }

    datasetStructSB.append("}],SelectStruct)");

    eclEntities->setProp("CONSTDATASETSTRUCT", datasetStructSB.toCharArray());
}

void ECLEngine::generateSelectStruct(HPCCSQLTreeWalker * selectsqlobj, IProperties* eclEntities,  const IArrayOf<ISQLExpression> & expectedcolumns, const char * datasource)
{
    StringBuffer selectStructSB = "SelectStruct := RECORD\n";

    ForEachItemIn(i, expectedcolumns)
    {
        selectStructSB.append(" ");
        ISQLExpression * col = &expectedcolumns.item(i);

        if (col->getExpType() == Value_ExpressionType)
        {
            const char * alias = col->getAlias();
            if (alias && *alias)
                selectStructSB.appendf("%s %s := ", col->getECLType(), alias);
            else
                selectStructSB.appendf("%s %s%d := ", col->getECLType(), col->getName(), i);
            col->toString(selectStructSB, false);
            selectStructSB.append("; ");

            if (i == 0 && expectedcolumns.length() == 1)
                eclEntities->setProp("SCALAROUTNAME", col->getNameOrAlias());
        }
        else if (col->getExpType() == Function_ExpressionType)
        {
            SQLFunctionExpression * funcexp = dynamic_cast<SQLFunctionExpression *>(col);
            IArrayOf<ISQLExpression> * funccols = funcexp->getParams();

            ECLFunctionDefCfg func = ECLFunctions::getEclFuntionDef(funcexp->getName());

            if (func.functionType == CONTENT_MODIFIER_FUNCTION_TYPE )
            {
                if (funccols->length() > 0)
                {
                    ISQLExpression * param = &funccols->item(0);
                    int paramtype = param->getExpType();

                    const char * alias = col->getAlias();
                    if (alias && *alias)
                        selectStructSB.append(alias);
                    else
                        selectStructSB.append(param->getName());
                    selectStructSB.append(" := ");
                    selectStructSB.append(func.eclFunctionName).append("( ");
                    if (paramtype == FieldValue_ExpressionType)
                    {
                        eclEntities->setProp("NONSCALAREXPECTED", "TRUE");

                        selectStructSB.append(datasource);
                        selectStructSB.append(".");
                        selectStructSB.append(param->getNameOrAlias());
                    }
                    else
                        param->toString(selectStructSB, false);
                }
            }
            else
            {
                const char * alias = col->getAlias();
                if (alias && *alias)
                    selectStructSB.append(alias);
                else
                {
                    selectStructSB.append(col->getName());
                    selectStructSB.append("out");
                    selectStructSB.append(i+1);
                }
                selectStructSB.append(" := ");

                selectStructSB.append(func.eclFunctionName).append("( ");

                if (selectsqlobj->hasGroupByColumns())
                {
                    selectStructSB.append("GROUP ");
                }
                else
                {
                    if (funcexp->isDistinct())
                    {
                        selectStructSB.append("DEDUP( ");
                        selectStructSB.append(datasource);
                        addFilterClause(selectsqlobj, selectStructSB);

                        for (int j = 0; j < funccols->length(); j++)
                        {
                            StringBuffer paramname = funccols->item(j).getName();
                            selectStructSB.append(", ");
                            selectStructSB.append(paramname);
                        }
                        selectStructSB.append(", HASH)");
                    }
                    else
                    {
                        selectStructSB.append(datasource);
                        addFilterClause(selectsqlobj, selectStructSB);
                    }
                }

                if ((strcmp(func.name,"COUNT"))!=0  && funccols->length() > 0)
                {

                    ISQLExpression &funccol = funccols->item(0);
                    const char * paramname = funccol.getName();
                    if (paramname && paramname[0]!='*')
                    {
                        selectStructSB.append(", ");
                        if (funccol.getExpType() != Value_ExpressionType)
                        {
                            selectStructSB.append(datasource);
                            selectStructSB.append(".");
                            selectStructSB.append(paramname);
                        }
                        else
                        {
                            funccol.toString(selectStructSB, false);
                        }
                    }
                }
            }

            //AS OF community_3.8.6-4 this is causing error:
            // (0,0): error C3000: assert(!cond) failed - file: /var/jenkins/workspace/<build number>/HPCC-Platform/ecl/hqlcpp/hqlhtcpp.cpp, line XXXXX
            //Bug reported: https://track.hpccsystems.com/browse/HPCC-8268
            //Leaving this code out until fix is produced.
            //UPDATE: Issue has been resolved as of 3.10.0

            //RODRIGO below if condition not completed yet
            //if (eclEntities.containsKey("PAYLOADINDEX") && !sqlParser.hasGroupByColumns() && !col.isDistinct())
            if (false && !selectsqlobj->hasGroupByColumns() && !funcexp->isDistinct())
            {
                    selectStructSB.append(", KEYED");
            }

            selectStructSB.append(" );");
        }
        else
        {
            eclEntities->setProp("NONSCALAREXPECTED", "TRUE");
            selectStructSB.appendf("%s %s := %s.%s;", col->getECLType(), col->getNameOrAlias(), datasource, col->getName());
        }

        selectStructSB.append("\n");
    }
    selectStructSB.append("END;\n");

    eclEntities->setProp("SELECTSTRUCT", selectStructSB.toCharArray());
}

bool containsPayload(const HPCCFile * indexfiletotest, const HPCCSQLTreeWalker * selectsqlobj)
{
    if (selectsqlobj)
    {
        const IArrayOf <ISQLExpression> * selectlist = selectsqlobj->getSelectList();

        for (int j = 0; j < selectlist->length(); j++)
        {
            ISQLExpression * exp = &selectlist->item(j);
            if (exp->getExpType() == FieldValue_ExpressionType)
            {
                SQLFieldValueExpression * currentselectcol = dynamic_cast<SQLFieldValueExpression *>(exp);
                if (!indexfiletotest->containsField(currentselectcol->queryField(), true))
                    return false;
            }
            else if (exp->getExpType() == Function_ExpressionType)
            {
                SQLFunctionExpression * currentfunccol = dynamic_cast<SQLFunctionExpression *>(exp);

                IArrayOf<ISQLExpression> * funcparams = currentfunccol->getParams();
                ForEachItemIn(paramidx, *funcparams)
                {
                    ISQLExpression * param = &(funcparams->item(paramidx));
                    if (param->getExpType() == FieldValue_ExpressionType)
                    {
                        SQLFieldValueExpression * currentselectcol = dynamic_cast<SQLFieldValueExpression *>(param);
                        if (!indexfiletotest->containsField(currentselectcol->queryField(), true))
                            return false;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

bool ECLEngine::processIndex(HPCCFile * indexfiletouse, StringBuffer & keyedandwild, HPCCSQLTreeWalker * selectsqlobj)
{
    bool isPayloadIndex = containsPayload(indexfiletouse, selectsqlobj);

    StringArray keyed;
    StringArray wild;
    StringArray uniquenames;

    ISQLExpression * whereclause = selectsqlobj->getWhereClause();

    if (!whereclause)
        return false;

    // Create keyed and wild string
    IArrayOf<HPCCColumnMetaData> * cols = indexfiletouse->getColumns();
    for (int i = 0; i < cols->length(); i++)
    {
        HPCCColumnMetaData currcol = cols->item(i);
        if (currcol.isKeyedField())
        {
            const char * keyedcolname = currcol.getColumnName();
            StringBuffer keyedorwild;

            if (whereclause->containsKey(keyedcolname))
            {
                keyedorwild.set(" ");
                whereclause->getExpressionFromColumnName(keyedcolname, keyedorwild);
                keyedorwild.append(" ");
                keyed.append(keyedorwild);
            }
            else
            {
                keyedorwild.setf(" %s ", keyedcolname);
                wild.append(keyedorwild);
            }
        }
    }

    if (isPayloadIndex)
    {
        if (keyed.length() > 0)
        {
            keyedandwild.append("KEYED( ");
            for (int i = 0; i < keyed.length(); i++)
            {
                keyedandwild.append(keyed.item(i));
                if (i < keyed.length() - 1)
                    keyedandwild.append(" AND ");
            }
            keyedandwild.append(" )");
        }

        if (wild.length() > 0)
        {
            // TODO should I bother making sure there's a KEYED entry ?
            for (int i = 0; i < wild.length(); i++)
            {
                keyedandwild.append(" and WILD( ");
                keyedandwild.append(wild.item(i));
                keyedandwild.append(" )");
            }
        }
        keyedandwild.append(" and ( ");
        whereclause->toString(keyedandwild, false);
        keyedandwild.append(" )");
    }
    else
    {
        // non-payload just AND the keyed expressions
        keyedandwild.append("( ");
        whereclause->toString(keyedandwild, false);
        keyedandwild.append(" )");
    }

    return isPayloadIndex;
}

void ECLEngine::findAppropriateIndex(HPCCFilePtr file, const char * indexhint, HPCCSQLTreeWalker * selectsqlobj, StringBuffer & indexname)
{
    StringArray indexhints;
    if (indexhint && *indexhint)
        indexhints.append(indexhint);

    if (file)
        file->getRelatedIndexes(indexhints);

    findAppropriateIndex(&indexhints, selectsqlobj, indexname);
}

void ECLEngine::findAppropriateIndex(StringArray * relindexes, HPCCSQLTreeWalker * selectsqlobj, StringBuffer & indexname)
{
    StringArray uniquenames;
    ISQLExpression * whereclause = selectsqlobj->getWhereClause();

    if (whereclause)
        whereclause->getUniqueExpressionColumnNames(uniquenames);
    else
        return;

    int totalparamcount = uniquenames.length();

    if (relindexes->length() <= 0 || totalparamcount <= 0)
        return;

    bool payloadIdxWithAtLeast1KeyedFieldFound = false;

    IntArray scores;
    for (int indexcounter = 0; indexcounter < relindexes->length(); indexcounter++)
    {
        scores.add(std::numeric_limits<int>::min(), indexcounter);

        const char * indexname = relindexes->item(indexcounter);
        if (!selectsqlobj->queryHPCCFileCache()->isHpccFileCached(indexname))
            selectsqlobj->queryHPCCFileCache()->cacheHpccFileByName(indexname);

        HPCCFilePtr indexfile = dynamic_cast<HPCCFile *>(selectsqlobj->queryHPCCFileCache()->getHpccFileByName(indexname));

        if (indexfile)
        {
            const IArrayOf<ISQLExpression> * expectedretcolumns =selectsqlobj->getSelectList();
            if (indexfile && indexfile->isFileKeyed() && indexfile->hasValidIdxFilePosField())
            {
                //The more fields this index has in common with the select columns higher score
                int commonparamscount = 0;
                for (int j = 0; j < expectedretcolumns->length(); j++)
                {
                    ISQLExpression * exp = &expectedretcolumns->item(j);
                    if (exp->getExpType() == FieldValue_ExpressionType)
                    {
                        SQLFieldValueExpression * fieldexp = dynamic_cast<SQLFieldValueExpression *>(exp);
                        if (indexfile->containsField(fieldexp->queryField(), true))
                            commonparamscount++;
                    }
                    else if (exp->getExpType() == Function_ExpressionType)
                    {
                        SQLFunctionExpression * currentfunccol = dynamic_cast<SQLFunctionExpression *>(exp);

                        IArrayOf<ISQLExpression> * funcparams = currentfunccol->getParams();
                        ForEachItemIn(paramidx, *funcparams)
                        {
                            ISQLExpression * param = &(funcparams->item(paramidx));
                            if (param->getExpType() == FieldValue_ExpressionType)
                            {
                                SQLFieldValueExpression * currentselectcol = dynamic_cast<SQLFieldValueExpression *>(param);
                                if (indexfile->containsField(currentselectcol->queryField(), true))
                                    commonparamscount++;
                            }
                        }
                    }
                }
                int commonparamsscore = commonparamscount * NumberOfCommonParamInThisIndex_WEIGHT;
                scores.replace(commonparamsscore, indexcounter);

                if (payloadIdxWithAtLeast1KeyedFieldFound && commonparamscount == 0)
                    break; // Don't bother with this index

                //The more keyed fields this index has in common with the where clause, the higher score
                //int localleftmostindex = -1;
                int keycolscount = 0;
                IArrayOf<HPCCColumnMetaData> * columns = indexfile->getColumns();
                ForEachItemIn(colidx, *columns)
                {
                    HPCCColumnMetaData currcol = columns->item(colidx);
                    if (currcol.isKeyedField())
                    {
                        ForEachItemIn(uniqueidx, uniquenames)
                        {
                            if(strcmp( uniquenames.item(uniqueidx), currcol.getColumnName())==0)
                                keycolscount++;
                        }
                    }
                }

                if (keycolscount == 0)
                {
                    scores.replace(std::numeric_limits<int>::min(), indexcounter);
                    continue;
                }

                int keycolsscore = keycolscount * NumberofColsKeyedInThisIndex_WEIGHT;
                scores.replace(keycolsscore + scores.item(indexcounter), indexcounter);
                if (commonparamscount == expectedretcolumns->length() && keycolscount > 0)
                        payloadIdxWithAtLeast1KeyedFieldFound = true; // during scoring, give this priority
            }
        }
    }

    int highscore = std::numeric_limits<int>::min();
    int highscoreidx = -1;
    for (int i = 0; i < scores.length(); i++)
    {
        if (highscore < scores.item(i))
        {
           highscore = scores.item(i);
           highscoreidx = i;
        }
    }
    if (highscoreidx != -1 && highscoreidx < relindexes->length())
        indexname.set(relindexes->item(highscoreidx));
}

void ECLEngine::addFilterClause(HPCCSQLTreeWalker * sqlobj, StringBuffer & sb)
{
    if (sqlobj->hasWhereClause())
    {
        StringBuffer where;
        sqlobj->getWhereClauseString(where);
        if (where.length()>0)
        {
            sb.append("( ").append(where.str()).append(" )");
        }
    }
}

void ECLEngine::addHavingCluse(HPCCSQLTreeWalker * sqlobj, StringBuffer & sb)
{
    StringBuffer having;
    sqlobj->getHavingClauseString(having);
    if (having.length()>0)
    {
        sb.append("( ").append(having.str()).append(" )");
    }
}

bool ECLEngine::appendTranslatedHavingClause(HPCCSQLTreeWalker * sqlobj, StringBuffer & sb, const char * latesDSName)
{
    bool success = false;
    if (sqlobj)
    {
        if (sqlobj->hasHavingClause())
        {
            Owned<IProperties> translator = createProperties(true);

            const IArrayOf<SQLTable> * tables = sqlobj->getTableList();
            ForEachItemIn(tableidx, *tables)
            {
                SQLTable table = tables->item(tableidx);
                translator->appendProp(table.getName(), "LEFT");
            }

            ISQLExpression * having = sqlobj->getHavingClause();
            StringBuffer havingclause;
            having->toECLStringTranslateSource(havingclause, translator, false, true, false, false);

            if (havingclause.length() > 0)
            {
                sb.appendf("%sHaving := HAVING( %s, %s );\n", latesDSName, latesDSName, havingclause.str());
            }
            success = true;
        }
    }
    return success;
}
