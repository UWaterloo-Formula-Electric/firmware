#!/usr/bin/env python
from __future__ import print_function
import logging, sys
logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

import cantools #$ If you get a failure on this line, you need to install the python package cantools: install pip and run 'pip install -r requirements.txt' (you may want to do this in a virtual env)
import os
import subprocess
import sys
import errno
import re
import operator
from templateLoad import FSAETemplater
ReceivedSignalsArray = []
SentSignalsArray = []
DeclaredVariablesSignalsArray = []

canTemplater = FSAETemplater()

def isSignalNameInArray(signal, array):
    for sig in array:
        if sig.name == signal.name:
            return True

    return False

def checkForDuplicateSignalReceive(signal):
    if isSignalNameInArray(signal, ReceivedSignalsArray):
        return True
    else:
        ReceivedSignalsArray.append(signal)
        return False

def checkForDuplicateSignalSend(signal):
    if isSignalNameInArray(signal, SentSignalsArray):
        return True
    else:
        SentSignalsArray.append(signal)
        return False

def checkForDuplicateSignalDeclaration(signal):
    if isSignalNameInArray(signal, DeclaredVariablesSignalsArray):
        return True
    else:
        DeclaredVariablesSignalsArray.append(signal)
        return False

def create_dir(path):
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

def fWrite(string, fileHandle):
    fileHandle.write(string + '\n')

def generateDependencyFile(depFile, dbFile, headerFile, nodeName, boardType, ScriptsDir):
    with open(depFile, 'w+') as depFileHandle:
        fWrite('{headerFile}: {dir}/generateCANHeadder.py {dbFile}'.format(headerFile=headerFile, dbFile=dbFile, dir=ScriptsDir), depFileHandle)
        fWrite('	{dir}/generateCANHeadder.py {target} {boardType}'.format(target=nodeName, boardType=boardType, dir=ScriptsDir), depFileHandle)

def getGitCommit():
    label = '0df2a9a'
    gitClean = '0'

    try:
        if call(["git", "branch"], stderr=STDOUT, stdout=open(os.devnull, 'w')) != 0:
            label = subprocess.check_output(["git", "describe" ,"--tags", "--always"]).strip()
    except Exception:
        pass

    gitCommit = label

    return gitCommit

def writeHeaderFileIncludeGuardAndIncludes(boardType, headerFileHandle, nodeName, isChargerDBC=False):
    templateData = {}
    if isChargerDBC:
        includeDefineName = '__{nodeName}_charger_can_H'.format(nodeName=nodeName)
    else:
        includeDefineName = '__{nodeName}_can_H'.format(nodeName=nodeName)
    templateData = {
        "includeDefineName": includeDefineName,
        "boardTypeInclude": "stm32f7xx_hal.h" if boardType == 'F7' else "stm32f0xx_hal.h",
        "heartbeatInclude": "#include \"canHeartbeat.h\"" if not isChargerDBC else '',
    }
    templateOutput = canTemplater.load("INCLUDE_HEADERS_BEGIN",templateData)
    fWrite(templateOutput,headerFileHandle)


def writeEndIncludeGuard(sourceFileHandle, headerFileHandle):
    fWrite(canTemplater.load("END_SOURCE"), sourceFileHandle)
    fWrite(canTemplater.load("END_HEADER"), headerFileHandle)
def getNodeAddressAndMessageGroupsAndWriteToHeaderFile(db, nodeName, headerFileHandle):
    nodeAddress = 0
    messageGroups = list()
    nodeName = nodeName.upper()
    for node in db.nodes:
        if node.name == nodeName:
            nodeAddress = node.comment.split("_")[0]
            fWrite('#define CAN_NODE_ADDRESS ' + nodeAddress, headerFileHandle);
            for messageGroup in node.comment.split("_")[1].split(","):
                if messageGroup != "":
                    fWrite('#define CAN_NODE_MESSAGE_GROUP_'+messageGroup, headerFileHandle);
                    messageGroups.append(messageGroup)
    fWrite('', headerFileHandle);

    return messageGroups

def writeSourceFileIncludes(nodeName, sourceFileHandle, isChargerDBC=False):
    templateData = {
        "nodeName": nodeName,
        "isCharger": "_charger" if isChargerDBC else "",
    }
    fWrite(canTemplater.load("INCLUDE_SOURCE_BEGIN",templateData),sourceFileHandle)

def writeDBCVersionAndGitCommitToSourceFile(gitCommit, db, sourceFileHandle, isChargerDBC=False):
    templateData = {
        "charger": "Charger" if isChargerDBC else "",
        "version": "1" if not db.version else '' + db.version + '',
        "gitFill": "" if isChargerDBC else "char gitCommit[] = \""+gitCommit+"\";"
    }
    fWrite(canTemplater.load("DBC_GIT_SOURCE",templateData), sourceFileHandle)

def isRxMessage(msg, nodeName):
    for signal in msg.signals:
        if nodeName in signal.receivers:
            return True
    return False

def isProCanMessage(msg):
    for signal in msg.signals:
        if 'PRO_CAN' in signal.name:
            return True
    return False

