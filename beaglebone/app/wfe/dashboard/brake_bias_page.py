import tkinter as tk

from cantools.typechecking import DecodeResultType
from page import Page
from constants import *

class BrakeBiasPage(Page):
    def __init__(self, *args, **kwargs):
        bg = kwargs.pop("bg", "#262626")
        Page.__init__(self, bg=bg, *args, **kwargs)

        # create a sliding vertical bar on a rectangle to represent brake bias,between front and rear
        self.front_pres = 0
        self.rear_pres = 0
        self.front_pres_label = tk.Label(self, text=f"{0:<4} PSI", font=("PT Mono", 20), bg=self["bg"], fg="#ffffff")
        self.front_pres_label.place(x=WIDTH/2 - 200, y=HEIGHT/2, anchor=tk.W)
        self.rear_pres_label = tk.Label(self, text=f"{0:<4} PSI", font=("PT Mono", 20), bg=self["bg"], fg="#ffffff")
        self.rear_pres_label.place(x=WIDTH/2 + 200, y=HEIGHT/2, anchor=tk.E)
        
        tk.Label(self, text="Front", font=("PT Mono", 20), bg=self["bg"], fg="#ffffff").place(x=WIDTH/2 - 200, y=HEIGHT/2-50, anchor=tk.CENTER)
        tk.Label(self, text="Rear", font=("PT Mono", 20), bg=self["bg"], fg="#ffffff").place(x=WIDTH/2 + 200, y=HEIGHT/2-50, anchor=tk.CENTER)

        tk.Label(self, text="Brake Bias", font=("PT Mono", 20), bg=self["bg"], fg="#ffffff").place(x=WIDTH/2, y=HEIGHT/2-125, anchor=tk.CENTER)
        self.bias_label = tk.Label(self, text="50% | 50%", font=("PT Mono", 20), bg=self["bg"], fg="#ffffff")
        self.bias_label.place(x=WIDTH/2, y=HEIGHT/2-75, anchor=tk.CENTER)

    def updateVCUData(self, decoded_data: DecodeResultType):
        self.front_pres = decoded_data["FrontBrakePressure"]
        self.front_pres_label.config(text=f'{self.front_pres:<4} PSI')
        self.updateBias()


    def updateRearPres(self, decoded_data: DecodeResultType):
        self.rear_pres = decoded_data["RearBrakePressure"]
        self.rear_pres_label.config(text=f'{self.rear_pres:<4} PSI')
        self.updateBias()

    def updateBias(self):
        front_bias = self.front_pres / (self.front_pres + self.rear_pres) * 100
        rear_bias = 100 - front_bias
        self.bias_label.config(text=f'{front_bias:.0f}% | {rear_bias:.0f}%')