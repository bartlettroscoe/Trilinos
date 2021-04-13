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
import gather_build_stats as GBS
import FindTribitsCiSupportDir
import GeneralScriptSupport as GSS

g_pp = pprint.PrettyPrinter(indent=2)


#############################################################################
#
# Test gather_build_stats.readBuildStatsTimingFileIntoDict()
#
#############################################################################


def readBuildStatsTimingFileIntoDictTest(testObj, buildStatsTimingFile,
    numKeys_expected, buildStatsTimingDict_expected, errMsg_expected,
  ):
  (buildStatsTimingDict, errMsg) = GBS.readBuildStatsTimingFileIntoDict(
    buildStatsTimingFile)
  testObj.assertEqual(errMsg, errMsg_expected)
  if numKeys_expected > 0:
    testObj.assertEqual(len(buildStatsTimingDict.keys()), numKeys_expected)
  testObj.assertEqual(buildStatsTimingDict, buildStatsTimingDict_expected)


class test_readBuildStatsTimingFileIntoDict(unittest.TestCase):

  def test_correct(self):
    buildStatsTimingFile = \
      g_testBaseDir+"/dummy_build_dir/some/base/dir/target1.timing"
    numKeys_expected = 6
    buildStatsTimingDict_expected = {
      'FileName': './some/base/dir/target1.o',
      'FileSize': '3300000',
      'elapsed_real_time_sec': '3.5',
      'max_resident_size_Kb': '240000',
      'num_filesystem_outputs': '20368',
      'num_involuntary_context_switch': '46',
      }
    errMsg_expected = ""
    readBuildStatsTimingFileIntoDictTest(self, buildStatsTimingFile,
      numKeys_expected, buildStatsTimingDict_expected, errMsg_expected)

  def test_missing_fail(self):
    buildStatsTimingFile = \
      g_testBaseDir+"/file_does_not_exist.timing"
    numKeys_expected = 0
    buildStatsTimingDict_expected = None
    errMsg_expected = buildStatsTimingFile+": ERROR: File does not exist!"
    readBuildStatsTimingFileIntoDictTest(self, buildStatsTimingFile,
      numKeys_expected, buildStatsTimingDict_expected, errMsg_expected)

  def test_two_data_rows_fail(self):
    buildStatsTimingFile = \
      g_testBaseDir+"/bad_timing_build_stats_files/target1.timing.two_data_rows"
    numKeys_expected = 0
    buildStatsTimingDict_expected = None
    errMsg_expected = buildStatsTimingFile+": ERROR: Contains 2 != 1 data rows!"
    readBuildStatsTimingFileIntoDictTest(self, buildStatsTimingFile,
      numKeys_expected, buildStatsTimingDict_expected, errMsg_expected)

  def test_empty_fail(self):
    buildStatsTimingFile = \
      g_testBaseDir+"/bad_timing_build_stats_files/target1.timing.empty"
    numKeys_expected = 0
    buildStatsTimingDict_expected = None
    errMsg_expected = buildStatsTimingFile+": ERROR: File is empty!"
    readBuildStatsTimingFileIntoDictTest(self, buildStatsTimingFile,
      numKeys_expected, buildStatsTimingDict_expected, errMsg_expected)

  # ToDo: Test with garbage file contents

  # ToDo: Test missing different required headers

  # ToDo: Test for missing data from required header


#
# Run the unit tests!
#

if __name__ == '__main__':

  unittest.main()