def parseCanDB(db, nodeName):

    nodeName = nodeName.upper()

    rxMessages = [msg for msg in db.messages if isRxMessage(msg, nodeName)]
    txMessages = [msg for msg in db.messages if nodeName in msg.senders]

    multiplexedRxMessages = [msg for msg in rxMessages if msg.is_multiplexed()]
    multiplexedTxMessages = [msg for msg in txMessages if msg.is_multiplexed()]

    dtcRxMessages = [msg for msg in rxMessages if 'DTC' in msg.name]
    dtcTxMessages = [msg for msg in txMessages if 'DTC' in msg.name]

    proCanRxMessages = [msg for msg in rxMessages if isProCanMessage(msg)]
    proCanTxMessages = [msg for msg in txMessages if isProCanMessage(msg)]

    heartbeatRxMessages = [msg for msg in rxMessages if 'Heartbeat' in msg.name]

    normalRxMessages = [msg for msg in rxMessages if msg not in multiplexedRxMessages and msg not in dtcRxMessages and msg not in proCanRxMessages and msg not in heartbeatRxMessages]
    normalTxMessages = [msg for msg in txMessages if msg not in multiplexedTxMessages and msg not in dtcTxMessages and msg not in proCanTxMessages]

    return (rxMessages, txMessages, normalRxMessages, normalTxMessages,
            multiplexedRxMessages, multiplexedTxMessages, dtcRxMessages, dtcTxMessages, proCanRxMessages, proCanTxMessages, heartbeatRxMessages)

def getStrippedSignalName(signalName):
    strippedSignalName = re.sub('\d+$', '', signalName)
    return strippedSignalName

def writeStructForMsg(msg, structName, fileHandle):
    msgSizeBits = msg.length * 8
    currentPos = 0
    startBits = list()
    multiplexedSignalCount = 1

    fWrite('struct {structName} {{'.format(structName=structName), fileHandle)

    if msg.comment == 'VERSION':
        fWrite('	int DBC : 8;', fileHandle)
        for i  in range(0,7):
            fWrite('	char git{i} : 8;'.format(i=str(i)), fileHandle)
    else:
        # Sort signals by start bit (maybe this is true already, but make sure)
        sortedSignals = sorted(msg.signals, key=operator.attrgetter('start'))
        for signal in sortedSignals:
            if not signal.start in startBits:
                signalName = signal.name
                if signal.start != currentPos:
                    fWrite('    uint64_t FILLER_{num} : {fillerSize};'.format(num=str(currentPos), fillerSize=str(signal.start - currentPos)), fileHandle)
                if signal.multiplexer_signal != None:
                    signalName = re.sub('\d+$', '', signalName) + str(multiplexedSignalCount)
                    multiplexedSignalCount += 1
                if signal.is_signed:
                    fWrite('    int64_t {signalName} : {size};'.format(signalName=signalName, size=signal.length), fileHandle)
                else:
                    fWrite('    uint64_t {signalName} : {size};'.format(signalName=signalName, size=signal.length), fileHandle)
                currentPos = signal.start + signal.length
                startBits.append(signal.start)

        if currentPos != msgSizeBits:
            fWrite('    uint64_t FILLER_END : {size};'.format(size=str(msgSizeBits - currentPos)), fileHandle)

    fWrite('};\n', fileHandle)

def writeSignalReceivedFunction(signal, fileHandle, variableName='', multiplexed=False, dtc=False):
    if checkForDuplicateSignalReceive(signal):
        return
    if variableName == '':
        variableName = signal.name
    
    dataTypeOutput = 'float'
    if signal.scale == 1:
        if signal.is_signed:
            dataTypeOutput = 'int64_t'
        else:
            dataTypeOutput = 'uint64_t'

    if multiplexed:
        finalStatement = "{signalName}[index] = outValue".format(signalName=variableName)
    elif dtc:
        finalStatement = "return outValue;"
    else:
        finalStatement = "{signalName} = outValue;".format(signalName=variableName)

    templateData = {
        "returnType": "void" if (multiplexed or not dtc) else "int",
        "signalName": variableName,
        "indexOpt": "int index, " if multiplexed else "",
        "inputSign": "" if signal.is_signed else "u",
        "dataTypeOutput": dataTypeOutput,
        "scaler": signal.scale,
        "offset": signal.offset,
        "finalStatement": finalStatement,
    }
    fWrite(canTemplater.load("SIGNAL_RECEIVED_FUNC",templateData), fileHandle)

def getSignalSendingFunctionName(signal, multiplexed):
    signalName = signal.name

    if multiplexed:
        signalName = getStrippedSignalName(signalName)

    return '{signalName}Sending'.format(signalName=signalName)

def writeSignalSendingFunction(signal, fileHandle, variableName='', multiplexed=False):
    if checkForDuplicateSignalSend(signal):
        return

    sendDataType = 'float'
    if signal.scale == 1:
        if signal.is_signed:
            sendDataType = 'int64_t'
        else:
            sendDataType = 'uint64_t'
    templateData = {
        "returnDataType": 'int64_t' if signal.is_signed else 'uint64_t',
        "signalName": variableName if variableName != '' else signal.name,
        "sendDataType": sendDataType,
        "params": "int index" if multiplexed else "",
        "optIndex": "[index]" if multiplexed else "",
        "scaler": signal.scale,
        "offset": signal.offset,
    }
    fWrite(canTemplater.load("SIGNAL_SENDING_FUNC",templateData), fileHandle)


def writeValueTableEnum(signal, headerFileHandle):
    if signal.choices is not None:
        fWrite('enum {sigName}_Values {{'.format(sigName=signal.name), headerFileHandle)
        for Value, Name in signal.choices.items():
            fWrite('{sigName}_{Name} = {Value},'.format(sigName=signal.name, Name=Name, Value=Value), headerFileHandle)

        fWrite('};\n', headerFileHandle)

