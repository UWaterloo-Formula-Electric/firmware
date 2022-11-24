
import json
import sys
import re
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QFileDialog
from PyQt5.QtWidgets import QMessageBox

from dependencies.UI.GUI import Ui_Dialog
from dependencies.Graph import *

def log():
     print("GUI running")

def error(message = "Error: Please check your input"):
     msg = QMessageBox()
     msg.setText(message)
     msg.setWindowTitle("Error")
     msg.exec_()

def showGraph():
     dbcFile = ui.DBCFile.text()
     data_dict = {}
     if (ui.FileLocation.toPlainText() != ''):
          # print(ui.FileLocation.toPlainText()[-4:])
          if (ui.FileLocation.toPlainText()[-4:] == ".log"):
               data_dict = json.loads(logToJsonDict(ui.FileLocation.toPlainText(), dbcFile))
               # print("log graph")
          elif(ui.FileLocation.toPlainText()[-4:] == ".csv"):
               data_dict = csvToDict(ui.FileLocation.toPlainText())
               # print("csv graph")
     else:
          error("Please select a file to graph")
          return
     # save all the checked signal in list which is named checked_list 
     checked_list = []
     root = ui.CANSignals.invisibleRootItem()
     all_signals = root.child(0)
     signal_count = root.child(0).childCount()
     for i in range(signal_count):
          child = all_signals.child(i)
          if (child.checkState(0) == QtCore.Qt.Checked):
               checked_list.append(child.text(0))
     # print(checked_list)

     # in data_dict, if signal name is selected or it's selected by regex, save into checked_data_dict (filtering)
     checked_data_dict = {}
     regexInput = ui.RegexInput.text()
     if (regexInput != ""):
          for i in data_dict:
               if (i in checked_list) or (re.search(regexInput, i) is not None):
                    checked_data_dict[i] = data_dict[i]
     else:
          for i in data_dict:
               if (i in checked_list):
                    checked_data_dict[i] = data_dict[i]
     # print(checked_data_dict)

     #obtain all other constraints and save in args_dict
     args_dict = {}
     args_dict["MaxXInput"] = ui.MaxXInput.text()
     args_dict["MinXInput"] = ui.MinXInput.text()
     args_dict["MaxYInput"] = ui.MaxYInput.text()
     args_dict["MinYInput"] = ui.MinYInput.text()
     # print(args_dict)

     graph(checked_data_dict, args_dict)

def getSignalNames():
     if (ui.FileLocation.toPlainText() != ''):
          if (ui.FileLocation.find(".log")):
               # print("log signal names")
               data_dict = json.loads(logToJsonDict(ui.FileLocation.toPlainText(), "../../../../../common/Data/2018CAR.dbc"))
          elif(ui.FileLocation.find(".csv")):
               # print("csv signal names")
               data_dict = csvToDict(ui.FileLocation.toPlainText())

     signalNames = []
     for key in data_dict.keys():
          if(signalNames.count(key) == 0):
               signalNames.append(key)
     signalNames.sort()
     return signalNames

def loadExtraUiLogic(ui):
     ui.SelectFile.clicked.connect(selectFile)
     ui.GraphBtn.clicked.connect(showGraph)
     ui.SelectDBC.clicked.connect(selectDBC)

def selectFile():
     fileLocation = QFileDialog.getOpenFileName(None, "Select file", "CAN Log File", "Log file (*.log);;CSV Log file (*.csv);; All Files (*.*)")[0]
     if fileLocation != "":
          ui.FileLocation.setText(fileLocation)
     loadSignalList(ui.CANSignals, getSignalNames())

def selectDBC():
     fileLocation = QFileDialog.getOpenFileName(None, "Select file", "DBC file (.dbc)", "*.dbc")[0]
     if fileLocation != "":
          ui.DBCFile.setText(fileLocation)

def loadSignalList(treeWidget, data_list):
     treeWidget.clear()
     root = QtWidgets.QTreeWidgetItem(treeWidget)
     root.setText(0, "All")
     root.setFlags(root.flags() | QtCore.Qt.ItemIsTristate | QtCore.Qt.ItemIsUserCheckable)
     for x in range(len(data_list)):
          signal = QtWidgets.QTreeWidgetItem(root)
          signal.setFlags(signal.flags() | QtCore.Qt.ItemIsUserCheckable)
          signal.setText(0, data_list[x])
          signal.setCheckState(0, QtCore.Qt.Unchecked)
     
if __name__ == "__main__":
     app = QtWidgets.QApplication(sys.argv)
     MainWindow = QtWidgets.QWidget()
     ui = Ui_Dialog()
     ui.setupUi(MainWindow)
     loadExtraUiLogic(ui)
     MainWindow.show()
     sys.exit(app.exec_())



