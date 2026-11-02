"""extract_units deve escrever UTF-8 com \\n mesmo quando o stdout do Python usa cp1252/CRLF (Windows)."""
import importlib.util
import io
import json
import os
import sys
import tempfile

here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location(
    "extract_units", os.path.join(here, "..", "..", "spike", "headless", "extract_units.py"))
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

dump = {"users": [{"live_game": {
    "fix_id": 7, "team_names": ["N. Runavík", "V. Gotu"],
    "units": [{"area": 1, "minute": 0, "possession": 0, "time": 0, "result": [0, 0],
               "event": {"type": 1, "team": 0}}]}}]}
with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False, encoding="utf-8") as f:
    json.dump(dump, f, ensure_ascii=False)
    path = f.name

raw = io.BytesIO()
fake = io.TextIOWrapper(raw, encoding="cp1252", newline="\r\n", write_through=True)
real, sys.stdout = sys.stdout, fake
try:
    mod.main(path)
finally:
    sys.stdout = real
    os.unlink(path)

data = raw.getvalue()
try:
    text = data.decode("utf-8")
except UnicodeDecodeError as e:
    sys.exit(f"FALHA: saída não é UTF-8 ({e})")
assert "\r" not in text, "FALHA: saída com CRLF"
assert "N. Runavík" in text, "FALHA: nome acentuado perdido"
print("OK")
