#!/usr/bin/env python3
# Copyright (C) 2021 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# taken from here, to open a .trace file in Perfetto in the browser.  Runs
# an http server at a port, and then browser can read the file from that
# since fileURL are often blocked. Use a system call to run this, and it
# will change the working directory to that of the file.
# from https://github.com/google/perfetto/blob/master/tools/open_trace_in_ui

import argparse
import http.server
import os
import socketserver
import sys
import webbrowser


class ANSI:
  END = '\033[0m'
  BOLD = '\033[1m'
  RED = '\033[91m'
  BLACK = '\033[30m'
  BLUE = '\033[94m'
  BG_YELLOW = '\033[43m'
  BG_BLUE = '\033[44m'


# HTTP Server used to open the trace in the browser.
class HttpHandler(http.server.SimpleHTTPRequestHandler):

  def end_headers(self):
    self.send_header('Access-Control-Allow-Origin', self.server.allow_origin)
    self.send_header('Cache-Control', 'no-cache')
    super().end_headers()

  def do_GET(self):
    if self.path != '/' + self.server.expected_fname:
      self.send_error(404, "File not found")
      return

    self.server.fname_get_completed = True
    super().do_GET()

  def do_POST(self):
    self.send_error(404, "File not found")


def prt(msg, colors=ANSI.END):
  print(colors + msg + ANSI.END)


def open_trace(path, open_browser, origin):
  # We reuse the HTTP+RPC port because it's the only one allowed by the CSP.
  PORT = 9001
  path = os.path.abspath(path)
  os.chdir(os.path.dirname(path))
  fname = os.path.basename(path)
  socketserver.TCPServer.allow_reuse_address = True
  with socketserver.TCPServer(('127.0.0.1', PORT), HttpHandler) as httpd:
    address = f'{origin}/#!/?url=http://127.0.0.1:{PORT}/{fname}'
    if open_browser:
      webbrowser.open_new_tab(address)
    else:
      print(f'Open URL in browser: {address}')

    httpd.expected_fname = fname
    httpd.fname_get_completed = None
    httpd.allow_origin = origin
    while httpd.fname_get_completed is None:
      httpd.handle_request()


def main():
  examples = '\n'.join(
      [ANSI.BOLD + 'Usage:' + ANSI.END, '  -i path/trace_file_name [-n]'])
  parser = argparse.ArgumentParser(
      epilog=examples, formatter_class=argparse.RawTextHelpFormatter)

  help = 'Input trace filename'
  parser.add_argument('-i', '--trace', help=help)
  parser.add_argument(
      '-n', '--no-open-browser', action='store_true', default=False)
  parser.add_argument('--origin', default='https://ui.perfetto.dev')

  args = parser.parse_args()
  trace_file = args.trace
  open_browser = not args.no_open_browser

  if trace_file is None:
    prt('Please specify trace file name with -i/--trace argument', ANSI.RED)
    sys.exit(1)
  elif not os.path.exists(trace_file):
    prt('%s not found ' % trace_file, ANSI.RED)
    sys.exit(1)

  prt('Opening the trace (%s) in the browser' % trace_file)
  open_trace(trace_file, open_browser, args.origin)


if __name__ == '__main__':
  sys.exit(main())
