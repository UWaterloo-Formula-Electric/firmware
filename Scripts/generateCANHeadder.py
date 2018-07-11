#!/usr/bin/env python
import logging, sys
logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

import cantools #$ If you get a failure on this line, you need to install the python package cantools: install pip and run 'pip install -r requirements.txt' (you may want to do this in a virtual env)
import os
from pprint import pprint
import subprocess
import sys
import errno
import re

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

if sys.argv and len(sys.argv) == 3:
    nodeName = sys.argv[1]
    boardType = sys.argv[2] # F0 or F7
else:
    print('Error: no target specified or no boardtype specified')
    sys.exit(1)

if not (boardType == 'F0' or boardType == 'F7'):
    print("Specifiy either F0 or F7 for boardtype")
    sys.exit(1)

commonDir = 'common-all'
genDir = 'Gen'

genIncDir = os.path.join(genDir, 'Inc')
genSrcDir = os.path.join(genDir, 'Src')
create_dir(genIncDir) # Create genIncDir if it doesn't already exist
create_dir(genSrcDir) # Create genSrcDir if it doesn't already exist

ScriptsDir = os.path.join(commonDir, 'Scripts')

depFile = os.path.join(genDir, 'canGen.d')

dataDir = os.path.join(commonDir, 'Data')
dbFile = os.path.join(dataDir, '2018CAR.dbc')

headerFile = os.path.join(genIncDir, nodeName + '_can.h')
sourceFile = os.path.join(genSrcDir, nodeName + '_can.c')

db = cantools.db.load_file(dbFile)

label = '0df2a9a'
gitClean = '0'

try:
    if call(["git", "branch"], stderr=STDOUT, stdout=open(os.devnull, 'w')) != 0:
        label = subprocess.check_output(["git", "describe" ,"--tags", "--always"]).strip()
except Exception:
    pass

gitCommit = label

def fWrite(string, fileHandle):
    fileHandle.write(string + '\n')

def generateDepedencyFile(headerFile, target):
    with open(depFile, 'w+') as depFileHandle:
        fWrite('{headerFile}: {dir}/generateCANHeadder.py {dbFile}'.format(headerFile=headerFile, dbFile=dbFile, dir=ScriptsDir), depFileHandle)
        fWrite('	{dir}/generateCANHeadder.py {target} {boardType}'.format(target=target, boardType=boardType, dir=ScriptsDir), depFileHandle)


headerFileHandle = open(headerFile, "w+")
sourceFileHandle = open(sourceFile, "w+")

#make .h file

fWrite("#ifndef __"+nodeName+"_can_H\n#define __"+nodeName+"_can_H\n", headerFileHandle);
if boardType == 'F7':
    fWrite("#include \"stm32f7xx_hal.h\"", headerFileHandle);
else:
    fWrite("#include \"stm32f0xx_hal.h\"", headerFileHandle);

fWrite("#include \"FreeRTOS.h\"", headerFileHandle);
fWrite("#include \"queue.h\"", headerFileHandle);

fWrite('//Message Filtering', headerFileHandle);
nodeAddress = 0
messageGroups = list()

for node in db.nodes:
    if node.name == nodeName:
        nodeAddress = node.comment.split("_")[0]
        fWrite('#define CAN_NODE_ADDRESS ' + nodeAddress, headerFileHandle);
        for messageGroup in node.comment.split("_")[1].split(","):
            if messageGroup != "":
                fWrite('#define CAN_NODE_MESSAGE_GROUP_'+messageGroup, headerFileHandle);
                messageGroups.append(messageGroup)
fWrite('', headerFileHandle);

fWrite('//DBC version:', sourceFileHandle)
fWrite('int DBCVersion = '+db.version+';', sourceFileHandle)
fWrite('char gitCommit[] = \"'+gitCommit+'\";', sourceFileHandle)

