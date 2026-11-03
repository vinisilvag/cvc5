"""
Benchmark runner to compare cvc5 configurations (default, CT, CCFV).

Typical usage:
  python3 run_bench.py --selftest
  python3 run_bench.py --bench-dir ~/smtlib/non-incremental --only-status unsat \
      --sample 200 --timeout 30 --jobs 8 --out results.jsonl
  python3 run_bench.py --bench-dir ~/smtlib/non-incremental --only-status unsat \
      --timeout 30 --jobs 8 --out results.jsonl        # run all (resumes if interrupted)

Output: a JSONL file with one line per (config, benchmark): status, wall, cpu, rss, cvc5 stats.
"""

import argparse
import json
import os
import random
import re
import signal
import subprocess
import tempfile
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

REPO_DIR = Path(__file__).resolve().parent
DEFAULT_CVC5 = REPO_DIR / "build" / "bin" / "cvc5"
CVC5 = os.environ.get("CVC5", str(DEFAULT_CVC5 if DEFAULT_CVC5.is_file() else "cvc5"))

COMMON = ["--lang=smt2", "--seed=1", "--stats"]

ct = [
    "--cbqi",
    "--e-matching",
    "--no-enum-inst",
    "--no-cegqi",
    "--no-mbqi",
    "--no-finite-model-find",
    "--no-sygus-inst",
    "--no-full-saturate-quant",
]

ccfv = [
    "--no-enum-inst",
    "--no-cegqi",
    "--no-mbqi",
    "--no-finite-model-find",
    "--no-sygus-inst",
    "--no-full-saturate-quant",
]

CONFIGS = {
    "cvc5-default": [],
    "cvc5-ct": ct,
    "ccfv-default": ["--ccfv=decision"],
    "ccfv-ct": ccfv + ["--ccfv=decision"],
}

STATUS_RE = re.compile(r"\(set-info\s+:status\s+(sat|unsat|unknown)\s*\)")
STAT_RE = re.compile(r"^\s*([A-Za-z0-9_:.\-\[\]<>]+)\s*=\s*(.+?)\s*$")


def expected_status(path):
    try:
        with open(path, "r", errors="replace") as f:
            m = STATUS_RE.search(f.read(65536))
        return m.group(1) if m else None
    except OSError:
        return None


def parse_stats(text):
    stats = {}
    for line in text.splitlines():
        m = STAT_RE.match(line)
        if not m:
            continue
        k, v = m.groups()
        try:
            stats[k] = float(v)
        except ValueError:
            stats[k] = v
    return stats


