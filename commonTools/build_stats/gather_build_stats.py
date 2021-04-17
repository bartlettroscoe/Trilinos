#!/usr/bin/env python
# -*- coding: utf-8 -*-

import csv

from FindTribitsCiSupportDir import *
import GeneralScriptSupport as GSS
import CDashQueryAnalyzeReport as CDQAR

from BuildStatsData import *


#
# Helper functions
#


# Robustly read all CSV build stats *timing files under a base dir and return
# as a list of dicts (LOD).
#
def readAllValidTimingFiles(baseDir, printErrMsg=True):
  listOfAllTimingFiles = getListOfAllTimingFiles(baseDir)
  allValidTimingFilesLOD = []
  for timingFile in listOfAllTimingFiles:
    timingFileFullPath = baseDir+"/"+timingFile
    (buildStatsTimingDict, errMsg) = \
      readBuildStatsTimingFileIntoDict(timingFileFullPath)
    if errMsg != "" and printErrMsg:
      print(errMsg)
    if not buildStatsTimingDict == None:
      allValidTimingFilesLOD.append(buildStatsTimingDict)
  return allValidTimingFilesLOD


# Robustly read a CSV build stats timing file created by magic_wrapper.py and
# return as dict.
#
# Returns tuple:
#
#  (timingBuildStatsDict, errorMsg)
#
# If the timing build stats file 'buildStatsTimingFile' exists and has the
# correct data, then 'timingBuildStatsDict' will be a simple dict with the
# contents of the CSV file.  Otherwise, 'timingBuildStatsDict' will be 'None'
# and 'errMsg' will contain an error.
#
# The provides for a very robust reading of these timing build stats files in
# case there are problems with the running of the magic_wrapper.py tool.
#
def readBuildStatsTimingFileIntoDict(buildStatsTimingFile):

  # Output data initialization
  buildStatsTimingDict = None
  errMsg = ""

  # Read CSV file into list of dicts and deal with direct errors
  (listOfDicts, errMsg) = robustReadCsvFileIntoListOfDicts(buildStatsTimingFile)

  # Check for other error conditions and log them in errMsg

  if errMsg == "" and not len(listOfDicts) == 1:
    errMsg = buildStatsTimingFile+": ERROR: Contains "+\
      str(len(listOfDicts))+" != 1 data rows!"

  if listOfDicts != None and errMsg == "":
     # No errors found to this point, so grab the first row as the build stats dict
     buildStatsTimingDict = listOfDicts[0]

  if buildStatsTimingDict != None and errMsg == "":
    errMsgBody = checkBuildStatsTimingDictHasError(buildStatsTimingDict)
    if errMsgBody != "":
      errMsg = buildStatsTimingFile+": "+errMsgBody

  # If there was an error found, make sure not to return a dict object
  if errMsg != "":
    buildStatsTimingDict = None

  return (buildStatsTimingDict, errMsg)


# Call readCsvFileIntoListOfDicts() but make robust to basic read errors.
#
# Returns:
#
#   (listOfDicts, errMsg)
#
# Returns a valid list of dicts listOfDicts!=None unless some error occurs.
# If some error occured, then errMsg will be sets to a string descrdibing what
# the problem was.
#
# No exception should ever be thrown from this function.  This is useful to
# use in cases where the existance or basic structure of a CSV file may be
# broken.
#
def robustReadCsvFileIntoListOfDicts(csvFile):
   listOfDicts = None
   errMsg = ""
   if os.path.exists(csvFile):
     try:
       listOfDicts = CDQAR.readCsvFileIntoListOfDicts(csvFile)
     except Exception as exceptObj:
       #print("exceptObj: "+str(exceptObj))
       if str(exceptObj).find("is empty which is not allowed") != -1:
         errMsg = csvFile+": ERROR: File is empty!"
       else:
         errMsg = csvFile+": ERROR: "+str(exceptObj)
       # NOTE: The above check is tried pretty tighlty to the implementation
       # of readCsvFileIntoListOfDicts() in looking for a specific substring
       # in the error message but it will still capture any error and report
       # it through errMsg so this is actaully pretty robust.
   else:
     errMsg = csvFile+": ERROR: File does not exist!"
   return (listOfDicts, errMsg)
# ToDo: Move the above function to CsvFileUtils.py!


# Write a dict of lists to a CSV File
#
def writeDictOfListsToCsvFile(dictOfLists, csvFile):
  keysList = dictOfLists.keys()
  keysList.sort()
  numTotalRows = len(dictOfLists.get(keysList[0]))  # All lists are same length
  numTotalKeys = len(keysList)
  with open(csvFile, "w") as csvFileHandle:
    csvWriter = csv.writer(csvFileHandle, delimiter=",", lineterminator="\n")
    csvWriter.writerow(keysList)
    for i in range(numTotalRows):
      rowList = []
      for aKey in keysList:
        rowList.append(dictOfLists.get(aKey)[i])
      csvWriter.writerow(rowList)
