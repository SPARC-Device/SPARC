from tkinter import *

root =Tk()
root.geometry("655x344")

def printname():
    print(f"hello {aval.get()}")


# f1=Frame(root,bg="grey")
# f1.pack()
a=Label(root,text="username:")
b=Label(root,text="Password:")
a.grid()
b.grid(row=1)

aval=StringVar()
bval=StringVar()

a_entry=Entry(root,textvariable= aval).grid(row=0,column=1)
b_entry=Entry(root,textvariable= bval).grid(row=1,column=1)

Button(text="submit",command=printname).grid(row=4,column=1)




root.mainloop()