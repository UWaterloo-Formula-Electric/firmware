import tkinter as tk

from page import Page
from constants import *

class ResetPage(Page):
    def __init__(self, *args, **kwargs):
        bg = kwargs.pop("bg", "#262626")
        Page.__init__(self, bg=bg, *args, **kwargs)

        self.canvas = tk.Canvas(self, bg=self["bg"], width=800, height=480, highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)

        tk.Label(self.canvas, text="Reset Watt-Hours", font=("PT Mono", 40), bg=self["bg"], fg="#ffffff").pack(pady=20)
 
        side = 80
        r = 20
        x = WIDTH/2 - 100

        self.button_bg = "#6D6D6D"
        self.button_active_bg = "#0044FF"
        self.up = self.round_rectangle(x - side/2, 120, side, side, r, fill=self.button_bg, outline="#ffffff")
        self.mid = self.round_rectangle(x - side/2 - side*1.3, 220, side, side, r, fill=self.button_bg, outline="#ffffff")
        self.down = self.round_rectangle(x - side/2, 320, side, side, r, fill=self.button_bg, outline="#ffffff")

        self.yes = tk.Label(self.canvas, text="Yes", font=("PT Mono", 20), bg=self.button_bg, fg="#ffffff")
        self.yes.place(x=x - side/2 + 14, y=120 + side/2 - 20)
        self.next = tk.Label(self.canvas, text="Next", font=("PT Mono", 20), bg=self.button_bg, fg="#ffffff")
        self.next.place(x=x - side/2 - side*1.3 + 10, y=220 + side/2 - 20)
        self.no = tk.Label(self.canvas, text="No", font=("PT Mono", 20), bg=self.button_bg, fg="#ffffff")
        self.no.place(x=x - side/2 + 22, y=320 + side/2 - 20)

        self.wh_text = tk.Label(self.canvas, text="", font=("PT Mono", 35), bg=self["bg"], fg="#ffffff")
        self.wh_text.place(x=WIDTH/2, y=235)

        # change the color to red
        # self.canvas.itemconfig(self.up, fill="red")

    def round_rectangle(self, x, y, w, h, radius=25, **kwargs):
        x1, y1, x2, y2 = x, y, x + w, y + h
        # radius = min(radius, w / 2, h / 2)
        points = [x1+radius, y1,
                  x1+radius, y1,
                  x2-radius, y1,
                  x2-radius, y1,
                  x2, y1,
                  x2, y1+radius,
                  x2, y1+radius,
                  x2, y2-radius,
                  x2, y2-radius,
                  x2, y2,
                  x2-radius, y2,
                  x2-radius, y2,
                  x1+radius, y2,
                  x1+radius, y2,
                  x1, y2,
                  x1, y2-radius,
                  x1, y2-radius,
                  x1, y1+radius,
                  x1, y1+radius,
                  x1, y1]

        return self.canvas.create_polygon(points, **kwargs, smooth=True)

    def updateEnergy(self, wh):
        self.wh_text.config(text=f"{wh:<4} Wh")

    def parse_buttons(self, up, mid, down):
        if up:
            self.canvas.itemconfig(self.up, fill=self.button_active_bg)
            self.yes.config(bg=self.button_active_bg)
        else:
            self.canvas.itemconfig(self.up, fill=self.button_bg)
            self.yes.config(bg=self.button_bg)

        if mid:
            self.canvas.itemconfig(self.mid, fill=self.button_active_bg)
            self.next.config(bg=self.button_active_bg)
        else:
            self.canvas.itemconfig(self.mid, fill=self.button_bg)
            self.next.config(bg=self.button_bg)

        if down:
            self.canvas.itemconfig(self.down, fill=self.button_active_bg)
            self.no.config(bg=self.button_active_bg)
        else:
            self.canvas.itemconfig(self.down, fill=self.button_bg)
            self.no.config(bg=self.button_bg)
