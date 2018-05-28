#!/usr/bin/env python

import cantools #$ pip install cantools
import os
from pprint import pprint
import subprocess
import sys

nodeName = sys.argv[1]

commonDir = 'common-f7'
genDir = 'Gen'

genIncDir = os.path.join(genDir, 'Inc')

ScriptsDir = os.path.join(commonDir, 'Scripts')

depFile = os.path.join(genDir, 'canGen.d')

dataDir = 'common-f7/data'
dbFile = os.path.join(dataDir, '2018CAR.dbc')

headerFile = os.path.join(genIncDir, nodeName + '_can.h')
sourceFile = os.path.join(genIncDir, nodeName + '_can.c')

db = cantools.db.load_file(dbFile)

label = '0df2a9a'
gitClean = '0';

try:
	if call(["git", "branch"], stderr=STDOUT, stdout=open(os.devnull, 'w')) != 0:
		label = subprocess.check_output(["git", "describe" ,"--tags", "--always"]).strip()
except Exception:
	pass

gitCommit = label

def fWrite(string, fileHandle):
    fileHandle.write(string + '\n')

def generateDepedencyFile(headerFile, target):
    with open(depFile, 'w') as depFileHandle:
        fWrite('{headerFile}: {dir}/generateCANHeadder.py {dbFile}'.format(headerFile=headerFile, dbFile=dbFile, dir=ScriptsDir), depFileHandle)
        fWrite('	{dir}/generateCANHeadder.py {target}'.format(target=target, dir=ScriptsDir), depFileHandle)



#make .h file

sys.stdout = open(headerFile, "w")
print("#ifndef __"+nodeName+"_can_H\n#define __"+nodeName+"_can_H\n")
print("#include \"can.h\"")

print('//Message Filtring')
nodeAddress = 0
messageGroups = list()

for node in db.nodes:
	if node.name == nodeName:
		nodeAddress = node.comment.split("_")[0]
		print('#define CAN_NODE_ADDRESS ' + nodeAddress)
		for messageGroup in node.comment.split("_")[1].split(","):
			if messageGroup != "":
				print('#define CAN_NODE_MESSAGE_GROUP_'+messageGroup)
				messageGroups.append(messageGroup)
print('')


print("#endif /*__"+nodeName+"_can_H */")


sys.stdout = open(sourceFile, "w")
print('//DBC version:')
print('int DBCVersion = '+db.version+';');
print('char gitCommit[] = \"'+gitCommit+'\";')

print('//'+nodeName+' can headder')
print("#include \""+nodeName+"_can.h\"")
print('#include \"CRC_CALC.h\"')

nodeList = list()




variables = list()
messages = list()
variablesPROCAN = list()

for mes in db.messages:
	messageUseful = 0
	for signal in mes.signals:
			if nodeName in signal.nodes:
				messageUseful = 1
				variables.append(signal)
	if messageUseful == 1:
		messages.append(mes)


print('__weak int sendCanMessage(int id,int length,void *data);')	
print('// Incoming variables')
for signal in variables:
	type = "int "
	if((signal.is_float)):
		type = "float " 
	print('volatile '+ type + signal.name + ';	// offset: ' + str(signal.offset)+ " scaler: "+ str(signal.scale))
	print('__weak void '+ signal.name+'Recived('+type+ 'newValue)\n{')
	print("	newValue = newValue * "+str(signal.scale)+";")
	print("	newValue = newValue + "+str(signal.offset)+";")
	print("	"+signal.name + " = newValue;")
	print("}\n")		

print('// Outgoing variables')
for mes in db.messages:
	if nodeName in mes.nodes:
		if mes.comment == 'PROCAN':
			variablesPROCAN.append('int '+mes.name+'_PRO_CAN_SEED = 127;');
			variablesPROCAN.append('int '+mes.name+'_PRO_CAN_COUNT = 0;');
		if mes.comment != 'VERSION':

			for signal in mes.signals:
				if signal.comment != "PROCAN":
					type = "int "
					if((signal.is_float)):
						type = "float " 
					print('volatile '+ type + signal.name + ';	// offset: ' + str(signal.offset)+ " scaler: "+ str(signal.scale))
					print('__weak '+type+ signal.name+'Sending()\n{')
					print('	'+type+'sendValue = '+ signal.name+';')
					print("	sendValue = sendValue - "+str(signal.offset)+";")
					print("	sendValue = sendValue / "+str(signal.scale)+";")
					print("	return sendValue;")
					print("}\n")
									
	

print('')
print('// PRO_CAN ')
for strings in variablesPROCAN:
	print(strings)
print('')

print('int init_can_driver(){')
print('	generate_CRC_lookup_table();')
print('	return 0;')
print('}')
print('')

messagesREL = list()
for mes in db.messages:
	if nodeName in mes.nodes:
		messagesREL.append(mes)
