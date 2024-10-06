from tkinter import Tk, Canvas

window = Tk()

window.geometry("800x480")
window.configure(bg="#ffffff")
window.title("UWFE Dashboard (LIGHT)")

canvas = Canvas(
    window,
    bg="#ffffff",
    height=480,
    width=800,
    bd=0,
    highlightthickness=0,
    relief="ridge"
)

canvas.place(x=0, y=0)

window.resizable(False, False)
window.mainloop()