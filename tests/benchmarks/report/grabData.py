# coding: utf-8

'''
grabData.py
'''

import os
import re
import numpy


class NoInfoError(Exception):
    '''
    '''
    def __init__(self, _value="No info found!"):
        '''
        '''
        self.value = _value

    def __str__(self):
        '''
        '''
        return repr(self.value)


def getBenchmarkData(resultPath, output=True):
    '''
    '''

    resultFiles = findResultFiles(resultPath)
    caseNames, cases = parseResultFiles(resultFiles, output)

    return caseNames, cases


def findResultFiles(resultPath):
    '''
    '''

    resultFiles = []

    for root, dirs, files in os.walk(resultPath):
        for f in files:
            if f.endswith(".results"):
                resultFiles.append(root + "/" + f)

    return resultFiles


def parseResultFiles(resultFiles, output=True):
    '''
    '''

    caseNames = []
    cases = {}

    for f in resultFiles:

        f_handle = open(f, "r", encoding="utf-8", errors="replace")

        content = f_handle.read()
        f_handle.close()

        try:

            if output:
                print("parsing " + f + " ...")

            caseName, mode, algorithm, nprocs, avgTime = \
                parseSingleResultFile(content)

        except NoInfoError as e:

            print("\nIn the file " + f + ": " + e.value + "\n")

            choose = input(
                    "Add extension \".broken\" to the broken file " +
                    "and ignore it? (Y/n)")

            if choose != "n":
                os.rename(f, f + ".broken")
            else:
                raise

        if caseName not in caseNames:
            caseNames.append(caseName)
            cases[caseName] = {}

        if mode not in cases[caseName].keys():
            cases[caseName][mode] = {}

        if algorithm not in cases[caseName][mode].keys():
            cases[caseName][mode][algorithm] = {}

        cases[caseName][mode][algorithm][int(nprocs)] = float(avgTime)

    return caseNames, cases


def parseSingleResultFile(content):
    '''
    '''
    caseInfoKey = \
        re.compile(
                "=*?" + "\n" + "-*?" + "\n" +
                "Case Name: " +
                "(?P<caseName>Nx(?P<Nx>\d*)Ny(?P<Ny>\d*)Nz(?P<Nz>\d*))" +
                "_" + "(?P<mode>.*?)" + "_" + "(?P<algorithm>.*?)" + "_" +
                "(?P<nprocs>\d*)" + "\n" +
                "Nx: " + "(?P=Nx)" + "\n" +
                "Ny: " + "(?P=Ny)" + "\n" +
                "Nz: " + "(?P=Nz)" + "\n" +
                "Mode: " + "(?P=mode)" + "\n" +
                "Config File: " + ".*?" + "\n" +
                "Number of Solves: " + "(?P<nRuns>\d*?)" + "\n" +
                "Output PETSc Log File \? " + "\d{1}" + "\n" +
                "Output PETSc Log File Name:\ " + "\S*?" + "\n" +
                "Output VTK file \? " + "\d{1}" + "\n" +
                "-*?" + "\n" + "=*?" + "\n" +
                "(?P<timings>.*?)" + "\n" +
                "=*?" + "\n" + "-*?" + "\n" +
                "End of (?P=caseName)_(?P=mode)_(?P=algorithm)_(?P=nprocs)\n" +
                "-*?" + "\n" + "=*?" + "\n",
                re.DOTALL)

    caseInfoMatch = caseInfoKey.finditer(content)

    for i, match in enumerate(caseInfoMatch):
        dataBlock = match.groupdict()

    try:
        dataBlock["timings"] = parseTimingBlocks(dataBlock["timings"])
    except UnboundLocalError:
        dataBlock = -1

    if dataBlock == -1:
        raise NoInfoError("No case info was found!")

    return dataBlock["caseName"], dataBlock["mode"], dataBlock["algorithm"], \
        dataBlock["nprocs"], dataBlock["timings"]["avgTime"]


def parseTimingBlocks(content):
    '''
    '''
    avgTimeKey = re.compile(
            "The averaged solving time is: (?P<avgTime>\d*.\d*)")

    avgTimeMatch = avgTimeKey.search(content)

    try:
        avgTime = avgTimeMatch.group(1)
    except AttributeError:
        avgTime = -1

    if avgTime == -1:
        raise NoInfoError("Averaged time was not found!")

    timingKey = re.compile(
            "Run \# (?P<run>\d*) \.\.\. \n" +
            ".*\n?" +
            "\tSolve Time: (?P<time>\d*\.\d*)")

    timingMatch = timingKey.finditer(content)

    times = numpy.zeros(0)

    for i, match in enumerate(timingMatch):
        times = numpy.append(times, float(match.groupdict()["time"]))

    if times.size == 0:
        raise NoInfoError("No timing of any run was found!")

    return {"avgTime": avgTime, "times": times}
