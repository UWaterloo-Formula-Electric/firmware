import tkinter as tk

class MainView(tk.Frame):
    def __init__(self, *args, **kwargs):
        tk.Frame.__init__(self, *args, **kwargs)

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)

        tk.Label(container, text="MainView", font=("Arial", 24)).place(relx=0.5, rely=0.5, anchor="center")

if __name__ == "__main__":
    root = tk.Tk()
    main = MainView(root)
    main.place(x=0, y=0, relwidth=1, relheight=1)
    root.wm_geometry("800x480")
    root.configure(bg="#3f3f3f")
    root.title("UWFE Dashboard (LIGHT)")
    root.mainloop()