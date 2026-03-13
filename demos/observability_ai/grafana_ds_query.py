#!/usr/bin/env python3
import argparse
import json
import os
import sys
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError


def main() -> int:
  parser = argparse.ArgumentParser(description="Call Grafana /api/ds/query with a payload JSON")
  parser.add_argument("--payload", required=True, help="Path to payload JSON")
  parser.add_argument("--output", default="-", help="Output path, '-' means stdout")
  args = parser.parse_args()

  grafana_url = os.environ.get("GRAFANA_URL", "").rstrip("/")
  token = os.environ.get("GRAFANA_TOKEN", "")
  if not grafana_url or not token:
    print("missing env: GRAFANA_URL / GRAFANA_TOKEN", file=sys.stderr)
    return 2

  with open(args.payload, "r", encoding="utf-8") as f:
    payload_obj = json.load(f)
  payload = json.dumps(payload_obj).encode("utf-8")

  url = f"{grafana_url}/api/ds/query"
  req = Request(
    url,
    data=payload,
    method="POST",
    headers={
      "Authorization": f"Bearer {token}",
      "Content-Type": "application/json",
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
