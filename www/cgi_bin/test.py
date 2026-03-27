#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/html")
print("Status: 200")
print()
print("<h1>CGI Test</h1>")
print(f"<p>Method: {os.environ.get('REQUEST_METHOD', 'N/A')}</p>")
print(f"<p>Query: {os.environ.get('QUERY_STRING', 'N/A')}</p>")