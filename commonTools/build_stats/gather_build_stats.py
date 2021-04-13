#!/usr/bin/env python
# -*- coding: utf-8 -*-


from FindTribitsCiSupportDir import *
import GeneralScriptSupport as GSS
import CDashQueryAnalyzeReport as CDQAR

from BuildStatsData import *

#
# Helper functions
#


# Robustly Read a CSV build stats timing file created by magic_wrapper.py and
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
