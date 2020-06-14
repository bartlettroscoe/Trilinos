#!/usr/bin/env python
# -*- coding: utf-8 -*-

import csv

from FindTribitsCiSupportDir import *
import GeneralScriptSupport as GSS
import CDashQueryAnalyzeReport as CDQAR


#
# Helper functions
#


# Read a CSV file of build stats into a dict of lists
#
def readBuildStatsCsvFileIntoDictOfLists(buildStatusCsvFileName):
  return readCsvFileIntoDictOfLists(buildStatusCsvFileName,
    getStandardBuildStatsColsAndTypesList() )


# Standard ste of build stats fields we want to read in
#
def getStandardBuildStatsColsAndTypesList():
  return [
    ColNameAndType('max_resident_size_Kb', 'float'),
    ColNameAndType('elapsed_real_time_sec', 'float'),
    ColNameAndType('FileName', 'string'),
    ColNameAndType('FileSize', 'float'),
    ]


# Read in a CSV file as a dict of lists.
#
def readCsvFileIntoDictOfLists(csvFileName,
    colNameAndTypeList,
  ):
  dictOfLists = {}
  with open(csvFileName, 'r') as csvFile:
    csvReader = csv.reader(csvFile)
    # Get the list of col headers and the index to the col headers we want 
    columnHeadersList = \
      CDQAR.getColumnHeadersFromCsvFileReader(csvFileName, csvReader)
    colNameTypeIdxList = \
      getColNameTypeIdxListGivenColNameAndTypeList(csvFileName, columnHeadersList,
        colNameAndTypeList)
    # Initial empty lists for each column to hold the data
    for colNameTypeIdx in colNameTypeIdxList:
      dictOfLists.update( { colNameTypeIdx.colName() : [] } )
    # Fill the columns of data
    dataRow = 0
    for lineList in csvReader:
      if not lineList: continue # Ingore blank line
      CDQAR.stripWhiltespaceFromStrList(lineList)
      #CDQAR.assertExpectedNumColsFromCsvFile(csvFileName, dataRow, lineList,
      #  columnHeadersList)
      # Read the row entries
      for colNameTypeIdx in colNameTypeIdxList:
        dictOfLists[colNameTypeIdx.colName()].append(
          colNameTypeIdx.convertFromStr(lineList[colNameTypeIdx.getIdx()]) )
      # Update for next row
      dataRow += 1
  # Return completed dict of lists
  return dictOfLists


def getColNameTypeIdxListGivenColNameAndTypeList(csvFileName, columnHeadersList,
    colNameAndTypesToGetList,
  ):
  colNameTypeIdxList = []
  for colNameAndTypeToGet in colNameAndTypesToGetList:
    colIdx = GSS.findInSequence(columnHeadersList, colNameAndTypeToGet.colName())
    if colIdx != -1:
      colNameTypeIdxList.append(ColNameTypeIdx(colNameAndTypeToGet, colIdx))
    else:
      raise Exception(
        "Error, the CSV file column header '"+colNameAndTypeToGet.colName()+"'"+\
        " does not exist in the list of column headers "+str(columnHeadersList)+\
        " from the CSV file '"+csvFileName+"'!")
  return colNameTypeIdxList


class ColNameAndType(object):
  def __init__(self, colName, colType):
    self.__colName = colName
    self.__colType = colType
    self.assertType()
  def colName(self):
    return self.__colName
  def colType(self):
    return self.__colType
  def __repr__(self):
    myStr = "ColNameAndType{"+self.__colName+","+str(self.__colType)+"}"
    return myStr
  def convertFromStr(self, strIn):
    if self.__colType == "string":
      return strIn
    elif self.__colType == "int":
      return int(strIn)
    elif self.__colType == "float":
      return float(strIn)
  def assertType(self):
    supportedTypes = [ "string", "int", "float" ]
    if -1 == GSS.findInSequence(supportedTypes, self.__colType):
      raise Exception(
        "Error, type '"+str(self.__colType)+"' is not supported!  Supported types"+\
        " include "+str(supportedTypes)+"!")
  def __eq__(self, other):
    return((self.__colName,self.__colType)==(other.__colName,other.__colType))
  def __ne__(self, other):
    return((self.__colName,self.__colType)!=(other.__colName,other.__colType))


class ColNameTypeIdx(object):
  def __init__(self, colNameAndType, colIdx):
    self.__colNameAndType = colNameAndType
    self.__colIdx = colIdx
  def colName(self):
    return self.__colNameAndType.colName()
  def getIdx(self):
    return self.__colIdx
  def convertFromStr(self, strIn):
    return self.__colNameAndType.convertFromStr(strIn)
  def __repr__(self):
    myStr = "ColNameTypeIdx{"+str(self.__colNameAndType)+","+str(self.__colIdx)+"}"
    return myStr
  def __eq__(self, other):
    return ((self.__colNameAndType,self.__colIdx)==(other.__colNameAndType,other.__colIdx))
  def __ne__(self, other):
    return ((self.__colNameAndType,self.__colIdx)!=(other.__colNameAndType,other.__colIdx))


#
#  Main()
# 