fWrite('//'+nodeName+' can headder', sourceFileHandle)
fWrite("#include \""+nodeName+"_can.h\"", sourceFileHandle)
fWrite('#include \"CRC_CALC.h\"', sourceFileHandle)
fWrite('#include \"userCan.h\"', sourceFileHandle)

nodeList = list()

rxVariables = list()
rxVariableArrays = {}
rxMessages = list()
txVariables = list()
txVariableArrays = {}
txMessages = list()

variablesPROCAN = list()
variablesPROCANHeader = list()

for mes in db.messages:
    messageUseful = 0
    for signal in mes.signals:
        if nodeName in signal.receivers:
            messageUseful = 1
            # checks if more than one of this receive signal exists
            if re.match('.+\d+$', signal.name):
                # gets name without numbered suffix
                name = re.sub('\d+$', '', signal.name)
                # checks if this signal is multiplexed
                if not signal.multiplexer_signal is None:
                    if name in rxVariableArrays:
                        # if receive signal belongs to some signal array in the set of signal arrays, increment the recorded size of this array
                        signalRef = rxVariableArrays[name]['signal']
                        if signalRef.offset == signal.offset and signalRef.scale == signal.scale and signalRef.multiplexer_signal == signal.multiplexer_signal: 
                            rxVariableArrays[name]['count'] += 1
                    else:
                        # make a new entry to set of receive signal arrays
                        rxVariableArrays[name] = {'signal': signal, 'count': 1}
            else:
                rxVariables.append(signal)
    if messageUseful == 1:
        rxMessages.append(mes)


fWrite('#pragma pack(push)', sourceFileHandle)
fWrite('#pragma pack(1)', sourceFileHandle)
fWrite('// Incoming variables', sourceFileHandle)

# Treat DTC signal/msgs differently
dtcQueueSize = 5
queueInitStrings = list()
for message in rxMessages:
    if 'DTC' in message.name:
        fWrite('typedef struct ' + message.name + '_unpacked {', headerFileHandle)
        for signal in message.signals:
            fWrite('int ' + signal.name + ';', headerFileHandle)
        fWrite('} ' + message.name + '_unpacked;', headerFileHandle)

        queueName = 'queue' + message.name
        fWrite('extern QueueHandle_t ' + queueName + ';', headerFileHandle)
        fWrite('QueueHandle_t ' + queueName + ';', sourceFileHandle)
        queueInitStrings.append(queueName + ' = xQueueCreate(' +str(dtcQueueSize) + ', sizeof(' + message.name + '_unpacked)); if (' + queueName + '== NULL) return HAL_ERROR;')
        for signal in message.signals:
            fWrite('int ' + signal.name + 'Received(int64_t newValue)\n{', sourceFileHandle)
            fWrite("	newValue = newValue * "+str(signal.scale)+";", sourceFileHandle)
            fWrite("	newValue = newValue + "+str(signal.offset)+";", sourceFileHandle)
            fWrite("	return newValue;", sourceFileHandle)
            fWrite("}\n", sourceFileHandle)

# Everything else
for signal in rxVariables:
    if not 'DTC' in signal.name:
        type = "float "
        fWrite('volatile '+ type + signal.name + ';	// offset: ' + str(signal.offset)+ " scaler: "+ str(signal.scale), sourceFileHandle)
        fWrite('extern volatile '+ type + signal.name + ';	// offset: ' + str(signal.offset)+ " scaler: "+ str(signal.scale), headerFileHandle)
        fWrite('void '+ signal.name+'Received(int64_t newValue)\n{', sourceFileHandle)
        fWrite("	float floatValue = (float)newValue * "+str(signal.scale)+";", sourceFileHandle)
        fWrite("	floatValue = floatValue + "+str(signal.offset)+";", sourceFileHandle)
        fWrite("	"+signal.name + " = floatValue;", sourceFileHandle)
        fWrite("}\n", sourceFileHandle)

