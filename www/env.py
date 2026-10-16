import os
print("Content-Type: text/plain")
print()
for k in sorted(os.environ):
    print(k + "=" + os.environ[k])