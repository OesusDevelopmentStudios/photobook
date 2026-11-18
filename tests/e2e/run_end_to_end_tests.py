#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

"""Runs the end-to-end tests against a real pictorio-db server.

Starts the server from a pictorio-db source checkout on a temporary folder, creates its admin account the
way a browser does (/admin/setup with the setup code from the server's output) and the test accounts with
the admin API, then runs the tests and removes everything again.

    tests/e2e/run_end_to_end_tests.py                      # ../pictorio-db, the build in build/
    tests/e2e/run_end_to_end_tests.py --server ../pictorio-db/.venv/bin/pictorio-db   # no new venv
    tests/e2e/run_end_to_end_tests.py -- some other test command

The server reads a machine ID of its own from a file in the temporary folder, so that a test can be the app on
the server's computer without the real machine ID. The tests learn the server's address, the accounts'
password, the library folder and that file from the environment variables PICTORIO_E2E_*; without them they
skip themselves.
"""

import argparse
import json
import os
import re
import secrets
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[2]
ADMIN = "admin"
# The accounts the tests log in with, as in tests/e2e/EndToEndServer.hpp
USERS = {
    "sample_user": [],
    "kid": ["rating:nsfw"],  # restricted
}
SERVER_START_TIMEOUT = 60  # seconds; the first start creates the database


class ServerProcess:
    """pictorio-db serve, with its output kept in a log file and searched for the setup code."""

    def __init__(self, command: list[str], environment: dict[str, str], log_path: Path) -> None:
        self._log_path = log_path
        self._setup_code: str | None = None
        self._found_setup_code = threading.Event()
        self._process = subprocess.Popen(
            command, env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
        )
        self._reader = threading.Thread(target=self._read_output, daemon=True)
        self._reader.start()

    def setup_code(self, timeout: float) -> str:
        if not self._found_setup_code.wait(timeout) or self._setup_code is None:
            raise RuntimeError("The server printed no setup code.")

        return self._setup_code

    def has_exited(self) -> bool:
        return self._process.poll() is not None

    def stop(self) -> None:
        self._process.terminate()
        try:
            self._process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            self._process.kill()
            self._process.wait()

        self._reader.join(timeout=5)

    def log(self) -> str:
        return self._log_path.read_text(errors="replace")

    def _read_output(self) -> None:
        assert self._process.stdout is not None
        with self._log_path.open("w") as log:
            for line in self._process.stdout:
                log.write(line)
                log.flush()
                match = re.search(r"setup code (\S+)", line)
                if match:
                    self._setup_code = match.group(1)
                    self._found_setup_code.set()

        self._found_setup_code.set()  # the server ended: nobody waits for a code that will not come


def main() -> int:
    arguments = parse_arguments()
    work_dir = Path(tempfile.mkdtemp(prefix="pictorio-e2e-", dir=arguments.temp_parent))
    server: ServerProcess | None = None
    succeeded = False
    try:
        server_command = arguments.server or install_server(arguments.pictorio_db, work_dir / "venv")
        library_dir = work_dir / "library"
        machine_id_file = work_dir / "machine-id"
        machine_id_file.write_text(secrets.token_hex(16) + "\n")
        port = free_port()
        server_url = f"http://127.0.0.1:{port}"
        server = ServerProcess(
            [str(server_command), "serve"],
            server_environment(work_dir / "data", library_dir, machine_id_file, port),
            work_dir / "server.log",
        )
        wait_until_answering(server, server_url)

        password = secrets.token_urlsafe(16)
        create_admin(server_url, server.setup_code(timeout=5), password)
        admin_token = log_in(server_url, ADMIN, password)
        for username, restricted_tags in USERS.items():
            create_user(server_url, admin_token, username, password, restricted_tags)

        test_environment = dict(os.environ)
        test_environment.update(
            {
                "PICTORIO_E2E_SERVER_URL": server_url,
                "PICTORIO_E2E_PASSWORD": password,
                "PICTORIO_E2E_LIBRARY_DIR": str(library_dir),
                "PICTORIO_E2E_MACHINE_ID_FILE": str(machine_id_file),
            }
        )
        test_command = arguments.test_command or [
            "ctest", "--test-dir", str(arguments.build_dir), "--output-on-failure", "-R", "^TestEndToEnd",
        ]
        print("Running:", " ".join(test_command), flush=True)
        succeeded = subprocess.run(test_command, env=test_environment).returncode == 0
    except (OSError, RuntimeError, subprocess.CalledProcessError, urllib.error.URLError) as error:
        print(f"error: {error}", file=sys.stderr)
    finally:
        if server is not None:
            server.stop()
            if not succeeded:
                print("--- server output ---", file=sys.stderr)
                print(server.log(), file=sys.stderr)

        if arguments.keep:
            print(f"Kept {work_dir}")
        else:
            shutil.rmtree(work_dir, ignore_errors=True)

    return 0 if succeeded else 1


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--pictorio-db", type=Path, default=REPOSITORY.parent / "pictorio-db",
                        help="the pictorio-db source checkout to install (default: next to this repository)")
    parser.add_argument("--server", type=Path,
                        help="an installed pictorio-db command to run instead of installing one")
    parser.add_argument("--build-dir", type=Path, default=REPOSITORY / "build",
                        help="the build whose tests run (default: build/)")
    parser.add_argument("--temp-parent", type=Path,
                        help="where the temporary folder goes (default: the system's temporary folder)")
    parser.add_argument("--keep", action="store_true", help="keep the temporary folder, e.g. to look at the log")
    parser.add_argument("test_command", nargs="*", help="after --: the command that runs the tests")
    return parser.parse_args()


