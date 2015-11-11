#!/usr/bin/python

import os
import sys
import time
import re
import fnmatch

## Early checks:

if len(sys.argv) < 4:
  print "Usage: $> runhtml.py BRANCH LOGDIR OUTFILE";
  print " BRANCH  ... Branch log files to be processed";
  print " LOGDIR  ... Directory where to find the log files";
  print " OUTFILE ... The output file where the HTML code will be written to";
  print "Aborting..."
  sys.exit(1)




######### Helper routines #########

# Helper function: Obtain execution time from log file:
def execution_time(logfilename):
  start_hh = 0;
  start_mm = 0;
  start_ss = 0;

  end_hh = 0;
  end_mm = 0;
  end_ss = 0;

  for line in open(logfilename):
    if re.search(r'^Build on', line) or re.search(r'^Starting on', line) or re.search(r'^Starting Configure', line):
      match = re.search(r'.*?([0-9]*[0-9]):([0-9][0-9]):([0-9][0-9]).*', line)
      start_hh = match.group(1)
      start_mm = match.group(2)
      start_ss = match.group(3)

    if re.search(r'Finished Build on', line) or re.search(r'^Finishing at', line) or re.search(r'^Finishing Configure', line):
      match = re.search(r'.*?([0-9]*[0-9]):([0-9][0-9]):([0-9][0-9]).*', line)
      end_hh = match.group(1)
      end_mm = match.group(2)
      end_ss = match.group(3)

  starttime = int(start_hh) * 3600 + int(start_mm) * 60 + int(start_ss)
  endtime   = int(  end_hh) * 3600 + int(  end_mm) * 60 + int(  end_ss)

  if starttime > endtime:
    if int(start_hh) < 13:  #12 hour format
      endtime += 12*3600
    else:
      endtime += 24*3600

  #print "Start time: " + str(start_hh) + ":" + str(start_mm) + ":" + str(start_ss)
  #print "End time: " + str(end_hh) + ":" + str(end_mm) + ":" + str(end_ss)
  #print "starttime: " + str(starttime)
  #print "endtime: " + str(endtime)

  return endtime - starttime

# Helper function: Convert number of seconds to format hh:mm:ss
def format_time(time_in_seconds):
  #print "time_in_seconds: " + str(time_in_seconds)
  if time_in_seconds > 1800:
    return "<td class=\"yellow\">" + str(time_in_seconds / 3600).zfill(2) + ":" + str((time_in_seconds % 3600) / 60).zfill(2) + ":" + str(time_in_seconds % 60).zfill(2) + "</td>"

  return "<td class=\"green\">" + str(time_in_seconds / 3600).zfill(2) + ":" + str((time_in_seconds % 3600) / 60).zfill(2) + ":" + str(time_in_seconds % 60).zfill(2) + "</td>"



###### Main execution body ##########


outfile = open(sys.argv[3], "w")
examples_summary_file = open(sys.argv[2] + "/examples_full.log", "w")

# Static HTML header:
outfile.write("""
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head><title>PETSc Test Summary</title>
<style type="text/css">
div.main {
  max-width: 1300px;
  background: white;
  margin-left: auto;
  margin-right: auto;
  padding: 20px;
  padding-top: 0;
  border: 5px solid #CCCCCC;
  border-radius: 10px;
  background: #FBFBFB;
}
table {
  /*border: 1px solid black;
  border-radius: 10px;*/
  padding: 3px;
  margin-top: 0;
}
td a:link, td a:visited, td a:focus, td a:active {
  font-weight: bold;
  text-decoration: underline;
  color: black;
}
td a:hover {
  font-weight: bold;
  text-decoration: underline;
  color: black;
}
th {
  padding: 10px;
  padding-top: 5px;
  padding-bottom: 5px;
  font-size: 1.1em;
  font-weight: bold;
  text-align: center;
}
td.desc {
  max-width: 650px;
  padding: 2px;
  font-size: 0.9em;
}
td.green {
  text-align: center;
  vertical-align: middle;
  padding: 2px;
  background: #01DF01;
  min-width: 50px;
}
td.yellow {
  text-align: center;
  vertical-align: middle;
  padding: 2px;
  background: #F4FA58;
  min-width: 50px;
}
td.red {
  text-align: center;
  vertical-align: middle;
  padding: 2px;
  background: #FE2E2E;
  min-width: 50px;
}
</style>
</head>
<body><div class="main"> """)


