#!/usr/bin/env python3
import argparse
import json
import os
import sys
from urllib.parse import urlencode
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError


def main() -> int:
  parser = argparse.ArgumentParser(description="Query Dynatrace Metrics API v2")
  parser.add_argument("--metric-selector", required=True, help="Dynatrace metric selector")
  parser.add_argument("--from", dest="from_ts", default="now-2h", help="From time, e.g. now-2h")
  parser.add_argument("--to", dest="to_ts", default="now", help="To time, e.g. now")
  parser.add_argument("--resolution", default="1m", help="Resolution, e.g. 1m")
  parser.add_argument("--output", default="-", help="Output path, '-' means stdout")
  args = parser.parse_args()

  base_url = os.environ.get("DYNATRACE_BASE_URL", "").rstrip("/")
  token = os.environ.get("DYNATRACE_API_TOKEN", "")

  if not base_url or not token:
    print("missing env: DYNATRACE_BASE_URL / DYNATRACE_API_TOKEN", file=sys.stderr)
    return 2

  params = {
    "metricSelector": args.metric_selector,
    "from": args.from_ts,
    "to": args.to_ts,
    "resolution": args.resolution,
  }
  url = f"{base_url}/api/v2/metrics/query?{urlencode(params)}"
  req = Request(
    url,
    method="GET",
    headers={
      "Authorization": f"Api-Token {token}",
      "Accept": "application/json",
    },
  )

  try:
    with urlopen(req, timeout=30) as resp:
      body = resp.read().decode("utf-8")
  except HTTPError as e:
    err = e.read().decode("utf-8", errors="replace")
    print(f"HTTPError {e.code}: {err}", file=sys.stderr)
    return 1
  except URLError as e:
    print(f"URLError: {e}", file=sys.stderr)
    return 1

  obj = json.loads(body)
  rendered = json.dumps(obj, ensure_ascii=False, indent=2)
  if args.output == "-":
    print(rendered)
  else:
    with open(args.output, "w", encoding="utf-8") as f:
      f.write(rendered + "\n")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
