"""Send one JSON request to a running CCad --serve-ui-map instance."""

import argparse
import json


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("method")
    parser.add_argument("--server", default="ccad_live_cmd")
    parser.add_argument("--args", default="{}", help="JSON object merged into the request")
    parser.add_argument("--arg", action="append", default=[], help="key=value request field")
    options = parser.parse_args()

    request = json.loads(options.args)
    if not isinstance(request, dict):
        raise ValueError("--args must be a JSON object")
    for field in options.arg:
        key, separator, raw_value = field.partition("=")
        if not separator or not key:
            raise ValueError("--arg requires key=value")
        try:
            value = json.loads(raw_value)
        except json.JSONDecodeError:
            value = raw_value
        request[key] = value
    request["method"] = options.method

    pipe_path = rf"\\.\pipe\{options.server}"
    with open(pipe_path, "r+b", buffering=0) as pipe:
        pipe.write((json.dumps(request, separators=(",", ":")) + "\n").encode("utf-8"))
        pipe.flush()
        response = json.loads(pipe.readline().decode("utf-8"))
    print(json.dumps(response, indent=2))


if __name__ == "__main__":
    main()
