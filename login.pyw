import tkinter as tk, server, client

def center_window(window, width, height):
    screen_width = window.winfo_screenwidth()
    screen_height = window.winfo_screenheight()

    x = (screen_width // 2) - (width // 2)
    y = (screen_height // 2) - (height // 2)

    window.geometry(f"{width}x{height}+{x}+{y}")

window = tk.Tk()
window.title("Login")
center_window(window, 600, 200)

username = tk.Entry(window, width=20)
username.place(x=80, y=75)

ip = tk.Entry(window, width=20)
ip.place(x=400, y=75)

def playy():
    ipp = ip.get()
    usernamee = username.get()
    window.destroy()
    if usernamee == "server":
        server.start()
    else:
        client.start(usernamee, ipp)

play = tk.Button(window, width=10, text="Play", command=playy)
play.place(x=110, y=100)

window.mainloop()