def writeSignalVariableAndVariableDeclaration(signal, sourceFileHandle, headerFileHandle):
    if checkForDuplicateSignalDeclaration(signal):
        return

    writeValueTableEnum(signal, headerFileHandle)

    dataType = dataTypeFromSignal(signal)

    
    # Write a semaphore for both RX and TX
    templateData = {
        "dataType": dataType,
        "name": signal.name
    }
    fWrite(canTemplater.load("SIGNAL_VAR_DECL_HEADER",templateData), headerFileHandle)
    fWrite(canTemplater.load("SIGNAL_VAR_DECL_SOURCE",templateData), sourceFileHandle)


def dataTypeFromSignal(signal):
    dataType = 'float'
    if signal.scale == 1:
        if signal.is_signed:
            dataType = 'int64_t'
        else:
            dataType = 'uint64_t'
    return dataType


def getReceivedSignalsFromMessage(msg, nodeName):
    return [signal for signal in msg.signals if nodeName.upper() in signal.receivers]

def writeNormalRxMessages(nodeName, normalRxMessages, sourceFileHandle, headerFileHandle):
    for msg in normalRxMessages:
        fWrite('// Struct and signal receive functions for msg {}'.format(msg.name), sourceFileHandle)
        writeStructForMsg(msg, msg.name, sourceFileHandle)

        for signal in getReceivedSignalsFromMessage(msg, nodeName):
            writeSignalVariableAndVariableDeclaration(signal, sourceFileHandle, headerFileHandle)
            writeSignalReceivedFunction(signal, sourceFileHandle)




def writeDTCRxMessages(nodeName, dtcRxMessages, sourceFileHandle, headerFileHandle):
    if len(dtcRxMessages) == 0:
        return

    # Create DTC Receive functions only once,
    # so just choose the first dtc message to base them on
    msg = dtcRxMessages[0]
    for signal in getReceivedSignalsFromMessage(msg, nodeName):
        fWrite('// signal received functions for multiplexed msg {}'.format(msg.name), sourceFileHandle)
        writeSignalReceivedFunction(signal, sourceFileHandle, dtc=True)



    # Now, create the struct
    for msg in dtcRxMessages:
        writeStructForMsg(msg, msg.name, sourceFileHandle)

        fWrite('typedef struct {msg.name}_unpacked {{'.format(**locals()), headerFileHandle)
        for signal in getReceivedSignalsFromMessage(msg, nodeName):
            fWrite('    int {signal.name};'.format(**locals()), headerFileHandle)
        fWrite('}} {msg.name}_unpacked;'.format(**locals()), headerFileHandle)

def getMultiplexerId(signal):
    if not signal.is_multiplexer:
        return signal.multiplexer_ids[0]
    else:
        return 0

def getMultiplexedMsgInfo(msg):
    for multiplexer in msg.signal_tree[0]:
        signalGroupsList = msg.signal_tree[0][multiplexer]
        signalGroup = signalGroupsList[0]
        numSignalsPerMessage = len(signalGroup)
        signalName = signalGroup[0]
        strippedSignalName = getStrippedSignalName(signalName)

    sortedSignals = sorted(msg.signals, key=getMultiplexerId, reverse=True)

    max_id = sortedSignals[0].multiplexer_ids[0]
    numSignals = (max_id+1) * numSignalsPerMessage

    # find if the signals are signed, or unsigned
    is_signed = False
    sampleSignal = ''
    for signal in msg.signals:
        if not signal.is_multiplexer:
            is_signed = signal.is_signed
            sampleSignal = signal
            break

    dataType = 'float'
    if signal.scale == 1:
        if signal.is_signed:
            dataType = 'int64_t'
        else:
            dataType = 'uint64_t'

    return (numSignalsPerMessage, strippedSignalName, numSignals, dataType, sampleSignal)

def writeIndexToMuxFunction(msgName, numSignalsPerMessage, sourceFileHandle, headerFileHandle):
    templateData = {
        "name": msgName,
        "numSignalsPerMessage": numSignalsPerMessage
    }
    fWrite(canTemplater.load("INDEX_TO_MUX_HEADER",templateData), headerFileHandle)
    fWrite(canTemplater.load("INDEX_TO_MUX_SOURCE",templateData), sourceFileHandle)

def writeMuxToIndexFunction(msgName, numSignals, numSignalsPerMessage, sourceFileHandle, headerFileHandle):
    templateData = {
        "name": msgName,
        "numSignals": numSignals,
        "numSignalsPerMessage": numSignalsPerMessage,
    }
    fWrite(canTemplater.load("MUX_TO_INDEX_HEADER",templateData), headerFileHandle)
    fWrite(canTemplater.load("MUX_TO_INDEX_SOURCE",templateData), sourceFileHandle)

