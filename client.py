def start(username, ip):
    import tkinter as tk
    from tkinter import ttk, messagebox, simpledialog
    import socket
    import threading
    import random
    import sys
    
    global questlog
    questlog = ""

    global mana
    mana = 0

    class questlogg(simpledialog.Dialog):
        def apply(self):
            sock.sendall(f"questlog:{self.result.encode()}")

        def handle_enter(self, event):
            self.text.insert(tk.INSERT, "\n")
            return "break"

        def body(self, master):
            tk.Label(master, text="Enter your text:").pack()
            self.text = tk.Text(master, width=100, height=40)
            self.text.insert("1.0", questlog)
            self.text.pack()
            self.text.bind("<Return>", self.handle_enter)
            return self.text  # initial focus
        
        def apply(self):
            self.result = self.text.get("1.0", tk.END).strip()

    def close():
        if not username == "test":
            sock.close()
        window.destroy()

    #config
    global health, level
    level = 1
    health = 0

    last_message = ""
    if ":" in ip:
        server_ip, server_port = ip.split(":")
        server_port = int(server_port)
    else:
        if ip == "":
            server_ip = "127.0.0.1"
        else:
            server_ip = ip
        server_port = 12345

    # ==== GUI ====
    window = tk.Tk()
    window.geometry("800x600")
    window.title("Client")
    window.protocol("WM_DELETE_WINDOW", close)

    logg = tk.Text(window, width=45)
    logg.config(state="disabled")
    logg.place(x=420, y=75)

    if not username == "dm":
        healthh = tk.Label(window, text="", font=("Arial", 20))
        healthh.place(x=550, y=20)

        name_label = tk.Label(window, text=username, font=("Arial", 20))
        name_label.place(x=370, y=0)

        levell = tk.Label(window, text="ERROR")
        levell.place(x=375, y=30)
        if username == "test":
            level = 1

        manaa = tk.Label(window, text="ERROR", font=("Arial", 20))
        manaa.place(x=0, y=20)


    # ==== LOG FUNCTION ====
    def log(text):
        logg.config(state="normal")
        logg.insert(tk.END, str(text) + "\n")
        logg.see(tk.END)
        logg.config(state="disabled")

    # ==== SOCKET FUNCTIONS ====
    def create_client(host, port):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((host, port))
            return sock
        except:
            window.withdraw()
            messagebox.showerror("No server", "Server connection failed")
            window.destroy()
            sys.exit()

    def receive_messages(sock):
        global health, level, questlog, mana
        while True:
            try:
                data = sock.recv(1024)
                if not data:
                    break
                message = data.decode('utf-8')

                if message.startswith("damage:"):
                    try:
                        dmg = int(message.split(":")[1])
                        health -= dmg
                        healthh["text"] = f"Health:{health}"
                    except:
                        log("ERROR")
                        
                elif message.startswith("data:"):
                    try:
                        health = int(message.split(":")[1])
                        healthh["text"] = f"Health:{health}"
                        level = int(message.split(":")[2])
                        levell["text"] = f"Level:{level}"
                        questlog = message.split(":")[3]
                        mana = int(message.split(":")[4])
                        manaa["text"] = f"mana:{mana}"
                        print(mana)
                    except:
                        log("ERROR")

                elif message.startswith("level:"):
                    try:
                        level = int(message.split(":")[1])
                        levell["text"] = f"Level:{level}"
                    except:
                        log("ERROR")

                elif message.startswith("questlogchange"):
                    questlog = message.split(":")[1]

                elif message.startswith("mana"):
                    mana = int(message.split(":")[1])
                    manaa["text"] = f"mana:{mana}"
                    print(mana)

                else:
                    window.after(0, log, message)

            except Exception as e:
                print(f"Error receiving message: {e}")
                break

    def send_message(sock, message):
        try:
            sock.sendall(message.encode('utf-8'))
        except Exception as e:
            print(f"Error sending message: {e}")

    # ==== BUTTON FUNCTIONS ====
    def send_damage():
        target = damage_player_var.get()
        dmg = damage_entry.get()
        if target and dmg.isdigit():
            send_message(sock, f"damage:{target}:{dmg}")

    # ==== DM TOOLS ====
    if username == "dm" or username == "test":
        def open_logfunc():
            dialog = questlogg(window, title="Input quest log")
            send_message(sock, f"questlog:{dialog.result}")

        open_log = tk.Button(window, text="Open quest log", command=open_logfunc)
        open_log.place(x=50, y=175)

        dice_box = tk.Text(window, width=10, height=1)
        dice_box.place(x=100, y=220)

        def roll():
            rollnum = random.randint(1, 20)
            dice_box.delete("1.0", tk.END)
            dice_box.insert("1.0", str(rollnum))
            msg = f"{username} rolled a {rollnum}"
            send_message(sock, msg)
        
        def levelup():
            send_message(sock, f"level:{level_player_var.get()}:{level_entry.get()}")

        # Player selector dropdown
        damage_player_var = tk.StringVar()
        damage_player_select = ttk.Combobox(window, textvariable=damage_player_var, values=["Death", "glorg"])
        damage_player_select.place(x=100, y=100)

        level_player_var = tk.StringVar()
        level_player_select = ttk.Combobox(window, textvariable=level_player_var, values=["Death", "glorg"])
        level_player_select.place(x=100, y=125)

        # Damage entry
        damage_entry = tk.Entry(window, width=10)
        damage_entry.place(x=250, y=100)

        level_entry = tk.Entry(window, width=10)
        level_entry.place(x=250, y=125)

        tk.Button(window, text="Roll Dice", command=roll).place(x=100, y=250)
        tk.Button(window, text="Damage Player", command=send_damage).place(x=320, y=95)
        tk.Button(window, text="Add Level", command=levelup).place(x=320, y=125) 

    def command(event):
        nonlocal last_message
        if not console.get() == "":
            send_message(sock, console.get())
            last_message = console.get()
            console.delete(0, tk.END)

    def up_arrow(event):
        if not last_message == "":
            console.delete(0, tk.END)
            console.insert(0, last_message)

    console = tk.Entry(window, width=60)
    console.place(x=420, y=470)
    console.bind("<Return>", command)
    console.bind("<Up>", up_arrow)

    # ==== CONNECT ====
    if not username == "test":
        sock = create_client(server_ip, server_port)
        sock.sendall(username.encode('utf-8'))

        threading.Thread(target=receive_messages, args=(sock,), daemon=True).start()

    window.mainloop()