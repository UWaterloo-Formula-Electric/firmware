import tkinter as tk
import time
from threading import Thread


class Page(tk.Frame):
    def __init__(self, *args, **kwargs):
        tk.Frame.__init__(self, *args, **kwargs)

    def show(self):
        self.lift()


class DashPage(Page):
    def __init__(self, *args, **kwargs):
        Page.__init__(self, *args, **kwargs)
        tk.Label(self, text="Page 1", font=("Helvetica", 16), background="#ff3f30").pack(side="top", fill="both", expand=True)

class DebugPage(Page):
    def __init__(self, *args, **kwargs):
        Page.__init__(self, *args, **kwargs)
        tk.Label(self, text="Page 2", font=("Helvetica", 16), fg="#ffffff", background="#3f3fff").pack(side="top", fill="both", expand=True)

class MainView(tk.Frame):
    def __init__(self, *args, **kwargs):
        tk.Frame.__init__(self, *args, **kwargs)
        self.dashPage = DashPage(self)
        self.debugPage = DebugPage(self)

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)
        
        tk.Label(container, text="MainView", font=("Arial", 24)).place(relx=0.5, rely=0.5, anchor="center")

        self.dashPage.place(in_=container, x=0, y=0, relwidth=1, relheight=1)
        self.debugPage.place(in_=container, x=0, y=0, relwidth=1, relheight=1)




def switch_thread():
    while True:
        time.sleep(1)
        print("Switching to DebugPage")
        main.debugPage.show()
        time.sleep(1)
        print("Switching to DashPage")
        main.dashPage.show()


if __name__ == "__main__":
    root = tk.Tk()
    main = MainView(root)
    main.place(x=0, y=0, relwidth=1, relheight=1)
    root.wm_geometry("800x480")
    root.configure(bg="#3f3f3f")
    root.title("UWFE Dashboard (LIGHT)")
    Thread(target=switch_thread, daemon=True).start()
    root.mainloop()