for name, signal in rxVariableArrays.items():
    type = "float "
    signalRef = signal['signal']
    if not 'DTC' in signalRef.name:
        # defines signal arrays
        fWrite('volatile '+ type + name + '[' + str(signal['count']) + '];	// offset: ' + str(signalRef.offset)+ " scaler: "+ str(signalRef.scale), sourceFileHandle)
        fWrite('extern volatile '+ type + name + '[' + str(signal['count']) + '];	// offset: ' + str(signalRef.offset)+ " scaler: "+ str(signalRef.scale), headerFileHandle)
        # defines the size of this signal array 
        fWrite('const int ' + name.upper() + '_COUNT = ' + str(signal['count']) + ';', sourceFileHandle)
        # function for receiving a signal to put in some index of the array
        fWrite('extern const int ' + name.upper() + '_COUNT;', headerFileHandle)
        fWrite('void '+ name + 'Received(int index, int64_t newValue)\n{', sourceFileHandle)
        fWrite("	float floatValue = (float)newValue * " + str(signalRef.scale) + ";", sourceFileHandle)
        fWrite("	floatValue = floatValue + " + str(signalRef.offset) + ";", sourceFileHandle)
        fWrite("	" + name + "[index] = floatValue;", sourceFileHandle)
        fWrite("}\n", sourceFileHandle)

fWrite('// Outgoing variables', sourceFileHandle)
for mes in db.messages:
    messageUseful = 0
    if nodeName in mes.senders:
        messageUseful = 1
        if mes.comment == 'PROCAN':
            variablesPROCAN.append('int '+mes.name+'_PRO_CAN_SEED = 127;');
            variablesPROCAN.append('int '+mes.name+'_PRO_CAN_COUNT = 0;');
            variablesPROCANHeader.append('int '+mes.name+'_PRO_CAN_SEED;');
            variablesPROCANHeader.append('int '+mes.name+'_PRO_CAN_COUNT;');
        else:
            for signal in mes.signals:
                if signal.comment != "PROCAN" and not signal.is_multiplexer:
                    # checks if more than one of this signal exists (denoted by its numbered suffix)
                    if re.match('.+\d+$', signal.name):
                        # remove the numbered suffix
                        name = re.sub('\d+$', '', signal.name)
                        if not signal.multiplexer_signal is None:
                            if name in txVariableArrays:
                                # if transmit signal exists in some signal array in a set of signal arrays, increment the size counter for that array
                                signalRef = txVariableArrays[name]['signal']
                                if signalRef.offset == signal.offset and signalRef.scale == signal.scale and signalRef.multiplexer_signal == signal.multiplexer_signal: 
                                    txVariableArrays[name]['count'] += 1
                            else:
                                # make a new entry for the set of transmit signal arrays
                                txVariableArrays[name] = {'signal': signal, 'count': 1}
                    else:
                        txVariables.append(signal)
    
    if messageUseful == 1:
        txMessages.append(mes)

for signal in txVariables:
    type = "int64_t "
    fWrite('extern volatile float ' + signal.name + ';	// offset: ' + str(signal.offset)+ " scaler: "+ str(signal.scale), headerFileHandle)
    fWrite('volatile float ' + signal.name + ';	// offset: ' + str(signal.offset)+ " scaler: "+ str(signal.scale), sourceFileHandle)
    fWrite('__weak '+type+ signal.name+'Sending()\n{', sourceFileHandle)
    fWrite('	float sendValue = '+ signal.name+';', sourceFileHandle)
    fWrite("	sendValue = sendValue - "+str(signal.offset)+";", sourceFileHandle)
    fWrite("	sendValue = sendValue / "+str(signal.scale)+";", sourceFileHandle)
    fWrite("	return sendValue;", sourceFileHandle)
    fWrite("}\n", sourceFileHandle)