def writeMultiplexedRxMessages(multiplexedRxMessages, sourceFileHandle, headerFileHandle):
    for msg in multiplexedRxMessages:


        (numSignalsPerMessage, strippedSignalName, numSignals, dataType, sampleSignal) = getMultiplexedMsgInfo(msg)
        templateData = {
            "msgName" : msg.name,
            "dataType": dataType,
            "signalName": strippedSignalName,
            "signalNameUpper": strippedSignalName.upper(),
            "count": numSignals,
        }
        fWrite(canTemplater.load("MULTIPLEX_RX_HEADER",templateData), headerFileHandle)
        fWrite(canTemplater.load("MULTIPLEX_RX_SOURCE",templateData), sourceFileHandle)

        writeSignalReceivedFunction(sampleSignal, sourceFileHandle, variableName=strippedSignalName, multiplexed=True)
        # write the signal receive function for the multiplexer signal. Set multiplexed to false since this signal isn't multiplexed
        multiplexerSignal = [signal for signal in msg.signals if signal.name is sampleSignal.multiplexer_signal][0]
        writeSignalReceivedFunction(multiplexerSignal, sourceFileHandle, multiplexed=False)
        writeSignalVariableAndVariableDeclaration(multiplexerSignal, sourceFileHandle, headerFileHandle)

        writeStructForMsg(msg, msg.name, sourceFileHandle)
        writeMuxToIndexFunction(msg.name, numSignals, numSignalsPerMessage, sourceFileHandle, headerFileHandle)

def writeProCanRxMessages(nodeName, proCanRxMessages, sourceFileHandle, headerFileHandle):
    # For now, just ignore PRO_CAN signals on receive
    for msg in proCanRxMessages:
        fWrite('// Struct and signal receive functions for msg {}'.format(msg.name), sourceFileHandle)
        writeStructForMsg(msg, msg.name, sourceFileHandle)

        for signal in getReceivedSignalsFromMessage(msg, nodeName):
            if not 'PRO_CAN' in signal.name:
                writeSignalVariableAndVariableDeclaration(signal, sourceFileHandle, headerFileHandle)
                writeSignalReceivedFunction(signal, sourceFileHandle)

def writeMessageSendFunction(msg, sourceFileHandle, headerFileHandle, proCAN=False, multiplexed=False, numSignalsPerMessage=0, dtc=False, isChargerMsg=False):
    if multiplexed:
        multiplexIndexString = 'int index'
    else:
        multiplexIndexString = ''

    fWrite('int sendCAN_{name}({multiplexIndexString}) {{'.format(name=msg.name, multiplexIndexString=multiplexIndexString), sourceFileHandle)
    fWrite('int sendCAN_{name}({multiplexIndexString});'.format(name=msg.name, multiplexIndexString=multiplexIndexString), headerFileHandle)

    structInstanceName = 'new_{name}'.format(name=msg.name)
    fWrite('    struct {structName} {instanceName} = {{0}};'.format(structName=msg.name, instanceName=structInstanceName), sourceFileHandle)

    if proCAN:
        fWrite('    {structName}.PRO_CAN_COUNT = {msgName}_PRO_CAN_COUNT++;'.format(structName=structInstanceName, msgName=msg.name), sourceFileHandle)
        fWrite('    {msgName}_PRO_CAN_COUNT = {msgName}_PRO_CAN_COUNT % 16;'.format(msgName=msg.name), sourceFileHandle)
        fWrite('    {structName}.PRO_CAN_CRC = calculate_base_CRC((void *) &{structName})^{msgName}_PRO_CAN_SEED;\n'.format(structName=structInstanceName, msgName=msg.name), sourceFileHandle)

    for signal in msg.signals:
        if multiplexed:
            if signal.is_multiplexer:
                indexToMuxFunctionName = "{msgName}IndexToMuxSelect".format(msgName=msg.name)
                fWrite('    {structName}.{signalName} = {indexToMuxFunction}(index);'.format(structName=structInstanceName, signalName=signal.name, indexToMuxFunction=indexToMuxFunctionName), sourceFileHandle)
            else:
                # Signal will include ALL possible multiplexed signals, so just include up to the number of signals per message
                if numSignalsPerMessage > 0 or signal.multiplexer_signal is None:
                    sendFunctionName = getSignalSendingFunctionName(signal, multiplexed)
                    strippedSignalName = getStrippedSignalName(signal.name)
                    fWrite('    {structName}.{signalName}{signalNumber} = {sendFunction}(index+{offset});'.format(structName=structInstanceName, signalName=strippedSignalName, sendFunction=sendFunctionName, signalNumber=str(numSignalsPerMessage), offset=str(numSignalsPerMessage-1)), sourceFileHandle)
                    numSignalsPerMessage -= 1
        elif proCAN:
            if not 'PRO_CAN' in signal.name:
                sendFunctionName = getSignalSendingFunctionName(signal, multiplexed)
                fWrite('    {structName}.{signalName} = {sendFunction}();'.format(structName=structInstanceName, signalName=signal.name, sendFunction=sendFunctionName), sourceFileHandle)
        else:
            sendFunctionName = getSignalSendingFunctionName(signal, multiplexed)
            fWrite('    {structName}.{signalName} = {sendFunction}();'.format(structName=structInstanceName, signalName=signal.name, sendFunction=sendFunctionName), sourceFileHandle)

    sendFunctionName = ''
    if isChargerMsg:
        sendFunctionName = 'sendCanMessageCharger'
    else:
        sendFunctionName = 'sendCanMessage'

    if dtc:
        fWrite('    return {sendFunctionName}({id}, {len}, (uint8_t *)&{structName});'.format(sendFunctionName=sendFunctionName, id=msg.frame_id, len=msg.length, structName=structInstanceName), sourceFileHandle)
    else:
        fWrite('    return {sendFunctionName}({id}, {len}, (uint8_t *)&{structName});'.format(sendFunctionName=sendFunctionName, id=msg.frame_id, len=msg.length, structName=structInstanceName), sourceFileHandle)
    fWrite('}', sourceFileHandle)

