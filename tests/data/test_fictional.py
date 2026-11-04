"""Testes do gerador de dados fictícios (tools/fictional_data.py).

Rodar da raiz do repositório:  python3 -m unittest discover -s tests/data -v
"""
import hashlib
import re
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import fictional_data as fd  # noqa: E402

SRC = ROOT / "upstream" / "support_files"


def read(p):
    return p.read_text(encoding="utf-8")


def teams_of(path):
    return re.findall(r"<team_name>(.*?)</team_name>", read(path))


class FictionalDataTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.out = Path(cls.tmp.name) / "support_files"
        cls.report = fd.build(SRC, cls.out)
        cls.files = sorted(p.relative_to(SRC) for p in (SRC / "definitions").rglob("*.xml"))
        every = [n for rel in cls.files if not fd.is_national_file(rel) for n in teams_of(SRC / rel)]
        cls.prefixes = fd.generic_prefixes(every)  # "England 374": times genéricos do próprio upstream

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_todo_arquivo_de_definicao_foi_gerado_e_e_xml_valido(self):
        self.assertGreaterEqual(len(self.files), 1200)
        for rel in self.files:
            dst = self.out / rel
            self.assertTrue(dst.exists(), rel)
            try:
                ET.parse(SRC / rel)
            except ET.ParseError:
                continue  # o próprio upstream traz alguns XML quebrados; eles passam sem alteração de estrutura
            ET.parse(dst)  # levanta se a saída ficou malformada

    def test_estrutura_igual_so_o_texto_dos_nomes_muda(self):
        keep = re.compile(r"<(team_name|first_team|name|short_name)>.*?</\1>")
        for rel in self.files:
            a = keep.sub("<x/>", read(SRC / rel))
            b = keep.sub("<x/>", read(self.out / rel))
            self.assertEqual(a, b, f"estrutura mudou em {rel}")

    def test_nenhum_clube_real_sobrevive(self):
        real = set()
        for rel in self.files:
            if not fd.is_national_file(rel):
                real.update(n for n in teams_of(SRC / rel) if not fd.is_generic(n, self.prefixes))
        national = {n for rel in self.files if fd.is_national_file(rel) for n in teams_of(SRC / rel)}
        survivors = {n for rel in self.files for n in teams_of(self.out / rel)}
        self.assertGreater(len(real), 3000)
        # nomes que o upstream também usa em seleções/regiões ("Aalborg" em league_eu) podem coexistir
        self.assertEqual(sorted((real - national) & survivors), [])

    def test_selecoes_e_times_genericos_ficam_como_estao(self):
        for rel in self.files:
            if fd.is_national_file(rel):
                self.assertEqual(teams_of(SRC / rel), teams_of(self.out / rel), rel)
        gen = [n for rel in self.files for n in teams_of(SRC / rel) if fd.is_generic(n, self.prefixes)]
        self.assertGreater(len(gen), 100)
        survivors = {n for rel in self.files for n in teams_of(self.out / rel)}
        for n in gen:
            self.assertIn(n, survivors)

    def test_mapa_um_para_um_e_nomes_unicos_em_cada_liga(self):
        m = self.report["team_map"]
        self.assertEqual(len(set(m.values())), len(m), "dois clubes reais viraram o mesmo fictício")
        for rel in self.files:
            if rel.name.startswith("league_"):
                before, after = teams_of(SRC / rel), teams_of(self.out / rel)
                # o upstream tem ligas com clube repetido (austria2): a saída não pode piorar isso
                self.assertEqual(len(before) - len(set(before)), len(after) - len(set(after)), rel)

    def test_competicoes_sem_marcas_reais(self):
        text = " ".join(
            " ".join(re.findall(r"<(?:name|short_name)>(.*?)</", read(self.out / rel))) for rel in self.files
        )
        for brand in ("UEFA", "FIFA", "Fifa", "Champions League", "Europa League", "Premier League",
                      "Bundesliga", "La Liga", "Serie A", "World Cup", "Copa América", "Gold Cup", "FA Cup"):
            self.assertNotIn(brand, text, brand)

    def test_nomes_de_liga_e_copa_unicos(self):
        seen = {}
        for rel in self.files:
            if "miscellaneous" in rel.parts or not rel.name.startswith(("league_", "cup_")):
                continue
            nm = re.search(r"<name>(.*?)</name>", read(self.out / rel))
            if nm:
                self.assertNotIn(nm.group(1), seen, f"{rel} repete nome de {seen.get(nm.group(1))}")
                seen[nm.group(1)] = rel

    def test_first_team_continua_apontando_para_um_clube_que_existe(self):
        src_all = {n for rel in self.files for n in teams_of(SRC / rel)}
        out_all = {n for rel in self.files for n in teams_of(self.out / rel)}
        checked = 0
        for rel in self.files:
            for ft in re.findall(r"<first_team>(.*?)</first_team>", read(SRC / rel)):
                if ft in src_all:  # referência válida no upstream
                    checked += 1
            for ft in re.findall(r"<first_team>(.*?)</first_team>", read(self.out / rel)):
                self.assertIn(ft, out_all, f"{rel}: first_team {ft!r} não existe mais")
        self.assertGreater(checked, 30)

    def test_determinismo(self):
        def digest(root):
            h = hashlib.sha256()
            for p in sorted((root / "definitions").rglob("*.xml")):
                h.update(p.relative_to(root).as_posix().encode())
                h.update(p.read_bytes())
            return h.hexdigest()

        with tempfile.TemporaryDirectory() as t:
            out2 = Path(t) / "s"
            fd.build(SRC, out2)
            self.assertEqual(digest(self.out), digest(out2))

    def test_demais_arquivos_do_support_files_sao_copiados(self):
        for rel in ("bygfoot.conf", "names/player_names_brazil.xml", "pixmaps/symbols/flag_brazil.png"):
            self.assertEqual((SRC / rel).read_bytes(), (self.out / rel).read_bytes(), rel)

    def test_nomes_plausiveis_sem_caracteres_perigosos(self):
        for n in self.report["team_map"].values():
            self.assertRegex(n, r"^[^<>&\"\\\x00-\x1f]{3,40}$")


if __name__ == "__main__":
    unittest.main()