for name, signal in txVariableArrays.items():
    type = "int64_t "
    signalRef = signal['signal']
    # defines signal array
    fWrite('extern volatile float ' + name + '[' + str(signal['count']) + '];	// offset: ' + str(signalRef.offset)+ " scaler: "+ str(signalRef.scale), headerFileHandle)
    fWrite('volatile float ' + name + '[' + str(signal['count']) + '];	// offset: ' + str(signalRef.offset)+ " scaler: "+ str(signalRef.scale), sourceFileHandle)
    # defines size of signal array
    fWrite('const int ' + name.upper() + '_COUNT = ' + str(signal['count']) + ';', sourceFileHandle)
    fWrite('extern const int ' + name.upper() + '_COUNT;', headerFileHandle)
    # function for tranmitting some signal in this signal array
    fWrite('__weak '+ type + name + 'Sending(int index)\n{', sourceFileHandle)
    fWrite('	float sendValue = ' + name + '[index];', sourceFileHandle)
    fWrite("	sendValue = sendValue - "+str(signalRef.offset)+";", sourceFileHandle)
    fWrite("	sendValue = sendValue / "+str(signalRef.scale)+";", sourceFileHandle)
    fWrite("	return sendValue;", sourceFileHandle)
    fWrite("}\n", sourceFileHandle)

fWrite('', sourceFileHandle)
fWrite('// PRO_CAN ', sourceFileHandle)
for string in variablesPROCAN:
    fWrite(string, sourceFileHandle)
for string in variablesPROCANHeader:
    fWrite('extern ' + string, headerFileHandle)
fWrite('', sourceFileHandle)

fWrite('int init_can_driver();', headerFileHandle)
fWrite('int init_can_driver(){', sourceFileHandle)
for string in queueInitStrings:
    fWrite('	' + string, sourceFileHandle)
fWrite('	generate_CRC_lookup_table();', sourceFileHandle)
fWrite('	return HAL_OK;', sourceFileHandle)
fWrite('}', sourceFileHandle)
fWrite('', sourceFileHandle)

for message in txMessages:
    fWrite('struct ' + message.name + '{', sourceFileHandle)
    totalSize = message.length*8
    currentPos = 0
    if message.comment == 'VERSION':
        fWrite('	int DBC : 8;', sourceFileHandle)
        for i  in range(0,7):
            fWrite('	char git' + str(i) + ' : 8;', sourceFileHandle)
    else:
        count = 1
        startBits = list()
        for signal in message.signals:
            # checks if signal does not overlap another signal in this message
            if not signal.start in startBits:
                startBits.append(signal.start)
                if signal.start != currentPos:
                    fWrite('	uint64_t FILLER_'+ str(signal.start) + ' : ' + str(signal.start - currentPos) + ';', sourceFileHandle)
                
                # denotes a multiplexer signal in this message by suffixing the signal with select
                if signal.is_multiplexer:
                    signalName = signal.name + 'Select'
                # denotes a signal from some signal array that exists in this message
                elif re.match('.+\d+$', signal.name):
                    signalName = re.sub('\d+$', '', signal.name) + str(count)
                    count += 1
                # denotes a normal signal in this message
                else:
                    signalName = signal.name
                    count = 1
                
                if signal.is_signed	:
                    fWrite('	         int64_t ' + signalName + ' : ' + str(signal.length) + ';', sourceFileHandle)
                else :
                    fWrite('	uint64_t ' + signalName + ' : ' + str(signal.length) + ';', sourceFileHandle)
                currentPos = signal.start + signal.length

        if currentPos != totalSize:
            fWrite('	uint64_t FILLER_END : ' + str(totalSize - currentPos) + ';', sourceFileHandle)
    fWrite('};', sourceFileHandle)
    fWrite('', sourceFileHandle)
fWrite('', sourceFileHandle)