def writeVersionSendFunction(msg, sourceFileHandle, headerFileHandle):
    templateData = {
        "name": msg.name,
        "id": msg.frame_id,
        "len": msg.length,
    }
    fWrite(canTemplater.load("VERSION_SEND_HEADER",templateData), headerFileHandle)
    fWrite(canTemplater.load("VERSION_SEND_SOURCE",templateData), sourceFileHandle)


def writeNormalTxMessages(normalTxMessages, sourceFileHandle, headerFileHandle, chargerMsg=False):
    for msg in normalTxMessages:
        fWrite('// Struct and signal send functions for msg {}'.format(msg.name), sourceFileHandle)
        writeStructForMsg(msg, msg.name, sourceFileHandle)

        if msg.comment == 'VERSION':
            # This is a message to send DBC and git commit, it is special
            writeVersionSendFunction(msg, sourceFileHandle, headerFileHandle)
        else:
            for signal in msg.signals:
                writeSignalVariableAndVariableDeclaration(signal, sourceFileHandle, headerFileHandle)
                writeSignalSendingFunction(signal, sourceFileHandle)
            writeMessageSendFunction(msg, sourceFileHandle, headerFileHandle, isChargerMsg=chargerMsg)

def writeDTCTxMessages(dtcTxMessages, sourceFileHandle, headerFileHandle, chargerMsg=False):
    for msg in dtcTxMessages:
        fWrite('// Struct and signal send functions for msg {}'.format(msg.name), sourceFileHandle)
        writeStructForMsg(msg, msg.name, sourceFileHandle)

        for signal in msg.signals:
            writeSignalVariableAndVariableDeclaration(signal, sourceFileHandle, headerFileHandle)
            writeSignalSendingFunction(signal, sourceFileHandle)
        writeMessageSendFunction(msg, sourceFileHandle, headerFileHandle, dtc=True, isChargerMsg=chargerMsg)

def writeMultiplexedTxMessages(multiplexedTxMessages, sourceFileHandle, headerFileHandle, chargerMsg=False):
    for msg in multiplexedTxMessages:
        fWrite('// Struct and signal receive functions for multiplexed msg {}'.format(msg.name), sourceFileHandle)

        (numSignalsPerMessage, strippedSignalName, numSignals, dataType, sampleSignal) = getMultiplexedMsgInfo(msg)

        fWrite('volatile {dataType} {name}[{count}];'.format(dataType=dataType, name=strippedSignalName, count=numSignals), sourceFileHandle)
        fWrite('#define {name}_COUNT ({count})'.format(name=strippedSignalName.upper(), count=numSignals), headerFileHandle)
        fWrite('extern volatile {dataType} {name}[{count}];\n'.format(dataType=dataType, name=strippedSignalName, count=numSignals), headerFileHandle)

        writeSignalSendingFunction(sampleSignal, sourceFileHandle, variableName=strippedSignalName, multiplexed=True)
        writeStructForMsg(msg, msg.name, sourceFileHandle)
        writeIndexToMuxFunction(msg.name, numSignalsPerMessage, sourceFileHandle, headerFileHandle)
        writeMessageSendFunction(msg, sourceFileHandle, headerFileHandle, multiplexed=True, numSignalsPerMessage=numSignalsPerMessage, isChargerMsg=chargerMsg)

def writeProCanSpecialVariables(msg, sourceFileHandle):
    fWrite('int {msgName}_PRO_CAN_SEED = 0;'.format(msgName=msg.name), sourceFileHandle)
    fWrite('int {msgName}_PRO_CAN_COUNT = 0;'.format(msgName=msg.name), sourceFileHandle)

def writeProCANTxMessages(proCanTxMessages, sourceFileHandle, headerFileHandle, chargerMsg=False):
    for msg in proCanTxMessages:
        fWrite('// Struct and signal send functions for msg {}'.format(msg.name), sourceFileHandle)
        writeStructForMsg(msg, msg.name, sourceFileHandle)

        writeProCanSpecialVariables(msg, sourceFileHandle)

        for signal in msg.signals:
            if not 'PRO_CAN' in signal.name:
                writeSignalVariableAndVariableDeclaration(signal, sourceFileHandle, headerFileHandle)
                writeSignalSendingFunction(signal, sourceFileHandle)
        writeMessageSendFunction(msg, sourceFileHandle, headerFileHandle, proCAN=True, isChargerMsg=chargerMsg)


