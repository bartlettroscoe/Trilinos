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
   buildStatsTimingDict = None
   errMsg = ""

   listOfDicts = CDQAR.readCsvFileIntoListOfDicts(buildStatsTimingFile)

   if not len(listOfDicts) == 1:
     errMsg = buildStatsTimingFile+": WARNING: Contains "+\
       str(len(listOfDicts))+" != 1 data rows!"
   # ToDo: Check for other problems!
   else:
     buildStatsTimingDict = listOfDicts[0] # Return the one dict

   return (buildStatsTimingDict, errMsg)
