import os
import numpy
import re


slurmFiles = []
runCount = {}
wallTime = {}
caseNames = []

keyword1 = "Case Name: "
keyword2 = "Solve Time:  "

for root, dirs, files in os.walk("results"):
    for file in files:
        if file.endswith(".result") != -1:
            slurmFiles.append(file)

print("Found following result files:")
for file in slurmFiles:
    print("\t" + file)

for file in slurmFiles:
    f = open("results/" + file, "r")
    for line in f:
        if line.find(keyword1) != -1:
            n1 = line.find(keyword1)
            n2 = line.find("\n")
            caseName = line[n1+len(keyword1):n2]
            caseName = re.sub('GPU(?P<num>\d{1}$)', 'GPU0\g<1>', caseName)
            caseName = re.sub('PETSc(?P<num>\d{2}$)', 'PETSc0\g<1>', caseName)
            if caseName in runCount:
                runCount[caseName] += 1
            else:
                caseNames.append(caseName)
                runCount[caseName] = 1
        if line.find(keyword2) != -1:
            n1 = line.find(keyword2)
            n2 = line.find("s wall")
            time = float(line[n1+len(keyword2):n2])
            if caseName in wallTime:
                wallTime[caseName] += time
            else:
                wallTime[caseName] = time
    f.close()

for case in caseNames:
    wallTime[case] /= runCount[case]

for case in sorted(caseNames):
    print(case, ": ",  "{:f} sec, ".format(wallTime[case]), "{:d} runs".format(runCount[case]))
