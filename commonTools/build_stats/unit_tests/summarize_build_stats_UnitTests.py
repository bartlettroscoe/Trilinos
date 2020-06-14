# @HEADER
# ************************************************************************
#
#            TriBITS: Tribal Build, Integrate, and Test System
#                    Copyright 2013 Sandia Corporation
#
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the Corporation nor the names of the
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ************************************************************************
# @HEADER

import os
import sys
import copy
import shutil
import unittest
import pprint

thisScriptsDir = os.path.dirname(os.path.abspath(__file__))
g_testBaseDir = thisScriptsDir
sys.path = [thisScriptsDir+"/.."] + sys.path
import summarize_build_stats as SBS

g_pp = pprint.PrettyPrinter(indent=2)



#############################################################################
#
# Test summarize_build_stats.readBuildStatsCsvFileIntoDictOfLists()
#
#############################################################################


class test_readBuildStatsCsvFileIntoDictOfLists(unittest.TestCase):

  def test_build_stats_big_little(self):
    buildStatsDOL = SBS.readBuildStatsCsvFileIntoDictOfLists(
      g_testBaseDir+"/build_stats.big.small.csv" )
    numCols_expected = 4
    numRows_expected = 21
    self.assertEqual(len(buildStatsDOL.keys()), numCols_expected)
    self.assertEqual(len(buildStatsDOL['max_resident_size_Kb']),numRows_expected)
    self.assertEqual(len(buildStatsDOL['elapsed_real_time_sec']), numRows_expected)
    self.assertEqual(len(buildStatsDOL['FileName']), numRows_expected)
    self.assertEqual(len(buildStatsDOL['FileSize']), numRows_expected)
    self.assertEqual(buildStatsDOL['max_resident_size_Kb'][0], 240000)
    self.assertEqual(buildStatsDOL['max_resident_size_Kb'][11], 730000)
    self.assertEqual(buildStatsDOL['max_resident_size_Kb'][20], 77000)
    self.assertEqual(buildStatsDOL['elapsed_real_time_sec'][0], 3.5)
    self.assertEqual(buildStatsDOL['elapsed_real_time_sec'][11], 48.2)
    self.assertEqual(buildStatsDOL['elapsed_real_time_sec'][20], 0.4)
    self.assertEqual(buildStatsDOL['FileName'][0],
      "commonTools/gtest/CMakeFiles/gtest.dir/gtest/gtest-all.cc.o")
    self.assertEqual(buildStatsDOL['FileName'][11],
      "packages/rol/adapters/epetra/test/sol/CMakeFiles/ROL_adapters_epetra_test_sol_EpetraSROMSampleGenerator.dir/test_02.cpp.o")
    self.assertEqual(buildStatsDOL['FileName'][20],
      "packages/adelus/test/vector_random/Adelus_vector_random.exe")
    self.assertEqual(buildStatsDOL['FileSize'][0], 3300000)
    self.assertEqual(buildStatsDOL['FileSize'][11], 17000000)
    self.assertEqual(buildStatsDOL['FileSize'][20], 5200000)



#############################################################################
#
# Test summarize_build_stats.readCsvFileIntoDictOfLists()
#
#############################################################################


def cnat(colName, colType="string"):
  return SBS.ColNameAndType(colName, colType)


class test_readCsvFileIntoDictOfLists(unittest.TestCase):

  def test_build_stats_big_little(self):
    buildStatsDOL = SBS.readCsvFileIntoDictOfLists(
      g_testBaseDir+"/build_stats.big.small.csv",
      [
        cnat('max_resident_size_Kb', 'float'),
        cnat('elapsed_real_time_sec', 'float'),
        cnat('FileName', 'string'),
        cnat('FileSize', 'float'),
        ]
      )
    numCols_expected = 4
    numRows_expected = 21
    self.assertEqual(len(buildStatsDOL.keys()), numCols_expected)
    self.assertEqual(len(buildStatsDOL['max_resident_size_Kb']),numRows_expected)
    self.assertEqual(len(buildStatsDOL['elapsed_real_time_sec']), numRows_expected)
    self.assertEqual(len(buildStatsDOL['FileName']), numRows_expected)
    self.assertEqual(len(buildStatsDOL['FileSize']), numRows_expected)
    self.assertEqual(buildStatsDOL['max_resident_size_Kb'][0], 240000)
    self.assertEqual(buildStatsDOL['max_resident_size_Kb'][11], 730000)
    self.assertEqual(buildStatsDOL['max_resident_size_Kb'][20], 77000)
    self.assertEqual(buildStatsDOL['elapsed_real_time_sec'][0], 3.5)
    self.assertEqual(buildStatsDOL['elapsed_real_time_sec'][11], 48.2)
    self.assertEqual(buildStatsDOL['elapsed_real_time_sec'][20], 0.4)
    self.assertEqual(buildStatsDOL['FileName'][0],
      "commonTools/gtest/CMakeFiles/gtest.dir/gtest/gtest-all.cc.o")
    self.assertEqual(buildStatsDOL['FileName'][11],
      "packages/rol/adapters/epetra/test/sol/CMakeFiles/ROL_adapters_epetra_test_sol_EpetraSROMSampleGenerator.dir/test_02.cpp.o")
    self.assertEqual(buildStatsDOL['FileName'][20],
      "packages/adelus/test/vector_random/Adelus_vector_random.exe")
    self.assertEqual(buildStatsDOL['FileSize'][0], 3300000)
    self.assertEqual(buildStatsDOL['FileSize'][11], 17000000)
    self.assertEqual(buildStatsDOL['FileSize'][20], 5200000)


  def test_invalid_col_header(self):
    threwExcept = True
    try:
      buildStatsDOL = SBS.readCsvFileIntoDictOfLists(
        g_testBaseDir+"/build_stats.big.small.csv",
        [
          cnat('missing_header', 'float'),
          ]
        )
      threwExcept = False
    except Exception, exceptObj:
      errMsg = str(exceptObj)
      subStrIdx = errMsg.find(
        "Error, the CSV file column header 'missing_header' does not exist")
      self.assertEqual(subStrIdx, 0)
      # ToDo: Do a better match with fail msg above
    if not threwExcept:
      self.assertFalse("ERROR: Did not thown an excpetion")


