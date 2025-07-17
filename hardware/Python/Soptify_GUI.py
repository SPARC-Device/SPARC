import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import requests
from io import BytesIO

# 💡 Your valid token here
access_token = "BQAzNc8DAdIis9S3fCOrlubUYl9hP67x3JFyczAx_86oiXWDj8Vy65EboKLNGyR2P6V5Deuv4qSB5EScMt21IWMcQw3c4Zw-B7s_4rmcgP2BGKFlXC1VRzrp7KF4JjbH0NpXFNXMpOsjoya_w-EfoeyNnwpHEFvLqOgTasac9PXO0hfNgt7NtAsq6CNIxOWfN8b4MflhwRg2AA4YrN-aHVFY9t-9ac2u92CPNVHikaGE3z_lDCwNt-VGdb-8iwW6WLc4h3Oth7Q5y0o8tBbIT5g8IVlA3dBWh9JU3Zlfua5kg9Kl9Iv8XYekW0V20GZ5"


def fetch_top_tracks():
    artist_name = entry.get()
    headers = {"Authorization": f"Bearer {access_token}"}

    if not artist_name.strip():
        result_label.config(text="Please enter an artist name!")
        return

    # Search for artist
    search_url = f"https://api.spotify.com/v1/search?q={artist_name}&type=artist&limit=1"
    result = requests.get(search_url, headers=headers).json()
    artists = result.get('artists', {}).get('items', [])

    if not artists:
        result_label.config(text='Artist not found or token expired!')
        return

    artist_id = artists[0]['id']

    # Get top tracks
    top_url = f"https://api.spotify.com/v1/artists/{artist_id}/top-tracks?market=US"
    tracks = requests.get(top_url, headers=headers).json().get("tracks", [])

    # Clear previous widgets
    for widget in scroll_frame.winfo_children():
        widget.destroy()

    # Display track info
    for track in tracks:
        name = track["name"]
        album = track["album"]["name"]
        images = track["album"]["images"]
        img_url = images[1]["url"] if images else None

        track_container = tk.Frame(scroll_frame, pady=10)
        track_container.pack(fill="x", padx=10)

        # Show image
        if img_url:
            img_data = requests.get(img_url).content
            img = Image.open(BytesIO(img_data)).resize((100, 100))
            tk_img = ImageTk.PhotoImage(img)
            img_label = tk.Label(track_container, image=tk_img)
            img_label.image = tk_img
            img_label.pack(side="left", padx=10)

        # Name and album
        text = f"{name}\nAlbum: {album}"
        name_label = tk.Label(track_container, text=text, font=("Segoe UI", 10, "bold"), anchor="w", justify="left", wraplength=250)
        name_label.pack(side="left", fill="both", expand=True)

# 🎨 GUI Setup
root = tk.Tk()
root.title("Spotify Top Tracks")
root.geometry("460x700")
root.configure(bg="#f4f4f4")

entry = tk.Entry(root, width=30, font=("Segoe UI", 14))
entry.pack(pady=10)

btn = tk.Button(root, text="Search", command=fetch_top_tracks, bg="#1DB954", fg="white", font=("Segoe UI", 12, "bold"))
btn.pack(pady=5)

result_label = tk.Label(root, text="", fg="red", font=("Segoe UI", 10), bg="#f4f4f4")
result_label.pack()

# 🔽 Scrollable area
canvas = tk.Canvas(root, borderwidth=0, background="#f4f4f4", height=550)
scroll_frame = tk.Frame(canvas, background="#f4f4f4")
scrollbar = tk.Scrollbar(root, orient="vertical", command=canvas.yview)
canvas.configure(yscrollcommand=scrollbar.set)

scrollbar.pack(side="right", fill="y")
canvas.pack(side="left", fill="both", expand=True)
canvas.create_window((0, 0), window=scroll_frame, anchor="nw")

# 🧠 Scroll update
def on_configure(event):
    canvas.configure(scrollregion=canvas.bbox("all"))

scroll_frame.bind("<Configure>", on_configure)

root.mainloop()