for message in rxMessages:
    fWrite('struct ' + message.name + ' {', sourceFileHandle)
    totalSize = message.length*8;
    currentPos = 0;
    count = 1
    startBits = list()        
    for signal in message.signals:
        # checks if signal does not overlap another signal in this message
        if not signal.start in startBits:
            startBits.append(signal.start)
            if signal.start != currentPos:
                fWrite('	uint64_t FILLER_'+ str(signal.start) + ' : ' + str(signal.start - currentPos) + ';', sourceFileHandle)
            
            # denotes a multiplexer signal in this message by suffixing the signal with select
            if signal.is_multiplexer:
                signalName = signal.name + 'Select'
            # denotes a signal from some signal array that exists in this message
            elif re.match('.+\d+$', signal.name):
                signalName = re.sub('\d+$', '', signal.name) + str(count)
                count += 1
            # denotes a normal signal in this message
            else:
                signalName = signal.name
                count = 1

            if signal.is_signed	:
                fWrite('	         int64_t ' + signalName + ' : ' + str(signal.length) + ';', sourceFileHandle)
            else :
                fWrite('	uint64_t ' + signalName + ' : ' + str(signal.length) + ';', sourceFileHandle)
            currentPos = signal.start + signal.length
    if currentPos != totalSize:
        fWrite('	uint64_t FILLER_END : ' + str(totalSize - currentPos) + ';', sourceFileHandle)
    fWrite('};', sourceFileHandle)
    fWrite('', sourceFileHandle)


fWrite('// Message Received callbacks, declared with weak linkage to be overwritten by user functions', sourceFileHandle)

for message in rxMessages:
    fWrite('__weak void CAN_Msg_' + str(message.name) + '_Callback(void)\n{ return; }', sourceFileHandle)
    fWrite('', sourceFileHandle)

fWrite('int parseCANData(int id, void * data);', headerFileHandle)
fWrite('int parseCANData(int id, void * data) {', sourceFileHandle)
fWrite('	switch(id) {', sourceFileHandle)

for message in rxMessages:
    fWrite('		case '+ str(message.frame_id) + ' : // '+str(message.name), sourceFileHandle)
    # for signal in message.signals:
    fWrite('		{', sourceFileHandle)

    fWrite('			struct ' + message.name + ' *in_'+message.name +' = data;', sourceFileHandle)
    if 'DTC' in message.name:
        unpackedStructName = str(message.name + '_unpacked')
        unpackedStructInstance = str('new_' + message.name)
        fWrite('			' + unpackedStructName + ' ' + unpackedStructInstance + ';', sourceFileHandle)
        for signal in message.signals:
            fWrite('			' + unpackedStructInstance + '.' + signal.name + ' = ' +signal.name+ 'Received(in_'+message.name +'->'+ signal.name+');', sourceFileHandle)

        fWrite('			xQueueSendFromISR(queue' + message.name + ', &' + unpackedStructInstance + ', NULL);', sourceFileHandle)
    else:
        startBits = list()
        signals = list()
        signalNames = list()
        signalsPerMessage = {}

        # gets signals to be sent by a message        
        for signal in message.signals:
            # checks if signal does not overlap with other signals in the message
            if nodeName in signal.receivers and not signal.start in startBits:
                startBits.append(signal.start)
 
                # gets signal arrays and records number of these signals per message
                if re.sub('\d+$', '', signal.name) in rxVariableArrays:
                    signalName = re.sub('\d+$', '', signal.name)
                    if not signalName in signalsPerMessage:
                        signalsPerMessage[signalName] = 1
                    else:
                        signalsPerMessage[signalName] += 1
                    if not signalName in signalNames:
                        signals.append(signal)
                        signalNames.append(signalName)
                else:
                    signals.append(signal)
        
        for signal in signals:
            # determine signal name
            if re.match('.+\d+$', signal.name):
                signalName = re.sub('\d+$', '', signal.name)
            else:
                signalName = signal.name

            # determine how to receive signal based on whether it was multiplexed or not
            if signalName in rxVariableArrays:
                for i in range(signalsPerMessage[signalName]):
                    fWrite('			' + signalName + 'Received(in_' + message.name + '->' + signal.multiplexer_signal + 'Select * ' + str(signalsPerMessage[signalName]) + ' + ' + str(i) + ', in_' + message.name + '->' + signalName + str(i + 1) +');', sourceFileHandle)
            else:
                fWrite('			'+signalName+ 'Received(in_' + message.name +'->'+ signalName+');', sourceFileHandle)

    fWrite('			CAN_Msg_' + str(message.name) + '_Callback();', sourceFileHandle)
    fWrite('			break;', sourceFileHandle)
    fWrite('		}', sourceFileHandle)