#############################################################################
#
# Test summarize_build_stats.getColNameTypeIdxListGivenColNameAndTypeList()
#
#############################################################################


def cnti(colName, colType, colIdx):
  return SBS.ColNameTypeIdx(SBS.ColNameAndType(colName, colType), colIdx)


class test_getColNameTypeIdxListGivenColNameAndTypeList(unittest.TestCase):


  def test_subset(self):
    colNameAndIdxList = SBS.getColNameTypeIdxListGivenColNameAndTypeList(
      "dummyFileName",
      [ "aaa", "bbb", "ccc", "ddd", "eee", "fff", "ggg", "hhh" ],
      [ cnat("bbb",'int'), cnat("ccc",'float'), cnat("fff",'string') ] )
    colNameAndIdxList_expected = [
       cnti("bbb",'int',1) , cnti("ccc",'float',2), cnti("fff",'string',5) ]
    self.assertEqual(colNameAndIdxList, colNameAndIdxList_expected)


  def test_missing_col_header(self):
    threwExcept = True
    try:
      colNameAndIdxList = SBS.getColNameTypeIdxListGivenColNameAndTypeList(
        "dummyFileName",
        [ "aaa", "bbb", "ccc", "ddd", "eee", "fff", "ggg", "hhh" ],
        [ cnat("bbb"), cnat("ccc"), cnat("ffg", "int") ] )
      threwExcept = False
    except Exception, errMsg:
      self.assertEqual( str(errMsg),
        "Error, the CSV file column header 'ffg' does not exist in the list"+\
        " of column headers ['aaa', 'bbb', 'ccc', 'ddd', 'eee', 'fff', 'ggg', 'hhh']"+\
        " from the CSV file 'dummyFileName'!" )
    if not threwExcept:
      self.assertFalse("ERROR: Did not thown an excpetion")



#############################################################################
#
# Test summarize_build_stats.ColNameTypeIdx
#
#############################################################################

class test_ColNameTypeIdx(unittest.TestCase):

  def test_float(self):
    colNameTypeIdx = SBS.ColNameTypeIdx(SBS.ColNameAndType("name", "float"), 5)
    asStr = str(colNameTypeIdx)
    self.assertEqual(asStr, "ColNameTypeIdx{ColNameAndType{name,float},5}")
    self.assertEqual(colNameTypeIdx.colName(), "name")
    self.assertEqual(colNameTypeIdx.getIdx(), 5)
    self.assertEqual(colNameTypeIdx.convertFromStr("10.5"), 10.5)

  def test_int(self):
    colNameTypeIdx = SBS.ColNameTypeIdx(SBS.ColNameAndType("name", "int"), 4)
    asStr = str(colNameTypeIdx)
    self.assertEqual(asStr, "ColNameTypeIdx{ColNameAndType{name,int},4}")
    self.assertEqual(colNameTypeIdx.colName(), "name")
    self.assertEqual(colNameTypeIdx.getIdx(), 4)
    self.assertEqual(colNameTypeIdx.convertFromStr("12"), 12)

  def test_string(self):
    colNameTypeIdx = SBS.ColNameTypeIdx(SBS.ColNameAndType("name", "string"), 3)
    asStr = str(colNameTypeIdx)
    self.assertEqual(asStr, "ColNameTypeIdx{ColNameAndType{name,string},3}")
    self.assertEqual(colNameTypeIdx.colName(), "name")
    self.assertEqual(colNameTypeIdx.getIdx(), 3)
    self.assertEqual(colNameTypeIdx.convertFromStr("some str"), "some str")

  def test_invalid_type(self):
    threwExcept = True
    try:
      colNameTypeIdx = SBS.ColNameTypeIdx(SBS.ColNameAndType("name", "invalid"), 2)
      threwExcept = False
    except Exception, errMsg:
      self.assertEqual( str(errMsg),
        "Error, type 'invalid' is not supported!  Supported types include"+\
        " ['string', 'int', 'float']!" )
    if not threwExcept:
      self.assertFalse("ERROR: Did not thown an excpetion")


#
# Run the unit tests!
#

if __name__ == '__main__':

  unittest.main()
