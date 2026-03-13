#!/usr/bin/env python3
import argparse
import json
import os
import sys
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError


def main() -> int:
  parser = argparse.ArgumentParser(description="Get Grafana dashboard JSON by UID")
  parser.add_argument("--uid", required=True, help="Grafana dashboard UID")
  parser.add_argument("--output", default="-", help="Output path, '-' means stdout")
  args = parser.parse_args()

  grafana_url = os.environ.get("GRAFANA_URL", "").rstrip("/")
  token = os.environ.get("GRAFANA_TOKEN", "")
  if not grafana_url or not token:
    print("missing env: GRAFANA_URL / GRAFANA_TOKEN", file=sys.stderr)
    return 2

  url = f"{grafana_url}/api/dashboards/uid/{args.uid}"
  req = Request(
    url,
    method="GET",
    headers={
      "Authorization": f"Bearer {token}",
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
