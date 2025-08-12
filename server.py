import tkinter as tk
from tkinter import messagebox
import socket
import threading
import json
import os
import sys
#import pythonnet
#pythonnet.load()
#import clr
#clr.AddReference(r"C:\Users\kanda\Desktop\code\python\dnd\pythonlib.dll")

#from python_lib import cryptograhpy

def start():
    global quest_log
    quest_log = ""

    if os.path.exists("serverdata.json") and os.path.exists("questlog.txt"):
        with open("serverdata.json", "r") as f:
            serverdata = json.load(f)
            players = serverdata["players"]
        with open("questlog.txt", "r") as f:
            result = f.read()
            quest_log = result
    else:
        window.withdraw()
        messagebox.showerror("No server", "Server connection failed")
        window.destroy()
        sys.exit()

    clients = {}
    health = {}
    level = {}
    mana = {}

    def close():
        with open("serverdata.json", "w") as f:
            json.dump(serverdata, f, indent=4)
            f.close()
        with open("questlog.txt", "w") as f:
            f.write(quest_log)
            f.close()
        window.destroy()

    window = tk.Tk()
    window.title("Server")
    window.geometry("800x600")
    window.protocol("WM_DELETE_WINDOW", close)

    logg = tk.Text(window, width=45, height=30)
    logg.config(state="disabled")
    logg.place(x=420, y=75)

    def log(text):
        logg.config(state="normal")
        logg.insert(tk.END, text + "\n")
        logg.see(tk.END)
        logg.config(state="disabled")

    name = tk.Label(window, text="Server", font=("Arial", 20))
    name.place(x=370, y=0)

    def send_to_client(username, message):
        client = clients.get(username)
        if client:
            try:
                client.sendall(message.encode("utf-8"))
            except Exception as e:
                log(f"Error sending to client {username}: {e}")
                clients.pop(username, None)

    def send_to_all_clients(message, exclude=None):
        for client_username, client in list(clients.items()):
            if client == exclude:
                continue
            try:
                client.sendall(message.encode("utf-8"))
            except Exception as e:
                log(f"Error sending to client {client_username}: {e}")
                clients.pop(client_username, None)

    def start_server(host="0.0.0.0", port=12345):
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind((host, port))
        server_socket.listen()
        log(f"Server started on {host}:{port}")

        def client_thread(conn, addr):
            global quest_log
            log(f"Connection from {addr}")
            try:
                username_data = conn.recv(1024)
                username = username_data.decode("utf-8").strip()
                if not username_data or not username in players:
                    conn.close()
                    log("disconnect")
                    return
                log(f"Username {username} connected from {addr}")

                if username in clients:
                    conn.sendall("Username already taken.".encode("utf-8"))
                    conn.close()
                    return

                clients[username] = conn
                try:
                    health[username] = int(serverdata["playersdata"][username]["Health"])
                    level[username] = int(serverdata["playersdata"][username]["Level"])
                    mana[username] = int(serverdata["playersdata"][username]["mana"])
                except:
                    health[username] = 100
                    level[username] = 1
                send_to_client(username, f"data:{health[username]}:{level[username]}:{quest_log}:{mana[username]}")

                while True:
                    data = conn.recv(1024)
                    if not data:
                        break
                    message = data.decode("utf-8").strip()
                    parts = message.split(":")

                    if parts[0] == "damage" and len(parts) == 3:
                        target_player = parts[1]
                        damage_amount = parts[2]
                        if clients.get(target_player):
                            send_to_client(target_player, f"damage:{damage_amount}")
                            health[target_player] -= int(damage_amount)
                            log(f"{target_player} took {damage_amount} damage and is now on {health[target_player]} Health")
                            send_to_all_clients(f"{target_player} took {damage_amount} damage and is now on {health[target_player]} Health")
                        else:
                            log(f"Target player '{target_player}' not found.")
                            try:
                                conn.sendall(f"Error: Target player '{target_player}' not connected.".encode("utf-8"))
                            except:
                                pass

                    elif parts[0] == "level" and len(parts) == 3:
                        target_player = parts[1]
                        leveladd = int(parts[2])
                        if clients.get(target_player):
                            level[target_player] += leveladd
                            log(f"{target_player} is now level {level[target_player]}")
                            send_to_client(target_player, f"level:{level[target_player]}")

                    elif parts[0] == "questlog" and len(parts) == 2:
                        quest_log = parts[1]
                        send_to_all_clients(f"questlogchange:{parts[1]}")

                    elif parts[0] == "mana" and len(parts) == 3:
                        target_player = parts[1]
                        mana[target_player] += int(parts[2])
                        send_to_client(target_player, f"mana:{mana[target_player]}")

                    else:
                        log(f"{username}: {message}")
                        send_to_all_clients(f"{username}: {message}")

            except Exception as e:
                log(f"Error with client {addr}: {e}")
            finally:
                serverdata["playersdata"][username]["Health"] = health[username]
                serverdata["playersdata"][username]["Level"] = level[username]
                clients.pop(username, None)
                conn.close()
                window.after(0, log, f"Disconnected from {username} ({addr})")

        def accept_clients():
            while True:
                conn, addr = server_socket.accept()
                threading.Thread(target=client_thread, args=(conn, addr), daemon=True).start()

        threading.Thread(target=accept_clients, daemon=True).start()

    start_server()
    window.mainloop()