# ToDo: Move the above function to CsvFileUtils.py!


# Assert that a build stats timing dict contains the required fields and has
# valid data and the type of the datal
#
# Returns errMsg=="" if there is no error.  Otherwise, errMsg describes the
# nature of the error.
#
def checkBuildStatsTimingDictHasError(buildStatsTimingDict):
  errMsg = ""
  for stdBuildStatColAndType in getStdBuildStatsColsAndTypesList():
    requiredFieldName = stdBuildStatColAndType.colName()
    requiredFieldType = stdBuildStatColAndType.colType()
    strVal = buildStatsTimingDict.get(requiredFieldName, None)
    if strVal == None:
      errMsg = "ERROR: The required field '"+requiredFieldName+"' is missing!"
      break
    try:
      convertedVal = stdBuildStatColAndType.convertFromStr(strVal)
    except Exception as exceptObj:
      errMsg = "ERROR: For field '"+requiredFieldName+"' the string value '"+strVal+"'"+\
        " could not be converted to the expected type '"+requiredFieldType+"'!"
  return errMsg


# Get list of all *.timing files under baseDir and return paths relative to
# baseDir.
#
def getListOfAllTimingFiles(baseDir):
  listOfAllTimingFiles = []
  for root, subdirs, files in os.walk(baseDir):
    relRoot = root.replace(baseDir+"/","")
    for aFile in files:
      if aFile.endswith(".timing"):
        listOfAllTimingFiles.append(relRoot+"/"+aFile)
  return listOfAllTimingFiles


# Fill in dict of lists for combined info from a list of dicts
#
# The output dict of lists will have the superset of keys from all of the
# input dicts in the listOfDicts and any non-existant values will be given the
# empty string "" instead of `None`.
#
def getDictOfListsFromListOfDicts(listOfDicts):
  numTotalRows = len(listOfDicts)
  supersetOfFieldNamesList = getSupersetOfFieldNamesList(listOfDicts)
  dictOfLists = {}
  # Create dict of lists with all empty values
  for keyName in supersetOfFieldNamesList:
    dictOfLists.update( { keyName : [""] * numTotalRows } )
  # Fill in the values from the dicts in the list
  for i in range(numTotalRows):
    aDict = listOfDicts[i]
    for key, value in aDict.items():
      dictOfLists.get(key)[i] = value
  # Return the completed data-structure
  return dictOfLists


# Get superset of all of the field names for a list of dicts
#
def getSupersetOfFieldNamesList(listOfDicts):
  supersetOfFieldNames = set()
  for aDict in listOfDicts:
    supersetOfFieldNames = supersetOfFieldNames.union(aDict.keys())
  return list(supersetOfFieldNames)


#
# Helper functions for main()
#


#
# Help message
#

usageHelp = r"""

Gather up build stats from *.timing CSV files created by the magic_wrapper.py
tool as a byproduct of building the various targets in a project.

This will discard any *.timing files that don't have valid values in at least
the fields:

  TODO: PUT THESE IN!

or if any other problem is found in a *.timing file.
"""


def injectCmndLineOptionsInParser(clp):

  clp.add_argument(
    "--base-dir", "-d", dest="baseDir", default="",
    help="Base directory for project build dir containing the *.timing files." )

  clp.add_argument("buildStatsCsvFile",
    help="The build status CSV file to created on output." )


def getCmndLineOptions():
  from argparse import ArgumentParser, RawDescriptionHelpFormatter
  clp = ArgumentParser(description=usageHelp,
    formatter_class=RawDescriptionHelpFormatter)
  injectCmndLineOptionsInParser(clp)
  options = clp.parse_args(sys.argv[1:])
  if options.baseDir == "":
    options.baseDir = os.getcwd()
  elif not os.path.exists(options.baseDir):
    print("Error, the base dir '"+options.baseDir+"' does not exist!")
  return options


#
#  Main()
#

if __name__ == '__main__':
  inOptions = getCmndLineOptions()
  print("Reading all *.timing files from under '"+inOptions.baseDir+"' ...")
  allValidTimingFilesListOfDicts = readAllValidTimingFiles(inOptions.baseDir)
  allValidTimingFilesDictOfLists = \
    getDictOfListsFromListOfDicts(allValidTimingFilesListOfDicts)
  writeDictOfListsToCsvFile(allValidTimingFilesDictOfLists,
    inOptions.buildStatsCsvFile)