def writeParseCanRxMessageFunction(nodeName, normalRxMessages, dtcRxMessages, multiplexedRxMessages, proCanRxMessages, heartbeatRxMessages, sourceFileHandle, headerFileHandle, isChargerDBC=False):
    msgCallbackPrototypes = []
    if isChargerDBC:
        functionPrototype = 'int parseChargerCANData(int id, void *data)'
    else:
        functionPrototype = 'int parseCANData(int id, void *data)'

    fWrite('{};'.format(functionPrototype), headerFileHandle)
    fWrite('{} {{'.format(functionPrototype), sourceFileHandle)
    fWrite('    switch (id) {', sourceFileHandle)

    for msg in normalRxMessages:
        fWrite('        case {id}:'.format(id=msg.frame_id), sourceFileHandle)
        fWrite('        {', sourceFileHandle)
        fWrite('            struct {structName} *in_{structName} = data;'.format(structName=msg.name), sourceFileHandle)
        for signal in getReceivedSignalsFromMessage(msg, nodeName):
            fWrite('            {signalName}Received(in_{structName}->{signalName});'.format(signalName=signal.name, structName=msg.name), sourceFileHandle)

        callbackName = 'CAN_Msg_{msgName}_Callback'.format(msgName=msg.name)
        msgCallbackPrototypes.append('void {callback}()'.format(callback=callbackName))
        fWrite('            {callback}();'.format(callback=callbackName), sourceFileHandle)
        fWrite('            break;\n        }', sourceFileHandle)
    for msg in heartbeatRxMessages:
        fWrite('        case {id}:'.format(id=msg.frame_id), sourceFileHandle)
        fWrite('        {', sourceFileHandle)
        txNode = msg.senders[0]
        fWrite('            heartbeatReceived(ID_{txNode});'.format(txNode=txNode), sourceFileHandle)
        callbackName = 'CAN_Msg_{msgName}_Callback'.format(msgName=msg.name)
        fWrite('            {callback}();'.format(callback=callbackName), sourceFileHandle)
        msgCallbackPrototypes.append('void {callback}()'.format(callback=callbackName))
        fWrite('            break;\n        }', sourceFileHandle)

    createdFatalCallback = False
    for msg in dtcRxMessages:
        fWrite('        case {id}:'.format(id=msg.frame_id), sourceFileHandle)
        fWrite('        {', sourceFileHandle)
        fWrite('            struct {structName} *in_{structName} = data;'.format(structName=msg.name), sourceFileHandle)
        fWrite('            struct {structName}_unpacked newDtc;'.format(structName=msg.name), sourceFileHandle)
        for signal in getReceivedSignalsFromMessage(msg, nodeName):
            fWrite('            newDtc.{signalName} = {signalName}Received(in_{structName}->{signalName});'.format(signalName=signal.name, structName=msg.name), sourceFileHandle)

        fWrite('            DEBUG_PRINT_ISR("DTC ({name}). Code %d, Severity %d, Data %d\\n", newDtc.DTC_CODE, newDtc.DTC_Severity, newDtc.DTC_Data);'.format(name=msg.name), sourceFileHandle)  
        callbackName = 'CAN_Msg_{msgName}_Callback'.format(msgName=msg.name)

        fatalCallbackName = 'DTC_Fatal_Callback'
        fatalCallbackPrototype = 'void {name}(BoardIDs board)'.format(name=fatalCallbackName)

        msgCallbackPrototypes.append('void {callback}(int DTC_CODE, int DTC_Severity, int DTC_Data)'.format(callback=callbackName))

        # only create one fatal callback
        if not createdFatalCallback:
            msgCallbackPrototypes.append('{callback}'.format(callback=fatalCallbackPrototype))
            createdFatalCallback = True

        fWrite('            if (newDtc.DTC_Severity == DTC_Severity_FATAL) {', sourceFileHandle)
        fWrite('                {fatalCallback}(ID_{nodeName});\n            }}'.format(fatalCallback=fatalCallbackName, nodeName=nodeName.upper()), sourceFileHandle)
        fWrite('            {callback}(newDtc.DTC_CODE, newDtc.DTC_Severity, newDtc.DTC_Data);'.format(callback=callbackName), sourceFileHandle)
        fWrite('            break;\n        }', sourceFileHandle)

    for msg in proCanRxMessages:
        fWrite('        case {id}:'.format(id=msg.frame_id), sourceFileHandle)
        fWrite('        {', sourceFileHandle)
        fWrite('            struct {structName} *in_{structName} = data;'.format(structName=msg.name), sourceFileHandle)
        for signal in getReceivedSignalsFromMessage(msg, nodeName):
            if not 'PRO_CAN' in signal.name:
                fWrite('            {signalName}Received(in_{structName}->{signalName});'.format(signalName=signal.name, structName=msg.name), sourceFileHandle)

        callbackName = 'CAN_Msg_{msgName}_Callback'.format(msgName=msg.name)
        msgCallbackPrototypes.append('void {callback}()'.format(callback=callbackName))
        fWrite('            {callback}();'.format(callback=callbackName), sourceFileHandle)
        fWrite('            break;\n        }', sourceFileHandle)

    for msg in multiplexedRxMessages:
        (numSignalsPerMessage, strippedSignalName, numSignals, dataType, sampleSignal) = getMultiplexedMsgInfo(msg)
        fWrite('        case {id}:'.format(id=msg.frame_id), sourceFileHandle)
        fWrite('        {', sourceFileHandle)
        fWrite('            struct {structName} *in_{structName} = data;'.format(structName=msg.name), sourceFileHandle)

        muxToIndexFunction = '{name}MuxSelectToIndex'.format(name=msg.name)
        signal = msg.signals[0]
        muxSignalName = ''
        if signal.is_multiplexer:
            muxSignalName = signal.name
        else:
            muxSignalName = signal.multiplexer_signal.name

        for signal in getReceivedSignalsFromMessage(msg, nodeName):
            if signal.is_multiplexer:
                fWrite('            {signalName}Received(in_{structName}->{signalName});'.format(signalName=signal.name, structName=msg.name), sourceFileHandle)
            else:
                # Signal will include ALL possible multiplexed signals, so just include up to the number of signals per message
                if numSignalsPerMessage > 0 or signal.multiplexer_signal is None:
                    strippedSignalName = getStrippedSignalName(signal.name)
                    fWrite('            {signalName}Received({mToI}(in_{structName}->{multiplexerName}, {signalNum}), in_{structName}->{signalName}{signalNum});'.format(signalName=strippedSignalName, structName=msg.name, mToI=muxToIndexFunction, multiplexerName=muxSignalName, signalNum=numSignalsPerMessage), sourceFileHandle)
                    numSignalsPerMessage -= 1

        callbackName = 'CAN_Msg_{msgName}_Callback'.format(msgName=msg.name)
        msgCallbackPrototypes.append('void {callback}(int baseIndex, int signalsInMessage)'.format(callback=callbackName))
        fWrite('            {callback}({mToI}(in_{structName}->{multiplexerName}, 0), {numSignalsPerMessage});'.format(callback=callbackName, mToI=muxToIndexFunction, structName=msg.name, multiplexerName=muxSignalName, numSignalsPerMessage=numSignalsPerMessage), sourceFileHandle)
        fWrite('            break;\n        }', sourceFileHandle)

    fWrite("""
        default:
        {
            // Ignore unkown messages
            break;
        }
    }

    return 0;
}""", sourceFileHandle)

    return msgCallbackPrototypes