def run_one(cfg, flags, bench, root, timeout, depth, extra):
    cmd = [CVC5, *COMMON, f"--tlimit={int(timeout * 1000)}", *flags, *extra, str(bench)]
    out_f, err_f = tempfile.TemporaryFile(), tempfile.TemporaryFile()
    t0 = time.monotonic()
    p = subprocess.Popen(cmd, stdout=out_f, stderr=err_f, start_new_session=True)
    killed = threading.Event()

    def kill():
        killed.set()
        try:
            os.killpg(p.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass

    timer = threading.Timer(timeout + 10, kill)
    timer.start()
    _, wstatus, ru = os.wait4(p.pid, 0)
    timer.cancel()
    wall = time.monotonic() - t0
    rc = os.waitstatus_to_exitcode(wstatus)

    out_f.seek(0)
    err_f.seek(0)
    out = out_f.read().decode(errors="replace")
    err = err_f.read().decode(errors="replace")
    out_f.close()
    err_f.close()

    first = out.strip().splitlines()[0].strip() if out.strip() else ""
    if (
        killed.is_set()
        or "interrupted by timeout" in err.lower()
        or wall >= 0.95 * timeout
    ):
        status = "timeout"
    elif first in ("sat", "unsat"):
        status = first
    elif first == "unknown":
        status = "timeout" if wall >= 0.95 * timeout else "unknown"
    elif "out of memory" in err.lower():
        status = "memout"
    else:
        status = "error"

    rel = Path(bench).relative_to(root)
    family = "/".join(rel.parts[:-1][:depth]) or "."
    return {
        "config": cfg,
        "bench": str(rel),
        "family": family,
        "status": status,
        "expected": expected_status(bench),
        "wall": round(wall, 4),
        "cpu": round(ru.ru_utime + ru.ru_stime, 4),
        "maxrss_mb": round(ru.ru_maxrss / 1024.0, 1),  # Linux: ru_maxrss in KB
        "rc": rc,
        "timeout": timeout,
        "stats": parse_stats(out + "\n" + err),
        "err": err.strip().splitlines()[:2] if status == "error" else [],
    }


def selftest(timeout):
    tiny = (
        "(set-logic UF)(declare-sort U 0)(declare-fun f (U) U)(declare-const a U)"
        "(assert (forall ((x U)) (= (f x) x)))(assert (not (= (f a) a)))(check-sat)\n"
    )
    with tempfile.NamedTemporaryFile("w", suffix=".smt2", delete=False) as f:
        f.write(tiny)
        path = f.name
    root = Path(path).parent
    ok = True
    for cfg, flags in CONFIGS.items():
        r = run_one(cfg, flags, path, root, min(timeout, 10), 1, [])
        nq = [k for k in r["stats"] if re.search("quant|inst|ccfv", k, re.I)]
        print(
            f"{cfg:14s} status={r['status']:8s} wall={r['wall']:.2f}s  stats(quant/inst/ccfv)={len(nq)}  {r['err']}"
        )
        ok &= r["status"] == "unsat"
    os.unlink(path)
    print("SELFTEST OK" if ok else "SELFTEST FAILED: check option names / --ccfv")
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bench-dir")
    ap.add_argument("--configs", nargs="*", default=list(CONFIGS))
    ap.add_argument("--timeout", type=float, default=30)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--out", default="results.jsonl")
    ap.add_argument("--family-depth", type=int, default=2)
    ap.add_argument("--only-status", choices=["sat", "unsat"])
    ap.add_argument("--sample", type=int)
    ap.add_argument("--sample-seed", type=int, default=0)
    ap.add_argument(
        "--extra", nargs="*", default=[], help="extra flags for all configs"
    )
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args()

    if a.selftest:
        raise SystemExit(selftest(a.timeout))
    if not a.bench_dir:
        ap.error("--bench-dir is required")

    root = Path(a.bench_dir).resolve()
    benches = sorted(root.rglob("*.smt2"))
    if a.only_status:
        benches = [b for b in benches if expected_status(b) == a.only_status]
    if a.sample and a.sample < len(benches):
        random.Random(a.sample_seed).shuffle(benches)
        benches = sorted(benches[: a.sample])
    print(
        f"{len(benches)} benchmarks x {len(a.configs)} configs, timeout={a.timeout}s, jobs={a.jobs}"
    )

    done = set()
    if os.path.exists(a.out):
        with open(a.out) as f:
            for line in f:
                r = json.loads(line)
                done.add((r["config"], r["bench"]))
        print(f"resuming: {len(done)} existing runs")

    # Interleaves configs per benchmark: reduces bias from machine load/temperature.
    todo = [
        (c, b)
        for b in benches
        for c in a.configs
        if (c, str(b.relative_to(root))) not in done
    ]
    lock = threading.Lock()
    counter = {"n": 0}

    def work(item):
        cfg, b = item
        r = run_one(cfg, CONFIGS[cfg], b, root, a.timeout, a.family_depth, a.extra)
        with lock:
            with open(a.out, "a") as f:
                f.write(json.dumps(r) + "\n")
            counter["n"] += 1
            err_msg = f"  {r['err']}" if r.get("err") else ""
            print(
                f"[{counter['n']}/{len(todo)}] {r['config']:14s} {r['status']:8s} "
                f"wall={r['wall']:6.2f}s {r['bench']}{err_msg}",
                flush=True,
            )

    with ThreadPoolExecutor(max_workers=a.jobs) as ex:
        list(ex.map(work, todo))
    print("done ->", a.out)


if __name__ == "__main__":
    main()
