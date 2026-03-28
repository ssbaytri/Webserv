#!/usr/bin/python3
import os
import sys

# Read POST body from stdin
length = os.environ.get("CONTENT_LENGTH")
data = sys.stdin.read(int(length)) if length else ""

print("Content-Type: text/plain")
print()  # blank line separates headers from body
print("Received POST data:")
print(data)
print(f"REQUEST_METHOD = {os.environ.get('REQUEST_METHOD')}")
print(f"QUERY_STRING = {os.environ.get('QUERY_STRING')}")