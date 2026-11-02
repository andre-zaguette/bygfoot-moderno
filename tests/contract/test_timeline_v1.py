"""Teste de contrato do MatchTimeline v1 (só biblioteca padrão).

Roda o bf-headless com --format timeline-v1 e confere as invariantes do contrato.
A comparação byte a byte com timeline-v1.sample.json só vale com o motor remendado
(BYGFOOT_BIN apontando para a cópia com patches/), porque o upstream original dá
partidas diferentes em cada sistema (ADR-002).
"""
import json
import os
import subprocess
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BF = os.path.join(ROOT, "spike", "headless", "bf-headless")
SAMPLE = os.path.join(ROOT, "tests", "contract", "timeline-v1.sample.json")
GOAL_TYPES = (10, 11)


def run(*args):
    return subprocess.run(["sh", BF, *args], capture_output=True)


def timeline(seed):
    r = run("--seed", str(seed), "--format", "timeline-v1")
    assert r.returncode == 0, r.stderr.decode("utf-8", "replace")
    return r.stdout, json.loads(r.stdout.decode("utf-8"))


class TimelineV1(unittest.TestCase):
    def test_invariants_over_several_seeds(self):
        for seed in (0, 1, 431199):
            raw, doc = timeline(seed)
            with self.subTest(seed=seed):
                self.assertEqual(doc["schemaVersion"], 1)
                self.assertEqual(doc["source"]["engine"], "bygfoot")
                self.assertEqual(doc["source"]["seed"], seed)
                self.assertEqual(len(doc["source"]["upstreamCommit"]), 40)
                units = doc["units"]
                self.assertTrue(units)
                prev = [0, 0]
                for i, u in enumerate(units):
                    self.assertIn(u["area"], (0, 1, 2))
                    self.assertIn(u["possession"], (0, 1))
                    self.assertIn(u["time"], (0, 1, 2, 3))
                    self.assertGreaterEqual(u["minute"], -1)
                    self.assertIsInstance(u["event"]["type"], int)
                    if u["result"] != prev:
                        self.assertTrue(
                            u["event"]["type"] in GOAL_TYPES or u["time"] == 3,
                            f"placar mudou no lance {i} sem gol (evento {u['event']['type']})")
                    prev = u["result"]
                self.assertEqual(units[-1]["result"], doc["match"]["final"])
                self.assertEqual(units[0]["event"]["type"], 1)   # START_MATCH
                self.assertEqual(units[-1]["event"]["type"], 4)  # END_MATCH
                self.assertTrue(raw.endswith(b"\n") and b"\r" not in raw)

    def test_players_have_id_and_name(self):
        _, doc = timeline(431199)
        saw = False
        for u in doc["units"]:
            for k in ("player", "player2"):
                if k in u["event"]:
                    saw = True
                    self.assertIsInstance(u["event"][k]["id"], int)
                    self.assertIsInstance(u["event"][k]["name"], str)
        self.assertTrue(saw, "nenhum lance com jogador na seed 431199")

    def test_sample_is_byte_identical_with_patched_engine(self):
        if not os.environ.get("BYGFOOT_BIN"):
            self.skipTest("defina BYGFOOT_BIN para a cópia remendada")
        raw, _ = timeline(431199)
        with open(SAMPLE, "rb") as f:
            self.assertEqual(raw, f.read())

    def test_unknown_format_is_rejected(self):
        r = run("--seed", "1", "--format", "xyz")
        self.assertEqual(r.returncode, 2)
        self.assertTrue(r.stderr)
        self.assertEqual(r.stdout, b"")

    def test_default_output_is_still_line_based(self):
        r = run("--seed", "1")
        self.assertEqual(r.returncode, 0)
        lines = r.stdout.decode("utf-8").splitlines()
        self.assertIn('"final"', lines[-1])


if __name__ == "__main__":
    unittest.main()
