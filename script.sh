#!/bin/bash
HOST="localhost:8080"
URL_DIR="/upload"
FS_DIR="./uploads"        # relative to where the SERVER was launched, so run the script from that same directory
MAX_BODY=10485760                 # your client_max_body_size

pass=0; fail=0
check() { # name, expected, actual
    if [ "$2" == "$3" ]; then echo "PASS  $1"; pass=$((pass+1))
    else echo "FAIL  $1 (expected '$2', got '$3')"; fail=$((fail+1)); fi
}
status() { curl -s -o /dev/null -w "%{http_code}" "$@"; }
location() { curl -s -D - -o /dev/null "$@" | grep -i '^location:' | tr -d '\r' | awk '{print $2}'; }

rm -rf "$FS_DIR"/t_* "$FS_DIR"/.t_*

# 1. New file
check "new file -> 201" 201 "$(status -X POST --data-binary 'hello' $HOST$URL_DIR/t_a.txt)"
check "new file content" "hello" "$(cat $FS_DIR/t_a.txt)"

# 2. Collisions get (1), (2), original untouched
check "2nd upload -> 201" 201 "$(status -X POST --data-binary 'second' $HOST$URL_DIR/t_a.txt)"
check "(1) exists" "second" "$(cat "$FS_DIR/t_a(1).txt")"
check "Location header uses new name" "$URL_DIR/t_a(2).txt" \
    "$(location -X POST --data-binary 'third' $HOST$URL_DIR/t_a.txt)"
check "(2) exists" "third" "$(cat "$FS_DIR/t_a(2).txt")"
check "original untouched" "hello" "$(cat $FS_DIR/t_a.txt)"

# 3. No extension / dotfile / dot in name
status -X POST --data-binary 'x' $HOST$URL_DIR/t_noext >/dev/null
status -X POST --data-binary 'y' $HOST$URL_DIR/t_noext >/dev/null
check "no extension -> t_noext(1)" "y" "$(cat "$FS_DIR/t_noext(1)")"
status -X POST --data-binary 'x' $HOST$URL_DIR/.t_hidden >/dev/null
status -X POST --data-binary 'y' $HOST$URL_DIR/.t_hidden >/dev/null
check "dotfile -> .t_hidden(1)" "y" "$(cat "$FS_DIR/.t_hidden(1)")"
status -X POST --data-binary 'x' $HOST$URL_DIR/t_a.tar.gz >/dev/null
status -X POST --data-binary 'y' $HOST$URL_DIR/t_a.tar.gz >/dev/null
check "multi-dot -> t_a.tar(1).gz" "y" "$(cat "$FS_DIR/t_a.tar(1).gz")"

# 4. Binary integrity (null bytes, all byte values)
head -c 65536 /dev/urandom > /tmp/t_bin
status -X POST --data-binary @/tmp/t_bin $HOST$URL_DIR/t_bin.dat >/dev/null
cmp -s /tmp/t_bin $FS_DIR/t_bin.dat && r=same || r=different
check "binary body intact" same $r

# 5. Empty body
check "empty body -> 201" 201 "$(status -X POST -H 'Content-Length: 0' $HOST$URL_DIR/t_empty)"

# 6. Many collisions (your loop limit)
for i in $(seq 1 12); do
    code=$(status -X POST --data-binary "$i" $HOST$URL_DIR/t_many.txt)
    echo "      upload #$i -> $code"
done

# 7. Error cases
check "trailing slash -> 400" 400 "$(status -X POST --data-binary 'x' $HOST$URL_DIR/)"
mkdir -p $FS_DIR/t_dir
check "target is a directory -> 400" 400 "$(status -X POST --data-binary 'x' $HOST$URL_DIR/t_dir)"
check "missing parent dir -> 404" 404 "$(status -X POST --data-binary 'x' $HOST$URL_DIR/nope/t_x.txt)"
check "traversal -> 403" 403 "$(status --path-as-is -X POST --data-binary 'x' $HOST$URL_DIR/../../t_evil)"
check "encoded traversal -> 403" 403 "$(status --path-as-is -X POST --data-binary 'x' "$HOST$URL_DIR/%2e%2e/t_evil")"
head -c $((MAX_BODY + 1)) /dev/zero > /tmp/t_big
check "body over limit -> 413" 413 "$(status -X POST --data-binary @/tmp/t_big $HOST$URL_DIR/t_big.bin)"
head -c $MAX_BODY /dev/zero > /tmp/t_exact
check "body exactly at limit -> 201" 201 "$(status -X POST --data-binary @/tmp/t_exact $HOST$URL_DIR/t_exact.bin)"
mkdir -p $FS_DIR/t_ro && chmod 555 $FS_DIR/t_ro
check "unwritable dir -> 403 (not as root)" 403 "$(status -X POST --data-binary 'x' $HOST$URL_DIR/t_ro/f.txt)"
chmod 755 $FS_DIR/t_ro
check "POST on a no-POST location -> 405" 405 "$(status -X POST --data-binary 'x' $HOST/images/f.txt)"

# 8. Transfer / connection behavior
check "chunked upload -> 201" 201 "$(status -X POST -H 'Transfer-Encoding: chunked' --data-binary 'chunky' $HOST$URL_DIR/t_chunk.txt)"
check "chunked content" "chunky" "$(cat $FS_DIR/t_chunk.txt)"
echo "      keep-alive (expect two 201s on ONE connection, check 'Re-using connection'):"
curl -sv -X POST --data-binary 'k1' $HOST$URL_DIR/t_ka.txt --next -X POST --data-binary 'k2' $HOST$URL_DIR/t_ka.txt 2>&1 \
    | grep -E "HTTP/1.1 201|Re-using"

# 9. Cleanup + summary
rm -rf "$FS_DIR"/t_* "$FS_DIR"/.t_* /tmp/t_bin /tmp/t_big /tmp/t_exact
echo; echo "passed: $pass  failed: $fail"
