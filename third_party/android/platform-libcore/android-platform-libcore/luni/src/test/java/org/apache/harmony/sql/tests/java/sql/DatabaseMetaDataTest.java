/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.apache.harmony.sql.tests.java.sql;

import dalvik.annotation.TestTargetClass;
import dalvik.annotation.TestTargets;
import dalvik.annotation.TestLevel;
import dalvik.annotation.TestTargetNew;

import java.sql.DatabaseMetaData;

import junit.framework.TestCase;

@TestTargetClass(java.sql.DatabaseMetaData.class)
public class DatabaseMetaDataTest extends TestCase {

    /*
     * Public statics test
     */
    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "Field testing",
        method = "!Constants",
        args = {}
    )
    public void testPublicStatics() {
        assertEquals(0, DatabaseMetaData.attributeNoNulls);
        assertEquals(1, DatabaseMetaData.attributeNullable);
        assertEquals(2, DatabaseMetaData.attributeNullableUnknown);
        assertEquals(1, DatabaseMetaData.bestRowNotPseudo);
        assertEquals(2, DatabaseMetaData.bestRowPseudo);
        assertEquals(2, DatabaseMetaData.bestRowSession);
        assertEquals(0, DatabaseMetaData.bestRowTemporary);
        assertEquals(1, DatabaseMetaData.bestRowTransaction);
        assertEquals(0, DatabaseMetaData.bestRowUnknown);
        assertEquals(0, DatabaseMetaData.columnNoNulls);
        assertEquals(1, DatabaseMetaData.columnNullable);
        assertEquals(2, DatabaseMetaData.columnNullableUnknown);
        assertEquals(1, DatabaseMetaData.functionColumnIn);
        assertEquals(2, DatabaseMetaData.functionColumnInOut);
        assertEquals(3, DatabaseMetaData.functionColumnOut);
        assertEquals(5, DatabaseMetaData.functionColumnResult);
        assertEquals(0, DatabaseMetaData.functionColumnUnknown);
        assertEquals(0, DatabaseMetaData.functionNoNulls);
        assertEquals(1, DatabaseMetaData.functionNoTable);
        assertEquals(1, DatabaseMetaData.functionNullable);
        assertEquals(2, DatabaseMetaData.functionNullableUnknown);
        assertEquals(0, DatabaseMetaData.functionResultUnknown);
        assertEquals(4, DatabaseMetaData.functionReturn);
        assertEquals(2, DatabaseMetaData.functionReturnsTable);
        assertEquals(0, DatabaseMetaData.importedKeyCascade);
        assertEquals(5, DatabaseMetaData.importedKeyInitiallyDeferred);
        assertEquals(6, DatabaseMetaData.importedKeyInitiallyImmediate);
        assertEquals(3, DatabaseMetaData.importedKeyNoAction);
        assertEquals(7, DatabaseMetaData.importedKeyNotDeferrable);
        assertEquals(1, DatabaseMetaData.importedKeyRestrict);
        assertEquals(4, DatabaseMetaData.importedKeySetDefault);
        assertEquals(2, DatabaseMetaData.importedKeySetNull);
        assertEquals(1, DatabaseMetaData.procedureColumnIn);
        assertEquals(2, DatabaseMetaData.procedureColumnInOut);
        assertEquals(4, DatabaseMetaData.procedureColumnOut);
        assertEquals(3, DatabaseMetaData.procedureColumnResult);
        assertEquals(5, DatabaseMetaData.procedureColumnReturn);
        assertEquals(0, DatabaseMetaData.procedureColumnUnknown);
        assertEquals(0, DatabaseMetaData.procedureNoNulls);
        assertEquals(1, DatabaseMetaData.procedureNoResult);
        assertEquals(1, DatabaseMetaData.procedureNullable);
        assertEquals(2, DatabaseMetaData.procedureNullableUnknown);
        assertEquals(0, DatabaseMetaData.procedureResultUnknown);
        assertEquals(2, DatabaseMetaData.procedureReturnsResult);
        assertEquals(2, DatabaseMetaData.sqlStateSQL);
        assertEquals(2, DatabaseMetaData.sqlStateSQL99);
        assertEquals(1, DatabaseMetaData.sqlStateXOpen);
        assertEquals(1, DatabaseMetaData.tableIndexClustered);
        assertEquals(2, DatabaseMetaData.tableIndexHashed);
        assertEquals(3, DatabaseMetaData.tableIndexOther);
        assertEquals(0, DatabaseMetaData.tableIndexStatistic);
        assertEquals(0, DatabaseMetaData.typeNoNulls);
        assertEquals(1, DatabaseMetaData.typeNullable);
        assertEquals(2, DatabaseMetaData.typeNullableUnknown);
        assertEquals(2, DatabaseMetaData.typePredBasic);
        assertEquals(1, DatabaseMetaData.typePredChar);
        assertEquals(0, DatabaseMetaData.typePredNone);
        assertEquals(3, DatabaseMetaData.typeSearchable);
        assertEquals(1, DatabaseMetaData.versionColumnNotPseudo);
        assertEquals(2, DatabaseMetaData.versionColumnPseudo);
        assertEquals(0, DatabaseMetaData.versionColumnUnknown);
    } // end method testPublicStatics

} // end class DatabaseMetaDataTest
