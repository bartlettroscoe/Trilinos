#!/usr/bin/env python
# -*- coding: utf-8 -*-


from FindTribitsCiSupportDir import *
import GeneralScriptSupport as GSS
import CDashQueryAnalyzeReport as CDQAR


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
   listOfDicts = None
   exceptObj = None
   if os.path.exists(buildStatsTimingFile):
     try:
       listOfDicts = CDQAR.readCsvFileIntoListOfDicts(buildStatsTimingFile)
     except Exception as exceptObj:
       #print("exceptObj: "+str(exceptObj))
       if str(exceptObj).find("is empty which is not allowed") != -1:
         errMsg = buildStatsTimingFile+": ERROR: File is empty!"
       else:
         errMsg = buildStatsTimingFile+": ERROR: "+str(exceptObj)
       # NOTE: The above check is tried pretty tighlty to the implementation
       # of readCsvFileIntoListOfDicts() in looking for a specific substring
       # in the error message but it will still capture any error and report
       # it through errMsg so this is actaully pretty robust.
   else:
     errMsg = buildStatsTimingFile+": ERROR: File does not exist!"

   # Check for other error conditions and log them in errMsg
   if listOfDicts == None:
     None: # The errMsg was set above!
   elif not len(listOfDicts) == 1:
     errMsg = buildStatsTimingFile+": ERROR: Contains "+\
       str(len(listOfDicts))+" != 1 data rows!"
   # ToDo: Check for other problems!
   else:
     # No errors found, so return the first row (which is the only row)
     buildStatsTimingDict = listOfDicts[0]

   return (buildStatsTimingDict, errMsg)
