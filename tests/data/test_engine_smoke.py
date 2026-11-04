"""O motor ORIGINAL do Bygfoot simula partidas com o dataset fictício (pula se o binário não existir)."""
import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import fictional_data as fd  # noqa: E402

BIN = Path(os.environ.get("BYGFOOT_BIN", ROOT / "build" / "patched" / "bygfoot"))
SRC = ROOT / "upstream" / "support_files"
COUNTRIES = ["faroe_islands", "brazil", "england", "spain", "argentina"]


@unittest.skipUnless(BIN.exists() and os.access(BIN, os.X_OK), f"binário do motor ausente: {BIN}")
class EngineSmoke(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.out = Path(cls.tmp.name) / "support_files"
        cls.report = fd.build(SRC, cls.out)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_engine(self, country, team):
        env = dict(os.environ, BYGFOOT_BIN=str(BIN), BYGFOOT_SUPPORT=str(self.out))
        return subprocess.run(
            ["sh", str(ROOT / "spike/headless/bf-headless"), "--seed", "7", "--weeks", "12",
             "--country", country, "--team", team, "--format", "timeline-v1"],
            capture_output=True, text=True, env=env, timeout=120)

    def test_partida_completa_com_clubes_ficticios(self):
        real = set(self.report["team_map"])
        for c in COUNTRIES:
            d = next((self.out / "definitions").glob(f"*/{c}"))
            lg = sorted(d.glob("league_*1.xml"))[0]
            team = re.findall(r"<team_name>(.*?)</team_name>", lg.read_text(encoding="utf-8"))[0]
            r = self.run_engine(c, team)
            self.assertEqual(r.returncode, 0, f"{c}: {r.stderr[-200:]}")
            m = json.loads(r.stdout)["match"]
            self.assertTrue(m["home"] and m["away"])
            self.assertNotIn(m["home"], real, c)
            self.assertNotIn(m["away"], real, c)

    def test_nome_de_clube_real_nao_existe_mais(self):
        r = self.run_engine("faroe_islands", "V. Gotu")  # existe só no upstream
        self.assertNotEqual(r.returncode, 0)


if __name__ == "__main__":
    unittest.main()