fWrite('		default:', sourceFileHandle)
fWrite('		{', sourceFileHandle)
fWrite('			return -1;', sourceFileHandle)
fWrite('		}', sourceFileHandle)
fWrite('	}', sourceFileHandle)
fWrite('	return(0);', sourceFileHandle)
fWrite('}', sourceFileHandle)

for message in txMessages:
    if not message.is_multiplexed:
        fWrite("int sendCAN_" + message.name +"();", headerFileHandle)
        fWrite("int sendCAN_" + message.name +"(){", sourceFileHandle)
    else:
        params = list()
        for signal in message.signals:
            if signal.is_multiplexer:
                params.append('uint' + str(signal.length) + '_t ' + signal.name + 'Select')

        fWrite("int sendCAN_" + message.name +"(" + ', '.join(params) + ");", headerFileHandle)
        fWrite("int sendCAN_" + message.name +"(" + ', '.join(params) + "){", sourceFileHandle)

    fWrite('	struct ' + message.name + ' new_'+message.name +';', sourceFileHandle)

    if message.comment == 'VERSION':
        fWrite('	new_'+message.name +'.DBC = DBCVersion;', sourceFileHandle)
        for i  in range(0,7):
            fWrite('	new_'+message.name +'.git'+str(i)+' = gitCommit['+str(i)+'];', sourceFileHandle)
    elif message.comment == 'PROCAN':
        fWrite('	new_'+message.name +'.PRO_CAN_COUNT= '+message.name+'_PRO_CAN_COUNT++;', sourceFileHandle)
        fWrite('	'+message.name+'_PRO_CAN_COUNT = '+message.name+'_PRO_CAN_COUNT % 16;', sourceFileHandle)
        fWrite('	new_'+message.name +'.PRO_CAN_CRC= calculate_base_CRC((void *) &new_'+message.name+')^'+message.name+'_PRO_CAN_SEED;', sourceFileHandle)
    else:
        startBits = list()
        signals = list()
        signalNames = list()
        signalsPerMessage = {}

        for signal in message.signals:
            # checks if signal does not overlap with others in message
            if signal.comment != 'PROCAN' and not signal.start in startBits:
                startBits.append(signal.start)

                if re.sub('\d+$', '', signal.name) in txVariableArrays:
                    # get signal array and record number of these signals in a message
                    signalName = re.sub('\d+$', '', signal.name)
                    if not signalName in signalsPerMessage:
                        signalsPerMessage[signalName] = 1
                    else:
                        signalsPerMessage[signalName] += 1
                    if not signalName in signalNames:
                        signals.append(signal)
                        signalNames.append(signalName)
                else:
                    signals.append(signal)
        
        for signal in signals:
            # determine signal name
            if re.match('.+\d+$', signal.name):
                signalName = re.sub('\d+$', '', signal.name)
            else:
                signalName = signal.name

            # determine how to send signal based on if it is a multiplexer, multiplexed signal, or just a regular signal
            if signal.is_multiplexer:
                fWrite('	new_' + message.name +'.' + signalName + 'Select' + ' = ' + signalName + 'Select;', sourceFileHandle)
            elif signalName in txVariableArrays:
                for i in range(signalsPerMessage[signalName]):
                    muxSelect = signal.multiplexer_signal + 'Select'
                    fWrite('	new_'+message.name +'.'+signalName + str(i + 1) + ' = '+signalName+'Sending(' + muxSelect + ' * ' + str(signalsPerMessage[signalName]) + ' + ' + str(i) + ');', sourceFileHandle) 
            else:
                fWrite('	new_' + message.name +'.' + signalName + ' = ' + signalName + 'Sending();', sourceFileHandle)

    fWrite('	return sendCanMessage('+str(message.frame_id)+','+str(message.length)+',(uint8_t *) &new_'+message.name +');', sourceFileHandle)
    fWrite('}', sourceFileHandle)