outfile.write("<center><span style=\"font-size:1.3em; font-weight: bold;\">PETSc Test Summary</span><br />Last update: " + time.strftime("%c") + "</center>\n")

outfile.write("<center><table>\n");

outfile.write("<tr><th></th><th colspan=\"2\">Configure</th><th></th><th></th> <th colspan=\"3\">Make</th><th></th><th></th> <th colspan=\"2\">Examples</th></tr>\n");
outfile.write("<tr><th>Arch</th><th>Status</th><th>Duration</th><th></th><th></th> <th>Warnings</th><th>Errors</th><th>Duration</th><th></th><th></th> <th>Problems?</th><th>Duration</th><td><a href=\"examples_full.log\">[all]</a></td></tr>\n");

num_builds = 0

for root, dirs, filenames in os.walk(sys.argv[2]):
  filenames.sort()
  for f in filenames:
    if fnmatch.fnmatch(f, "build_" + sys.argv[1] + "_*.log"):

      num_builds += 1

      # form other file names:
      match = re.search("build_" + sys.argv[1] + "_arch-(.*).log", f)
      logfile_build = f
      logfile_build_full = os.path.join(root, f)
      logfile_make  = "make_"  + sys.argv[1] + "_arch-" + match.group(1) + ".log"
      logfile_make_full = os.path.join(root, logfile_make)
      logfile_examples  = "examples_"  + sys.argv[1] + "_arch-" + match.group(1) + ".log"
      logfile_examples_full = os.path.join(root, logfile_examples)
      logfile_configure = "configure_" + sys.argv[1] + "_arch-" + match.group(1) + ".log"
      logfile_configure_full = os.path.join(root, logfile_configure)

      print "Processing " + match.group(1)

      ### Start table row
      outfile.write("<tr><td>" + match.group(1) + "</td>")

      #
      # Check if some logs are missing. If so, don't process further and write 'incomplete' to table:
      #
      if not os.path.isfile(logfile_configure_full) or not os.path.isfile(logfile_make_full) or not os.path.isfile(logfile_examples_full):
        print "  -- incomplete logs!"

        # Configure section
        outfile.write("<td class=\"red\">Incomplete</td>")
        outfile.write("<td class=\"red\">Incomplete</td>")
        if os.path.isfile(logfile_configure_full): outfile.write("<td><a href=\"" + logfile_configure + "\">[log]</a></td>")
        else: outfile.write("<td></td>")

        # Make/Build section
        outfile.write("<td></td>")
        outfile.write("<td class=\"red\" colspan=\"2\"><a href=\"" + logfile_build + "\">[build.log]</a></td>")
        if os.path.isfile(logfile_make_full):
          outfile.write("<td class=\"red\" colspan=\"2\"><a href=\"" + logfile_make + "\">[make.log]</a></td>")
        else:
          outfile.write("<td colspan=\"2\" class=\"red\">Incomplete</td>")

        # Examples section
        outfile.write("<td></td>")
        outfile.write("<td class=\"red\">Incomplete</td>")
        outfile.write("<td class=\"red\">Incomplete</td>")
        if os.path.isfile(logfile_examples_full): outfile.write("<td><a href=\"" + logfile_examples + "\">[log]</a></td>")
        else: outfile.write("<td></td>\n")
        continue


      #
      ### Configure section
      #

      # Checking for successful completion
      configure_success = False
      for line in open(logfile_configure_full):
        if re.search(r'Configure stage complete', line):
          outfile.write("<td class=\"green\">Success</td>")
          outfile.write(format_time(execution_time(logfile_configure_full)))
          configure_success = True

      if configure_success == False:
          outfile.write("<td class=\"red\">Fail</td>")
          outfile.write("<td class=\"red\">Fail</td>")
      outfile.write("<td><a href=\"" + logfile_configure + "\">[log]</a></td>")

      #
      ### Make section
      #

      outfile.write("<td></td>")
      # Warnings:
      warning_list = []
      exclude_warnings = ["unrecognized .pragma",
                          "warning: .SSL",
                          "warning: .BIO_",
                          "warning: treating 'c' input as 'c..' when in C.. mode",
                          "warning: statement not reached",
                          "warning: loop not entered at top",
                          "is deprecated",
                          "is superseded",
                          "warning: no debug symbols in executable (-arch x86_64)",
                          "(aka 'const double *') doesn't match specified 'MPI' type tag that requires 'double *'",
                          "(aka 'const int *') doesn't match specified 'MPI' type tag that requires 'int *'",
                          "(aka 'const long *') doesn't match specified 'MPI' type tag that requires 'long long *'",
                          "(aka 'long *') doesn't match specified 'MPI' type tag that requires 'long long *'",
                          "cusp/complex.h", "cusp/detail/device/generalized_spmv/coo_flat.h",
                          "thrust/detail/vector_base.inl", "thrust/detail/tuple_transform.h", "detail/tuple.inl", "detail/launch_closure.inl"]
      for line in open(logfile_make_full):
        if re.search(r'[Ww]arning[: ]', line):
          has_serious_warning = True
          for warning in exclude_warnings:
            if line.find(warning):
              has_serious_warning = False
              break
          if has_serious_warning == True:
            warning_list.append(line)
      num_warnings = len(warning_list)
      if num_warnings > 0:
        outfile.write("<td class=\"yellow\">" + str(num_warnings) + "</td>")
      else:
        outfile.write("<td class=\"green\">" + str(num_warnings) + "</td>")

      # Errors:
      error_list = []
      error_list_with_context = []
      f = open(logfile_make_full)
      lines = f.readlines()
      for i in range(len(lines)):
        if re.search(" [Kk]illed", lines[i]) or re.search(" [Ff]atal[: ]", lines[i]) or re.search(" [Ee][Rr][Rr][Oo][Rr][: ]", lines[i]):
          error_list.append(lines[i])
          if i > 1:
            error_list_with_context.append(lines[i-2])
          if i > 0:
            error_list_with_context.append(lines[i-1])
          error_list_with_context.append(lines[i])
          if i+1 < len(lines):
            error_list_with_context.append(lines[i+1])
          if i+2 < len(lines):
            error_list_with_context.append(lines[i+2])
          if i+3 < len(lines):
            error_list_with_context.append(lines[i+3])
          if i+4 < len(lines):
            error_list_with_context.append(lines[i+4])
          if i+5 < len(lines):
            error_list_with_context.append(lines[i+5])

      num_errors = len(error_list)
      if num_errors > 0:
        outfile.write("<td class=\"red\">" + str(num_errors) + "</td>")
      else:
        outfile.write("<td class=\"green\">" + str(num_errors) + "</td>")
      outfile.write(format_time(execution_time(logfile_make_full)))
      outfile.write("<td><a href=\"filtered-" + logfile_make + "\">[log]</a><a href=\"" + logfile_make + "\">[full]</a></td>")

      # Write filtered output file:
      filtered_logfile = os.path.join(root, "filtered-" + logfile_make)
      filtered_file = open(filtered_logfile, "w")
      filtered_file.write("---- WARNINGS ----\n")
      for warning_line in warning_list:
        filtered_file.write(warning_line)
      filtered_file.write("\n---- ERRORS ----\n")
      for error_line in error_list_with_context:
        filtered_file.write(error_line)
      filtered_file.close()

      #
      ### Examples section
      #
      outfile.write("<td></td>")
      example_problem_num = 0
      write_to_summary = True
      if match.group(1).startswith("c-exodus-dbg-builder") or match.group(1).startswith("cuda-single"):
        write_to_summary = False
      for line in open(logfile_examples_full):
        if write_to_summary:
          examples_summary_file.write(line)
        if re.search(r'[Pp]ossible [Pp]roblem', line):
          example_problem_num += 1

      if example_problem_num < 1:
         outfile.write("<td class=\"green\">0</td>")
      else:
         outfile.write("<td class=\"yellow\">" + str(example_problem_num) + "</td>")
      outfile.write(format_time(execution_time(logfile_examples_full)))
      outfile.write("<td><a href=\"" + logfile_examples + "\">[log]</a></td>")

      ### End of row
      outfile.write("</tr>\n")

# write footer:
outfile.write("</table>")
outfile.write("Total number of builds: " + str(num_builds))
outfile.write("<br />This page is an automated summary of the output generated by the PETSc testsuite.<br /> It is generated by $PETSC_DIR/src/contrib/nightlysummary/runhtml.py.</center>\n")
outfile.write("</div></body></html>")
outfile.close()
examples_summary_file.close()

#print "Testing execution time: "
#print format_time(execution_time(sys.argv[2]))
