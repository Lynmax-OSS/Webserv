# slowread.py  (doesn't read stdin for 5s)
import sys, time
time.sleep(5)
data = sys.stdin.read()
print("Content-Type: text/plain")
print()
print(len(data))