import time
import tkinter as tk
from tkinter import scrolledtext

from constants import TagEnum
from page import Page

class DebugPage(Page):
    def __init__(self, *args, **kwargs):
        bg = kwargs.pop("bg", "#262626")
        Page.__init__(self, bg=bg, *args, **kwargs)

        # Create the scrollable text area
        self.debug_text_area = scrolledtext.ScrolledText(self,  font=("Helvetica", -14),
                                                         width=100, height=30,
                                                         bg="#000000", fg="#ffffff")
        self.debug_text_area.pack(expand=True, fill="both")

    _update_debug_first = True
    def update_debug_text(self, dtc_origin, dtc_code, dtc_data, dtc_desc):
        if isinstance(dtc_code, int):
            dtc_code = f"{dtc_code:02d}"
        if isinstance(dtc_data, int):
            dtc_data = f"{dtc_data:03d}"

        time_str = f"{time.strftime('%H:%M:%S')} "
        if not self._update_debug_first:
            time_str = "\n" + time_str
        self._update_debug_first = False

        self.debug_text_area.insert(tk.INSERT, time_str, TagEnum.TIME.value)
        self.debug_text_area.insert(tk.INSERT, dtc_origin, TagEnum.ORIGIN.value)
        self.debug_text_area.insert(tk.INSERT, " | ")
        self.debug_text_area.insert(tk.INSERT, dtc_code, TagEnum.CODE.value)
        self.debug_text_area.insert(tk.INSERT, " | ")
        self.debug_text_area.insert(tk.INSERT, dtc_data, TagEnum.DATA.value)
        self.debug_text_area.insert(tk.INSERT, " | ")
        self.debug_text_area.insert(tk.INSERT, dtc_desc, TagEnum.DESC.value)

        self.debug_text_area.tag_config(TagEnum.TIME.value, foreground="#359a10")

        self.debug_text_area.tag_config(TagEnum.ORIGIN.value, foreground="#70a2ff")
        self.debug_text_area.tag_config(TagEnum.CODE.value, foreground="#ec70ff")
        self.debug_text_area.tag_config(TagEnum.DATA.value, foreground="#ff6b6b")
        self.debug_text_area.tag_config(TagEnum.DESC.value, foreground="#ffffff")
        # Scroll to the bottom to show the latest message
        self.debug_text_area.yview("end")
