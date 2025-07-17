import json
import requests


artist=input("name a band:")

access_token="BQDwNwUS38wipDNnnJwpUmr8H3munumqtJLxcs3a5Le62hMf3S0o_-ugd8HWFfNFTbTk5VKexB7HNYhkx2U-iNREXiu8ijfdje_DG5d493K3vBg42Wg8LE3TN-O5476vHTGaqrWgRNp_a5nXXHIKm4N-LeTbm5PI0iRB0pJE1nShX_aP4YYB287CnM-UfNB9zi5f7sFrLAhyglE7lMdeHCCDAuLPu-om5ER2yoOYZ2nVN2CgR3Aayn6rYwbgYpw7l6wkGUSF22KKrV6EMxHNQKQeoaVD-DCVkfIjK2fBwoGzjU0uunG2yOAcQSFeq0_S"

headers={
    "Authorization": f"Bearer {access_token}"
}

search_url=f"https://api.spotify.com/v1/search?q={artist}&type=artist"

search_response = requests.get(search_url,headers=headers)
search_data=search_response.json()

#extract artist ID from result
try:
    artist_id=search_data["artists"]["items"][0]["id"]
    artist_name=search_data["artists"]["items"][0]["name"]
except IndexError:
    print("Artist not found.")
    exit()
1
# get top tracks of the artist
top_tracks_url=f"https://api.spotify.com/v1/artists/{artist_id}/top-tracks?market=US"
tracks_response=requests.get(top_tracks_url,headers=headers)
tracks_data=tracks_response.json()

#print the track list
print(f"\nTop Tracks for{artist_name}:\n")
for idx,track in enumerate(tracks_data["tracks"],1):
    album=track["album"]
    images=album["images"]
    # pick the largest image
    album_art_url=images[0]["url"] if images else "No image available"

    print(f"{idx}. {track['name']}-Album:{album['name']}")
    print(f"    Album art URL: {album_art_url}")