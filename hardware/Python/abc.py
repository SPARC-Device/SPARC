import tkinter as tk

root = tk.Tk()
root.title("Test GUI")

entry = tk.Entry(root)
entry.pack()

def show_text():
    print("Text entered:", entry.get())

btn = tk.Button(root, text="Get Text", command=show_text)
btn.pack()

root.mainloop()