fWrite('void configCANFilters(CAN_HandleTypeDef* canHandle);', headerFileHandle)
fWrite('__weak void configCANFilters(CAN_HandleTypeDef* canHandle)\n{', sourceFileHandle)
if boardType == 'F0':
    fWrite('	CAN_FilterConfTypeDef  sFilterConfig;', sourceFileHandle)
else: # F7
    fWrite('	CAN_FilterTypeDef  sFilterConfig;', sourceFileHandle)
fWrite('	// Filter msgs to this nodes Id to fifo 0', sourceFileHandle)
fWrite('	uint32_t filterID = CAN_NODE_ADDRESS<<8;', sourceFileHandle)
fWrite('	filterID = filterID << 3; // Filter ID is left aligned to 32 bits', sourceFileHandle)
fWrite('	uint32_t filterMask = 0xFF00;', sourceFileHandle)
fWrite('	filterMask = filterMask << 3; // Filter masks are also left aligned to 32 bits', sourceFileHandle)
fWrite('	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;', sourceFileHandle)
fWrite('	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;', sourceFileHandle)
fWrite('	sFilterConfig.FilterIdHigh = (filterID>>16) & 0xFFFF;', sourceFileHandle)
fWrite('	sFilterConfig.FilterIdLow = (filterID & 0xFFFF);', sourceFileHandle)
fWrite('	sFilterConfig.FilterMaskIdHigh = (filterMask>>16) & 0xFFFF;', sourceFileHandle)
fWrite('	sFilterConfig.FilterMaskIdLow = (filterMask & 0xFFFF);', sourceFileHandle)
fWrite('	sFilterConfig.FilterFIFOAssignment = 0;', sourceFileHandle)
fWrite('	sFilterConfig.FilterActivation = ENABLE;', sourceFileHandle)
if boardType == 'F0':
    fWrite('	sFilterConfig.BankNumber = 0;', sourceFileHandle)
    fWrite('	sFilterConfig.FilterNumber = 0;', sourceFileHandle)
else:
    fWrite('	sFilterConfig.FilterBank = 0;', sourceFileHandle)
    # From the reference manual, it seems that setting SlaveStartFilterBank to 0 means all filters are used for the enabled CAN peripheral
    fWrite('	sFilterConfig.SlaveStartFilterBank = 0;\n', sourceFileHandle) # TODO: Verify this is the correct config
fWrite('	if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)', sourceFileHandle)
fWrite('	{', sourceFileHandle)
fWrite('	  Error_Handler();', sourceFileHandle)
fWrite('	}', sourceFileHandle)

fWrite('\n	// Filter msgs to the broadcast Id to fifo 0', sourceFileHandle)
fWrite('	filterID = 0xFF<<8;', sourceFileHandle)
fWrite('	filterID = filterID << 3; // Filter ID is left aligned to 32 bits', sourceFileHandle)
fWrite('	filterMask = 0xFF00;', sourceFileHandle)
fWrite('	filterMask = filterMask << 3; // Filter masks are also left aligned to 32 bits', sourceFileHandle)
fWrite('	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;', sourceFileHandle)
fWrite('	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;', sourceFileHandle)
fWrite('	sFilterConfig.FilterIdHigh = (filterID>>16) & 0xFFFF;', sourceFileHandle)
fWrite('	sFilterConfig.FilterIdLow = (filterID & 0xFFFF);', sourceFileHandle)
fWrite('	sFilterConfig.FilterMaskIdHigh = (filterMask>>16) & 0xFFFF;', sourceFileHandle)
fWrite('	sFilterConfig.FilterMaskIdLow = (filterMask & 0xFFFF);', sourceFileHandle)
fWrite('	sFilterConfig.FilterFIFOAssignment = 0;', sourceFileHandle)
fWrite('	sFilterConfig.FilterActivation = ENABLE;', sourceFileHandle)
if boardType == 'F0':
    fWrite('	sFilterConfig.BankNumber = 1;', sourceFileHandle)
    fWrite('	sFilterConfig.FilterNumber = 1;', sourceFileHandle)
