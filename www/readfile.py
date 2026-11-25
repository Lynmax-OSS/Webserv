import os
print("Content-Type: text/plain")
print()
print(os.getcwd())
with open("data.txt") as f:
    print(f.read())