# A virtual environment of its own, so the server's exact dependency versions never touch another Python.
def install_server(source: Path, venv: Path) -> Path:
    if not (source / "pyproject.toml").is_file():
        raise RuntimeError(f"No pictorio-db checkout at {source}; give its folder with --pictorio-db.")

    print(f"Installing pictorio-db from {source}…", flush=True)
    subprocess.run([sys.executable, "-m", "venv", str(venv)], check=True)
    subprocess.run([str(venv / "bin" / "pip"), "install", "--quiet", str(source)], check=True)
    return venv / "bin" / "pictorio-db"


def free_port() -> int:
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def server_environment(data_dir: Path, library_dir: Path, machine_id_file: Path, port: int) -> dict[str, str]:
    environment = {name: value for name, value in os.environ.items() if not name.startswith("PICTORIO_")}
    environment.update(
        {
            "PICTORIO_DATA_DIR": str(data_dir),
            "PICTORIO_LIBRARY_DIR": str(library_dir),
            "PICTORIO_HOST_MACHINE_ID_FILE": str(machine_id_file),
            "PICTORIO_HOST": "127.0.0.1",
            "PICTORIO_PORT": str(port),
        }
    )
    return environment


def wait_until_answering(server: ServerProcess, server_url: str) -> None:
    deadline = time.monotonic() + SERVER_START_TIMEOUT
    while time.monotonic() < deadline:
        if server.has_exited():
            raise RuntimeError("The server stopped while starting.")

        try:
            with urllib.request.urlopen(f"{server_url}/api/v1/server/info", timeout=2):
                return
        except (urllib.error.URLError, ConnectionError):
            time.sleep(0.2)

    raise RuntimeError(f"The server did not answer within {SERVER_START_TIMEOUT} s.")


def create_admin(server_url: str, setup_code: str, password: str) -> None:
    form = urllib.parse.urlencode(
        {"setup_code": setup_code, "username": ADMIN, "password": password, "password_repeated": password}
    )
    request = urllib.request.Request(f"{server_url}/admin/setup", data=form.encode(), method="POST")
    with urllib.request.urlopen(request):
        pass  # 200: the account exists; a refusal raises HTTPError


def log_in(server_url: str, username: str, password: str) -> str:
    answer = send_json(
        server_url, "POST", "/api/v1/auth/tokens",
        {"username": username, "password": password, "device_name": "end-to-end tests"},
    )
    return answer["token"]


def create_user(server_url: str, token: str, username: str, password: str, restricted_tags: list[str]) -> None:
    send_json(
        server_url, "POST", "/api/v1/admin/users",
        {"username": username, "password": password, "restricted_tags": restricted_tags},
        token,
    )


def send_json(server_url: str, method: str, path: str, body: dict, token: str | None = None) -> dict:
    request = urllib.request.Request(
        server_url + path, data=json.dumps(body).encode(), method=method, headers={"Content-Type": "application/json"}
    )
    if token is not None:
        request.add_header("Authorization", f"Bearer {token}")

    with urllib.request.urlopen(request) as response:
        return json.load(response)


if __name__ == "__main__":
    sys.exit(main())