def writeSetupCanFilters(boardType, messageGroups, sourceFileHandle, headerFileHandle, functionName='configCANFilters', isChargerDBC=False):
    templateHolder = ""
    i = 1
    for messageGroup in messageGroups:
        i = i + 1
        templateData = {
            "msgGrp": messageGroup,
            "msgGrpNmbr": i,
        }
        templateHolder = templateHolder + canTemplater.load("SETUP_CAN_FILTERS_PARTIAL",templateData)

    finalTemplateData = {
        "filterID": '0x5000;' if isChargerDBC else '0xFF<<8;',
        "functionName": functionName,
        "extraMessageTemplate": templateHolder,
    }
    fWrite(canTemplater.load("SETUP_CAN_FILTERS_HEADER",finalTemplateData), headerFileHandle)
    fWrite(canTemplater.load("SETUP_CAN_FILTERS_SOURCE",finalTemplateData), sourceFileHandle)

def writeInitFunction(sourceFileHandle, headerFileHandle):
    prototype = 'int init_can_driver()'
    fWrite('{prototype};'.format(prototype=prototype), headerFileHandle)
    fWrite('{prototype} {{'.format(prototype=prototype), sourceFileHandle)
    fWrite('    return HAL_OK;', sourceFileHandle)
    fWrite('}', sourceFileHandle)

def writeMsgCallbacks(msgCallbackPrototypes, sourceFileHandle, headerFileHandle):
    for prototype in msgCallbackPrototypes:
        fWrite('{};\n'.format(prototype), headerFileHandle)
        fWrite('__weak {} {{}}\n'.format(prototype), sourceFileHandle)

def generateCANHeaderFromDB(dbFile, headerFileName, sourceFileName, nodeName, boardType, isChargerDBC=False):
    db = cantools.db.load_file(dbFile)

    gitCommit = getGitCommit()

    headerFileHandle = open(headerFileName, "w+")
    sourceFileHandle = open(sourceFileName, "w+")

    writeHeaderFileIncludeGuardAndIncludes(boardType, headerFileHandle, nodeName, isChargerDBC=isChargerDBC)

    if isChargerDBC:
        fWrite('#undef CAN_NODE_ADDRESS\n', sourceFileHandle)
        fWrite('#define CAN_NODE_ADDRESS 0xF4\n', sourceFileHandle)

    if (not isChargerDBC):
        messageGroups = getNodeAddressAndMessageGroupsAndWriteToHeaderFile(db, nodeName, headerFileHandle)
    else:
        messageGroups = []

    writeSourceFileIncludes(nodeName, sourceFileHandle, isChargerDBC=isChargerDBC)
    writeDBCVersionAndGitCommitToSourceFile(gitCommit, db, sourceFileHandle, isChargerDBC=isChargerDBC)


    (rxMessages, txMessages, normalRxMessages, normalTxMessages, multiplexedRxMessages,
     multiplexedTxMessages, dtcRxMessages, dtcTxMessages, proCanRxMessages, proCanTxMessages, heartbeatRxMessages) = parseCanDB(db, nodeName)

    # print normalRxMessages
    writeNormalRxMessages(nodeName, normalRxMessages, sourceFileHandle, headerFileHandle)

    if (not isChargerDBC):
        # print dtcRxMessages
        writeDTCRxMessages(nodeName, dtcRxMessages, sourceFileHandle, headerFileHandle)

        # print multiplexed Rx Messages
        writeMultiplexedRxMessages(multiplexedRxMessages, sourceFileHandle, headerFileHandle)

    # print normal Tx Messages
    writeNormalTxMessages(normalTxMessages, sourceFileHandle, headerFileHandle, chargerMsg=isChargerDBC)
    if (not isChargerDBC):
        # print dtc Tx Messages
        writeDTCTxMessages(dtcTxMessages, sourceFileHandle, headerFileHandle)

        # print multiplexed Tx Messages
        writeMultiplexedTxMessages(multiplexedTxMessages, sourceFileHandle, headerFileHandle)

        writeProCanRxMessages(nodeName, proCanRxMessages, sourceFileHandle, headerFileHandle)

        writeProCANTxMessages(proCanTxMessages, sourceFileHandle, headerFileHandle)

    # print parse can message function
    msgCallbackPrototypes = writeParseCanRxMessageFunction(nodeName, normalRxMessages, dtcRxMessages, multiplexedRxMessages, proCanRxMessages, heartbeatRxMessages, sourceFileHandle, headerFileHandle, isChargerDBC=isChargerDBC)

    if isChargerDBC:
        writeSetupCanFilters(boardType, messageGroups, sourceFileHandle, headerFileHandle, functionName='configCANFiltersCharger', isChargerDBC=True)
    else:
        writeSetupCanFilters(boardType, messageGroups, sourceFileHandle, headerFileHandle)

    if not isChargerDBC:
        writeInitFunction(sourceFileHandle, headerFileHandle)

    writeMsgCallbacks(msgCallbackPrototypes, sourceFileHandle, headerFileHandle)

    writeEndIncludeGuard(sourceFileHandle, headerFileHandle)

    headerFileHandle.close()
    sourceFileHandle.close()