else:
    fWrite('	sFilterConfig.FilterBank = 1;', sourceFileHandle)
    # From the reference manual, it seems that setting SlaveStartFilterBank to 0 means all filters are used for the enabled CAN peripheral
    fWrite('	sFilterConfig.SlaveStartFilterBank = 0;\n', sourceFileHandle) # TODO: Verify this is the correct config
fWrite('	if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)', sourceFileHandle)
fWrite('	{', sourceFileHandle)
fWrite('	  Error_Handler();', sourceFileHandle)
fWrite('	}', sourceFileHandle)

i = 1
for messageGroup in messageGroups:
    i = i + 1
    fWrite('\n	// Filter msgs to the broadcast Id to fifo 0', sourceFileHandle)
    fWrite('	filterID = ' + messageGroup +'<<12;', sourceFileHandle)
    fWrite('	filterID = filterID << 3; // Filter ID is left aligned to 32 bits', sourceFileHandle)
    fWrite('	filterMask = 0xFF00;', sourceFileHandle)
    fWrite('	filterMask = filterMask << 3; // Filter masks are also left aligned to 32 bits', sourceFileHandle)
    fWrite('	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;', sourceFileHandle)
    fWrite('	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;', sourceFileHandle)
    fWrite('	sFilterConfig.FilterIdHigh = (filterID>>16) & 0xFFFF;', sourceFileHandle)
    fWrite('	sFilterConfig.FilterIdLow = (filterID & 0xFFFF);', sourceFileHandle)
    fWrite('	sFilterConfig.FilterMaskIdHigh = (filterMask>>16) & 0xFFFF;', sourceFileHandle)
    fWrite('	sFilterConfig.FilterMaskIdLow = (filterMask & 0xFFFF);', sourceFileHandle)
    fWrite('	sFilterConfig.FilterFIFOAssignment = 0;', sourceFileHandle)
    fWrite('	sFilterConfig.FilterActivation = ENABLE;', sourceFileHandle)
    if boardType == 'F0':
        fWrite('	sFilterConfig.BankNumber = ' + str(i) + ';\n', sourceFileHandle)
        fWrite('	sFilterConfig.FilterNumber = ' + str(i) + ';\n', sourceFileHandle)
    else:
        fWrite('	sFilterConfig.FilterBank = ' + str(i) + ';\n', sourceFileHandle)
        # From the reference manual, it seems that setting SlaveStartFilterBank to 0 means all filters are used for the enabled CAN peripheral
        fWrite('	sFilterConfig.SlaveStartFilterBank = 0;\n', sourceFileHandle) # TODO: Verify this is the correct config
    fWrite('	if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)', sourceFileHandle)
    fWrite('	{', sourceFileHandle)
    fWrite('	  Error_Handler();', sourceFileHandle)
    fWrite('	}', sourceFileHandle)

fWrite('}', sourceFileHandle)
fWrite('#pragma pack(pop)', sourceFileHandle)

fWrite("#endif /*__"+nodeName+"_can_H */", headerFileHandle);

headerFileHandle.close()
sourceFileHandle.close()

generateDepedencyFile(headerFile, nodeName)
generateDepedencyFile(sourceFile, nodeName)
