# Export the active compiler's standard-library headers for the LSP, without
# changing normal build flags or mixing headers from different C libraries.
import json
import os
from pathlib import Path
import shlex
import subprocess

from SCons.Script import COMMAND_LINE_TARGETS

Import("env")


def include_toolchain_headers(source, target, env):
    database_path = Path(env.subst("$COMPILATIONDB_PATH"))
    entries = json.loads(database_path.read_text(encoding="utf-8"))
    include_paths = {}

    for entry in entries:
        arguments = entry.get("arguments") or shlex.split(entry["command"])
        language = "c" if Path(entry["file"]).suffix == ".c" else "c++"
        key = (arguments[0], language)
        if key not in include_paths:
            result = subprocess.run(
                [arguments[0], "-E", "-x", language, "-v", "-"],
                input="",
                text=True,
                capture_output=True,
                check=True,
            )
            paths = []
            collecting = False
            for line in result.stderr.splitlines():
                if line.strip() == "#include <...> search starts here:":
                    collecting = True
                elif line.strip() == "End of search list.":
                    break
                elif collecting:
                    paths.append(str(Path(line.strip()).resolve()))
            if not paths:
                raise RuntimeError(f"No include paths reported by {arguments[0]}")
            include_paths[key] = paths

        entry["arguments"] = arguments + [
            argument for path in include_paths[key] for argument in ("-isystem", path)
        ]
        entry.pop("command", None)

    database_path.write_text(json.dumps(entries, indent=2) + "\n", encoding="utf-8")


if "compiledb" in COMMAND_LINE_TARGETS:
    output_path = os.environ.get("TELESCOPE_COMPILE_COMMANDS_PATH")
    if output_path:
        env.Replace(COMPILATIONDB_PATH=output_path)
    env.AlwaysBuild(env.Alias("compiledb"))
    env.AddPostAction("compiledb", include_toolchain_headers)