def main(argv):
    if argv and len(argv) == 2:
        nodeName = argv[0]
        boardType = argv[1] # F0 or F7
    else:
        print('Error: no target specified or no boardtype specified')
        sys.exit(1)

    print('Generating CAN source and header files for Board: {nodeName}, Type: {boardType}'.format(nodeName=nodeName, boardType=boardType))

    if not (boardType == 'F0' or boardType == 'F7'):
        print("ERROR: Specifiy either F0 or F7 for boardtype")
        sys.exit(1)

    commonDir = 'common'
    genDir = 'Gen'

    genIncDir = os.path.join(genDir, nodeName, 'Inc')
    genSrcDir = os.path.join(genDir, nodeName, 'Src')
    create_dir(genIncDir) # Create genIncDir if it doesn't already exist
    create_dir(genSrcDir) # Create genSrcDir if it doesn't already exist

    ScriptsDir = os.path.join(commonDir, 'Scripts')

    depFile = os.path.join(genDir, 'canGen.d')

    dataDir = os.path.join(commonDir, 'Data')
    mainDbFile = os.path.join(dataDir, '2018CAR.dbc')

    headerFile = os.path.join(genIncDir, nodeName + '_can.h')
    sourceFile = os.path.join(genSrcDir, nodeName + '_can.c')

    generateCANHeaderFromDB(mainDbFile, headerFile, sourceFile, nodeName, boardType)

    if (nodeName == 'bmu'):
        chargerDbFile = os.path.join(dataDir, 'ChargerMessages.dbc')

        chargerHeaderFile = os.path.join(genIncDir, nodeName + '_charger_can.h')
        chargerSourceFile = os.path.join(genSrcDir, nodeName + '_charger_can.c')

        generateCANHeaderFromDB(chargerDbFile, chargerHeaderFile, chargerSourceFile, nodeName, boardType, isChargerDBC=True)


class TestObj(dict):
    def __getattr__(self,name):
        return self[name]

def test():
    #Test generate templates
    headerFile = open('templates/outputs/headerFileIncludeGuard.txt', "w+")
    writeHeaderFileIncludeGuardAndIncludes('F7',headerFile,'PDU',True)

    headerFile = open('templates/outputs/headerFileEnd.txt', "w+")
    sourceFile = open('templates/outputs/sourceFileEnd.txt', "w+")
    writeEndIncludeGuard(sourceFile, headerFile)

    sourceFile = open('templates/outputs/sourceFileInclude.txt','w+')
    writeSourceFileIncludes("PDU",sourceFile,True)


    sourceFile = open('templates/outputs/sourceDBCVersionGit.txt','w+')
    db = TestObj(version='4')
    writeDBCVersionAndGitCommitToSourceFile("fd3414",db,sourceFile, False)

    sourceFile = open('templates/outputs/sourceFunctionRecieved.txt','w+')
    signal = TestObj(is_signed=False, name="Ping",scale=2,offset=10)
    writeSignalReceivedFunction(signal,sourceFile,'ping',False,True)

    sourceFile = open('templates/outputs/sourceFunctionSending.txt','w+')
    signal = TestObj(is_signed=False, name="Ping",scale=2,offset=10)
    writeSignalSendingFunction(signal,sourceFile,'',False)

    sourceFile = open('templates/outputs/sourceSignalVarDecl.txt','w+')
    headerFile = open('templates/outputs/headerSignalVarDecl.txt','w+')
    signal = TestObj(is_signed=False, name="Ping",scale=2,offset=10,choices=None)
    writeSignalVariableAndVariableDeclaration(signal,sourceFile,headerFile)

    sourceFile = open('templates/outputs/sourceIndexToMux.txt','w+')
    headerFile = open('templates/outputs/headerIndexToMux.txt','w+')
    writeIndexToMuxFunction("myMsg",23,sourceFile,headerFile)

    sourceFile = open('templates/outputs/sourceMuxToIndex.txt','w+')
    headerFile = open('templates/outputs/headerMuxToIndex.txt','w+')
    writeMuxToIndexFunction("myMsg",3,7,sourceFile,headerFile)

    #Not tested rn as it needs access to CAN DB
    #writeMultiplexedRxMessages()
    
    message = TestObj(name="Ping",frame_id='5',length=24)
    sourceFile = open('templates/outputs/sourceSendVersion.txt','w+')
    headerFile = open('templates/outputs/headerSendVersion.txt','w+')
    writeVersionSendFunction(message,sourceFile,headerFile)

    sourceFile = open('templates/outputs/sourceCANFilters.txt','w+')
    headerFile = open('templates/outputs/headerCANFilters.txt','w+')
    messageGroups = [
        "0xFFFF",
        "0x0000",
        "0xF0F1"
    ]
    writeSetupCanFilters("F7",messageGroups,sourceFile,headerFile)
if __name__ == '__main__':
    main(sys.argv[1:])
    #test()