for message in messagesREL:
	print('struct ' + message.name + '{') 
	totalSize = message.length*8;
	currentPos = 0;
	if message.comment == 'VERSION':
		print('	int DBC : 8;')
		for i  in range(0,7):
			print('	char git'+str(i)+' : 8;')


	else:
		for signal in message.signals:
			if signal.start != currentPos:
				print('	unsigned int FILLER_'+ str(signal.start) + ' : ' + str(signal.start - currentPos) + ';')
			if signal.is_signed	:
				print('	         int ' + signal.name + ' : ' + str(signal.length) + ';')
			else :
				print('	unsigned int ' + signal.name + ' : ' + str(signal.length) + ';')
			currentPos = signal.start + signal.length
		if currentPos != totalSize:
			print('	unsigned int FILLER_END : ' + str(totalSize - currentPos) + ';')
	print('};') 
	print('') 
print('') 


for message in messages:
	print('struct ' + message.name + ' {') 
	totalSize = message.length*8;
	currentPos = 0;
	for signal in message.signals:
		if signal.start != currentPos:
			print('	unsigned int FILLER_'+ str(signal.start) + ' : ' + str(signal.start - currentPos) + ';')
		if signal.is_signed	:
			print('		     int ' + signal.name + ' : ' + str(signal.length) + ';')
		else :
			print('	unsigned int ' + signal.name + ' : ' + str(signal.length) + ';')
		currentPos = signal.start + signal.length
	if currentPos != totalSize:
		print('	unsigned int FILLER_END : ' + str(totalSize - currentPos) + ';')
	print('};') 
	print('') 

print('int parseCANData(int id, void * data) {') 
print('	switch(id) {') 
for message in messages:
	print('		case '+ str(message.frame_id) + ' : // '+str(message.name)) 
	# for signal in message.signals:
	print('		{') 
	
	print('			struct ' + message.name + ' *new_'+message.name +' = data;')
	for signal in message.signals:
		if nodeName in signal.nodes:
			print('			'+signal.name+ 'Recived(new_'+message.name +'->'+ signal.name+');')

	print('			break;') 
	print('		}')
print('	}')
print('	return(0);')  
print('}') 

messagesTransmit = list()

for mes in db.messages:
	if nodeName in mes.nodes:
		messagesTransmit.append(mes) 
for message in messagesTransmit:
	print("int sendCAN_" + message.name +"(){")
	print('	struct ' + message.name + ' new_'+message.name +';')
	if message.comment == 'VERSION':
		print('	new_'+message.name +'.DBC = DBCVersion;')
		for i  in range(0,7):
			print('	new_'+message.name +'.git'+str(i)+' = gitCommit['+str(i)+'];')
	else:
		for signal in message.signals:
			if signal.comment != 'PROCAN':
				print('	new_'+message.name +'.'+signal.name+' = '+signal.name+'Sending();')
				
		if message.comment == 'PROCAN':
			print('	new_'+message.name +'.PRO_CAN_COUNT= '+message.name+'_PRO_CAN_COUNT++;')
			print('	'+message.name+'_PRO_CAN_COUNT = '+message.name+'_PRO_CAN_COUNT % 16;')
			print('	new_'+message.name +'.PRO_CAN_CRC= calculate_base_CRC((void *) &new_'+message.name+')^'+message.name+'_PRO_CAN_SEED;')
	print('	return sendCanMessage('+str(message.frame_id)+','+str(message.length)+',(uint8_t *) &new_'+message.name +');')
 	print("}")


print('__weak void configCANFilters(CAN_HandleTypeDef* canHandle)\n{')
print('	CAN_FilterConfTypeDef  sFilterConfig;')
print('	sFilterConfig.FilterNumber = 0;')
print('	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;')
print('	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;')
print('	sFilterConfig.FilterIdHigh = 0x0000;')
print('	sFilterConfig.FilterIdLow = 0x0000 + ((CAN_NODE_ADDRESS)<<8);')
print('	sFilterConfig.FilterMaskIdHigh = 0x0000;')
print('	sFilterConfig.FilterMaskIdLow = 0xFF00;')
print('	sFilterConfig.FilterFIFOAssignment = 0;')
print('	sFilterConfig.FilterActivation = ENABLE;')
print('	sFilterConfig.BankNumber = 0;\n')
print('	if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)')
print('	{')
print('	  Error_Handler();')
print('	}')

print('\n	sFilterConfig.FilterIdLow = 0x0000 + ('+str(0xFF)+'<<8);')
print('	sFilterConfig.FilterFIFOAssignment = 1;')
print('	sFilterConfig.BankNumber = 1;\n')
print('	if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)')
print('	{')
print('	  Error_Handler();')
print('	}')
i = 1
for messageGroup in messageGroups:
	i = i + 1
	print('\n	sFilterConfig.FilterIdLow = 0x0000 + ('+messageGroup+'<<12);')
	print('	sFilterConfig.FilterFIFOAssignment = 2;')
	print('	sFilterConfig.BankNumber = '+str(i)+';\n')
	print('	if(HAL_CAN_ConfigFilter(canHandle, &sFilterConfig) != HAL_OK)')
	print('	{')
	print('	  Error_Handler();')
	print('	}')
	
print('}')


generateDepedencyFile(headerFile, nodeName)
generateDepedencyFile(sourceFile, nodeName)
